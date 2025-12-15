/* ============================================================================
 * PATTERN CREATION TEST
 * 
 * Test if system creates useful patterns and edges from simple input
 * ============================================================================ */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

/* Forward declarations */
typedef struct MelvinGraph MelvinGraph;
extern MelvinGraph* melvin_create(void);
extern void run_episode(MelvinGraph *g, const uint8_t *input, uint32_t input_len,
                       const uint8_t *target, uint32_t target_len);
extern void melvin_get_output(MelvinGraph *g, uint32_t **output, uint32_t *length);
extern float melvin_get_error_rate(MelvinGraph *g);
extern uint32_t melvin_get_pattern_count(MelvinGraph *g);
extern void melvin_get_pattern_info(MelvinGraph *g, uint32_t pattern_id, 
                                    uint32_t **node_ids, uint32_t *length, float *strength);
extern float melvin_get_edge_weight(MelvinGraph *g, uint32_t from_id, uint32_t to_id);

int main(void) {
    printf("=================================================================\n");
    printf("PATTERN CREATION TEST: Can System Learn Patterns?\n");
    printf("=================================================================\n\n");
    
    MelvinGraph *g = melvin_create();
    if (!g) {
        fprintf(stderr, "Failed to create graph\n");
        return 1;
    }
    
    printf("Test: Train on 'cat' → 'cats' (pluralization)\n");
    printf("Goal: System should create pattern '_at' and learn to add 's'\n\n");
    
    uint8_t input[] = {'c', 'a', 't'};
    uint8_t target[] = {'c', 'a', 't', 's'};
    
    printf("Training episodes:\n");
    for (int ep = 0; ep < 20; ep++) {
        run_episode(g, input, 3, target, 4);
        
        if (ep % 5 == 4) {
            uint32_t *output;
            uint32_t output_len;
            melvin_get_output(g, &output, &output_len);
            
            printf("  Episode %2d: Output: ", ep + 1);
            for (uint32_t i = 0; i < output_len && i < 10; i++) {
                printf("%c", (uint8_t)output[i]);
            }
            printf(" | Error: %.3f\n", melvin_get_error_rate(g));
        }
    }
    
    printf("\n=== PATTERN ANALYSIS ===\n");
    uint32_t pattern_count = melvin_get_pattern_count(g);
    printf("Patterns created: %u\n", pattern_count);
    
    for (uint32_t p = 0; p < pattern_count && p < 10; p++) {
        uint32_t *node_ids;
        uint32_t length;
        float strength;
        melvin_get_pattern_info(g, p, &node_ids, &length, &strength);
        
        printf("  Pattern %u: ", p);
        for (uint32_t i = 0; i < length; i++) {
            if (node_ids[i] == 256) {
                printf("_");  /* Blank node */
            } else {
                printf("%c", (uint8_t)node_ids[i]);
            }
        }
        printf(" (strength=%.3f)\n", strength);
    }
    
    printf("\n=== EDGE ANALYSIS ===\n");
    printf("Key edges learned:\n");
    float edge_ca = melvin_get_edge_weight(g, 'c', 'a');
    float edge_at = melvin_get_edge_weight(g, 'a', 't');
    float edge_ts = melvin_get_edge_weight(g, 't', 's');
    printf("  c→a: weight=%.3f\n", edge_ca);
    printf("  a→t: weight=%.3f\n", edge_at);
    printf("  t→s: weight=%.3f\n", edge_ts);
    
    printf("\n=== GENERALIZATION TEST ===\n");
    printf("Testing on 'bat' (never seen before)...\n");
    uint8_t test_input[] = {'b', 'a', 't'};
    run_episode(g, test_input, 3, NULL, 0);
    
    uint32_t *test_output;
    uint32_t test_output_len;
    melvin_get_output(g, &test_output, &test_output_len);
    
    printf("Input:  bat\n");
    printf("Expected: bats\n");
    printf("Got:     ");
    for (uint32_t i = 0; i < test_output_len && i < 10; i++) {
        printf("%c", (uint8_t)test_output[i]);
    }
    printf("\n");
    
    /* Check if it learned */
    int correct = 0;
    uint8_t expected[] = {'b', 'a', 't', 's'};
    for (uint32_t i = 0; i < 4 && i < test_output_len; i++) {
        if (test_output[i] == expected[i]) correct++;
    }
    
    float accuracy = (float)correct / 4.0f;
    printf("Accuracy: %.0f%% ", accuracy * 100.0f);
    if (accuracy >= 0.75f) {
        printf("✓ PASSED - System generalized!\n");
    } else {
        printf("✗ FAILED - Needs more learning\n");
    }
    
    return 0;
}

