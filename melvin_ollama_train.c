/* Melvin-Ollama Automated Training
 * 
 * Runs automatically - no user input needed
 * Ollama -> Melvin -> Ollama loop for training
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <time.h>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#endif

/* Forward declarations for Melvin functions */
typedef struct MelvinGraph MelvinGraph;

extern MelvinGraph* melvin_create(void);
extern void melvin_destroy(MelvinGraph *g);
extern MelvinGraph* melvin_load_brain(const char *filename);
extern int melvin_save_brain(MelvinGraph *g, const char *filename);
extern void inject_input_from_port(MelvinGraph *g, const uint8_t *bytes, uint32_t length, uint32_t port_id);
extern void run_episode(MelvinGraph *g, const uint8_t *input, uint32_t input_len,
                       const uint8_t *target, uint32_t target_len);
extern void melvin_get_output(MelvinGraph *g, uint32_t **output, uint32_t *length);

#define OLLAMA_HOST "localhost"
#define OLLAMA_PORT 11434
#define BUFFER_SIZE 4096

/* Simple HTTP POST to Ollama API */
char* ollama_generate(const char *prompt) {
    /* Build JSON body (escape quotes in prompt) */
    char json_body[4096];
    char escaped_prompt[2048];
    int j = 0;
    for (int i = 0; prompt[i] && j < sizeof(escaped_prompt) - 1; i++) {
        if (prompt[i] == '"') {
            escaped_prompt[j++] = '\\';
            escaped_prompt[j++] = '"';
        } else if (prompt[i] == '\\') {
            escaped_prompt[j++] = '\\';
            escaped_prompt[j++] = '\\';
        } else if (prompt[i] == '\n') {
            escaped_prompt[j++] = '\\';
            escaped_prompt[j++] = 'n';
        } else {
            escaped_prompt[j++] = prompt[i];
        }
    }
    escaped_prompt[j] = '\0';
    
    snprintf(json_body, sizeof(json_body),
        "{\"model\":\"llama3.2\",\"prompt\":\"%s\",\"stream\":false}",
        escaped_prompt);
    
    size_t json_len = strlen(json_body);
    
    char request[8192];
    snprintf(request, sizeof(request),
        "POST /api/generate HTTP/1.1\r\n"
        "Host: %s:%d\r\n"
        "Content-Type: application/json\r\n"
        "Content-Length: %zu\r\n"
        "\r\n"
        "%s",
        OLLAMA_HOST, OLLAMA_PORT,
        json_len,
        json_body);
    
    char response[32768] = {0};
    
#ifdef _WIN32
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        return NULL;
    }
    
    SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == INVALID_SOCKET) {
        WSACleanup();
        return NULL;
    }
    
    struct sockaddr_in server;
    server.sin_family = AF_INET;
    server.sin_port = htons(OLLAMA_PORT);
    inet_pton(AF_INET, "127.0.0.1", &server.sin_addr);
    
    if (connect(sock, (struct sockaddr*)&server, sizeof(server)) == SOCKET_ERROR) {
        closesocket(sock);
        WSACleanup();
        return NULL;
    }
    
    send(sock, request, (int)strlen(request), 0);
    int bytes_received = recv(sock, response, sizeof(response) - 1, 0);
    closesocket(sock);
    WSACleanup();
    
    if (bytes_received <= 0) {
        return NULL;
    }
    response[bytes_received] = '\0';
#else
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) return NULL;
    
    struct sockaddr_in server;
    server.sin_family = AF_INET;
    server.sin_port = htons(OLLAMA_PORT);
    inet_pton(AF_INET, "127.0.0.1", &server.sin_addr);
    
    if (connect(sock, (struct sockaddr*)&server, sizeof(server)) < 0) {
        close(sock);
        return NULL;
    }
    
    send(sock, request, strlen(request), 0);
    int bytes_received = recv(sock, response, sizeof(response) - 1, 0);
    close(sock);
    
    if (bytes_received <= 0) {
        return NULL;
    }
    response[bytes_received] = '\0';
#endif
    
    /* Extract JSON response (simple parsing - find "response":"..." ) */
    char *response_start = strstr(response, "\"response\":\"");
    if (!response_start) {
        return NULL;
    }
    
    response_start += 12;  /* Skip "response":" */
    char *response_end = strchr(response_start, '"');
    if (!response_end) {
        return NULL;
    }
    
    size_t len = response_end - response_start;
    char *result = malloc(len + 1);
    if (!result) return NULL;
    
    memcpy(result, response_start, len);
    result[len] = '\0';
    
    /* Unescape JSON strings (\n, \", etc.) */
    char *out = result;
    for (const char *in = result; *in; in++) {
        if (*in == '\\' && in[1] == 'n') {
            *out++ = '\n';
            in++;
        } else if (*in == '\\' && in[1] == '"') {
            *out++ = '"';
            in++;
        } else {
            *out++ = *in;
        }
    }
    *out = '\0';
    
    return result;
}

