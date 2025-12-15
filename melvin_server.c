/* ============================================================================
 * MELVIN HTTP SERVER
 * 
 * Simple HTTP server for web chat interface with melvin
 * ============================================================================ */

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#define close_socket closesocket
#define SOCKET_ERROR_CODE WSAGetLastError()
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>
#define SOCKET int
#define INVALID_SOCKET -1
#define SOCKET_ERROR -1
#define close_socket close
#define SOCKET_ERROR_CODE errno
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <errno.h>

/* Include melvin */
typedef struct MelvinGraph MelvinGraph;
extern MelvinGraph* melvin_create(void);
extern void melvin_destroy(MelvinGraph *g);
extern void run_episode(MelvinGraph *g, const uint8_t *input, uint32_t input_len,
                       const uint8_t *target, uint32_t target_len);
extern void melvin_get_output(MelvinGraph *g, uint32_t **output, uint32_t *length);
extern float melvin_get_error_rate(MelvinGraph *g);

/* Global melvin instance */
static MelvinGraph *g_melvin = NULL;

/* HTTP Server Configuration */
#define DEFAULT_PORT 8080
#define BUFFER_SIZE 8192
#define MAX_PATH_LENGTH 256

/* Get port from environment or use default */
static int get_port(void) {
    const char *port_str = getenv("PORT");
    if (port_str) {
        int port = atoi(port_str);
        if (port > 0 && port < 65536) {
            return port;
        }
    }
    return DEFAULT_PORT;
}

/* HTTP Response Helpers */
void send_response(SOCKET client, int status, const char *status_text, 
                   const char *content_type, const char *body, size_t body_len) {
    char response[BUFFER_SIZE];
    int len = snprintf(response, sizeof(response),
        "HTTP/1.1 %d %s\r\n"
        "Content-Type: %s\r\n"
        "Content-Length: %zu\r\n"
        "Access-Control-Allow-Origin: *\r\n"
        "Access-Control-Allow-Methods: GET, POST, OPTIONS\r\n"
        "Access-Control-Allow-Headers: Content-Type\r\n"
        "Connection: close\r\n"
        "\r\n",
        status, status_text, content_type, body_len);
    
    send(client, response, len, 0);
    if (body && body_len > 0) {
        send(client, body, body_len, 0);
    }
}

void send_json(SOCKET client, int status, const char *json) {
    send_response(client, status, "OK", "application/json", json, strlen(json));
}

void send_error(SOCKET client, int status, const char *message) {
    char json[512];
    snprintf(json, sizeof(json), "{\"error\":\"%s\"}", message);
    send_json(client, status, json);
}

/* Parse HTTP request */
typedef struct {
    char method[16];
    char path[MAX_PATH_LENGTH];
    char body[BUFFER_SIZE];
    size_t body_len;
} HttpRequest;

int parse_request(const char *buffer, size_t len, HttpRequest *req) {
    memset(req, 0, sizeof(HttpRequest));
    
    /* Parse first line: METHOD PATH HTTP/1.1 */
    if (sscanf(buffer, "%15s %255s", req->method, req->path) != 2) {
        return -1;
    }
    
    /* Find body (after \r\n\r\n) */
    const char *body_start = strstr(buffer, "\r\n\r\n");
    if (body_start) {
        body_start += 4;
        size_t body_size = len - (body_start - buffer);
        
        /* Validate body size - reject if too large */
        if (body_size >= sizeof(req->body)) {
            return -1; /* Body too large */
        }
        
        if (body_size > 0) {
            memcpy(req->body, body_start, body_size);
            req->body_len = body_size;
        }
    }
    
    return 0;
}

/* Extract JSON value (simple parser) */
int extract_json_string(const char *json, const char *key, char *value, size_t value_size) {
    char search[128];
    snprintf(search, sizeof(search), "\"%s\"", key);
    const char *key_pos = strstr(json, search);
    if (!key_pos) return -1;
    
    /* Find colon after key */
    const char *colon = strchr(key_pos, ':');
    if (!colon) return -1;
    
    /* Find value (string or number) */
    const char *value_start = colon + 1;
    while (*value_start == ' ' || *value_start == '\t') value_start++;
    
    if (*value_start == '"') {
        /* String value - handle escaped quotes */
        value_start++;
        const char *value_end = value_start;
        while (*value_end && *value_end != '"') {
            if (*value_end == '\\' && *(value_end + 1) == '"') {
                value_end += 2; /* Skip escaped quote */
            } else {
                value_end++;
            }
        }
        if (!*value_end) return -1; /* No closing quote */
        
        /* Copy value, handling escape sequences */
        size_t len = 0;
        const char *src = value_start;
        while (src < value_end && len < value_size - 1) {
            if (*src == '\\' && *(src + 1) == '"') {
                value[len++] = '"';
                src += 2;
            } else if (*src == '\\' && *(src + 1) == '\\') {
                value[len++] = '\\';
                src += 2;
            } else if (*src == '\\' && *(src + 1) == 'n') {
                value[len++] = '\n';
                src += 2;
            } else {
                value[len++] = *src++;
            }
        }
        value[len] = '\0';
    } else {
        /* Number or other - just copy until comma or } */
        const char *value_end = value_start;
        while (*value_end && *value_end != ',' && *value_end != '}' && 
               *value_end != '\n' && *value_end != '\r' && *value_end != ' ') {
            value_end++;
        }
        size_t len = value_end - value_start;
        if (len >= value_size) len = value_size - 1;
        memcpy(value, value_start, len);
        value[len] = '\0';
    }
    
    return 0;
}

