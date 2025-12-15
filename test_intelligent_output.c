/* ============================================================================
 * PROVE INTELLIGENT OUTPUTS
 * 
 * Train system on examples, then test on novel inputs
 * Intelligent outputs = generalization, not just memorization
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

/* Train on examples */
void train(MelvinGraph *g, const uint8_t *input, uint32_t input_len,
           const uint8_t *target, uint32_t target_len, int episodes) {
    for (int i = 0; i < episodes; i++) {
        run_episode(g, input, input_len, target, target_len);
    }
}

/* Test on novel input - pure inference */
void test(MelvinGraph *g, const uint8_t *input, uint32_t input_len,
          const char *expected, const char *test_name) {
    run_episode(g, input, input_len, NULL, 0);  /* No target - pure inference */
    
    uint32_t *output;
    uint32_t output_len;
    melvin_get_output(g, &output, &output_len);
    
    /* Convert output to string for comparison */
    char output_str[256] = {0};
    for (uint32_t i = 0; i < output_len && i < 255; i++) {
        if (output[i] < 256) {
            output_str[i] = (char)output[i];
        }
    }
    
    printf("Test: %s\n", test_name);
    printf("  Input:    %s\n", input);
    printf("  Expected: %s\n", expected);
    printf("  Got:      %s\n", output_str);
    
    bool matches = (strncmp(output_str, expected, strlen(expected)) == 0);
    if (matches) {
        printf("  ✓ INTELLIGENT OUTPUT (generalized correctly)\n");
    } else {
        /* Check if it's close (partial match) */
        bool partial = (strstr(output_str, expected) != NULL) || 
                      (strlen(output_str) > 0 && output_str[0] == expected[0]);
        if (partial) {
            printf("  ~ Partial match (learning in progress)\n");
        } else {
            printf("  ✗ Output doesn't match (not generalized yet)\n");
        }
    }
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
    
    printf("PHASE 1: TRAINING (System learns patterns)\n");
    printf("------------------------------------------------\n");
    
    /* Train on pluralization examples */
    printf("Training: 'cat' → 'cats' (20 episodes)\n");
    train(g, (uint8_t*)"cat", 3, (uint8_t*)"cats", 4, 20);
    
    printf("Training: 'dog' → 'dogs' (20 episodes)\n");
    train(g, (uint8_t*)"dog", 3, (uint8_t*)"dogs", 4, 20);
    
    printf("Training: 'pen' → 'pens' (20 episodes)\n");
    train(g, (uint8_t*)"pen", 3, (uint8_t*)"pens", 4, 20);
    
    printf("\nPHASE 2: TESTING (Novel inputs - never seen before)\n");
    printf("------------------------------------------------\n");
    
    /* Test on novel inputs - intelligent system should generalize */
    test(g, (uint8_t*)"bat", 3, "bats", 
         "Generalization: 'bat' (never seen) → should output 'bats'");
    
    test(g, (uint8_t*)"hat", 3, "hats",
         "Generalization: 'hat' (never seen) → should output 'hats'");
    
    test(g, (uint8_t*)"mat", 3, "mats",
         "Generalization: 'mat' (never seen) → should output 'mats'");
    
    printf("=================================================================\n");
    printf("INTELLIGENCE PROOF:\n");
    printf("If system outputs correct pluralizations for novel inputs,\n");
    printf("it has learned the ABSTRACT RULE (not just memorization).\n");
    printf("=================================================================\n");
    
    return 0;
}

