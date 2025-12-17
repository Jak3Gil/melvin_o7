/* Test END_MARKER learning */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

typedef struct MelvinGraph MelvinGraph;
extern MelvinGraph* melvin_create(void);
extern void run_episode(MelvinGraph *g, const uint8_t *input, uint32_t input_len,
                       const uint8_t *target, uint32_t target_len);
extern void melvin_get_output(MelvinGraph *g, uint32_t **output, uint32_t *length);
extern void melvin_destroy(MelvinGraph *g);

int main(void) {
    printf("=== END_MARKER TEST ===\n\n");
    
    MelvinGraph *g = melvin_create();
    if (!g) {
        printf("Failed to create graph\n");
        return 1;
    }
    printf("Graph created successfully\n");
    
    /* Train simple pattern */
    printf("Training 'hi' -> 'hello'...\n");
    for (int i = 0; i < 5; i++) {
        run_episode(g, (const uint8_t*)"hi", 2, (const uint8_t*)"hello", 5);
        printf("  Episode %d complete\n", i + 1);
    }
    
    printf("\nTesting output...\n");
    run_episode(g, (const uint8_t*)"hi", 2, NULL, 0);
    
    uint32_t *output;
    uint32_t len;
    melvin_get_output(g, &output, &len);
    
    printf("Output (%u chars): ", len);
    for (uint32_t i = 0; i < len; i++) {
        printf("%c", (char)output[i]);
    }
    printf("\n");
    
    melvin_destroy(g);
    printf("\nTest complete!\n");
    return 0;
}