/* Handle /api/chat endpoint */
void handle_chat(SOCKET client, const HttpRequest *req) {
    if (!g_melvin) {
        send_error(client, 500, "Melvin not initialized");
        return;
    }
    
    /* Extract message from JSON body */
    char message[BUFFER_SIZE] = {0};
    if (extract_json_string(req->body, "message", message, sizeof(message)) != 0) {
        send_error(client, 400, "Missing 'message' field in JSON");
        return;
    }
    
    if (strlen(message) == 0) {
        send_error(client, 400, "Message cannot be empty");
        return;
    }
    
    /* Process message through melvin */
    const uint8_t *input = (const uint8_t*)message;
    uint32_t input_len = strlen(message);
    
    /* Run episode (no target - pure inference/chat) */
    run_episode(g_melvin, input, input_len, NULL, 0);
    
    /* Get output */
    uint32_t *output;
    uint32_t output_len;
    melvin_get_output(g_melvin, &output, &output_len);
    
    /* Convert output to string and escape for JSON */
    char response_text[BUFFER_SIZE] = {0};
    char escaped_response[BUFFER_SIZE * 2] = {0}; /* Enough space for escaped chars */
    size_t escaped_len = 0;
    size_t response_pos = 0; /* Track position in response_text */
    
    /* Convert output array to string, handling all values correctly */
    for (uint32_t i = 0; i < output_len && response_pos < sizeof(response_text) - 1; i++) {
        if (output[i] < 256) {
            response_text[response_pos++] = (char)output[i];
        }
        /* Skip values >= 256 (they're not valid ASCII/UTF-8 bytes) */
    }
    response_text[response_pos] = '\0'; /* Ensure null termination */
    
    /* Escape JSON special characters */
    for (size_t i = 0; i < strlen(response_text) && escaped_len < sizeof(escaped_response) - 1; i++) {
        char c = response_text[i];
        if (c == '"') {
            escaped_response[escaped_len++] = '\\';
            escaped_response[escaped_len++] = '"';
        } else if (c == '\\') {
            escaped_response[escaped_len++] = '\\';
            escaped_response[escaped_len++] = '\\';
        } else if (c == '\n') {
            escaped_response[escaped_len++] = '\\';
            escaped_response[escaped_len++] = 'n';
        } else if (c == '\r') {
            escaped_response[escaped_len++] = '\\';
            escaped_response[escaped_len++] = 'r';
        } else if (c == '\t') {
            escaped_response[escaped_len++] = '\\';
            escaped_response[escaped_len++] = 't';
        } else if (c >= 32 && c < 127) {
            /* Printable ASCII */
            escaped_response[escaped_len++] = c;
        } else {
            /* Skip non-printable characters */
        }
    }
    escaped_response[escaped_len] = '\0';
    
    /* Get system state */
    float error_rate = melvin_get_error_rate(g_melvin);
    
    /* Build JSON response */
    char json_response[BUFFER_SIZE * 2];
    snprintf(json_response, sizeof(json_response),
        "{\"response\":\"%s\",\"error_rate\":%.3f}",
        escaped_response, error_rate);
    
    send_json(client, 200, json_response);
}

/* Handle /api/status endpoint */
void handle_status(SOCKET client) {
    if (!g_melvin) {
        send_error(client, 500, "Melvin not initialized");
        return;
    }
    
    float error_rate = melvin_get_error_rate(g_melvin);
    
    char json[256];
    snprintf(json, sizeof(json),
        "{\"status\":\"running\",\"error_rate\":%.3f}",
        error_rate);
    
    send_json(client, 200, json);
}

