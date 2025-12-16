/* ============================================================================
 * SCALING LAWS TEST: Real Questions
 * 
 * Tests:
 * 1. Data Efficiency: Learn from 100 sentences (not billions)
 * 2. Generalization: Zero-shot on unseen patterns
 * 3. Transform vs Parrot: Does it learn rules or memorize?
 * 4. Context Window: Can it handle long sequences?
 * 5. Catastrophic Forgetting: Does old knowledge degrade?
 * ============================================================================ */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdint.h>

/* Forward declarations */
typedef struct MelvinGraph MelvinGraph;
extern MelvinGraph* melvin_create(void);
extern void run_episode(MelvinGraph *g, const uint8_t *input, uint32_t input_len,
                       const uint8_t *target, uint32_t target_len);
extern void melvin_get_output(MelvinGraph *g, uint32_t **output, uint32_t *length);
extern void melvin_destroy(MelvinGraph *g);

/* Test helpers */
void train(MelvinGraph *g, const char *input, const char *target, int episodes) {
    for (int i = 0; i < episodes; i++) {
        run_episode(g, (const uint8_t*)input, strlen(input), 
                   (const uint8_t*)target, strlen(target));
    }
}

int test_output(MelvinGraph *g, const char *input, const char *expected) {
    run_episode(g, (const uint8_t*)input, strlen(input), NULL, 0);
    
    uint32_t *output;
    uint32_t length;
    melvin_get_output(g, &output, &length);
    
    /* Check if output matches expected */
    if (length != strlen(expected)) {
        printf("  ❌ Length mismatch: got %u, expected %zu\n", length, strlen(expected));
        printf("     Got: \"");
        for (uint32_t i = 0; i < length && i < 50; i++) {
            printf("%c", (char)output[i]);
        }
        printf("\"\n");
        return 0;
    }
    
    for (uint32_t i = 0; i < length; i++) {
        if (output[i] != (uint8_t)expected[i]) {
            printf("  ❌ Mismatch at pos %u: got '%c', expected '%c'\n", 
                   i, (char)output[i], expected[i]);
            printf("     Got: \"");
            for (uint32_t j = 0; j < length; j++) {
                printf("%c", (char)output[j]);
            }
            printf("\"\n");
            return 0;
        }
    }
    
    printf("  ✓ Correct: \"%s\" → \"%s\"\n", input, expected);
    return 1;
}

/* ============================================================================
 * TEST 1: TRANSFORMATION (Not Parroting)
 * Learn pluralization rule: add "s"
 * ============================================================================ */
void test_pluralization() {
    printf("\n========================================\n");
    printf("TEST 1: PLURALIZATION (Transform, Not Parrot)\n");
    printf("========================================\n");
    printf("Training on singular → plural...\n");
    
    MelvinGraph *g = melvin_create();
    
    /* Train on 20 examples (data efficiency) */
    const char *train_data[][2] = {
        {"cat", "cats"},
        {"dog", "dogs"},
        {"bat", "bats"},
        {"rat", "rats"},
        {"hat", "hats"},
        {"mat", "mats"},
        {"car", "cars"},
        {"bar", "bars"},
        {"pig", "pigs"},
        {"bug", "bugs"},
        {"cup", "cups"},
        {"map", "maps"},
        {"pen", "pens"},
        {"pot", "pots"},
        {"bed", "beds"},
        {"leg", "legs"},
        {"arm", "arms"},
        {"eye", "eyes"},
        {"ear", "ears"},
        {"toe", "toes"}
    };
    
    for (int i = 0; i < 20; i++) {
        train(g, train_data[i][0], train_data[i][1], 10);
        if (i % 5 == 4) {
            printf("  Trained %d/20 pairs...\n", i + 1);
        }
    }
    
    printf("\nTesting generalization (zero-shot):\n");
    
    /* Test on UNSEEN words (zero-shot) */
    int correct = 0;
    int total = 5;
    
    correct += test_output(g, "fox", "foxs");  /* Will it add 's'? */
    correct += test_output(g, "box", "boxs");
    correct += test_output(g, "cow", "cows");
    correct += test_output(g, "hen", "hens");
    correct += test_output(g, "ant", "ants");
    
    printf("\n📊 Zero-shot accuracy: %d/%d (%.1f%%)\n", correct, total, 
           100.0f * correct / total);
    
    if (correct >= 4) {
        printf("✅ PASS: Learned transformation rule!\n");
    } else {
        printf("❌ FAIL: Still parroting, not generalizing\n");
    }
    
    melvin_destroy(g);
}

/* ============================================================================
 * TEST 2: COMPLETION (Creative/Predictive)
 * Complete sentences with context
 * ============================================================================ */
void test_completion() {
    printf("\n========================================\n");
    printf("TEST 2: SENTENCE COMPLETION (Predictive)\n");
    printf("========================================\n");
    printf("Training on sentence completions...\n");
    
    MelvinGraph *g = melvin_create();
    
    /* Train: partial → complete */
    train(g, "the cat sat on the", "the cat sat on the mat", 20);
    train(g, "the dog ran in the", "the dog ran in the park", 20);
    train(g, "the bird flew over the", "the bird flew over the tree", 20);
    train(g, "the fish swam in the", "the fish swam in the pond", 20);
    
    printf("\nTesting completion:\n");
    
    int correct = 0;
    correct += test_output(g, "the cat sat on the", "the cat sat on the mat");
    correct += test_output(g, "the dog ran in the", "the dog ran in the park");
    
    /* Zero-shot: new sentence structure */
    printf("\nZero-shot (new structure):\n");
    correct += test_output(g, "the cow stood in the", "the cow stood in the field");
    
    printf("\n📊 Completion accuracy: %d/3\n", correct);
    
    if (correct >= 2) {
        printf("✅ PASS: Can complete from context!\n");
    } else {
        printf("❌ FAIL: Not using context for completion\n");
    }
    
    melvin_destroy(g);
}

