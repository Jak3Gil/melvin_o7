/* ============================================================================
 * DIAGNOSE: Why aren't outputs intelligent?
 * Check what patterns learned and how they activate
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
extern uint32_t melvin_get_pattern_count(MelvinGraph *g);
extern void melvin_get_pattern_info(MelvinGraph *g, uint32_t pattern_id, 
                                    uint32_t **node_ids, uint32_t *length, float *strength);
extern void melvin_get_pattern_predictions(MelvinGraph *g, uint32_t pattern_id,
                                           uint32_t **predicted_nodes, float **prediction_weights,
                                           uint32_t *prediction_count);

int main(void) {
    MelvinGraph *g = melvin_create();
    if (!g) {
        fprintf(stderr, "Failed to create graph\n");
        return 1;
    }
    
    printf("Training on 'cat' → 'cats' (30 episodes)...\n");
    for (int i = 0; i < 30; i++) {
        run_episode(g, (uint8_t*)"cat", 3, (uint8_t*)"cats", 4);
    }
    
    printf("\n=== PATTERNS CREATED ===\n");
    uint32_t pattern_count = melvin_get_pattern_count(g);
    printf("Total patterns: %u\n\n", pattern_count);
    
    /* Find pattern '_at' */
    for (uint32_t p = 0; p < pattern_count; p++) {
        uint32_t *node_ids;
        uint32_t length;
        float strength;
        melvin_get_pattern_info(g, p, &node_ids, &length, &strength);
        
        if (strength > 0.001f) {
            printf("Pattern %u: ", p);
            for (uint32_t i = 0; i < length; i++) {
                if (node_ids[i] == 256) {
                    printf("_");
                } else {
                    printf("%c", (uint8_t)node_ids[i]);
                }
            }
            printf(" (strength=%.4f)\n", strength);
            
            /* Check predictions */
            uint32_t *predicted_nodes;
            float *prediction_weights;
            uint32_t prediction_count;
            melvin_get_pattern_predictions(g, p, &predicted_nodes, &prediction_weights, &prediction_count);
            
            if (prediction_count > 0) {
                printf("  → predicts: ");
                for (uint32_t i = 0; i < prediction_count && i < 5; i++) {
                    printf("'%c'(%.2f) ", (uint8_t)predicted_nodes[i], prediction_weights[i]);
                }
                printf("\n");
            }
        }
    }
    
    printf("\n=== TESTING GENERALIZATION ===\n");
    printf("Input: 'bat' (never seen before)\n");
    printf("Expected: Pattern '_at' should match and predict 's'\n\n");
    
    run_episode(g, (uint8_t*)"bat", 3, NULL, 0);
    
    uint32_t *output;
    uint32_t output_len;
    melvin_get_output(g, &output, &output_len);
    
    printf("Output: ");
    for (uint32_t i = 0; i < output_len && i < 20; i++) {
        if (output[i] < 256) {
            printf("%c", (uint8_t)output[i]);
        }
    }
    printf("\n");
    
    /* Check if output ends with 's' */
    if (output_len > 0 && output[output_len - 1] == 's') {
        printf("✓ Output ends with 's' - INTELLIGENT!\n");
    } else {
        printf("✗ Output does not end with 's' - not generalizing\n");
    }
    
    return 0;
}

