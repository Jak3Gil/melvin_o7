/* Test: Just save brain to .m file and verify format */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

typedef struct MelvinGraph MelvinGraph;

extern MelvinGraph* melvin_create(void);
extern void melvin_destroy(MelvinGraph *g);
extern void run_episode(MelvinGraph *g, const uint8_t *input, uint32_t input_len,
                       const uint8_t *target, uint32_t target_len);
extern int melvin_save_brain(MelvinGraph *g, const char *filename);
extern uint32_t melvin_get_pattern_count(MelvinGraph *g);

int main(void) {
    printf("Testing brain save to .m file...\n");
    
    MelvinGraph *g = melvin_create();
    if (!g) {
        printf("ERROR: Failed to create graph\n");
        return 1;
    }
    
    /* Train a bit */
    printf("Training...\n");
    for (int i = 0; i < 20; i++) {
        run_episode(g, (const uint8_t*)"cat", 3, (const uint8_t*)"cats", 4);
    }
    
    uint32_t patterns = melvin_get_pattern_count(g);
    printf("Patterns learned: %u\n", patterns);
    
    /* Save */
    const char *brain_file = "test_save_only.m";
    printf("Saving to %s...\n", brain_file);
    int result = melvin_save_brain(g, brain_file);
    if (result != 0) {
        printf("ERROR: Failed to save brain\n");
        melvin_destroy(g);
        return 1;
    }
    
    printf("SUCCESS: Brain saved!\n");
    printf("Check %s to verify format\n", brain_file);
    
    melvin_destroy(g);
    return 0;
}

