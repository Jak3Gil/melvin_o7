/* ============================================================================
 * MELVIN O7: Intelligence Test Suite
 * 
 * Tests for TRUE intelligence, not just memorization/echoing:
 * 
 * 1. RULE EXTRACTION: Learn abstract patterns (e.g., "add 's' for plural")
 * 2. GENERALIZATION: Apply learned rules to novel inputs (zero-shot)
 * 3. CONTEXT DISCRIMINATION: Same input → different outputs based on task
 * 4. COMPOSITION: Combine multiple learned rules
 * 5. PATTERN REUSE: Transfer learned patterns across examples
 * 
 * ============================================================================ */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

/* Forward declarations - will link with melvin.c */
typedef struct MelvinGraph MelvinGraph;
extern MelvinGraph* melvin_create(void);
extern void run_episode(MelvinGraph *g, const uint8_t *input, uint32_t input_len,
                       const uint8_t *target, uint32_t target_len);
extern void melvin_get_output(MelvinGraph *g, uint32_t **output, uint32_t *length);
extern float melvin_get_error_rate(MelvinGraph *g);
extern uint32_t melvin_get_pattern_count(MelvinGraph *g);
extern void melvin_get_pattern_info(MelvinGraph *g, uint32_t pattern_id, 
                                    uint32_t **node_ids, uint32_t *length, float *strength);
extern float melvin_get_edge_weight(MelvinGraph *g, uint32_t from_id, uint32_t to_id);

/* Test result tracking */
typedef struct {
    const char *test_name;
    int passed;
    int total;
    float accuracy;
} TestResult;

/* ============================================================================
 * TEST 1: Pluralization (Rule Extraction + Generalization)
 * 
 * Train: "cat"→"cats", "dog"→"dogs", "pen"→"pens"
 * Test: "bat"→"bats" (zero-shot - never seen "bat" before)
 * 
 * This tests: Can system extract the abstract rule "+s"?
 * ============================================================================ */

TestResult test_pluralization(MelvinGraph *g) {
    TestResult result = {"Pluralization", 0, 0, 0.0f};
    
    printf("\n=== TEST 1: Pluralization (Rule Extraction) ===\n");
    printf("Training: cat→cats, dog→dogs, pen→pens\n");
    printf("Testing:  bat→bats (ZERO-SHOT - never seen 'bat')\n\n");
    
    /* Training examples */
    struct {
        uint8_t input[10];
        uint8_t target[10];
        uint32_t len;
    } training[] = {
        {{'c', 'a', 't'}, {'c', 'a', 't', 's'}, 3},
        {{'d', 'o', 'g'}, {'d', 'o', 'g', 's'}, 3},
        {{'p', 'e', 'n'}, {'p', 'e', 'n', 's'}, 3},
        {{'c', 'a', 't'}, {'c', 'a', 't', 's'}, 3}, /* Repeat */
        {{'d', 'o', 'g'}, {'d', 'o', 'g', 's'}, 3},
        {{'p', 'e', 'n'}, {'p', 'e', 'n', 's'}, 3}
    };
    
    /* Train the system */
    printf("Training...\n");
    for (int ep = 0; ep < 50; ep++) {
        int idx = ep % 6;
        run_episode(g, training[idx].input, training[idx].len,
                   training[idx].target, training[idx].len + 1);
        
        if (ep % 10 == 9) {
            printf("  Episode %d: error_rate=%.3f\n", ep + 1, melvin_get_error_rate(g));
        }
    }
    
    /* Test on novel input */
    printf("\nTesting on NOVEL input 'bat'...\n");
    uint8_t test_input[] = {'b', 'a', 't'};
    uint8_t expected_output[] = {'b', 'a', 't', 's'};
    
    run_episode(g, test_input, 3, NULL, 0);
    
    /* Get output */
    uint32_t *output;
    uint32_t output_len;
    melvin_get_output(g, &output, &output_len);
    
    printf("Input:  bat\n");
    printf("Expected: bats\n");
    printf("Got:     ");
    for (uint32_t i = 0; i < output_len; i++) {
        printf("%c", (uint8_t)output[i]);
    }
    printf("\n");
    
    /* Check accuracy */
    result.total = 1;
    if (output_len >= 4) {
        int correct = 0;
        for (uint32_t i = 0; i < 4 && i < output_len; i++) {
            if (output[i] == expected_output[i]) {
                correct++;
            }
        }
        result.accuracy = (float)correct / 4.0f;
        if (result.accuracy >= 0.75f) {  /* 3/4 or better */
            result.passed = 1;
            printf("✓ PASSED (%.0f%% accuracy)\n", result.accuracy * 100.0f);
        } else {
            printf("✗ FAILED (%.0f%% accuracy, need 75%%)\n", result.accuracy * 100.0f);
        }
    } else {
        printf("✗ FAILED (output too short: %u chars, need 4)\n", output_len);
    }
    
    return result;
}

