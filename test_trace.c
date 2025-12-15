/* Trace execution to see WHY patterns aren't winning */

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
    
    printf("=== TRAINING: 'cat' → 'cats' (50 episodes) ===\n");
    for (int i = 0; i < 50; i++) {
        run_episode(g, (uint8_t*)"cat", 3, (uint8_t*)"cats", 4);
    }
    
    printf("\n=== LEARNED KNOWLEDGE ===\n");
    printf("Patterns: %u\n", melvin_get_pattern_count(g));
    
    /* Show patterns that predict 's' */
    for (uint32_t p = 0; p < melvin_get_pattern_count(g); p++) {
        uint32_t *node_ids;
        uint32_t length;
        float strength;
        melvin_get_pattern_info(g, p, &node_ids, &length, &strength);
        
        uint32_t *predicted;
        float *weights;
        uint32_t pred_count;
        melvin_get_pattern_predictions(g, p, &predicted, &weights, &pred_count);
        
        /* Only show patterns that predict 's' */
        bool predicts_s = false;
        for (uint32_t i = 0; i < pred_count; i++) {
            if (predicted[i] == 's') {
                predicts_s = true;
                break;
            }
        }
        
        if (predicts_s && strength > 0.001f) {
            printf("\nPattern %u: ", p);
            for (uint32_t i = 0; i < length; i++) {
                if (node_ids[i] == 256) printf("_");
                else printf("%c", (uint8_t)node_ids[i]);
            }
            printf(" (strength=%.4f)\n", strength);
            printf("  Predicts: ");
            for (uint32_t i = 0; i < pred_count; i++) {
                printf("'%c'(%.3f) ", (uint8_t)predicted[i], weights[i]);
            }
            printf("\n");
        }
    }
    
    /* Show key edges */
    printf("\n=== KEY EDGES ===\n");
    printf("c→a: %.4f\n", melvin_get_edge_weight(g, 'c', 'a'));
    printf("a→t: %.4f\n", melvin_get_edge_weight(g, 'a', 't'));
    printf("t→s: %.4f\n", melvin_get_edge_weight(g, 't', 's'));
    printf("t→c: %.4f\n", melvin_get_edge_weight(g, 't', 'c'));
    printf("t→a: %.4f\n", melvin_get_edge_weight(g, 't', 'a'));
    printf("t→t: %.4f\n", melvin_get_edge_weight(g, 't', 't'));
    printf("t→o: %.4f\n", melvin_get_edge_weight(g, 't', 'o'));
    printf("o→t: %.4f\n", melvin_get_edge_weight(g, 'o', 't'));
    
    printf("\n=== TEST: 'bat' (novel input) ===\n");
    run_episode(g, (uint8_t*)"bat", 3, NULL, 0);
    
    uint32_t *output;
    uint32_t output_len;
    melvin_get_output(g, &output, &output_len);
    
    printf("Output: ");
    for (uint32_t i = 0; i < output_len && i < 20; i++) {
        printf("%c", (uint8_t)output[i]);
    }
    printf("\n");
    
    printf("\nQUESTION: Why didn't pattern '_at' → 's' win?\n");
    printf("HYPOTHESIS: Edge weights dominate pattern predictions\n");
    
    return 0;
}

