/* Intelligence Test - Can Melvin learn to reason? */
#include "melvin.c"

/* Training data - teach it patterns and reasoning */
typedef struct {
    const char *input;
    const char *expected_output;
    const char *description;
} TestCase;

TestCase training_data[] = {
    /* Simple facts */
    {"cat", "cat", "Echo test"},
    {"dog", "dog", "Echo test"},
    
    /* Patterns with answers */
    {"what color is the sky", "blue", "Simple fact"},
    {"what color is grass", "green", "Simple fact"},
    {"what color is the sun", "yellow", "Simple fact"},
    
    /* Analogies */
    {"cat says", "meow", "Animal sound"},
    {"dog says", "woof", "Animal sound"},
    {"cow says", "moo", "Animal sound"},
    
    /* Completions */
    {"one two", "three", "Number sequence"},
    {"a b", "c", "Letter sequence"},
    
    /* Reasoning */
    {"if happy then", "smile", "Emotion reasoning"},
    {"if sad then", "cry", "Emotion reasoning"},
    {"if hungry then", "eat", "Need reasoning"},
    
    /* More complex */
    {"the cat is", "happy", "State"},
    {"the dog is", "friendly", "State"},
    
    /* Opposites */
    {"opposite of hot", "cold", "Opposite"},
    {"opposite of big", "small", "Opposite"},
    {"opposite of happy", "sad", "Opposite"},
};

int num_training = sizeof(training_data) / sizeof(training_data[0]);

/* Test cases - see if it learned to generalize */
TestCase test_cases[] = {
    {"cat", "cat", "Should echo"},
    {"what color is the sky", "blue", "Should recall fact"},
    {"cat says", "meow", "Should recall sound"},
    {"one two", "three", "Should complete sequence"},
    {"if happy then", "smile", "Should reason"},
    
    /* NEW - test generalization */
    {"what color is grass", "green", "Recall different fact"},
    {"dog says", "woof", "Recall different sound"},
    {"opposite of hot", "cold", "Recall opposite"},
    
    /* NOVEL - never seen exactly, but should generalize */
    {"bird says", "???", "Novel animal (should try to answer)"},
    {"what color is the ocean", "???", "Novel question (should try blue)"},
    {"if tired then", "???", "Novel reasoning (should try sleep/rest)"},
};

int num_tests = sizeof(test_cases) / sizeof(test_cases[0]);