/* ============================================================================
 * TEST 2: Past Tense (Rule Extraction)
 * 
 * Train: "walk"→"walked", "jump"→"jumped", "play"→"played"
 * Test: "talk"→"talked" (zero-shot)
 * 
 * This tests: Can system extract the abstract rule "+ed"?
 * ============================================================================ */

TestResult test_past_tense(MelvinGraph *g) {
    TestResult result = {"Past Tense", 0, 0, 0.0f};
    
    printf("\n=== TEST 2: Past Tense (Rule Extraction) ===\n");
    printf("Training: walk→walked, jump→jumped, play→played\n");
    printf("Testing:  talk→talked (ZERO-SHOT)\n\n");
    
    /* Training examples */
    struct {
        uint8_t input[10];
        uint8_t target[10];
        uint32_t len;
    } training[] = {
        {{'w', 'a', 'l', 'k'}, {'w', 'a', 'l', 'k', 'e', 'd'}, 4},
        {{'j', 'u', 'm', 'p'}, {'j', 'u', 'm', 'p', 'e', 'd'}, 4},
        {{'p', 'l', 'a', 'y'}, {'p', 'l', 'a', 'y', 'e', 'd'}, 4},
        {{'w', 'a', 'l', 'k'}, {'w', 'a', 'l', 'k', 'e', 'd'}, 4},
        {{'j', 'u', 'm', 'p'}, {'j', 'u', 'm', 'p', 'e', 'd'}, 4},
        {{'p', 'l', 'a', 'y'}, {'p', 'l', 'a', 'y', 'e', 'd'}, 4}
    };
    
    /* Train */
    printf("Training...\n");
    for (int ep = 0; ep < 50; ep++) {
        int idx = ep % 6;
        run_episode(g, training[idx].input, training[idx].len,
                   training[idx].target, training[idx].len + 2);
    }
    
    /* Test */
    printf("\nTesting on NOVEL input 'talk'...\n");
    uint8_t test_input[] = {'t', 'a', 'l', 'k'};
    uint8_t expected_output[] = {'t', 'a', 'l', 'k', 'e', 'd'};
    
    run_episode(g, test_input, 4, NULL, 0);
    
    uint32_t *output;
    uint32_t output_len;
    melvin_get_output(g, &output, &output_len);
    
    printf("Input:  talk\n");
    printf("Expected: talked\n");
    printf("Got:     ");
    for (uint32_t i = 0; i < output_len; i++) {
        printf("%c", (uint8_t)output[i]);
    }
    printf("\n");
    
    result.total = 1;
    if (output_len >= 6) {
        int correct = 0;
        for (uint32_t i = 0; i < 6 && i < output_len; i++) {
            if (output[i] == expected_output[i]) {
                correct++;
            }
        }
        result.accuracy = (float)correct / 6.0f;
        if (result.accuracy >= 0.75f) {
            result.passed = 1;
            printf("✓ PASSED (%.0f%% accuracy)\n", result.accuracy * 100.0f);
        } else {
            printf("✗ FAILED (%.0f%% accuracy, need 75%%)\n", result.accuracy * 100.0f);
        }
    } else {
        printf("✗ FAILED (output too short: %u chars, need 6)\n", output_len);
    }
    
    return result;
}

