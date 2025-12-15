/* ============================================================================
 * MELVIN WASM BRIDGE
 * 
 * JavaScript interface for Melvin compiled to WebAssembly
 * ============================================================================ */

#include <emscripten.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

/* Include melvin - we'll compile melvin.c with this */
typedef struct MelvinGraph MelvinGraph;
extern MelvinGraph* melvin_create(void);
extern void melvin_destroy(MelvinGraph *g);
extern void run_episode(MelvinGraph *g, const uint8_t *input, uint32_t input_len,
                       const uint8_t *target, uint32_t target_len);
extern void melvin_get_output(MelvinGraph *g, uint32_t **output, uint32_t *length);
extern float melvin_get_error_rate(MelvinGraph *g);
extern int melvin_save_brain(MelvinGraph *g, const char *filename);
extern MelvinGraph* melvin_load_brain(const char *filename);

/* Global instance */
static MelvinGraph *g_melvin = NULL;

/* Initialize Melvin */
EMSCRIPTEN_KEEPALIVE
int melvin_init(void) {
    if (g_melvin) {
        melvin_destroy(g_melvin);
    }
    g_melvin = melvin_create();
    return g_melvin != NULL ? 1 : 0;
}

/* Process a message and return output */
EMSCRIPTEN_KEEPALIVE
char* melvin_process_message(const char *input) {
    if (!g_melvin) {
        if (!melvin_init()) {
            return NULL;
        }
    }
    
    if (!input) {
        return NULL;
    }
    
    uint32_t input_len = strlen(input);
    if (input_len == 0) {
        return NULL;
    }
    
    /* Run episode */
    run_episode(g_melvin, (const uint8_t*)input, input_len, NULL, 0);
    
    /* Get output */
    uint32_t *output;
    uint32_t output_len;
    melvin_get_output(g_melvin, &output, &output_len);
    
    /* Convert to string */
    char *result = (char*)malloc(output_len + 1);
    if (!result) {
        return NULL;
    }
    
    for (uint32_t i = 0; i < output_len; i++) {
        if (output[i] < 256) {
            result[i] = (char)output[i];
        } else {
            result[i] = '?';
        }
    }
    result[output_len] = '\0';
    
    return result;
}

/* Get error rate */
EMSCRIPTEN_KEEPALIVE
float melvin_get_error(void) {
    if (!g_melvin) {
        return 0.0f;
    }
    return melvin_get_error_rate(g_melvin);
}

/* Free string returned by melvin_process_message */
EMSCRIPTEN_KEEPALIVE
void melvin_free_string(char *str) {
    if (str) {
        free(str);
    }
}

/* Cleanup */
EMSCRIPTEN_KEEPALIVE
void melvin_cleanup(void) {
    if (g_melvin) {
        melvin_destroy(g_melvin);
        g_melvin = NULL;
    }
}