/* Serve static file */
void serve_file(SOCKET client, const char *path) {
    /* Security: prevent directory traversal */
    if (strstr(path, "..") != NULL) {
        send_error(client, 403, "Forbidden");
        return;
    }
    
    /* Map root to index.html */
    if (strcmp(path, "/") == 0) {
        path = "/index.html";
    }
    
    /* Build file path */
    char filepath[MAX_PATH_LENGTH];
    snprintf(filepath, sizeof(filepath), "web%s", path);
    
    /* Open file */
    FILE *f = fopen(filepath, "rb");
    if (!f) {
        send_error(client, 404, "File not found");
        return;
    }
    
    /* Get file size */
    fseek(f, 0, SEEK_END);
    long file_size = ftell(f);
    fseek(f, 0, SEEK_SET);
    
    /* Determine content type */
    const char *content_type = "text/plain";
    if (strstr(path, ".html")) content_type = "text/html";
    else if (strstr(path, ".css")) content_type = "text/css";
    else if (strstr(path, ".js")) content_type = "application/javascript";
    else if (strstr(path, ".json")) content_type = "application/json";
    
    /* Read file */
    char *file_content = malloc(file_size);
    if (!file_content) {
        fclose(f);
        send_error(client, 500, "Memory error");
        return;
    }
    
    size_t read = fread(file_content, 1, file_size, f);
    fclose(f);
    
    /* Send response */
    send_response(client, 200, "OK", content_type, file_content, read);
    free(file_content);
}

/* Handle HTTP request */
void handle_request(SOCKET client) {
    char buffer[BUFFER_SIZE];
    int bytes_received = recv(client, buffer, sizeof(buffer) - 1, 0);
    
    if (bytes_received <= 0) {
        close_socket(client);
        return;
    }
    
    buffer[bytes_received] = '\0';
    
    HttpRequest req;
    if (parse_request(buffer, bytes_received, &req) != 0) {
        send_error(client, 400, "Invalid request");
        close_socket(client);
        return;
    }
    
    /* Handle OPTIONS (CORS preflight) */
    if (strcmp(req.method, "OPTIONS") == 0) {
        send_response(client, 200, "OK", "text/plain", "", 0);
        close_socket(client);
        return;
    }
    
    /* Route requests */
    if (strcmp(req.path, "/api/chat") == 0 && strcmp(req.method, "POST") == 0) {
        handle_chat(client, &req);
    } else if (strcmp(req.path, "/api/status") == 0 && strcmp(req.method, "GET") == 0) {
        handle_status(client);
    } else {
        /* Serve static file */
        serve_file(client, req.path);
    }
    
    close_socket(client);
}

/* Initialize networking */
int init_networking(void) {
#ifdef _WIN32
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        fprintf(stderr, "WSAStartup failed\n");
        return -1;
    }
#endif
    return 0;
}

void cleanup_networking(void) {
#ifdef _WIN32
    WSACleanup();
#endif
}

/* Main server loop */
int main(void) {
    printf("MELVIN HTTP SERVER\n");
    printf("==================\n\n");
    
    /* Initialize melvin */
    printf("Initializing Melvin...\n");
    g_melvin = melvin_create();
    if (!g_melvin) {
        fprintf(stderr, "Failed to create Melvin instance\n");
        return 1;
    }
    printf("Melvin initialized successfully\n\n");
    
    /* Initialize networking */
    if (init_networking() != 0) {
        melvin_destroy(g_melvin);
        return 1;
    }
    
    /* Create socket */
    SOCKET server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == INVALID_SOCKET) {
        fprintf(stderr, "Socket creation failed: %d\n", SOCKET_ERROR_CODE);
        cleanup_networking();
        melvin_destroy(g_melvin);
        return 1;
    }
    
    /* Set socket options (reuse address) */
    int opt = 1;
#ifdef _WIN32
    setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, (char*)&opt, sizeof(opt));
#else
    setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
#endif
    
    /* Get port from environment (Railway sets this) */
    int port = get_port();
    
    /* Bind socket */
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);
    
    if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
        fprintf(stderr, "Bind failed: %d\n", SOCKET_ERROR_CODE);
        close_socket(server_socket);
        cleanup_networking();
        melvin_destroy(g_melvin);
        return 1;
    }
    
    /* Listen */
    if (listen(server_socket, 5) == SOCKET_ERROR) {
        fprintf(stderr, "Listen failed: %d\n", SOCKET_ERROR_CODE);
        close_socket(server_socket);
        cleanup_networking();
        melvin_destroy(g_melvin);
        return 1;
    }
    
    printf("Server listening on port %d\n", port);
    printf("Melvin is ready to chat!\n\n");
    
    /* Main loop */
    while (1) {
        struct sockaddr_in client_addr;
#ifdef _WIN32
        int addr_len = sizeof(client_addr);
#else
        socklen_t addr_len = sizeof(client_addr);
#endif
        
        SOCKET client_socket = accept(server_socket, (struct sockaddr*)&client_addr, &addr_len);
        if (client_socket == INVALID_SOCKET) {
            fprintf(stderr, "Accept failed: %d\n", SOCKET_ERROR_CODE);
            continue;
        }
        
        /* Handle request (sequential for simplicity) */
        handle_request(client_socket);
    }
    
    /* Cleanup (never reached, but good practice) */
    close_socket(server_socket);
    cleanup_networking();
    melvin_destroy(g_melvin);
    
    return 0;
}
