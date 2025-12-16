/* Quick check: Does the fix work? */
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
    printf("Quick Check: Edge properties vs binary checks\n");
    printf("==============================================\n\n");
    
    MelvinGraph *g = melvin_create();
    
    /* Train: cat → cats (30 times) */
    printf("Training: cat → cats (30 episodes)...\n");
    for (int i = 0; i < 30; i++) {
        run_episode(g, (const uint8_t*)"cat", 3, (const uint8_t*)"cats", 4);
        if ((i + 1) % 10 == 0) printf("  Episode %d/30\n", i + 1);
    }
    
    /* Test 1: Trained input */
    printf("\nTest 1: cat → ?\n");
    run_episode(g, (const uint8_t*)"cat", 3, NULL, 0);
    uint32_t *output;
    uint32_t length;
    melvin_get_output(g, &output, &length);
    
    printf("  Output: \"");
    for (uint32_t i = 0; i < length; i++) {
        printf("%c", (char)output[i]);
    }
    printf("\" (expected: \"cats\")\n");
    
    int test1_pass = (length == 4 && 
                      output[0] == 'c' && output[1] == 'a' && 
                      output[2] == 't' && output[3] == 's');
    printf("  %s\n", test1_pass ? "✓ PASS" : "✗ FAIL");
    
    /* Test 2: Generalization */
    printf("\nTest 2: bat → ? (zero-shot)\n");
    run_episode(g, (const uint8_t*)"bat", 3, NULL, 0);
    melvin_get_output(g, &output, &length);
    
    printf("  Output: \"");
    for (uint32_t i = 0; i < length; i++) {
        printf("%c", (char)output[i]);
    }
    printf("\" (expected: \"bats\")\n");
    
    int test2_pass = (length == 4 && 
                      output[0] == 'b' && output[1] == 'a' && 
                      output[2] == 't' && output[3] == 's');
    printf("  %s\n", test2_pass ? "✓ PASS" : "✗ FAIL");
    
    /* Test 3: Different generalization */
    printf("\nTest 3: mat → ? (zero-shot)\n");
    run_episode(g, (const uint8_t*)"mat", 3, NULL, 0);
    melvin_get_output(g, &output, &length);
    
    printf("  Output: \"");
    for (uint32_t i = 0; i < length; i++) {
        printf("%c", (char)output[i]);
    }
    printf("\" (expected: \"mats\")\n");
    
    int test3_pass = (length == 4 && 
                      output[0] == 'm' && output[1] == 'a' && 
                      output[2] == 't' && output[3] == 's');
    printf("  %s\n", test3_pass ? "✓ PASS" : "✗ FAIL");
    
    printf("\n==============================================\n");
    printf("Results: %d/3 tests passed\n", test1_pass + test2_pass + test3_pass);
    
    if (test1_pass + test2_pass + test3_pass >= 2) {
        printf("✅ SUCCESS: Edge properties working!\n");
        printf("   - Learning from training data\n");
        printf("   - Generalizing to new inputs\n");
    } else {
        printf("❌ PROBLEM: Still not learning properly\n");
        printf("   - Check if edge weights are being updated\n");
        printf("   - Check if success_count is incrementing\n");
    }
    
    melvin_destroy(g);
    return 0;
}

