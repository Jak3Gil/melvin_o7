/* ============================================================================
 * REAL INTELLIGENCE TEST
 * 
 * Tests if Melvin is THINKING (using patterns/abstraction) or PARROTING (memorizing edges)
 * 
 * Test 1: Edge Generalization - "cat"→"cats" should generalize to "rat"→"rats"
 * Test 2: Pattern Prediction - "cat cat cat"→"cat" should predict "bat bat bat"→"bat"
 * Test 3: Context Completion - "the cat sat on the"→"mat" should complete "the X on the"→"mat"
 * ============================================================================ */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* External Melvin API */
typedef struct MelvinGraph MelvinGraph;

extern MelvinGraph* melvin_create(void);
extern void run_episode(MelvinGraph *g, const uint8_t *input, uint32_t input_len,
                       const uint8_t *target, uint32_t target_len);
extern void melvin_get_output(MelvinGraph *g, uint32_t **output, uint32_t *length);
extern void melvin_destroy(MelvinGraph *g);

/* Helper: Train multiple episodes */
void train(MelvinGraph *g, const char *input, const char *target, int episodes) {
    printf("Training: '%s' → '%s' (%d episodes)\n", input, target, episodes);
    for (int i = 0; i < episodes; i++) {
        run_episode(g, (const uint8_t*)input, strlen(input),
                   (const uint8_t*)target, strlen(target));
    }
}

/* Helper: Generate output (chat mode, no target) */
void generate(MelvinGraph *g, const char *input, char *output_buf, size_t buf_size) {
    run_episode(g, (const uint8_t*)input, strlen(input), NULL, 0);
    
    uint32_t *output = NULL;
    uint32_t length = 0;
    melvin_get_output(g, &output, &length);
    
    if (length > 0 && length < buf_size) {
        for (uint32_t i = 0; i < length; i++) {
            output_buf[i] = (char)output[i];
        }
        output_buf[length] = '\0';
    } else {
        output_buf[0] = '\0';
    }
}

/* Helper: Check if output matches expected */
int check_match(const char *output, const char *expected) {
    if (strcmp(output, expected) == 0) {
        printf("  ✓ PASS: Got '%s'\n", output);
        return 1;
    } else {
        printf("  ✗ FAIL: Got '%s', expected '%s'\n", output, expected);
        return 0;
    }
}

/* Test 1: Edge Generalization */
int test_edge_generalization(void) {
    printf("\n=== TEST 1: Edge Generalization ===\n");
    printf("Can it generalize learned edges to new inputs?\n\n");
    
    MelvinGraph *g = melvin_create();
    char output[256];
    int passed = 0;
    
    /* Train: pluralization */
    train(g, "cat", "cats", 30);
    train(g, "dog", "dogs", 30);
    train(g, "bat", "bats", 30);
    
    /* Test: New words with same pattern */
    printf("\nTesting generalization:\n");
    
    generate(g, "rat", output, sizeof(output));
    passed += check_match(output, "rats");
    
    generate(g, "mat", output, sizeof(output));
    passed += check_match(output, "mats");
    
    generate(g, "hat", output, sizeof(output));
    passed += check_match(output, "hats");
    
    melvin_destroy(g);
    
    printf("\nResult: %d/3 passed\n", passed);
    return passed >= 2;  /* Allow 1 failure */
}

/* Test 2: Pattern Prediction */
int test_pattern_prediction(void) {
    printf("\n=== TEST 2: Pattern Prediction ===\n");
    printf("Can it use patterns to predict next token?\n\n");
    
    MelvinGraph *g = melvin_create();
    char output[256];
    int passed = 0;
    
    /* Train: repetition prediction */
    train(g, "cat cat cat", "cat", 30);
    train(g, "dog dog dog", "dog", 30);
    train(g, "bat bat bat", "bat", 30);
    
    /* Test: New word with same pattern */
    printf("\nTesting pattern prediction:\n");
    
    generate(g, "rat rat rat", output, sizeof(output));
    passed += check_match(output, "rat");
    
    generate(g, "mat mat mat", output, sizeof(output));
    passed += check_match(output, "mat");
    
    melvin_destroy(g);
    
    printf("\nResult: %d/2 passed\n", passed);
    return passed >= 1;
}