/* ============================================================================
 * TEST 3: DATA EFFICIENCY
 * Learn from 10-50 examples (not millions)
 * ============================================================================ */
void test_data_efficiency() {
    printf("\n========================================\n");
    printf("TEST 3: DATA EFFICIENCY\n");
    printf("========================================\n");
    
    int episodes[] = {5, 10, 20, 50};
    
    for (int e = 0; e < 4; e++) {
        printf("\nTraining with %d episodes:\n", episodes[e]);
        
        MelvinGraph *g = melvin_create();
        
        train(g, "cat", "cats", episodes[e]);
        train(g, "dog", "dogs", episodes[e]);
        train(g, "bat", "bats", episodes[e]);
        
        int correct = 0;
        correct += test_output(g, "cat", "cats");
        correct += test_output(g, "dog", "dogs");
        correct += test_output(g, "bat", "bats");
        correct += test_output(g, "rat", "rats");  /* Zero-shot */
        
        printf("📊 Accuracy with %d episodes: %d/4 (%.1f%%)\n", 
               episodes[e], correct, 25.0f * correct);
        
        melvin_destroy(g);
    }
}

/* ============================================================================
 * TEST 4: CONTEXT WINDOW
 * Can it handle long sequences? (No limit?)
 * ============================================================================ */
void test_context_window() {
    printf("\n========================================\n");
    printf("TEST 4: CONTEXT WINDOW (Sequence Length)\n");
    printf("========================================\n");
    
    MelvinGraph *g = melvin_create();
    
    /* Test progressively longer sequences */
    int lengths[] = {10, 50, 100, 500};
    
    for (int l = 0; l < 4; l++) {
        int len = lengths[l];
        
        /* Create long sequence: "a b c ... z a b c ..." */
        char *input = malloc(len * 2);
        char *target = malloc(len * 2 + 2);
        
        for (int i = 0; i < len; i++) {
            input[i * 2] = 'a' + (i % 26);
            input[i * 2 + 1] = ' ';
            target[i * 2] = input[i * 2];
            target[i * 2 + 1] = ' ';
        }
        input[len * 2 - 1] = '\0';
        target[len * 2 - 1] = 'x';  /* Add 'x' at end */
        target[len * 2] = '\0';
        
        printf("\nSequence length: %d chars\n", len * 2);
        
        /* Train */
        train(g, input, target, 10);
        
        /* Test */
        run_episode(g, (const uint8_t*)input, strlen(input), NULL, 0);
        uint32_t *output;
        uint32_t length;
        melvin_get_output(g, &output, &length);
        
        /* Check if last char is 'x' */
        if (length > 0 && output[length - 1] == 'x') {
            printf("  ✓ Handled %d chars, correct completion\n", len * 2);
        } else {
            printf("  ❌ Failed at %d chars\n", len * 2);
        }
        
        free(input);
        free(target);
    }
    
    melvin_destroy(g);
}

/* ============================================================================
 * TEST 5: CATASTROPHIC FORGETTING
 * Does old knowledge degrade when learning new?
 * ============================================================================ */
void test_catastrophic_forgetting() {
    printf("\n========================================\n");
    printf("TEST 5: CATASTROPHIC FORGETTING\n");
    printf("========================================\n");
    
    MelvinGraph *g = melvin_create();
    
    /* Phase 1: Learn first set */
    printf("\nPhase 1: Learning cats/dogs...\n");
    train(g, "cat", "cats", 30);
    train(g, "dog", "dogs", 30);
    
    int phase1_correct = 0;
    phase1_correct += test_output(g, "cat", "cats");
    phase1_correct += test_output(g, "dog", "dogs");
    printf("Phase 1 accuracy: %d/2\n", phase1_correct);
    
    /* Phase 2: Learn second set (different patterns) */
    printf("\nPhase 2: Learning numbers...\n");
    train(g, "one", "1", 30);
    train(g, "two", "2", 30);
    train(g, "three", "3", 30);
    
    int phase2_correct = 0;
    phase2_correct += test_output(g, "one", "1");
    phase2_correct += test_output(g, "two", "2");
    printf("Phase 2 accuracy: %d/2\n", phase2_correct);
    
    /* Test Phase 1 again - did we forget? */
    printf("\nRetesting Phase 1 (forgetting check):\n");
    int retest_correct = 0;
    retest_correct += test_output(g, "cat", "cats");
    retest_correct += test_output(g, "dog", "dogs");
    
    float retention = 100.0f * retest_correct / phase1_correct;
    printf("\n📊 Retention: %.1f%% (lost %.1f%%)\n", retention, 100.0f - retention);
    
    if (retention >= 80.0f) {
        printf("✅ PASS: <20%% forgetting\n");
    } else {
        printf("❌ FAIL: >20%% forgetting\n");
    }
    
    melvin_destroy(g);
}

/* ============================================================================
 * MAIN: Run all scaling law tests
 * ============================================================================ */
int main(void) {
    printf("╔════════════════════════════════════════╗\n");
    printf("║  SCALING LAWS AUDIT: The REAL Tests  ║\n");
    printf("╚════════════════════════════════════════╝\n");
    
    test_pluralization();
    test_completion();
    test_data_efficiency();
    test_context_window();
    test_catastrophic_forgetting();
    
    printf("\n╔════════════════════════════════════════╗\n");
    printf("║           TESTS COMPLETE              ║\n");
    printf("╚════════════════════════════════════════╝\n");
    
    return 0;
}

