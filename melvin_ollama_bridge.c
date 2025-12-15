/* Melvin-Ollama Bridge
 * 
 * Connects two separate systems:
 * - Ollama (llama3) output → Melvin input
 * - Melvin output → Ollama input
 * 
 * No changes to either system - just a pipe/bridge
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <signal.h>
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

/* Global for signal handler */
#ifndef _WIN32
MelvinGraph *g_global = NULL;
#endif

/* Signal handler for graceful shutdown on Ctrl+C */
#ifdef _WIN32
BOOL WINAPI CtrlHandler(DWORD dwType) {
    if (dwType == CTRL_C_EVENT) {
        printf("\n\n[Interrupt] Received Ctrl+C - Exiting gracefully...\n");
        fflush(stdout);
        return TRUE;
    }
    return FALSE;
}
#else
#include <signal.h>
void signal_handler(int sig) {
    if (sig == SIGINT && g_global) {
        printf("\n\n[Interrupt] Received SIGINT - Saving brain...\n");
        fflush(stdout);
        melvin_save_brain(g_global, "melvin_brain.m");
        melvin_destroy(g_global);
        printf("Brain saved. Exiting.\n");
        exit(0);
    }
}
#endif

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
    printf("Melvin-Ollama Bridge\n");
    printf("Connecting: Ollama (llama3.2) <-> Melvin o7\n");
    printf("=================================================================\n\n");
    
    /* Set up signal handler for graceful shutdown */
#ifdef _WIN32
    SetConsoleCtrlHandler(CtrlHandler, TRUE);
#endif
    
    /* Create Melvin graph */
    MelvinGraph *g = melvin_create();
    if (!g) {
        printf("ERROR: Failed to create Melvin graph\n");
        return 1;
    }
    
#ifndef _WIN32
    g_global = g;  /* For signal handler */
    signal(SIGINT, signal_handler);
