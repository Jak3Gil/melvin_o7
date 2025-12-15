/* Debug: WHY aren't patterns learning predictions? */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

/* Expose internal pattern matching */
typedef struct {
    uint32_t *node_ids;
    uint32_t length;
    /* ... other fields ... */
} Pattern;

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
    MelvinGraph *g = melvin_create();
    
    printf("=== INITIAL TRAINING (10 episodes) ===\n");
    for (int ep = 0; ep < 10; ep++) {
        printf("\nEpisode %d: 'cat' → 'cats'\n", ep + 1);
        run_episode(g, (uint8_t*)"cat", 3, (uint8_t*)"cats", 4);
        
        printf("  Patterns: %u\n", melvin_get_pattern_count(g));
        
        /* Check if ANY pattern has predictions */
        bool has_predictions = false;
        for (uint32_t p = 0; p < melvin_get_pattern_count(g); p++) {
            uint32_t *predicted;
            float *weights;
            uint32_t pred_count;
            melvin_get_pattern_predictions(g, p, &predicted, &weights, &pred_count);
            
            if (pred_count > 0) {
                has_predictions = true;
                
                uint32_t *node_ids;
                uint32_t length;
                float strength;
                melvin_get_pattern_info(g, p, &node_ids, &length, &strength);
                
                printf("    Pattern %u: ", p);
                for (uint32_t i = 0; i < length; i++) {
                    if (node_ids[i] == 256) printf("_");
                    else printf("%c", (uint8_t)node_ids[i]);
                }
                printf(" → predicts %u nodes: ", pred_count);
                for (uint32_t i = 0; i < pred_count && i < 3; i++) {
                    printf("'%c'(%.3f) ", (uint8_t)predicted[i], weights[i]);
                }
                printf("\n");
            }
        }
        
        if (!has_predictions) {
            printf("    ⚠ NO PATTERNS HAVE PREDICTIONS YET\n");
        }
    }
    
    printf("\n=== ANALYSIS ===\n");
    printf("If patterns have NO predictions after 10 episodes,\n");
    printf("then learn_pattern_predictions() is not working.\n");
    printf("\nPossible causes:\n");
    printf("1. Patterns not matching input correctly\n");
    printf("2. Target indexing is wrong\n");
    printf("3. Patterns aren't created when expected\n");
    
    return 0;
}

