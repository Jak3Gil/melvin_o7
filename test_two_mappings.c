/* Test two separate input->output mappings */
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
    
    printf("=== Two Mappings Test ===\n\n");
    
    /* Train with MORE epochs */
    printf("Training 50 epochs each:\n");
    printf("  'a' -> 'cat'\n");
    printf("  'b' -> 'dog'\n\n");
    
    for (int i = 0; i < 50; i++) {
        run_episode(g, (const uint8_t*)"a", 1, (const uint8_t*)"cat", 3);
        run_episode(g, (const uint8_t*)"b", 1, (const uint8_t*)"dog", 3);
    }
    
    /* Check edge weights */
    printf("Edge weights:\n");
    printf("  'a' -> 'c': %.3f\n", melvin_get_edge_weight(g, 'a', 'c'));
    printf("  'b' -> 'd': %.3f\n", melvin_get_edge_weight(g, 'b', 'd'));
    printf("  'a' -> 'd': %.3f (should be ~0)\n", melvin_get_edge_weight(g, 'a', 'd'));
    printf("  'b' -> 'c': %.3f (should be ~0)\n\n", melvin_get_edge_weight(g, 'b', 'c'));
    
    /* Test 'a' */
    run_episode(g, (const uint8_t*)"a", 1, NULL, 0);
    uint32_t *out; uint32_t len;
    melvin_get_output(g, &out, &len);
    printf("Input 'a' -> '");
    for (uint32_t i = 0; i < len && i < 10; i++) printf("%c", (char)out[i]);
    printf("' (expected 'cat')\n");
    
    /* Test 'b' */
    run_episode(g, (const uint8_t*)"b", 1, NULL, 0);
    melvin_get_output(g, &out, &len);
    printf("Input 'b' -> '");
    for (uint32_t i = 0; i < len && i < 10; i++) printf("%c", (char)out[i]);
    printf("' (expected 'dog')\n");
    
    return 0;
}
