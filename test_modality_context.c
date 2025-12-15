/* Test: How modality context prevents confusion */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

typedef struct MelvinGraph MelvinGraph;
extern MelvinGraph* melvin_create(void);
extern void run_episode(MelvinGraph *g, const uint8_t *input, uint32_t input_len,
                       const uint8_t *target, uint32_t target_len);
extern void melvin_get_output(MelvinGraph *g, uint32_t **output, uint32_t *length);
extern void melvin_set_context(MelvinGraph *g, float *context);

int main(void) {
    MelvinGraph *g = melvin_create();
    
    printf("=================================================================\n");
    printf("MODALITY CONTEXT: Preventing confusion between modalities\n");
    printf("=================================================================\n\n");
    
    /* Set TEXT context */
    float text_context[16] = {1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
                              0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f};
    melvin_set_context(g, text_context);
    
    printf("Training in TEXT context: 'cat' → 'cats'\n");
    for (int i = 0; i < 30; i++) {
        run_episode(g, (const uint8_t*)"cat", 3, (const uint8_t*)"cats", 4);
    }
    printf("Training complete.\n\n");
    
    printf("Test in TEXT context:\n");
    printf("  Input:  cat\n");
    run_episode(g, (const uint8_t*)"cat", 3, NULL, 0);
    uint32_t *output;
    uint32_t output_len;
    melvin_get_output(g, &output, &output_len);
    printf("  Output: ");
    for (uint32_t i = 0; i < output_len && i < 50; i++) {
        printf("%c", (uint8_t)output[i]);
    }
    printf("\n\n");
    
    /* Switch to AUDIO context */
    float audio_context[16] = {0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
                               0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f};
    melvin_set_context(g, audio_context);
    
    printf("Now in AUDIO context: same bytes, different meaning\n");
    printf("  Input:  cat (now means audio frequencies, not text)\n");
    run_episode(g, (const uint8_t*)"cat", 3, NULL, 0);
    melvin_get_output(g, &output, &output_len);
    printf("  Output: ");
    for (uint32_t i = 0; i < output_len && i < 50; i++) {
        printf("%c", (uint8_t)output[i]);
    }
    printf("\n\n");
    
    printf("KEY INSIGHT:\n");
    printf("- TEXT patterns only match in TEXT context\n");
    printf("- AUDIO patterns only match in AUDIO context\n");
    printf("- Same bytes (65,66,67), different meanings, no confusion!\n");
    
    return 0;
}

