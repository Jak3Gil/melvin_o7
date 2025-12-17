/* What is the system actually learning? */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

typedef struct MelvinGraph MelvinGraph;
extern MelvinGraph* melvin_create(void);
extern void run_episode(MelvinGraph *g, const uint8_t *input, uint32_t input_len,
                       const uint8_t *target, uint32_t target_len);
extern void melvin_get_output(MelvinGraph *g, uint32_t **output, uint32_t *length);
extern float melvin_get_edge_weight(MelvinGraph *g, uint32_t from_id, uint32_t to_id);

int main(void) {
    MelvinGraph *g = melvin_create();
    
    printf("=== What Does The System Learn? ===\n\n");
    
    printf("Training: 'a' -> 'cat' (10 times)\n");
    for (int i = 0; i < 10; i++) {
        run_episode(g, (const uint8_t*)"a", 1, (const uint8_t*)"cat", 3);
    }
    
    printf("\n=== Edge Weights After Training ===\n");
    
    // Check critical edges
    printf("Input->Output edges:\n");
    printf("  'a' -> 'c': %.3f (should be strong - 'a' input should start 'cat')\n", 
           melvin_get_edge_weight(g, 'a', 'c'));
    printf("  'a' -> 'a': %.3f (should be weak - not echoing)\n", 
           melvin_get_edge_weight(g, 'a', 'a'));
    
    printf("\nSequence edges (target):\n");
    printf("  'c' -> 'a': %.3f (should be strong - 'cat' sequence)\n",
           melvin_get_edge_weight(g, 'c', 'a'));
    printf("  'a' -> 't': %.3f (should be strong - 'cat' sequence)\n",
           melvin_get_edge_weight(g, 'a', 't'));
    
    printf("\nEND_MARKER edges:\n");
    printf("  't' -> END(257): %.3f (should be strong - 't' ends sequence)\n",
           melvin_get_edge_weight(g, 't', 257));
    printf("  'a' -> END(257): %.3f (should be ZERO - 'a' doesn't end sequence)\n",
           melvin_get_edge_weight(g, 'a', 257));
    
    printf("\nCompeting edges from 'c':\n");
    printf("  'c' -> 'a': %.3f (should win - correct sequence)\n",
           melvin_get_edge_weight(g, 'c', 'a'));
    printf("  'c' -> 't': %.3f (should be weak or zero)\n",
           melvin_get_edge_weight(g, 'c', 't'));
    
    printf("\n=== Generation Test ===\n");
    run_episode(g, (const uint8_t*)"a", 1, NULL, 0);
    
    uint32_t *output;
    uint32_t len;
    melvin_get_output(g, &output, &len);
    
    printf("Output: '");
    for (uint32_t i = 0; i < len; i++) {
        printf("%c", (char)output[i]);
    }
    printf("' (expected 'cat')\n");
    
    printf("\n=== Analysis ===\n");
    printf("If 'a'->'c' is weak: Not learning input->output mapping\n");
    printf("If 'a'->'END' is strong: Learning wrong END associations\n");
    printf("If output is 'a': System is echoing input, not following edges\n");
    
    return 0;
}
