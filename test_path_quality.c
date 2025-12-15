#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

/* Forward declarations */
typedef struct MelvinGraph MelvinGraph;
extern MelvinGraph* melvin_create(void);
extern void melvin_destroy(MelvinGraph *g);
extern void run_episode(MelvinGraph *g, const uint8_t *input, uint32_t input_len,
                       const uint8_t *target, uint32_t target_len);
extern void melvin_get_output(MelvinGraph *g, uint32_t **output, uint32_t *length);

void print_output(MelvinGraph *g, const char *label) {
    uint32_t *output;
    uint32_t output_len;
    melvin_get_output(g, &output, &output_len);
    
    printf("%s: \"", label);
    for (uint32_t i = 0; i < output_len; i++) {
        if (output[i] < 256) {
            printf("%c", (char)output[i]);
        }
    }
    printf("\" (length: %u)\n", output_len);
}

void train_pair(MelvinGraph *g, const char *input, const char *target, int times) {
    printf("Training '%s' -> '%s' (%d times)...\n", input, target, times);
    for (int i = 0; i < times; i++) {
        run_episode(g, (const uint8_t*)input, strlen(input), 
                   (const uint8_t*)target, strlen(target));
    }
}

int main(void) {
    printf("========================================\n");
    printf("PATH QUALITY TESTING\n");
    printf("========================================\n\n");
    
    MelvinGraph *g = melvin_create();
    if (!g) {
        fprintf(stderr, "Failed to create graph\n");
        return 1;
    }
    
    /* Test 1: Simple training and inference */
    printf("Test 1: Simple Training\n");
    printf("----------------------\n");
    train_pair(g, "hello", "world", 10);
    printf("Now test 'hello' -> should output 'world':\n");
    run_episode(g, (const uint8_t*)"hello", 5, NULL, 0);
    print_output(g, "Output");
    printf("\n");
    
    /* Test 2: Pattern generalization */
    printf("Test 2: Pattern Generalization\n");
    printf("-------------------------------\n");
    train_pair(g, "cat", "cats", 10);
    train_pair(g, "dog", "dogs", 10);
    train_pair(g, "bird", "birds", 10);
    printf("Now test 'bat' -> should output 'bats' (generalization):\n");
    run_episode(g, (const uint8_t*)"bat", 3, NULL, 0);
    print_output(g, "Output");
    printf("Expected: \"bats\"\n\n");
    
    /* Test 3: Q&A pattern */
    printf("Test 3: Q&A Pattern\n");
    printf("-------------------\n");
    train_pair(g, "What is 2+2?", "4", 10);
    train_pair(g, "What is 3+3?", "6", 10);
    train_pair(g, "What is 4+4?", "8", 10);
    printf("Now test 'What is 5+5?' -> should output '10':\n");
    run_episode(g, (const uint8_t*)"What is 5+5?", 12, NULL, 0);
    print_output(g, "Output");
    printf("Expected: \"10\"\n\n");
    
    /* Test 4: Sequential coherence */
    printf("Test 4: Sequential Coherence\n");
    printf("-----------------------------\n");
    train_pair(g, "The cat sat", "on the mat", 10);
    printf("Now test 'The cat sat' -> should output 'on the mat':\n");
    run_episode(g, (const uint8_t*)"The cat sat", 11, NULL, 0);
    print_output(g, "Output");
    printf("Expected: \"on the mat\"\n\n");
    
    /* Test 5: Multiple training, then inference */
    printf("Test 5: Multiple Patterns\n");
    printf("--------------------------\n");
    train_pair(g, "hello", "hi", 5);
    train_pair(g, "hello", "world", 5);
    printf("Now test 'hello' -> should prefer 'world' (more training):\n");
    run_episode(g, (const uint8_t*)"hello", 5, NULL, 0);
    print_output(g, "Output");
    printf("\n");
    
    /* Test 6: Context sensitivity */
    printf("Test 6: Context Sensitivity\n");
    printf("---------------------------\n");
    train_pair(g, "capital of France", "Paris", 10);
    train_pair(g, "capital of Italy", "Rome", 10);
    printf("Now test 'capital of France' -> should output 'Paris':\n");
    run_episode(g, (const uint8_t*)"capital of France", 17, NULL, 0);
    print_output(g, "Output");
    printf("Expected: \"Paris\"\n\n");
    
    printf("========================================\n");
    printf("TESTING COMPLETE\n");
    printf("========================================\n");
    
    melvin_destroy(g);
    return 0;
}
