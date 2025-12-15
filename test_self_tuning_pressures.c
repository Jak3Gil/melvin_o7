/* Test: Show how self-tuning pressures change with data */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

typedef struct MelvinGraph MelvinGraph;
extern MelvinGraph* melvin_create(void);
extern void run_episode(MelvinGraph *g, const uint8_t *input, uint32_t input_len,
                       const uint8_t *target, uint32_t target_len);

/* Access internal state (for testing) */
float get_learning_pressure(MelvinGraph *g);
float get_pattern_confidence(MelvinGraph *g);
float get_output_variance(MelvinGraph *g);
float get_loop_pressure(MelvinGraph *g);

int main(void) {
    MelvinGraph *g = melvin_create();
    
    printf("=================================================================\n");
    printf("SELF-TUNING PRESSURES: How they change with data\n");
    printf("=================================================================\n\n");
    
    printf("Training 'cat' → 'cats'...\n\n");
    printf("Episode | Error  | LearnPress | PatternConf | OutputVar | LoopPress\n");
    printf("--------|--------|------------|-------------|-----------|----------\n");
    
    for (int episode = 1; episode <= 50; episode++) {
        run_episode(g, (const uint8_t*)"cat", 3, (const uint8_t*)"cats", 4);
        
        if (episode % 5 == 0 || episode <= 10) {
            /* Note: These functions don't exist yet - this is what we'd expect */
            /* For now, just show episode count */
            printf("  %3d   | (need accessor functions)\n", episode);
        }
    }
    
    printf("\n=================================================================\n");
    printf("EXPECTED BEHAVIOR:\n");
    printf("  - Learning pressure should decrease as error decreases\n");
    printf("  - Pattern confidence should increase as patterns succeed\n");
    printf("  - Output variance should decrease as system converges\n");
    printf("  - Loop pressure should spike when loops detected, then decay\n");
    printf("=================================================================\n");
    
    return 0;
}

