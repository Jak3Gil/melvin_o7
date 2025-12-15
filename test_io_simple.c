/* Simple I/O Test: Feed inputs, see outputs */

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
    
    printf("=================================================================\n");
    printf("MELVIN O7: Input → Output Test\n");
    printf("=================================================================\n\n");
    
    printf("PHASE 1: Training\n");
    printf("------------------\n");
    printf("Training 'cat' → 'cats' (30 episodes)...\n");
    for (int i = 0; i < 30; i++) {
        run_episode(g, (const uint8_t*)"cat", 3, (const uint8_t*)"cats", 4);
    }
    printf("Training complete.\n\n");
    
    printf("PHASE 2: Testing (Input → Output)\n");
    printf("----------------------------------\n\n");
    
    /* Test 1: Trained input */
    printf("Input:  cat\n");
    run_episode(g, (const uint8_t*)"cat", 3, NULL, 0);
    uint32_t *output;
    uint32_t output_len;
    melvin_get_output(g, &output, &output_len);
    printf("Output: ");
    for (uint32_t i = 0; i < output_len && i < 50; i++) {
        printf("%c", (uint8_t)output[i]);
    }
    printf("\n\n");
    
    /* Test 2: Novel input (generalization) */
    printf("Input:  bat\n");
    run_episode(g, (const uint8_t*)"bat", 3, NULL, 0);
    melvin_get_output(g, &output, &output_len);
    printf("Output: ");
    for (uint32_t i = 0; i < output_len && i < 50; i++) {
        printf("%c", (uint8_t)output[i]);
    }
    printf("\n\n");
    
    /* Test 3: Another novel input */
    printf("Input:  mat\n");
    run_episode(g, (const uint8_t*)"mat", 3, NULL, 0);
    melvin_get_output(g, &output, &output_len);
    printf("Output: ");
    for (uint32_t i = 0; i < output_len && i < 50; i++) {
        printf("%c", (uint8_t)output[i]);
    }
    printf("\n\n");
    
    printf("=================================================================\n");
    printf("What we see:\n");
    printf("- Inputs are processed\n");
    printf("- Outputs are generated\n");
    printf("- System attempts to learn patterns\n");
    printf("=================================================================\n");
    
    return 0;
}