/* Test 3: Context Completion */
int test_context_completion(void) {
    printf("\n=== TEST 3: Context Completion ===\n");
    printf("Can it complete sentences using learned context?\n\n");
    
    MelvinGraph *g = melvin_create();
    char output[256];
    int passed = 0;
    
    /* Train: context → completion */
    train(g, "the cat sat on the", "mat", 30);
    train(g, "the dog ran to the", "park", 30);
    train(g, "the bird flew to the", "tree", 30);
    
    /* Test: New sentence with similar context */
    printf("\nTesting context completion:\n");
    
    generate(g, "the rat sat on the", output, sizeof(output));
    /* Should output one of: "mat", "park", "tree" */
    if (strcmp(output, "mat") == 0 || strcmp(output, "park") == 0 || strcmp(output, "tree") == 0) {
        printf("  ✓ PASS: Got '%s' (valid completion)\n", output);
        passed++;
    } else {
        printf("  ✗ FAIL: Got '%s', expected one of {mat, park, tree}\n", output);
    }
    
    generate(g, "the ant went to the", output, sizeof(output));
    if (strcmp(output, "mat") == 0 || strcmp(output, "park") == 0 || strcmp(output, "tree") == 0) {
        printf("  ✓ PASS: Got '%s' (valid completion)\n", output);
        passed++;
    } else {
        printf("  ✗ FAIL: Got '%s', expected one of {mat, park, tree}\n", output);
    }
    
    melvin_destroy(g);
    
    printf("\nResult: %d/2 passed\n", passed);
    return passed >= 1;
}

/* Test 4: Data Efficiency */
int test_data_efficiency(void) {
    printf("\n=== TEST 4: Data Efficiency ===\n");
    printf("Can it learn from just 10 examples?\n\n");
    
    MelvinGraph *g = melvin_create();
    char output[256];
    int passed = 0;
    
    /* Train with ONLY 10 episodes (not 30) */
    printf("Training with only 10 episodes each:\n");
    train(g, "cat", "cats", 10);
    train(g, "dog", "dogs", 10);
    train(g, "bat", "bats", 10);
    
    /* Test: Should still generalize */
    printf("\nTesting after minimal training:\n");
    
    generate(g, "rat", output, sizeof(output));
    passed += check_match(output, "rats");
    
    generate(g, "hat", output, sizeof(output));
    passed += check_match(output, "hats");
    
    melvin_destroy(g);
    
    printf("\nResult: %d/2 passed\n", passed);
    printf("Data efficiency: %s\n", passed >= 1 ? "GOOD" : "POOR");
    return passed >= 1;
}

/* Test 5: Catastrophic Forgetting */
int test_catastrophic_forgetting(void) {
    printf("\n=== TEST 5: Catastrophic Forgetting ===\n");
    printf("Does learning new data destroy old learning?\n\n");
    
    MelvinGraph *g = melvin_create();
    char output[256];
    
    /* Phase 1: Learn A */
    printf("Phase 1: Learning animals:\n");
    train(g, "cat", "cats", 20);
    train(g, "dog", "dogs", 20);
    
    generate(g, "cat", output, sizeof(output));
    int before_correct = check_match(output, "cats");
    
    /* Phase 2: Learn B (different domain) */
    printf("\nPhase 2: Learning colors:\n");
    train(g, "red", "reds", 20);
    train(g, "blue", "blues", 20);
    
    /* Phase 3: Test A again */
    printf("\nPhase 3: Testing original learning:\n");
    generate(g, "cat", output, sizeof(output));
    int after_correct = check_match(output, "cats");
    
    melvin_destroy(g);
    
    printf("\nResult: %s catastrophic forgetting\n", 
           after_correct ? "NO" : "YES");
    return after_correct;
}

int main(void) {
    printf("╔══════════════════════════════════════════════════════════╗\n");
    printf("║         MELVIN REAL INTELLIGENCE TEST SUITE             ║\n");
    printf("║                                                          ║\n");
    printf("║  Testing: Is it THINKING or PARROTING?                  ║\n");
    printf("╚══════════════════════════════════════════════════════════╝\n");
    
    int tests_passed = 0;
    int total_tests = 5;
    
    if (test_edge_generalization()) tests_passed++;
    if (test_pattern_prediction()) tests_passed++;
    if (test_context_completion()) tests_passed++;
    if (test_data_efficiency()) tests_passed++;
    if (test_catastrophic_forgetting()) tests_passed++;
    
    printf("\n╔══════════════════════════════════════════════════════════╗\n");
    printf("║                    FINAL RESULTS                         ║\n");
    printf("╠══════════════════════════════════════════════════════════╣\n");
    printf("║  Tests Passed: %d/%d                                      ║\n", tests_passed, total_tests);
    printf("║                                                          ║\n");
    
    if (tests_passed >= 4) {
        printf("║  Verdict: ✓ THINKING (abstraction + generalization)     ║\n");
        printf("║  Status:  BEATS scaling laws expectations               ║\n");
    } else if (tests_passed >= 2) {
        printf("║  Verdict: ~ PARTIAL (some generalization)               ║\n");
        printf("║  Status:  Needs tuning                                  ║\n");
    } else {
        printf("║  Verdict: ✗ PARROTING (pure memorization)               ║\n");
        printf("║  Status:  Fundamental issues                            ║\n");
    }
    
    printf("╚══════════════════════════════════════════════════════════╝\n");
    
    return tests_passed >= 4 ? 0 : 1;
}