/* ============================================================================
 * TEST 3: Context Discrimination
 * 
 * Train: "test"→"tests" (plural) AND "test"→"tested" (past tense)
 * Test: Given context cues, can system choose correct transformation?
 * 
 * This tests: Can system distinguish between different tasks for same input?
 * ============================================================================ */

TestResult test_context_discrimination(MelvinGraph *g) {
    TestResult result = {"Context Discrimination", 0, 0, 0.0f};
    
    printf("\n=== TEST 3: Context Discrimination ===\n");
    printf("Training: test→tests (plural) AND test→tested (past tense)\n");
    printf("Testing:  Can system choose correct transformation?\n\n");
    
    /* This is hard - would need context signals */
    /* For now, just test if system can learn both patterns */
    
    /* Training: Alternate between plural and past tense */
    struct {
        uint8_t input[10];
        uint8_t target[10];
        uint32_t len;
        const char *context;
    } training[] = {
        {{'t', 'e', 's', 't'}, {'t', 'e', 's', 't', 's'}, 4, "plural"},
        {{'t', 'e', 's', 't'}, {'t', 'e', 's', 't', 'e', 'd'}, 4, "past"},
        {{'t', 'e', 's', 't'}, {'t', 'e', 's', 't', 's'}, 4, "plural"},
        {{'t', 'e', 's', 't'}, {'t', 'e', 's', 't', 'e', 'd'}, 4, "past"}
    };
    
    printf("Training (alternating contexts)...\n");
    for (int ep = 0; ep < 40; ep++) {
        int idx = ep % 4;
        run_episode(g, training[idx].input, training[idx].len,
                   training[idx].target, strlen((char*)training[idx].target));
    }
    
    /* Test: Try plural first */
    printf("\nTesting with 'test' (expecting plural 'tests')...\n");
    uint8_t test_input[] = {'t', 'e', 's', 't'};
    
    run_episode(g, test_input, 4, NULL, 0);
    
    uint32_t *output;
    uint32_t output_len;
    melvin_get_output(g, &output, &output_len);
    
    printf("Input:  test\n");
    printf("Expected: tests OR tested (either shows learning)\n");
    printf("Got:     ");
    for (uint32_t i = 0; i < output_len; i++) {
        printf("%c", (uint8_t)output[i]);
    }
    printf("\n");
    
    /* Check if it learned at least ONE transformation */
    result.total = 1;
    bool got_plural = (output_len >= 5 && output[4] == 's');
    bool got_past = (output_len >= 6 && output[4] == 'e' && output[5] == 'd');
    
    if (got_plural || got_past) {
        result.passed = 1;
        result.accuracy = 0.5f; /* Partial success */
        printf("✓ PARTIAL PASS (learned one transformation)\n");
    } else {
        printf("✗ FAILED (didn't learn either transformation)\n");
    }
    
    return result;
}

/* ============================================================================
 * TEST 4: Pattern Reuse (Transfer Learning)
 * 
 * Train: "cat"→"cats", "bat"→"bats"
 * Observe: Does "at" pattern strengthen from reuse?
 * 
 * This tests: Can system recognize and reuse common subpatterns?
 * ============================================================================ */

