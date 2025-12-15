/* Test: Does MORE data improve a SINGLE pattern? */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

typedef struct MelvinGraph MelvinGraph;
extern MelvinGraph* melvin_create(void);
extern void run_episode(MelvinGraph *g, const uint8_t *input, uint32_t input_len,
                       const uint8_t *target, uint32_t target_len);
extern void melvin_get_output(MelvinGraph *g, uint32_t **output, uint32_t *length);
extern uint32_t melvin_get_pattern_count(MelvinGraph *g);
extern float melvin_get_error_rate(MelvinGraph *g);

int main(void) {
    MelvinGraph *g = melvin_create();
    
    printf("=================================================================\n");
    printf("TEST: Does more data improve learning of ONE pattern?\n");
    printf("=================================================================\n\n");
    
    printf("Training ONLY 'cat' → 'cats' with increasing data...\n\n");
    
    for (int episode = 1; episode <= 100; episode++) {
        run_episode(g, (const uint8_t*)"cat", 3, (const uint8_t*)"cats", 4);
        
        if (episode % 10 == 0 || episode <= 10) {
            uint32_t pattern_count = melvin_get_pattern_count(g);
            float error_rate = melvin_get_error_rate(g);
            
            /* Test output */
            run_episode(g, (const uint8_t*)"cat", 3, NULL, 0);
            uint32_t *output;
            uint32_t output_len;
            melvin_get_output(g, &output, &output_len);
            
            printf("Episode %3d: Error=%.3f, Patterns=%u, Output=", 
                   episode, error_rate, pattern_count);
            for (uint32_t i = 0; i < output_len && i < 10; i++) {
                printf("%c", (uint8_t)output[i]);
            }
            
            if (output_len > 0 && output[output_len - 1] == 's') {
                printf(" ✓ ends with s");
            }
            printf("\n");
        }
    }
    
    printf("\n=================================================================\n");
    printf("OBSERVATION: Does error decrease? Do outputs improve?\n");
    printf("=================================================================\n");
    
    return 0;
}

