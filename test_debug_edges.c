/* Debug: Check if edges are being created/strengthened */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

typedef struct MelvinGraph MelvinGraph;
extern MelvinGraph* melvin_create(void);
extern void run_episode(MelvinGraph *g, const uint8_t *input, uint32_t input_len,
                       const uint8_t *target, uint32_t target_len);
extern void melvin_get_output(MelvinGraph *g, uint32_t **output, uint32_t *length);
extern float melvin_get_edge_weight(MelvinGraph *g, uint32_t from, uint32_t to);
extern uint32_t melvin_get_edge_use_count(MelvinGraph *g, uint32_t from, uint32_t to);
extern uint32_t melvin_get_edge_success_count(MelvinGraph *g, uint32_t from, uint32_t to);
extern void melvin_destroy(MelvinGraph *g);

int main(void) {
    printf("DEBUG: Edge Weight and Success Count Tracking\n");
    printf("==============================================\n\n");
    
    MelvinGraph *g = melvin_create();
    
    /* Check edges before training */
    printf("Before training:\n");
    printf("  c→a: weight=%.3f, use=%u, success=%u\n",
           melvin_get_edge_weight(g, 'c', 'a'),
           melvin_get_edge_use_count(g, 'c', 'a'),
           melvin_get_edge_success_count(g, 'c', 'a'));
    printf("  a→t: weight=%.3f, use=%u, success=%u\n",
           melvin_get_edge_weight(g, 'a', 't'),
           melvin_get_edge_use_count(g, 'a', 't'),
           melvin_get_edge_success_count(g, 'a', 't'));
    printf("  t→s: weight=%.3f, use=%u, success=%u\n\n",
           melvin_get_edge_weight(g, 't', 's'),
           melvin_get_edge_use_count(g, 't', 's'),
           melvin_get_edge_success_count(g, 't', 's'));
    
    /* Train 5 times */
    for (int i = 0; i < 5; i++) {
        run_episode(g, (const uint8_t*)"cat", 3, (const uint8_t*)"cats", 4);
        printf("After episode %d:\n", i + 1);
        printf("  c→a: weight=%.3f, use=%u, success=%u\n",
               melvin_get_edge_weight(g, 'c', 'a'),
               melvin_get_edge_use_count(g, 'c', 'a'),
               melvin_get_edge_success_count(g, 'c', 'a'));
        printf("  a→t: weight=%.3f, use=%u, success=%u\n",
               melvin_get_edge_weight(g, 'a', 't'),
               melvin_get_edge_use_count(g, 'a', 't'),
               melvin_get_edge_success_count(g, 'a', 't'));
        printf("  t→s: weight=%.3f, use=%u, success=%u\n\n",
               melvin_get_edge_weight(g, 't', 's'),
               melvin_get_edge_use_count(g, 't', 's'),
               melvin_get_edge_success_count(g, 't', 's'));
    }
    
    /* Test output */
    printf("\nTest: cat → ?\n");
    run_episode(g, (const uint8_t*)"cat", 3, NULL, 0);
    uint32_t *output;
    uint32_t length;
    melvin_get_output(g, &output, &length);
    
    printf("Output: \"");
    for (uint32_t i = 0; i < length; i++) {
        printf("%c", (char)output[i]);
    }
    printf("\"\n");
    
    melvin_destroy(g);
    return 0;
}

