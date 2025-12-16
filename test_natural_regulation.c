/* Test Natural Pattern Regulation: Verify patterns naturally regulate each other */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

typedef struct MelvinGraph MelvinGraph;
extern MelvinGraph* melvin_create(void);
extern void run_episode(MelvinGraph *g, const uint8_t *input, uint32_t input_len,
                       const uint8_t *target, uint32_t target_len);
extern void melvin_get_output(MelvinGraph *g, uint32_t **output, uint32_t *length);
extern void melvin_destroy(MelvinGraph *g);

int main(void) {
    MelvinGraph *g = melvin_create();
    
    printf("=================================================================\n");
    printf("MELVIN O7: Natural Pattern Regulation Test\n");
    printf("=================================================================\n\n");
    
    printf("Testing: Patterns naturally regulate each other based on:\n");
    printf("  1. Confidence similarity (high-confidence patterns connect to high-confidence)\n");
    printf("  2. Hierarchy similarity (similar hierarchy levels connect more strongly)\n");
    printf("  3. System state emerges from natural pattern interactions\n\n");
    
    printf("PHASE 1: Training with varied success rates\n");
    printf("-------------------------------------------\n");
    
    /* Train some patterns to be high-confidence (repeated correct predictions) */
    printf("Training 'hello' → 'hello' (high confidence pattern)...\n");
    for (int i = 0; i < 20; i++) {
        run_episode(g, (const uint8_t*)"hello", 5, (const uint8_t*)"hello", 5);
    }
    
    /* Train some patterns to be low-confidence (mixed correct/incorrect) */
    printf("Training 'test' → 'test' (10x), then 'test' → 'wrong' (5x)...\n");
    for (int i = 0; i < 10; i++) {
        run_episode(g, (const uint8_t*)"test", 4, (const uint8_t*)"test", 4);
    }
    for (int i = 0; i < 5; i++) {
        run_episode(g, (const uint8_t*)"test", 4, (const uint8_t*)"wrong", 5);
    }
    
    /* Train another high-confidence pattern */
    printf("Training 'world' → 'world' (high confidence pattern)...\n");
    for (int i = 0; i < 20; i++) {
        run_episode(g, (const uint8_t*)"world", 5, (const uint8_t*)"world", 5);
    }
    
    printf("\nTraining complete.\n\n");
    
    printf("PHASE 2: Testing natural regulation\n");
    printf("-----------------------------------\n");
    printf("Input:  hello\n");
    run_episode(g, (const uint8_t*)"hello", 5, NULL, 0);
    uint32_t *output;
    uint32_t output_len;
    melvin_get_output(g, &output, &output_len);
    printf("Output: ");
    for (uint32_t i = 0; i < output_len && i < 50; i++) {
        printf("%c", (uint8_t)output[i]);
    }
    printf("\n\n");
    
    printf("Input:  world\n");
    run_episode(g, (const uint8_t*)"world", 5, NULL, 0);
    melvin_get_output(g, &output, &output_len);
    printf("Output: ");
    for (uint32_t i = 0; i < output_len && i < 50; i++) {
        printf("%c", (uint8_t)output[i]);
    }
    printf("\n\n");
    
    printf("Input:  test\n");
    run_episode(g, (const uint8_t*)"test", 4, NULL, 0);
    melvin_get_output(g, &output, &output_len);
    printf("Output: ");
    for (uint32_t i = 0; i < output_len && i < 50; i++) {
        printf("%c", (uint8_t)output[i]);
    }
    printf("\n\n");
    
    printf("=================================================================\n");
    printf("Expected behavior:\n");
    printf("  - High-confidence patterns ('hello', 'world') should work well\n");
    printf("  - Low-confidence pattern ('test') may be less reliable\n");
    printf("  - Patterns naturally cluster based on confidence similarity\n");
    printf("  - System state emerges from pattern interactions\n");
    printf("=================================================================\n");
    
    melvin_destroy(g);
    return 0;
}
