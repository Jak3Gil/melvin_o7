/* Debug: When do patterns actually match during output generation? */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

typedef struct MelvinGraph MelvinGraph;
extern MelvinGraph* melvin_create(void);
extern void run_episode(MelvinGraph *g, const uint8_t *input, uint32_t input_len,
                       const uint8_t *target, uint32_t target_len);
extern void melvin_get_output(MelvinGraph *g, uint32_t **output, uint32_t *length);

int main(void) {
    MelvinGraph *g = melvin_create();
    
    /* Simple case: just train on "at" → "s" */
    printf("Training 'at' → 'ats' (30 episodes)...\n");
    for (int i = 0; i < 30; i++) {
        run_episode(g, (uint8_t*)"at", 2, (uint8_t*)"ats", 3);
    }
    
    printf("\nTesting 'at' (should output 'ats')...\n");
    run_episode(g, (uint8_t*)"at", 2, NULL, 0);
    
    uint32_t *output;
    uint32_t output_len;
    melvin_get_output(g, &output, &output_len);
    
    printf("Output: ");
    for (uint32_t i = 0; i < output_len && i < 20; i++) {
        printf("%c", (uint8_t)output[i]);
    }
    printf("\n\n");
    
    if (output_len > 0 && output[output_len - 1] == 's') {
        printf("✓ SUCCESS! Output ends with 's'\n");
    } else {
        printf("✗ FAIL: Output doesn't end with 's'\n");
        printf("\nDEBUG HYPOTHESIS:\n");
        printf("1. Pattern 'at' should match input 'at'\n");
        printf("2. Pattern should predict 's'\n");
        printf("3. When output contains 'at', pattern should fire again\n");
        printf("4. Context logic should select 's' because pattern predicts it\n");
        printf("\nIf this fails, the problem is likely:\n");
        printf("- Pattern not firing at the right time\n");
        printf("- Wave propagation selecting 't' or 'a' with higher activation\n");
        printf("- History penalty not strong enough to prevent 'a→t' loop\n");
    }
    
    return 0;
}

