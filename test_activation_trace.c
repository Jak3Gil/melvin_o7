/* Trace: Is pattern activating 's' node? Why isn't it being selected? */

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
    
    /* Train */
    for (int i = 0; i < 20; i++) {
        run_episode(g, (uint8_t*)"cat", 3, (uint8_t*)"cats", 4);
    }
    
    printf("TRAINED. Testing 'cat'...\n\n");
    run_episode(g, (uint8_t*)"cat", 3, NULL, 0);
    
    uint32_t *output;
    uint32_t output_len;
    melvin_get_output(g, &output, &output_len);
    
    printf("Output: ");
    for (uint32_t i = 0; i < output_len && i < 20; i++) {
        printf("%c", (uint8_t)output[i]);
    }
    printf("\n\n");
    
    /* Analyze */
    printf("ANALYSIS:\n");
    printf("Expected: Pattern 'at' should match output 'at', activate, predict 's'\n");
    printf("Reality: Output loops instead of ending with 's'\n\n");
    
    printf("POSSIBLE CAUSES:\n");
    printf("1. Pattern fires but 's' node doesn't get enough activation\n");
    printf("2. Pattern fires but other nodes (t,c,a) have higher activation\n");
    printf("3. Pattern doesn't fire because output never contains 'at' in correct position\n");
    printf("4. Pattern fires once, marks as has_fired=true, never fires again\n\n");
    
    printf("KEY INSIGHT:\n");
    printf("Pattern can only fire ONCE per episode (has_fired flag).\n");
    printf("If output is 'tctatc', pattern 'at' matches at position 2-3 and 4-5.\n");
    printf("But it only fires ONCE, so it only predicts 's' once.\n");
    printf("After that, edges take over and create loops.\n\n");
    
    printf("SOLUTION: Remove has_fired restriction OR allow patterns to fire\n");
    printf("multiple times if they keep matching (different positions).\n");
    
    return 0;
}

