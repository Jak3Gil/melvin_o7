/* Test: Single pattern training - does it work? */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

typedef struct MelvinGraph MelvinGraph;
extern MelvinGraph* melvin_create(void);
extern void run_episode(MelvinGraph *g, const uint8_t *input, uint32_t input_len,
                       const uint8_t *target, uint32_t target_len);
extern void melvin_get_output(MelvinGraph *g, uint32_t **output, uint32_t *length);

int main(void) {
    MelvinGraph *g = melvin_create();
    
    printf("Training ONLY 'cat' → 'cats' (50 episodes)...\n\n");
    for (int i = 0; i < 50; i++) {
        run_episode(g, (const uint8_t*)"cat", 3, (const uint8_t*)"cats", 4);
    }
    
    printf("Test 1: 'cat' (trained)\n");
    run_episode(g, (const uint8_t*)"cat", 3, NULL, 0);
    
    uint32_t *output;
    uint32_t output_len;
    melvin_get_output(g, &output, &output_len);
    
    printf("  Output: ");
    for (uint32_t i = 0; i < output_len && i < 20; i++) {
        printf("%c", (uint8_t)output[i]);
    }
    printf("\n");
    if (output_len > 0 && output[output_len - 1] == 's') {
        printf("  ✓ INTELLIGENT: Ends with 's'\n\n");
    } else {
        printf("  ✗ Doesn't end with 's'\n\n");
    }
    
    printf("Test 2: 'bat' (novel - generalization test)\n");
    run_episode(g, (const uint8_t*)"bat", 3, NULL, 0);
    melvin_get_output(g, &output, &output_len);
    
    printf("  Output: ");
    for (uint32_t i = 0; i < output_len && i < 20; i++) {
        printf("%c", (uint8_t)output[i]);
    }
    printf("\n");
    if (output_len > 0 && output[output_len - 1] == 's') {
        printf("  ✓ INTELLIGENT: Generalized '_at' → 's' pattern!\n\n");
    } else {
        printf("  ✗ Didn't generalize\n\n");
    }
    
    return 0;
}

