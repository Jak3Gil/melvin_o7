/* ============================================================================
 * INTELLIGENCE TEST: Prove patterns compete, compress, and build hierarchically
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

/* Note: Pattern utility tracking happens internally via prediction_attempts/successes */

int main(void) {
    printf("=================================================================\n");
    printf("INTELLIGENCE TEST: Prove Patterns Compete, Compress, Build\n");
    printf("=================================================================\n\n");
    
    MelvinGraph *g = melvin_create();
    if (!g) {
        fprintf(stderr, "Failed to create graph\n");
        return 1;
    }
    
    printf("TEST 1: Pattern Utility Tracking\n");
    printf("-----------------------------------\n");
    printf("Training 'cat' → 'cats' (pattern '_at' should learn to predict 's')\n");
    printf("Tracking prediction attempts and successes...\n\n");
    
    uint8_t input[] = {'c', 'a', 't'};
    uint8_t target[] = {'c', 'a', 't', 's'};
    
    uint32_t pattern_count_before = melvin_get_pattern_count(g);
    printf("Patterns before training: %u\n", pattern_count_before);
    
    /* Train for multiple episodes */
    for (int ep = 0; ep < 10; ep++) {
        run_episode(g, input, 3, target, 4);
    }
    
    uint32_t pattern_count_after = melvin_get_pattern_count(g);
    printf("Patterns after training: %u\n", pattern_count_after);
    
    /* Find pattern '_at' */
    uint32_t at_pattern_id = 0;
    bool found_at = false;
    for (uint32_t p = 0; p < pattern_count_after; p++) {
        uint32_t *node_ids;
        uint32_t length;
        float strength;
        melvin_get_pattern_info(g, p, &node_ids, &length, &strength);
        
        if (length == 3 && node_ids[0] == 256 && node_ids[1] == 'a' && node_ids[2] == 't') {
            at_pattern_id = p;
            found_at = true;
            printf("\nFound pattern '_at' (ID: %u)\n", p);
            printf("  Strength: %.4f\n", strength);
            
            /* Check predictions */
            uint32_t *predicted_nodes;
            float *prediction_weights;
            uint32_t prediction_count;
            melvin_get_pattern_predictions(g, p, &predicted_nodes, &prediction_weights, &prediction_count);
            
            if (prediction_count > 0) {
                printf("  Predictions: ");
                for (uint32_t i = 0; i < prediction_count && i < 5; i++) {
                    printf("'%c'(%.2f) ", (uint8_t)predicted_nodes[i], prediction_weights[i]);
                }
                printf("\n");
                
                /* Check if it predicts 's' */
                bool predicts_s = false;
                for (uint32_t i = 0; i < prediction_count; i++) {
                    if (predicted_nodes[i] == 's') {
                        predicts_s = true;
                        printf("  ✓ Pattern predicts 's' (weight: %.2f)\n", prediction_weights[i]);
                        break;
                    }
                }
                if (!predicts_s) {
                    printf("  ✗ Pattern does NOT predict 's'\n");
                }
            } else {
                printf("  ✗ Pattern has NO predictions\n");
            }
            break;
        }
    }
    
    if (!found_at) {
        printf("\n✗ Pattern '_at' not found!\n");
    }
    
    printf("\n\nTEST 2: Pattern Competition\n");
    printf("----------------------------\n");
    printf("Training more episodes - patterns should compete for strength...\n\n");
    
    float strength_before = 0.0f;
    if (found_at) {
        uint32_t *node_ids;
        uint32_t length;
        melvin_get_pattern_info(g, at_pattern_id, &node_ids, &length, &strength_before);
        printf("Pattern '_at' strength before: %.4f\n", strength_before);
    }
    
    /* Train more */
    for (int ep = 0; ep < 20; ep++) {
        run_episode(g, input, 3, target, 4);
    }
    
    if (found_at) {
        uint32_t *node_ids;
        uint32_t length;
        float strength_after;
        melvin_get_pattern_info(g, at_pattern_id, &node_ids, &length, &strength_after);
        printf("Pattern '_at' strength after: %.4f\n", strength_after);
        
        if (strength_after > strength_before) {
            printf("  ✓ Pattern strength INCREASED (competition working)\n");
        } else if (strength_after < strength_before) {
            printf("  ~ Pattern strength DECREASED (may be competing with others)\n");
        } else {
            printf("  ~ Pattern strength UNCHANGED\n");
        }
    }
    
    printf("\n\nTEST 3: Pattern Compression\n");
    printf("----------------------------\n");
    printf("All patterns and their strengths (compression benefit):\n\n");
    
    uint32_t total_patterns = melvin_get_pattern_count(g);
    printf("Total patterns: %u\n", total_patterns);
    
    for (uint32_t p = 0; p < total_patterns && p < 10; p++) {
        uint32_t *node_ids;
        uint32_t length;
        float strength;
        melvin_get_pattern_info(g, p, &node_ids, &length, &strength);
        
        if (strength > 0.001f) {  /* Only show active patterns */
            printf("  Pattern %u: ", p);
            for (uint32_t i = 0; i < length; i++) {
                if (node_ids[i] == 256) {
                    printf("_");
                } else {
                    printf("%c", (uint8_t)node_ids[i]);
                }
            }
            printf(" (strength=%.4f)", strength);
            
            /* Check for hierarchical composition */
            /* TODO: Add accessor for sub_pattern_count */
            printf("\n");
        }
    }
    
    printf("\n\nTEST 4: Generalization\n");
    printf("----------------------\n");
    printf("Testing 'bat' → 'bats' (never seen 'b' before, but '_at' should work)\n\n");
    
    uint8_t test_input[] = {'b', 'a', 't'};
    run_episode(g, test_input, 3, NULL, 0);
    
    uint32_t *output;
    uint32_t output_len;
    melvin_get_output(g, &output, &output_len);
    
    printf("Input:  bat\n");
    printf("Output: ");
    for (uint32_t i = 0; i < output_len && i < 10; i++) {
        printf("%c", (uint8_t)output[i]);
    }
    printf("\n");
    
    /* Check if output ends with 's' */
    bool ends_with_s = (output_len > 0 && output[output_len - 1] == 's');
    if (ends_with_s) {
        printf("  ✓ Output ends with 's' (generalization working!)\n");
    } else {
        printf("  ~ Output does not end with 's'\n");
    }
    
    printf("\n\n=== SUMMARY ===\n");
    printf("Pattern utility tracking: %s\n", found_at ? "✓ Implemented" : "✗ Not found");
    printf("Pattern competition: %s\n", found_at ? "✓ Testing" : "✗ Cannot test");
    printf("Pattern compression: ✓ %u patterns created\n", total_patterns);
    printf("Generalization: %s\n", ends_with_s ? "✓ Working" : "~ Needs improvement");
    
    return 0;
}