#endif
    
    /* Try to load existing brain */
    MelvinGraph *loaded = melvin_load_brain("melvin_brain.m");
    if (loaded) {
        melvin_destroy(g);
        g = loaded;
#ifndef _WIN32
        g_global = g;
#endif
        printf("Loaded existing brain from melvin_brain.m\n");
    } else {
        printf("Starting with fresh brain (melvin_brain.m will be created)\n");
    }
    fflush(stdout);
    
    printf("\nBridge ready - Starting CONTINUOUS training loop!\n");
    printf("Training: Ollama -> Melvin -> Ollama (continuous learning)\n");
    printf("Press Ctrl+C to stop and save brain\n\n");
    fflush(stdout);
    
    /* Training prompts - cycle through these */
    const char *training_prompts[] = {
        "Say hello",
        "What is 2+2?",
        "Write a simple sentence",
        "Count to three",
        "Tell me a fact",
        "What is the weather?",
        "Explain simply",
        "Give an example"
    };
    int num_prompts = sizeof(training_prompts) / sizeof(training_prompts[0]);
    int round = 0;
    
    /* Continuous training loop - runs until Ctrl+C */
    while (1) {
        round++;
        
        /* Select prompt (cycle through array) */
        const char *input = training_prompts[round % num_prompts];
        
        /* Print round info with timestamp-like format */
        printf("[Round %d] [Input] %s\n", round, input);
        fflush(stdout);
        
        /* Step 1: Send to Ollama, get output */
        printf("  [Ollama] Processing... ");
        fflush(stdout);
        char *ollama_output = ollama_generate(input);
        if (!ollama_output) {
            printf("ERROR: Failed to get response from Ollama\n");
            fflush(stdout);
            continue;
        }
        
        /* Show first 100 chars of Ollama output */
        int output_preview_len = strlen(ollama_output);
        if (output_preview_len > 100) output_preview_len = 100;
        printf("Output: %.100s%s\n", ollama_output, (strlen(ollama_output) > 100) ? "..." : "");
        fflush(stdout);
        
        /* Step 2: Feed Ollama output to Melvin (as byte input) */
        printf("  [Melvin] Processing (%u bytes)... ", (unsigned int)strlen(ollama_output));
        fflush(stdout);
        uint8_t *bytes = (uint8_t*)ollama_output;
        uint32_t len = (uint32_t)strlen(ollama_output);
        
        if (len == 0) {
            printf("ERROR: Empty input\n");
            fflush(stdout);
            continue;
        }
        
        /* Run Melvin episode - generate output (no target yet, we'll train with Ollama feedback) */
        run_episode(g, bytes, len, NULL, 0);
        
        /* Get Melvin's output */
        uint32_t *melvin_output;
        uint32_t melvin_output_len;
        melvin_get_output(g, &melvin_output, &melvin_output_len);
        
        /* Show preview of Melvin output */
        printf("Generated %u bytes", melvin_output_len);
        if (melvin_output_len == 0) {
            printf(" (no output generated - graph may need more learning)");
        }
        printf(": ");
        for (uint32_t i = 0; i < melvin_output_len && i < 80; i++) {
            if (melvin_output[i] < 32 || melvin_output[i] > 126) {
                printf("[%02x]", melvin_output[i]);
            } else {
                printf("%c", (char)melvin_output[i]);
            }
        }
        if (melvin_output_len > 80) printf("...");
        printf("\n");
        fflush(stdout);
        
        /* Step 3: Send Melvin output back to Ollama */
        if (melvin_output_len > 0) {
            /* Convert Melvin output to string */
            char melvin_str[2048] = {0};
            for (uint32_t i = 0; i < melvin_output_len && i < sizeof(melvin_str) - 1; i++) {
                if (melvin_output[i] < 32 || melvin_output[i] > 126) {
                    melvin_str[i] = '?';  /* Replace non-printable */
                } else {
                    melvin_str[i] = (char)melvin_output[i];
                }
            }
            
            printf("  [Ollama] Receiving Melvin output... ");
            fflush(stdout);
            char *ollama_response = ollama_generate(melvin_str);
            if (ollama_response) {
                int response_preview = strlen(ollama_response);
                if (response_preview > 80) response_preview = 80;
                printf("Response: %.80s%s\n", ollama_response, (strlen(ollama_response) > 80) ? "..." : "");
                fflush(stdout);
                
                /* CRITICAL: Use Ollama's response as training target for Melvin */
                /* This is how Melvin learns from Ollama's feedback */
                printf("  [Melvin] Training on Ollama feedback... ");
                fflush(stdout);
                
                uint8_t *target_bytes = (uint8_t*)ollama_response;
                uint32_t target_len = (uint32_t)strlen(ollama_response);
                
                /* Run training episode with Ollama's response as target */
                run_episode(g, bytes, len, target_bytes, target_len);
                
                printf("Training complete\n");
                fflush(stdout);
                
                free(ollama_response);
            } else {
                printf("No response\n");
                fflush(stdout);
            }
        }
        
        free(ollama_output);
        
        /* Save brain periodically (every 10 rounds to reduce I/O, but always save) */
        if (round % 10 == 0) {
            printf("  [Save] Brain saved to melvin_brain.m (Round %d)\n", round);
            fflush(stdout);
        }
        melvin_save_brain(g, "melvin_brain.m");
        
        /* Small delay between rounds (reduced for faster training) */
        #ifdef _WIN32
        Sleep(500);  /* 0.5 second */
        #else
        usleep(500000);  /* 0.5 second */
        #endif
    }
    
    /* Final save (if we somehow exit the loop) */
    melvin_save_brain(g, "melvin_brain.m");
    printf("\n[Final] Brain saved. Exiting.\n");
    melvin_destroy(g);
    
    return 0;
    
    /* This code won't normally be reached due to infinite loop */
    /* But handle Ctrl+C gracefully */
    
    return 0;
}