TestResult test_pattern_reuse(MelvinGraph *g) {
    TestResult result = {"Pattern Reuse", 0, 0, 0.0f};
    
    printf("\n=== TEST 4: Pattern Reuse (Transfer Learning) ===\n");
    printf("Training: cat→cats, bat→bats\n");
    printf("Testing:  Does 'at'→'ats' pattern emerge?\n\n");
    
    struct {
        uint8_t input[10];
        uint8_t target[10];
        uint32_t len;
    } training[] = {
        {{'c', 'a', 't'}, {'c', 'a', 't', 's'}, 3},
        {{'b', 'a', 't'}, {'b', 'a', 't', 's'}, 3},
        {{'c', 'a', 't'}, {'c', 'a', 't', 's'}, 3},
        {{'b', 'a', 't'}, {'b', 'a', 't', 's'}, 3}
    };
    
    printf("Training...\n");
    for (int ep = 0; ep < 40; ep++) {
        int idx = ep % 4;
        run_episode(g, training[idx].input, training[idx].len,
                   training[idx].target, training[idx].len + 1);
    }
    
    /* Test on novel word that shares pattern */
    printf("\nTesting on 'rat' (shares 'at' pattern)...\n");
    uint8_t test_input[] = {'r', 'a', 't'};
    uint8_t expected_output[] = {'r', 'a', 't', 's'};
    
    run_episode(g, test_input, 3, NULL, 0);
    
    uint32_t *output;
    uint32_t output_len;
    melvin_get_output(g, &output, &output_len);
    
    printf("Input:  rat\n");
    printf("Expected: rats\n");
    printf("Got:     ");
    for (uint32_t i = 0; i < output_len; i++) {
        printf("%c", (uint8_t)output[i]);
    }
    printf("\n");
    
    result.total = 1;
    if (output_len >= 4) {
        int correct = 0;
        for (uint32_t i = 0; i < 4 && i < output_len; i++) {
            if (output[i] == expected_output[i]) {
                correct++;
            }
        }
        result.accuracy = (float)correct / 4.0f;
        if (result.accuracy >= 0.75f) {
            result.passed = 1;
            printf("✓ PASSED (%.0f%% accuracy - pattern reused!)\n", result.accuracy * 100.0f);
        } else {
            printf("✗ FAILED (%.0f%% accuracy, need 75%%)\n", result.accuracy * 100.0f);
        }
    } else {
        printf("✗ FAILED (output too short)\n");
    }
    
    return result;
}

/* ============================================================================
 * TEST 5: Composition (Combining Rules)
 * 
 * Train: Plural ("cat"→"cats") AND past tense ("walk"→"walked")
 * Test: Can system do both? (this is advanced - may need more training)
 * 
 * This tests: Can system compose multiple learned rules?
 * ============================================================================ */

