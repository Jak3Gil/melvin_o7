/* ============================================================================
 * SIMPLE OUTPUT TEST: Just show inputs and outputs
 * ============================================================================ */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "melvin.c"

void print_output(MelvinGraph *g) {
    printf("Output: \"");
    for (uint32_t i = 0; i < g->output_length && i < 200; i++) {
        uint8_t b = (uint8_t)g->output_buffer[i];
        if (b >= 32 && b < 127) {
            printf("%c", (char)b);
        } else {
            printf("[%u]", b);
        }
    }
    printf("\" (length: %u)\n", g->output_length);
}

int main() {
    printf("========================================\n");
    printf("SIMPLE OUTPUT TEST\n");
    printf("========================================\n\n");
    
    MelvinGraph *g = melvin_create();
    if (!g) {
        printf("ERROR: Failed to create graph\n");
        return 1;
    }
    
    /* Test 1 */
    printf("Test 1:\n");
    printf("  Input:  \"hello\"\n");
    printf("  Target: \"world\"\n");
    run_episode(g, (const uint8_t*)"hello", 5, (const uint8_t*)"world", 5);
    print_output(g);
    printf("\n");
    
    /* Test 2 */
    printf("Test 2:\n");
    printf("  Input:  \"What is the capital of France?\"\n");
    printf("  Target: \"Paris\"\n");
    g->output_length = 0;
    run_episode(g, (const uint8_t*)"What is the capital of France?", 31,
               (const uint8_t*)"Paris", 5);
    print_output(g);
    printf("\n");
    
    /* Test 3: Train then test */
    printf("Test 3: Training\n");
    printf("  Training 'cat' -> 'cats' (5 times)...\n");
    for (int i = 0; i < 5; i++) {
        g->output_length = 0;
        run_episode(g, (const uint8_t*)"cat", 3, (const uint8_t*)"cats", 4);
    }
    
    printf("  Now test 'bat' -> should output 'bats':\n");
    g->output_length = 0;
    run_episode(g, (const uint8_t*)"bat", 3, (const uint8_t*)"bats", 4);
    print_output(g);
    printf("  Expected: \"bats\"\n");
    printf("\n");
    
    /* Test 4: Simple question */
    printf("Test 4:\n");
    printf("  Input:  \"What is 2+2?\"\n");
    printf("  Target: \"4\"\n");
    g->output_length = 0;
    run_episode(g, (const uint8_t*)"What is 2+2?", 12, (const uint8_t*)"4", 1);
    print_output(g);
    printf("\n");
    
    /* Test 5: Multi-word */
    printf("Test 5:\n");
    printf("  Input:  \"The cat sat\"\n");
    printf("  Target: \"on the mat\"\n");
    g->output_length = 0;
    run_episode(g, (const uint8_t*)"The cat sat", 11, (const uint8_t*)"on the mat", 10);
    print_output(g);
    printf("\n");
    
    melvin_destroy(g);
    return 0;
}
