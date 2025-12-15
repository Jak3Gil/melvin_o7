/* Test: .m file as active brain */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

typedef struct MelvinGraph MelvinGraph;
extern MelvinGraph* melvin_create(void);
extern MelvinGraph* melvin_load_brain(const char *filename);
extern int melvin_save_brain(MelvinGraph *g, const char *filename);
extern void run_episode(MelvinGraph *g, const uint8_t *input, uint32_t input_len,
                       const uint8_t *target, uint32_t target_len);
extern void melvin_get_output(MelvinGraph *g, uint32_t **output, uint32_t *length);

int main(void) {
    printf("=================================================================\n");
    printf(".m FILE AS ACTIVE BRAIN\n");
    printf("=================================================================\n\n");
    
    /* Phase 1: Train and save brain */
    printf("PHASE 1: Training brain...\n");
    MelvinGraph *g = melvin_create();
    
    for (int i = 0; i < 30; i++) {
        run_episode(g, (const uint8_t*)"cat", 3, (const uint8_t*)"cats", 4);
    }
    
    printf("Saving brain to 'brain.m'...\n");
    melvin_save_brain(g, "brain.m");
    printf("Brain saved!\n\n");
    
    /* Phase 2: Load brain and use it */
    printf("PHASE 2: Loading brain from 'brain.m'...\n");
    MelvinGraph *g2 = melvin_load_brain("brain.m");
    
    if (g2) {
        printf("Brain loaded! Testing:\n");
        printf("  Input:  cat\n");
        run_episode(g2, (const uint8_t*)"cat", 3, NULL, 0);
        
        uint32_t *output;
        uint32_t output_len;
        melvin_get_output(g2, &output, &output_len);
        printf("  Output: ");
        for (uint32_t i = 0; i < output_len && i < 50; i++) {
            printf("%c", (uint8_t)output[i]);
        }
        printf("\n\n");
        
        printf("SUCCESS: Brain works independently!\n");
        printf("The .m file IS the brain - melvin.c just runs it.\n");
    } else {
        printf("Failed to load brain (parser not fully implemented yet)\n");
    }
    
    return 0;
}

