#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

/* Forward declarations */
typedef struct MelvinGraph MelvinGraph;
extern MelvinGraph* melvin_create(void);
extern void melvin_destroy(MelvinGraph *g);
extern void run_episode(MelvinGraph *g, const uint8_t *input, uint32_t input_len,
                       const uint8_t *target, uint32_t target_len);
extern void melvin_get_output(MelvinGraph *g, uint32_t **output, uint32_t *length);

/* Access pattern count and node activation (we'll need to expose these) */
/* For now, let's just trace what happens */

void print_activation_trace(MelvinGraph *g, const char *label) {
    printf("\n=== %s ===\n", label);
    
    /* Get output to see what was selected */
    uint32_t *output;
    uint32_t output_len;
    melvin_get_output(g, &output, &output_len);
    
    printf("Output length: %u\n", output_len);
    if (output_len > 0) {
        printf("Output: \"");
        for (uint32_t i = 0; i < output_len && i < 20; i++) {
            if (output[i] < 256) {
                printf("%c", (char)output[i]);
            } else {
                printf("[%u]", output[i]);
            }
        }
        printf("\"\n");
    }
}

int main(void) {
    printf("========================================\n");
    printf("ACTIVATION DEBUG TEST\n");
    printf("========================================\n\n");
    
    MelvinGraph *g = melvin_create();
    if (!g) {
        fprintf(stderr, "Failed to create graph\n");
        return 1;
    }
    
    /* Simple test: train "hello" -> "world" */
    printf("Training 'hello' -> 'world' (5 times)...\n");
    for (int i = 0; i < 5; i++) {
        run_episode(g, (const uint8_t*)"hello", 5, (const uint8_t*)"world", 5);
        printf("  Episode %d complete\n", i + 1);
    }
    
    print_activation_trace(g, "After Training");
    
    /* Now test */
    printf("\nTesting 'hello' -> should output 'world':\n");
    run_episode(g, (const uint8_t*)"hello", 5, NULL, 0);
    
    print_activation_trace(g, "After Test");
    
    melvin_destroy(g);
    return 0;
}
