/* Debug why outputs are empty */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

typedef struct MelvinGraph MelvinGraph;
extern MelvinGraph* melvin_create(void);
extern void run_episode(MelvinGraph *g, const uint8_t *input, uint32_t input_len,
                       const uint8_t *target, uint32_t target_len);
extern void melvin_get_output(MelvinGraph *g, uint32_t **output, uint32_t *length);
extern void melvin_destroy(MelvinGraph *g);

/* Access internal state for debugging */
typedef struct {
    float activation[256];
    bool exists[256];
} NodeDebug;

extern void debug_get_node_activations(MelvinGraph *g, float *activations);
extern uint32_t debug_get_pattern_count(MelvinGraph *g);

int main(void) {
    MelvinGraph *g = melvin_create();
    
    printf("=================================================================\n");
    printf("DEBUG: Why are outputs empty?\n");
    printf("=================================================================\n\n");
    
    printf("Step 1: Training 'cat' → 'cat' (5 episodes)...\n");
    for (int i = 0; i < 5; i++) {
        run_episode(g, (const uint8_t*)"cat", 3, (const uint8_t*)"cat", 3);
    }
    
    printf("\nStep 2: Check activations before test...\n");
    float activations[256];
    for (int i = 0; i < 256; i++) activations[i] = 0.0f;
    
    /* Try to get activations - if function doesn't exist, we'll see */
    printf("Checking node activations for 'c', 'a', 't'...\n");
    printf("  (We can't access internals directly, but we'll check output)\n\n");
    
    printf("Step 3: Test input 'cat'...\n");
    run_episode(g, (const uint8_t*)"cat", 3, NULL, 0);
    
    uint32_t *output;
    uint32_t output_len;
    melvin_get_output(g, &output, &output_len);
    
    printf("Output length: %u\n", output_len);
    if (output_len > 0) {
        printf("Output: ");
        for (uint32_t i = 0; i < output_len && i < 50; i++) {
            printf("%c", (uint8_t)output[i]);
        }
        printf("\n");
    } else {
        printf("ERROR: Output is empty!\n");
        printf("This means select_output_node returned an invalid node or no node passed quality threshold.\n");
    }
    
    melvin_destroy(g);
    return 0;
}
