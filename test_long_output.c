/* Test: Can Melvin generate essay-length output? */

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
    MelvinGraph *g = melvin_create();
    
    printf("=== LONG OUTPUT TEST ===\n\n");
    
    /* Train with progressively longer outputs */
    printf("Training with longer sequences...\n");
    
    /* Short to long pattern */
    const char *train_pairs[][2] = {
        {"hi", "hello there, how are you today?"},
        {"yo", "hey what's up, good to see you!"},
        {"sup", "not much, just hanging out here."},
        {"hey", "hi there! nice to meet you today."},
    };
    
    for (int ep = 0; ep < 2; ep++) {
        printf("Starting epoch %d\n", ep + 1);
        fflush(stdout);
        for (int i = 0; i < 4; i++) {
            printf("  Training pair %d\n", i + 1);
            fflush(stdout);
            run_episode(g, (const uint8_t*)train_pairs[i][0], strlen(train_pairs[i][0]),
                       (const uint8_t*)train_pairs[i][1], strlen(train_pairs[i][1]));
            printf("  Pair %d complete\n", i + 1);
            fflush(stdout);
        }
        printf("Epoch %d complete\n", ep + 1);
        fflush(stdout);
    }
    
    printf("Training complete!\n");
    fflush(stdout);
    
    printf("\n=== Testing Long Output Generation ===\n\n");
    fflush(stdout);
    
    /* Test 1: Trained input */
    printf("Input: 'hi' (2 chars)\n");
    printf("Expected: ~30 chars\n");
    run_episode(g, (const uint8_t*)"hi", 2, NULL, 0);
    uint32_t *output;
    uint32_t len;
    melvin_get_output(g, &output, &len);
    printf("Output (%u chars): ", len);
    for (uint32_t i = 0; i < len && i < 100; i++) {
        printf("%c", (char)output[i]);
    }
    printf("\n\n");
    
    /* Test 2: Novel input */
    printf("Input: 'yo' (2 chars)\n");
    run_episode(g, (const uint8_t*)"yo", 2, NULL, 0);
    melvin_get_output(g, &output, &len);
    printf("Output (%u chars): ", len);
    for (uint32_t i = 0; i < len && i < 100; i++) {
        printf("%c", (char)output[i]);
    }
    printf("\n\n");
    
    /* Test 3: Completely novel */
    printf("Input: 'ok' (2 chars, never seen)\n");
    run_episode(g, (const uint8_t*)"ok", 2, NULL, 0);
    melvin_get_output(g, &output, &len);
    printf("Output (%u chars): ", len);
    for (uint32_t i = 0; i < len && i < 100; i++) {
        printf("%c", (char)output[i]);
    }
    printf("\n\n");
    
    /* Test 4: Single char input */
    printf("Input: 'x' (1 char, never seen)\n");
    run_episode(g, (const uint8_t*)"x", 1, NULL, 0);
    melvin_get_output(g, &output, &len);
    printf("Output (%u chars): ", len);
    for (uint32_t i = 0; i < len && i < 100; i++) {
        printf("%c", (char)output[i]);
    }
    printf("\n\n");
    
    printf("=== Output Length Analysis ===\n");
    printf("If outputs are >10 chars from 2-char inputs, variable length is working!\n");
    
    melvin_destroy(g);
    return 0;
}

