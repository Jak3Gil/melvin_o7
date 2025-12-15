/* ============================================================================
 * TEST OUTPUTS: See what the system actually generates
 * ============================================================================ */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "melvin.c"

void print_output(MelvinGraph *g, const char *label) {
    printf("%s: \"", label);
    for (uint32_t i = 0; i < g->output_length && i < 200; i++) {
        char c = (char)g->output_buffer[i];
        if (c >= 32 && c < 127) {
            printf("%c", c);
        } else {
            printf("?");
        }
    }
    printf("\" (length: %u)\n", g->output_length);
}

int main() {
    printf("========================================\n");
    printf("SYSTEM OUTPUTS TEST\n");
    printf("========================================\n\n");
    
    MelvinGraph *g = melvin_create();
    if (!g) {
        printf("ERROR: Failed to create graph\n");
        return 1;
    }
    
    /* Test 1: Simple input */
    printf("TEST 1: Simple Input\n");
    printf("--------------------\n");
    const char *input1 = "hello";
    const char *target1 = "world";
    
    printf("Input:  \"%s\"\n", input1);
    printf("Target: \"%s\"\n", target1);
    
    run_episode(g, (const uint8_t*)input1, strlen(input1),
               (const uint8_t*)target1, strlen(target1));
    print_output(g, "Output");
    printf("\n");
    
    /* Test 2: Question */
    printf("TEST 2: Question\n");
    printf("--------------------\n");
    const char *input2 = "What is the capital of France?";
    const char *target2 = "Paris";
    
    printf("Input:  \"%s\"\n", input2);
    printf("Target: \"%s\"\n", target2);
    
    g->output_length = 0;
    run_episode(g, (const uint8_t*)input2, strlen(input2),
               (const uint8_t*)target2, strlen(target2));
    print_output(g, "Output");
    printf("\n");
    
    /* Test 3: Train multiple times then test */
    printf("TEST 3: Training then Testing\n");
    printf("--------------------\n");
    
    printf("Training on 'cat' -> 'cats' (10 times)...\n");
    for (int i = 0; i < 10; i++) {
        g->output_length = 0;
        run_episode(g, (const uint8_t*)"cat", 3,
                   (const uint8_t*)"cats", 4);
    }
    
    printf("Now testing 'bat' -> should output 'bats'?\n");
    g->output_length = 0;
    run_episode(g, (const uint8_t*)"bat", 3,
               (const uint8_t*)"bats", 4);
    print_output(g, "Output");
    printf("Expected: \"bats\"\n");
    printf("\n");
    
    /* Test 4: Multi-word input */
    printf("TEST 4: Multi-Word Input\n");
    printf("--------------------\n");
    const char *input4 = "The quick brown fox";
    const char *target4 = "jumps over";
    
    printf("Input:  \"%s\"\n", input4);
    printf("Target: \"%s\"\n", target4);
    
    g->output_length = 0;
    run_episode(g, (const uint8_t*)input4, strlen(input4),
               (const uint8_t*)target4, strlen(target4));
    print_output(g, "Output");
    printf("\n");
    
    /* Test 5: Show pattern info */
    printf("TEST 5: Pattern Information\n");
    printf("--------------------\n");
    printf("Total patterns: %u\n", g->pattern_count);
    
    uint32_t patterns_with_outputs = 0;
    for (uint32_t p = 0; p < g->pattern_count && p < 20; p++) {
        Pattern *pat = &g->patterns[p];
        if (pat->prediction_count > 0 || pat->pattern_prediction_count > 0) {
            patterns_with_outputs++;
            printf("Pattern %u: ", p);
            for (uint32_t i = 0; i < pat->length && i < 10; i++) {
                if (pat->node_ids[i] == BLANK_NODE) {
                    printf("_");
                } else {
                    printf("%c", (char)pat->node_ids[i]);
                }
            }
            printf(" -> ");
            if (pat->pattern_prediction_count > 0) {
                printf("predicts %u patterns, ", pat->pattern_prediction_count);
            }
            if (pat->prediction_count > 0) {
                printf("predicts %u nodes", pat->prediction_count);
            }
            printf("\n");
        }
    }
    printf("Patterns with predictions: %u\n", patterns_with_outputs);
    printf("\n");
    
    /* Test 6: Raw output bytes */
    printf("TEST 6: Raw Output Bytes\n");
    printf("--------------------\n");
    g->output_length = 0;
    run_episode(g, (const uint8_t*)"test", 4,
               (const uint8_t*)"result", 6);
    
    printf("Output bytes: ");
    for (uint32_t i = 0; i < g->output_length && i < 50; i++) {
        printf("%u ", g->output_buffer[i]);
    }
    printf("\n");
    
    printf("Output as chars: ");
    for (uint32_t i = 0; i < g->output_length && i < 50; i++) {
        uint8_t b = (uint8_t)g->output_buffer[i];
        if (b >= 32 && b < 127) {
            printf("%c", (char)b);
        } else {
            printf("[%u]", b);
        }
    }
    printf("\n");
    
    melvin_destroy(g);
    return 0;
}
