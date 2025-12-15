/* FINAL TRACE: What actually happens step-by-step during output generation? */

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
extern float melvin_get_edge_weight(MelvinGraph *g, uint32_t from_id, uint32_t to_id);

int main(void) {
    MelvinGraph *g = melvin_create();
    
    /* Simple training */
    printf("Training 'cat' → 'cats' (20 episodes)...\n");
    for (int i = 0; i < 20; i++) {
        run_episode(g, (uint8_t*)"cat", 3, (uint8_t*)"cats", 4);
    }
    
    printf("\n=== LEARNED STATE ===\n");
    printf("Patterns: %u\n", melvin_get_pattern_count(g));
    
    /* Find pattern "_at" or "at" */
    for (uint32_t p = 0; p < melvin_get_pattern_count(g); p++) {
        uint32_t *node_ids;
        uint32_t length;
        float strength;
        melvin_get_pattern_info(g, p, &node_ids, &length, &strength);
        
        if (length == 2 && node_ids[1] == 't' && (node_ids[0] == 'a' || node_ids[0] == 256)) {
            printf("\nPattern %u: ", p);
            for (uint32_t i = 0; i < length; i++) {
                if (node_ids[i] == 256) printf("_");
                else printf("%c", (uint8_t)node_ids[i]);
            }
            printf(" (strength=%.6f)\n", strength);
            
            uint32_t *predicted;
            float *weights;
            uint32_t pred_count;
            melvin_get_pattern_predictions(g, p, &predicted, &weights, &pred_count);
            
            printf("  Predicts %u nodes:\n", pred_count);
            for (uint32_t i = 0; i < pred_count; i++) {
                printf("    '%c' (weight=%.6f)\n", (uint8_t)predicted[i], weights[i]);
            }
        }
    }
    
    printf("\n=== EDGE WEIGHTS ===\n");
    printf("c→a: %.6f\n", melvin_get_edge_weight(g, 'c', 'a'));
    printf("a→t: %.6f\n", melvin_get_edge_weight(g, 'a', 't'));
    printf("t→s: %.6f\n", melvin_get_edge_weight(g, 't', 's'));
    printf("t→o: %.6f\n", melvin_get_edge_weight(g, 't', 'o'));
    printf("o→t: %.6f\n", melvin_get_edge_weight(g, 'o', 't'));
    
    printf("\n=== TEST: 'cat' (trained input) ===\n");
    printf("Expected: Should output 'cats' (or at least end with 's')\n");
    run_episode(g, (uint8_t*)"cat", 3, NULL, 0);
    
    uint32_t *output;
    uint32_t output_len;
    melvin_get_output(g, &output, &output_len);
    
    printf("Actual output: ");
    for (uint32_t i = 0; i < output_len && i < 20; i++) {
        printf("%c", (uint8_t)output[i]);
    }
    printf("\n");
    
    if (output_len > 0 && output[output_len - 1] == 's') {
        printf("✓ Output ends with 's' - INTELLIGENT!\n");
    } else {
        printf("✗ Output does NOT end with 's'\n");
        printf("\nTHE PROBLEM: Edges creating loops (t→o→t) dominate over pattern predictions\n");
        printf("HYPOTHESIS: Pattern 'at' matches output 'at', predicts 's',\n");
        printf("            but edges t→o or t→something_else are stronger\n");
    }
    
    return 0;
}