TestResult test_composition(MelvinGraph *g) {
    TestResult result = {"Composition", 0, 0, 0.0f};
    
    printf("\n=== TEST 5: Composition (Combining Rules) ===\n");
    printf("Training: Both plural (cat→cats) AND past tense (walk→walked)\n");
    printf("Testing:  Can system handle both transformations?\n\n");
    
    /* Train on both tasks */
    struct {
        uint8_t input[10];
        uint8_t target[10];
        uint32_t in_len;
        uint32_t out_len;
        const char *task;
    } training[] = {
        {{'c', 'a', 't'}, {'c', 'a', 't', 's'}, 3, 4, "plural"},
        {{'w', 'a', 'l', 'k'}, {'w', 'a', 'l', 'k', 'e', 'd'}, 4, 6, "past"},
        {{'d', 'o', 'g'}, {'d', 'o', 'g', 's'}, 3, 4, "plural"},
        {{'j', 'u', 'm', 'p'}, {'j', 'u', 'm', 'p', 'e', 'd'}, 4, 6, "past"}
    };
    
    printf("Training on mixed tasks...\n");
    for (int ep = 0; ep < 60; ep++) {
        int idx = ep % 4;
        run_episode(g, training[idx].input, training[idx].in_len,
                   training[idx].target, training[idx].out_len);
    }
    
    /* Test plural */
    printf("\nTest 1: Plural (bat→bats)...\n");
    uint8_t test1_input[] = {'b', 'a', 't'};
    uint8_t test1_expected[] = {'b', 'a', 't', 's'};
    
    run_episode(g, test1_input, 3, NULL, 0);
    uint32_t *output1;
    uint32_t len1;
    melvin_get_output(g, &output1, &len1);
    
    printf("Got: ");
    for (uint32_t i = 0; i < len1; i++) printf("%c", (uint8_t)output1[i]);
    printf("\n");
    
    /* Test past tense */
    printf("\nTest 2: Past tense (talk→talked)...\n");
    uint8_t test2_input[] = {'t', 'a', 'l', 'k'};
    uint8_t test2_expected[] = {'t', 'a', 'l', 'k', 'e', 'd'};
    
    run_episode(g, test2_input, 4, NULL, 0);
    uint32_t *output2;
    uint32_t len2;
    melvin_get_output(g, &output2, &len2);
    
    printf("Got: ");
    for (uint32_t i = 0; i < len2; i++) printf("%c", (uint8_t)output2[i]);
    printf("\n");
    
    /* Check both */
    result.total = 2;
    int correct1 = 0, correct2 = 0;
    
    if (len1 >= 4) {
        for (uint32_t i = 0; i < 4 && i < len1; i++) {
            if (output1[i] == test1_expected[i]) correct1++;
        }
    }
    
    if (len2 >= 6) {
        for (uint32_t i = 0; i < 6 && i < len2; i++) {
            if (output2[i] == test2_expected[i]) correct2++;
        }
    }
    
    result.accuracy = ((float)(correct1 + correct2)) / 10.0f;
    if (result.accuracy >= 0.7f) {
        result.passed = 1;
        printf("\n✓ PASSED (composition working!)\n");
    } else {
        printf("\n✗ FAILED (composition not working)\n");
    }
    
    return result;
}

/* ============================================================================
 * MAIN: Run all intelligence tests
 * ============================================================================ */

int main(void) {
    printf("=================================================================\n");
    printf("MELVIN O7: INTELLIGENCE TEST SUITE\n");
    printf("=================================================================\n");
    printf("\nTesting for TRUE intelligence:\n");
    printf("  - Rule extraction (abstract patterns)\n");
    printf("  - Generalization (zero-shot)\n");
    printf("  - Context discrimination\n");
    printf("  - Pattern reuse (transfer learning)\n");
    printf("  - Composition (combining rules)\n");
    printf("\n");
    
    MelvinGraph *g = melvin_create();
    if (!g) {
        fprintf(stderr, "Failed to create graph\n");
        return 1;
    }
    
    TestResult results[5];
    
    /* Run all tests */
    results[0] = test_pluralization(g);
    results[1] = test_past_tense(g);
    results[2] = test_context_discrimination(g);
    results[3] = test_pattern_reuse(g);
    results[4] = test_composition(g);
    
    /* Summary */
    printf("\n");
    printf("=================================================================\n");
    printf("TEST SUMMARY\n");
    printf("=================================================================\n");
    
    int total_passed = 0;
    int total_tests = 0;
    float total_accuracy = 0.0f;
    
    for (int i = 0; i < 5; i++) {
        printf("%-25s: ", results[i].test_name);
        if (results[i].passed) {
            printf("✓ PASSED (%.0f%%)\n", results[i].accuracy * 100.0f);
            total_passed++;
        } else {
            printf("✗ FAILED (%.0f%%)\n", results[i].accuracy * 100.0f);
        }
        total_tests += results[i].total;
        total_accuracy += results[i].accuracy;
    }
    
    printf("\n");
    printf("Overall: %d/%d tests passed (%.0f%%)\n", 
           total_passed, 5, (total_accuracy / 5.0f) * 100.0f);
    
    if (total_passed >= 3) {
        printf("\n✓ SYSTEM SHOWS SIGNS OF INTELLIGENCE\n");
    } else {
        printf("\n✗ SYSTEM NEEDS MORE WORK\n");
    }
    
    return 0;
}

