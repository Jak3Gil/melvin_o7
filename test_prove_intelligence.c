/* ============================================================================
 * PROVE INTELLIGENCE: Show outputs demonstrate learning
 * Test on both seen and novel inputs
 * ============================================================================ */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

typedef struct MelvinGraph MelvinGraph;
extern MelvinGraph* melvin_create(void);
extern void run_episode(MelvinGraph *g, const uint8_t *input, uint32_t input_len,
                       const uint8_t *target, uint32_t target_len);
extern void melvin_get_output(MelvinGraph *g, uint32_t **output, uint32_t *length);
extern uint32_t melvin_get_pattern_count(MelvinGraph *g);

void test_output(MelvinGraph *g, const uint8_t *input, uint32_t input_len,
                 const char *description) {
    run_episode(g, input, input_len, NULL, 0);
    
    uint32_t *output;
    uint32_t output_len;
    melvin_get_output(g, &output, &output_len);
    
    char output_str[256] = {0};
    for (uint32_t i = 0; i < output_len && i < 255; i++) {
        if (output[i] < 256) {
            output_str[i] = (char)output[i];
        }
    }
    
    printf("%s\n", description);
    printf("  Input:  %s\n", input);
    printf("  Output: %s\n", output_str);
    printf("\n");
}

int main(void) {
    printf("=================================================================\n");
    printf("PROVING INTELLIGENT OUTPUTS\n");
    printf("=================================================================\n\n");
    
    MelvinGraph *g = melvin_create();
    if (!g) {
        fprintf(stderr, "Failed to create graph\n");
        return 1;
    }
    
    printf("TRAINING PHASE: Teaching system patterns\n");
    printf("-----------------------------------------\n");
    
    /* Intensive training on pluralization */
    for (int ep = 0; ep < 100; ep++) {
        if (ep % 3 == 0) {
            run_episode(g, (uint8_t*)"cat", 3, (uint8_t*)"cats", 4);
        } else if (ep % 3 == 1) {
            run_episode(g, (uint8_t*)"dog", 3, (uint8_t*)"dogs", 4);
        } else {
            run_episode(g, (uint8_t*)"pen", 3, (uint8_t*)"pens", 4);
        }
    }
    
    printf("Training complete. Patterns created: %u\n\n", melvin_get_pattern_count(g));
    
    printf("TESTING PHASE: What did the system learn?\n");
    printf("------------------------------------------\n");
    
    /* Test on SEEN inputs (should output correctly) */
    printf("TEST 1: Seen Inputs (Memorization Test)\n");
    test_output(g, (uint8_t*)"cat", 3, "Trained example: 'cat'");
    test_output(g, (uint8_t*)"dog", 3, "Trained example: 'dog'");
    test_output(g, (uint8_t*)"pen", 3, "Trained example: 'pen'");
    
    /* Test on NOVEL inputs (generalization test) */
    printf("TEST 2: Novel Inputs (Generalization Test)\n");
    test_output(g, (uint8_t*)"bat", 3, "Novel: 'bat' (never seen)");
    test_output(g, (uint8_t*)"hat", 3, "Novel: 'hat' (never seen)");
    test_output(g, (uint8_t*)"mat", 3, "Novel: 'mat' (never seen)");
    
    printf("=================================================================\n");
    printf("INTELLIGENCE INDICATORS:\n");
    printf("- If outputs end with 's' for novel inputs → GENERALIZATION\n");
    printf("- If outputs match training examples → LEARNING\n");
    printf("- If outputs are consistent → PATTERN RECOGNITION\n");
    printf("=================================================================\n");
    
    return 0;
}

