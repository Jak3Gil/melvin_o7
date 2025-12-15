/* ============================================================================
 * UTILITY TRACKING TEST: Show patterns competing based on actual performance
 * ============================================================================ */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

typedef struct MelvinGraph MelvinGraph;
extern MelvinGraph* melvin_create(void);
extern void run_episode(MelvinGraph *g, const uint8_t *input, uint32_t input_len,
                       const uint8_t *target, uint32_t target_len);
extern uint32_t melvin_get_pattern_count(MelvinGraph *g);
extern void melvin_get_pattern_info(MelvinGraph *g, uint32_t pattern_id, 
                                    uint32_t **node_ids, uint32_t *length, float *strength);
extern void melvin_get_pattern_predictions(MelvinGraph *g, uint32_t pattern_id,
                                           uint32_t **predicted_nodes, float **prediction_weights,
                                           uint32_t *prediction_count);

int main(void) {
    printf("=================================================================\n");
    printf("UTILITY TRACKING: Patterns Compete Based on Performance\n");
    printf("=================================================================\n\n");
    
    MelvinGraph *g = melvin_create();
    if (!g) {
        fprintf(stderr, "Failed to create graph\n");
        return 1;
    }
    
    uint8_t input[] = {'c', 'a', 't'};
    uint8_t target[] = {'c', 'a', 't', 's'};
    
    printf("Training 'cat' → 'cats' over 30 episodes\n");
    printf("Tracking pattern '_at' strength and predictions...\n\n");
    
    printf("Episode | Pattern '_at' Strength | Predicts 's'? | Prediction Weight\n");
    printf("--------|------------------------|---------------|------------------\n");
    
    uint32_t at_pattern_id = 0;
    bool found_at = false;
    
    for (int ep = 0; ep < 30; ep++) {
        run_episode(g, input, 3, target, 4);
        
        /* Find pattern '_at' */
        if (!found_at || ep % 5 == 0) {
            uint32_t pattern_count = melvin_get_pattern_count(g);
            for (uint32_t p = 0; p < pattern_count; p++) {
                uint32_t *node_ids;
                uint32_t length;
                float strength;
                melvin_get_pattern_info(g, p, &node_ids, &length, &strength);
                
                if (length == 3 && node_ids[0] == 256 && node_ids[1] == 'a' && node_ids[2] == 't') {
                    at_pattern_id = p;
                    found_at = true;
                    
                    /* Check predictions */
                    uint32_t *predicted_nodes;
                    float *prediction_weights;
                    uint32_t prediction_count;
                    melvin_get_pattern_predictions(g, p, &predicted_nodes, &prediction_weights, &prediction_count);
                    
                    bool predicts_s = false;
                    float s_weight = 0.0f;
                    for (uint32_t i = 0; i < prediction_count; i++) {
                        if (predicted_nodes[i] == 's') {
                            predicts_s = true;
                            s_weight = prediction_weights[i];
                            break;
                        }
                    }
                    
                    printf("  %2d    |      %.6f        |      %s      |      %.2f\n",
                           ep + 1, strength, predicts_s ? "YES" : "NO ", s_weight);
                    break;
                }
            }
        }
    }
    
    printf("\n=== ANALYSIS ===\n");
    if (found_at) {
        uint32_t *node_ids;
        uint32_t length;
        float final_strength;
        melvin_get_pattern_info(g, at_pattern_id, &node_ids, &length, &final_strength);
        
        uint32_t *predicted_nodes;
        float *prediction_weights;
        uint32_t prediction_count;
        melvin_get_pattern_predictions(g, at_pattern_id, &predicted_nodes, &prediction_weights, &prediction_count);
        
        bool predicts_s = false;
        float s_weight = 0.0f;
        for (uint32_t i = 0; i < prediction_count; i++) {
            if (predicted_nodes[i] == 's') {
                predicts_s = true;
                s_weight = prediction_weights[i];
                break;
            }
        }
        
        printf("Pattern '_at' final state:\n");
        printf("  Strength: %.6f\n", final_strength);
        printf("  Predicts 's': %s\n", predicts_s ? "YES" : "NO");
        if (predicts_s) {
            printf("  Prediction weight: %.2f\n", s_weight);
            printf("  ✓ Pattern learned to predict 's' correctly\n");
        }
        
        if (final_strength > 0.0001f) {
            printf("  ✓ Pattern survived competition (strength > 0)\n");
        } else {
            printf("  ~ Pattern strength very low (may be pruned soon)\n");
        }
    } else {
        printf("✗ Pattern '_at' not found\n");
    }
    
    printf("\n=== COMPRESSION TEST ===\n");
    printf("Total patterns created: %u\n", melvin_get_pattern_count(g));
    printf("Patterns represent compression of repeated sequences\n");
    
    return 0;
}

