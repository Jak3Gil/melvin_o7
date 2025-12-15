/* ============================================================================
 * PATTERN DEBUG: Why aren't patterns predicting correctly?
 * ============================================================================ */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

typedef struct MelvinGraph MelvinGraph;
extern MelvinGraph* melvin_create(void);
extern void run_episode(MelvinGraph *g, const uint8_t *input, uint32_t input_len,
                       const uint8_t *target, uint32_t target_len);
extern void melvin_get_output(MelvinGraph *g, uint32_t **output, uint32_t *length);

/* Access pattern internals */
typedef struct {
    uint32_t *node_ids;
    uint32_t length;
    float strength;
    float activation;
    uint32_t *predicted_nodes;
    float *prediction_weights;
    uint32_t prediction_count;
    bool has_fired;
} PatternDebug;

extern uint32_t melvin_get_pattern_count(MelvinGraph *g);
extern void melvin_debug_pattern(MelvinGraph *g, uint32_t p, PatternDebug *out);

/* Simplified accessor - we'll use a hack to access internals */
int main(void) {
    printf("=================================================================\n");
    printf("PATTERN DEBUG: Trace Pattern Learning\n");
    printf("=================================================================\n\n");
    
    MelvinGraph *g = melvin_create();
    if (!g) {
        fprintf(stderr, "Failed to create graph\n");
        return 1;
    }
    
    uint8_t input[] = {'c', 'a', 't'};
    uint8_t target[] = {'c', 'a', 't', 's'};
    
    printf("Training 'cat' → 'cats' for 5 episodes...\n\n");
    
    for (int ep = 0; ep < 5; ep++) {
        printf("=== EPISODE %d ===\n", ep + 1);
        run_episode(g, input, 3, target, 4);
        
        uint32_t *output;
        uint32_t output_len;
        melvin_get_output(g, &output, &output_length);
        
        printf("Input:  cat\n");
        printf("Target: cats\n");
        printf("Output: ");
        for (uint32_t i = 0; i < output_len && i < 10; i++) {
            printf("%c", (uint8_t)output[i]);
        }
        printf("\n\n");
        
        /* Try to access pattern info - we'll need to add accessor functions */
        printf("Patterns: %u\n", melvin_get_pattern_count(g));
        printf("---\n\n");
    }
    
    return 0;
}