int main(void) {
    printf("=================================================================\n");
    printf("Melvin-Ollama Automated Training\n");
    printf("Ollama (llama3.2) <-> Melvin o7 Training Loop\n");
    printf("=================================================================\n\n");
    
    /* Create Melvin graph */
    MelvinGraph *g = melvin_create();
    if (!g) {
        printf("ERROR: Failed to create Melvin graph\n");
        return 1;
    }
    
    /* Try to load existing brain */
    MelvinGraph *loaded = melvin_load_brain("melvin_brain.m");
    if (loaded) {
        melvin_destroy(g);
        g = loaded;
        printf("✓ Loaded existing brain from melvin_brain.m\n");
    } else {
        printf("✓ Starting with fresh brain\n");
    }
    
    printf("\nStarting automated training loop...\n");
    printf("Training examples will cycle through...\n\n");
    
    /* Training prompts */
    const char *training_prompts[] = {
        "Hello",
        "What is a cat?",
        "Count to five",
        "Say hello world",
        "What is learning?",
        "Explain patterns",
        "Describe intelligence",
        NULL
    };
    
    int iteration = 0;
    int max_iterations = 20;  /* Train for 20 iterations */
    
    for (int i = 0; i < max_iterations; i++) {
        iteration++;
        const char *prompt = training_prompts[i % 7];  /* Cycle through prompts */
        
        printf("--- Iteration %d/%d ---\n", iteration, max_iterations);
        printf("Prompt: %s\n", prompt);
        
        /* Step 1: Send to Ollama, get output */
        printf("[Ollama] Generating...\n");
        char *ollama_output = ollama_generate(prompt);
        if (!ollama_output) {
            printf("ERROR: Failed to get response from Ollama\n");
            continue;
        }
        
        printf("[Ollama] Output: %.100s%s\n", ollama_output, 
               strlen(ollama_output) > 100 ? "..." : "");
        
        /* Step 2: Feed Ollama output to Melvin (as byte input) */
        printf("[Melvin] Processing...\n");
        uint8_t *bytes = (uint8_t*)ollama_output;
        uint32_t len = (uint32_t)strlen(ollama_output);
        
        /* Inject into Melvin with port 0 (text) */
        inject_input_from_port(g, bytes, len, 0);
        
        /* Run Melvin episode (generate output) */
        run_episode(g, bytes, len, NULL, 0);
        
        /* Get Melvin's output */
        uint32_t *melvin_output;
        uint32_t melvin_output_len;
        melvin_get_output(g, &melvin_output, &melvin_output_len);
        
        printf("[Melvin] Generated %u bytes\n", melvin_output_len);
        
        /* Step 3: Send Melvin output back to Ollama (optional - for feedback loop) */
        if (melvin_output_len > 0 && i % 3 == 0) {  /* Every 3rd iteration */
            /* Convert Melvin output to string (simplified) */
            char melvin_str[512] = {0};
            for (uint32_t j = 0; j < melvin_output_len && j < sizeof(melvin_str) - 1; j++) {
                if (melvin_output[j] >= 32 && melvin_output[j] <= 126) {
                    melvin_str[j] = (char)melvin_output[j];
                } else {
                    melvin_str[j] = '?';
                }
            }
            
            printf("[Ollama] Receiving Melvin feedback...\n");
            char *ollama_response = ollama_generate(melvin_str);
            if (ollama_response) {
                printf("[Ollama] Feedback: %.100s%s\n", ollama_response,
                       strlen(ollama_response) > 100 ? "..." : "");
            }
            free(ollama_response);
        }
        
        free(ollama_output);
        
        /* Save brain every 5 iterations */
        if (iteration % 5 == 0) {
            melvin_save_brain(g, "melvin_brain.m");
            printf("[Melvin] Brain saved (iteration %d)\n", iteration);
        }
        
        printf("\n");
        
        /* Small delay to avoid overwhelming */
        #ifdef _WIN32
        Sleep(1000);  /* 1 second */
        #else
        sleep(1);
        #endif
    }
    
    /* Final save */
    melvin_save_brain(g, "melvin_brain.m");
    printf("=================================================================\n");
    printf("Training complete!\n");
    printf("Brain saved to melvin_brain.m\n");
    printf("Total iterations: %d\n", iteration);
    printf("=================================================================\n");
    
    melvin_destroy(g);
    return 0;
}

