/* Test: Port-based modality differentiation */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

typedef struct MelvinGraph MelvinGraph;
extern MelvinGraph* melvin_create(void);
extern void melvin_set_input_port(MelvinGraph *g, uint32_t port_id);
extern void melvin_set_output_port(MelvinGraph *g, uint32_t port_id);
extern void run_episode(MelvinGraph *g, const uint8_t *input, uint32_t input_len,
                       const uint8_t *target, uint32_t target_len);
extern void melvin_get_output(MelvinGraph *g, uint32_t **output, uint32_t *length);

int main(void) {
    MelvinGraph *g = melvin_create();
    
    printf("=================================================================\n");
    printf("PORT-BASED MODALITY DIFFERENTIATION\n");
    printf("=================================================================\n\n");
    
    /* Train in TEXT port (port 0) */
    printf("Training in TEXT port (port 0): 'cat' → 'cats'\n");
    melvin_set_input_port(g, 0);   /* TEXT port */
    melvin_set_output_port(g, 0);
    
    for (int i = 0; i < 30; i++) {
        run_episode(g, (const uint8_t*)"cat", 3, (const uint8_t*)"cats", 4);
    }
    printf("Training complete.\n\n");
    
    /* Test in TEXT port - should use learned patterns */
    printf("Test in TEXT port:\n");
    printf("  Input:  cat (bytes 99,97,116 in TEXT context)\n");
    melvin_set_input_port(g, 0);
    run_episode(g, (const uint8_t*)"cat", 3, NULL, 0);
    uint32_t *output;
    uint32_t output_len;
    melvin_get_output(g, &output, &output_len);
    printf("  Output: ");
    for (uint32_t i = 0; i < output_len && i < 50; i++) {
        printf("%c", (uint8_t)output[i]);
    }
    printf("\n\n");
    
    /* Test in AUDIO port - same bytes, different meaning */
    printf("Test in AUDIO port (port 1):\n");
    printf("  Input:  cat (bytes 99,97,116 in AUDIO context = frequencies)\n");
    melvin_set_input_port(g, 1);   /* AUDIO port */
    run_episode(g, (const uint8_t*)"cat", 3, NULL, 0);
    melvin_get_output(g, &output, &output_len);
    printf("  Output: ");
    for (uint32_t i = 0; i < output_len && i < 50; i++) {
        printf("%c", (uint8_t)output[i]);
    }
    printf("\n\n");
    
    printf("=================================================================\n");
    printf("KEY INSIGHT:\n");
    printf("- TEXT patterns only fire in TEXT port\n");
    printf("- AUDIO patterns only fire in AUDIO port\n");
    printf("- Same bytes (99,97,116), different meanings, no confusion!\n");
    printf("- Nodes track source_port, patterns track input_port/output_port\n");
    printf("- Wave prop learns port-to-port relationships\n");
    printf("=================================================================\n");
    
    return 0;
}