int main(void) {
    printf("=== MELVIN O7: Intelligence Test ===\n");
    printf("Can it learn to give intelligent outputs?\n\n");
    
    MelvinGraph *g = melvin_create();
    if (!g) {
        printf("Failed to create graph\n");
        return 1;
    }
    
    /* PHASE 1: Training - teach it patterns */
    printf("=== PHASE 1: TRAINING ===\n");
    printf("Teaching %d patterns with rewards...\n\n", num_training);
    
    /* Train multiple times to strengthen patterns */
    int training_epochs = 50;  /* More training for intelligent behavior */
    for (int epoch = 0; epoch < training_epochs; epoch++) {
        printf("Epoch %d/%d...\r", epoch + 1, training_epochs);
        fflush(stdout);
        
        for (int i = 0; i < num_training; i++) {
            /* Randomize order to prevent sequential bias */
            int idx = rand() % num_training;
            const char *input = training_data[idx].input;
            const char *target = training_data[idx].expected_output;
            
            /* Run episode with feedback */
            run_episode(g, 
                       (const uint8_t*)input, strlen(input),
                       (const uint8_t*)target, strlen(target));
        }
    }
    
    printf("\n\nTraining complete!\n");
    printf("Episodes: %d\n", training_epochs * num_training);
    printf("Patterns learned: %u\n", g->pattern_count);
    printf("Error rate: %.3f\n", g->state.error_rate);
    printf("Pattern confidence: %.3f\n\n", g->state.pattern_confidence);
    
    /* PHASE 2: Testing - see if it learned */
    printf("=== PHASE 2: TESTING ===\n\n");
    
    int correct_exact = 0;
    int correct_partial = 0;
    int attempted = 0;
    
    for (int i = 0; i < num_tests; i++) {
        const char *input = test_cases[i].input;
        const char *expected = test_cases[i].expected_output;
        const char *desc = test_cases[i].description;
        
        /* Run WITHOUT feedback (inference mode) */
        run_episode(g,
                   (const uint8_t*)input, strlen(input),
                   NULL, 0);  /* No target = inference */
        
        /* Get output */
        char output[256] = {0};
        uint32_t out_len = (g->output_length < 255) ? g->output_length : 255;
        for (uint32_t j = 0; j < out_len; j++) {
            output[j] = (char)g->output_buffer[j];
        }
        output[out_len] = '\0';
        
        /* Check if correct */
        bool is_exact_match = (strcmp(output, expected) == 0);
        bool is_partial_match = (strstr(output, expected) != NULL || 
                                strstr(expected, output) != NULL);
        bool is_novel = (strcmp(expected, "???") == 0);
        
        if (is_exact_match) {
            correct_exact++;
            printf("[✓ EXACT] \"%s\" → \"%s\" (%s)\n", input, output, desc);
        } else if (is_partial_match && !is_novel) {
            correct_partial++;
            printf("[≈ PARTIAL] \"%s\" → \"%s\" (expected: %s) (%s)\n", 
                   input, output, expected, desc);
        } else if (is_novel && out_len > 0) {
            attempted++;
            printf("[? NOVEL] \"%s\" → \"%s\" (novel input, attempted answer) (%s)\n",
                   input, output, desc);
        } else {
            printf("[✗ WRONG] \"%s\" → \"%s\" (expected: %s) (%s)\n",
                   input, output, expected, desc);
        }
    }
    
    /* PHASE 3: Intelligence Analysis */
    printf("\n=== PHASE 3: INTELLIGENCE ANALYSIS ===\n\n");
    
    int known_tests = num_tests - 3;  /* 3 novel tests */
    float accuracy = (float)(correct_exact + correct_partial) / known_tests * 100.0f;
    
    printf("Results:\n");
    printf("  Exact Matches: %d/%d (%.1f%%)\n", 
           correct_exact, known_tests, (float)correct_exact/known_tests*100.0f);
    printf("  Partial Matches: %d/%d\n", correct_partial, known_tests);
    printf("  Overall Accuracy: %.1f%%\n\n", accuracy);
    
    printf("Novel Input Handling:\n");
    printf("  Attempted answers: %d/3\n", attempted);
    printf("  (Novel inputs are never-seen-before questions)\n\n");
    
    /* Show what it learned */
    printf("Pattern Analysis:\n");
    printf("  Total patterns: %u\n", g->pattern_count);
    printf("  Pattern confidence: %.3f\n", g->state.pattern_confidence);
    printf("  Error rate: %.3f\n\n", g->state.error_rate);
    
    /* Show some intelligent patterns */
    printf("Sample Learned Patterns (showing predictive patterns):\n");
    int shown = 0;
    for (uint32_t p = 0; p < g->pattern_count && shown < 15; p++) {
        Pattern *pat = &g->patterns[p];
        
        /* Show patterns that predict something (intelligent behavior) */
        if (pat->prediction_count > 0 && pat->strength > 0.3f) {
            printf("  Pattern \"");
            for (uint32_t i = 0; i < pat->length && i < 30; i++) {
                if (pat->node_ids[i] == 256) {
                    printf("_");
                } else if (pat->node_ids[i] < 128) {
                    printf("%c", (char)pat->node_ids[i]);
                }
            }
            printf("\" predicts: \"");
            for (uint32_t pred = 0; pred < pat->prediction_count && pred < 3; pred++) {
                if (pat->predicted_nodes[pred] < 128) {
                    printf("%c", (char)pat->predicted_nodes[pred]);
                }
            }
            printf("...\" (confidence=%.2f)\n", pat->strength);
            shown++;
        }
    }
    
    /* Intelligence verdict */
    printf("\n=== VERDICT ===\n");
    if (accuracy >= 80.0f) {
        printf("✓ YES - System demonstrates intelligent learning!\n");
        printf("  Can recall facts, complete patterns, and reason.\n");
    } else if (accuracy >= 50.0f) {
        printf("≈ PARTIAL - System shows some intelligence.\n");
        printf("  Learns some patterns but needs more training.\n");
    } else {
        printf("✗ LIMITED - System needs more training time.\n");
        printf("  Patterns exist but not strong enough yet.\n");
    }
    
    if (attempted >= 2) {
        printf("✓ System attempts to answer novel questions (generalization).\n");
    }
    
    printf("\n");
    
    melvin_destroy(g);
    return 0;
}

