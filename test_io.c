/* ============================================================================
 * SIMPLE I/O TEST: Feed inputs from .m file, read outputs
 * That's it - nothing else
 * ============================================================================ */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

typedef struct MelvinGraph MelvinGraph;
extern MelvinGraph* melvin_create(void);
extern void run_episode(MelvinGraph *g, const uint8_t *input, uint32_t input_len,
                       const uint8_t *target, uint32_t target_len);
extern void melvin_get_output(MelvinGraph *g, uint32_t **output, uint32_t *length);

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <input.m>\n", argv[0]);
        return 1;
    }
    
    FILE *f = fopen(argv[1], "rb");
    if (!f) {
        fprintf(stderr, "Failed to open %s\n", argv[1]);
        return 1;
    }
    
    /* Read entire file as byte sequence */
    fseek(f, 0, SEEK_END);
    long file_size = ftell(f);
    fseek(f, 0, SEEK_SET);
    
    uint8_t *input_bytes = malloc(file_size);
    if (!input_bytes) {
        fprintf(stderr, "Failed to allocate memory\n");
        fclose(f);
        return 1;
    }
    
    size_t bytes_read = fread(input_bytes, 1, file_size, f);
    fclose(f);
    
    if (bytes_read == 0) {
        fprintf(stderr, "File is empty\n");
        free(input_bytes);
        return 1;
    }
    
    /* Create system */
    MelvinGraph *g = melvin_create();
    if (!g) {
        fprintf(stderr, "Failed to create graph\n");
        free(input_bytes);
        return 1;
    }
    
    /* Feed input (no target - just inference) */
    run_episode(g, input_bytes, bytes_read, NULL, 0);
    
    /* Read output */
    uint32_t *output;
    uint32_t output_length;
    melvin_get_output(g, &output, &output_length);
    
    /* Write output as bytes to stdout - pure I/O, nothing else */
    for (uint32_t i = 0; i < output_length; i++) {
        if (output[i] < 256) {
            putchar((uint8_t)output[i]);
        }
    }
    
    free(input_bytes);
    return 0;
}

