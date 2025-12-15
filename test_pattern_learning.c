/* ============================================================================
 * TEST PATTERN-TO-PATTERN LEARNING
 * 
 * Test if system automatically learns pattern chains from complex inputs
 * Multi-word inputs, pattern composition, concept formation
 * ============================================================================ */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "melvin.c"

void print_pattern_info(MelvinGraph *g, uint32_t pattern_id) {
    if (pattern_id >= g->pattern_count) return;
    
    Pattern *pat = &g->patterns[pattern_id];
    printf("  Pattern %u: ", pattern_id);
    
    /* Print pattern sequence */
    for (uint32_t i = 0; i < pat->length; i++) {
        if (pat->node_ids[i] == BLANK_NODE) {
            printf("_");
        } else {
            printf("%c", (char)pat->node_ids[i]);
        }
    }
    
    printf(" (strength=%.3f, predictions=%u, pattern_predictions=%u)\n", 
           pat->strength, pat->prediction_count, pat->pattern_prediction_count);
    
    /* Print pattern predictions */
    if (pat->pattern_prediction_count > 0) {
        printf("    → Predicts patterns: ");
        for (uint32_t i = 0; i < pat->pattern_prediction_count; i++) {
            uint32_t pred_pat = pat->predicted_patterns[i];
            float weight = pat->pattern_prediction_weights[i];
            printf("%u(%.2f) ", pred_pat, weight);
        }
        printf("\n");
    }
    
    /* Print node predictions */
    if (pat->prediction_count > 0) {
        printf("    → Predicts nodes: ");
        for (uint32_t i = 0; i < pat->prediction_count && i < 5; i++) {
            printf("%c(%.2f) ", (char)pat->predicted_nodes[i], pat->prediction_weights[i]);
        }
        if (pat->prediction_count > 5) printf("...");
        printf("\n");
    }
}

int main() {
    printf("========================================\n");
    printf("TEST: Pattern-to-Pattern Learning\n");
    printf("Complex Multi-Word Inputs\n");
    printf("========================================\n\n");
    
    MelvinGraph *g = melvin_create();
    if (!g) {
        printf("ERROR: Failed to create graph\n");
        return 1;
    }
    
    /* Test 1: Complex question-answer pairs */
    printf("TEST 1: Learning from Q&A pairs\n");
    printf("--------------------------------\n");
    
    const char *questions[] = {
        "What is the capital of France?",
        "What is the capital of Germany?",
        "What is the capital of Italy?",
        "What is the capital of Spain?",
    };
    
    const char *answers[] = {
        "Paris",
        "Berlin",
        "Rome",
        "Madrid",
    };
    
    printf("Training on Q&A pairs...\n");
    for (int i = 0; i < 4; i++) {
        printf("\n  Input:  \"%s\"\n", questions[i]);
        printf("  Target: \"%s\"\n", answers[i]);
        
        run_episode(g, (const uint8_t*)questions[i], strlen(questions[i]),
                   (const uint8_t*)answers[i], strlen(answers[i]));
        
        printf("  Output: \"");
        for (uint32_t j = 0; j < g->output_length; j++) {
            printf("%c", (char)g->output_buffer[j]);
        }
        printf("\"\n");
    }
    
    printf("\nPatterns learned:\n");
    for (uint32_t p = 0; p < g->pattern_count && p < 20; p++) {
        print_pattern_info(g, p);
    }
    
    /* Test 2: Test generalization */
    printf("\n\nTEST 2: Generalization Test\n");
    printf("--------------------------------\n");
    printf("New question: \"What is the capital of Japan?\"\n");
    
    const char *new_question = "What is the capital of Japan?";
    const char *expected = "Tokyo";
    
    g->output_length = 0;  /* Reset output */
    run_episode(g, (const uint8_t*)new_question, strlen(new_question),
               (const uint8_t*)expected, strlen(expected));
    
    printf("Output: \"");
    for (uint32_t i = 0; i < g->output_length; i++) {
        printf("%c", (char)g->output_buffer[i]);
    }
    printf("\"\n");
    printf("Expected: \"%s\"\n", expected);
    
    /* Check if pattern chains were learned */
    printf("\nPattern chains learned:\n");
    bool found_chain = false;
    for (uint32_t p = 0; p < g->pattern_count; p++) {
        Pattern *pat = &g->patterns[p];
        if (pat->pattern_prediction_count > 0) {
            printf("  Pattern %u predicts %u other patterns\n", p, pat->pattern_prediction_count);
            found_chain = true;
        }
    }
    
    if (!found_chain) {
        printf("  WARNING: No pattern-to-pattern chains found!\n");
    }
    
    /* Test 3: Complex multi-sentence input */
    printf("\n\nTEST 3: Complex Multi-Sentence Input\n");
    printf("--------------------------------\n");
    
    const char *complex_input = "The cat sat on the mat. The dog ran in the park.";
    const char *complex_target = "Animals are active.";
    
    printf("Input:  \"%s\"\n", complex_input);
    printf("Target: \"%s\"\n", complex_target);
    
    g->output_length = 0;
    run_episode(g, (const uint8_t*)complex_input, strlen(complex_input),
               (const uint8_t*)complex_target, strlen(complex_target));
    
    printf("Output: \"");
    for (uint32_t i = 0; i < g->output_length; i++) {
        printf("%c", (char)g->output_buffer[i]);
    }
    printf("\"\n");
    
    /* Test 4: Check pattern composition */
    printf("\n\nTEST 4: Pattern Composition Analysis\n");
    printf("--------------------------------\n");
    
    printf("Total patterns: %u\n", g->pattern_count);
    
    uint32_t patterns_with_predictions = 0;
    uint32_t patterns_with_pattern_predictions = 0;
    uint32_t total_pattern_predictions = 0;
    
    for (uint32_t p = 0; p < g->pattern_count; p++) {
        Pattern *pat = &g->patterns[p];
        if (pat->prediction_count > 0) {
            patterns_with_predictions++;
        }
        if (pat->pattern_prediction_count > 0) {
            patterns_with_pattern_predictions++;
            total_pattern_predictions += pat->pattern_prediction_count;
        }
    }
    
    printf("Patterns with node predictions: %u\n", patterns_with_predictions);
    printf("Patterns with pattern predictions: %u\n", patterns_with_pattern_predictions);
    printf("Total pattern-to-pattern links: %u\n", total_pattern_predictions);
    
    if (patterns_with_pattern_predictions == 0) {
        printf("\n❌ FAIL: No pattern-to-pattern learning detected!\n");
        printf("   System is not learning pattern chains.\n");
    } else {
        printf("\n✓ SUCCESS: Pattern-to-pattern learning is working!\n");
        printf("   Average %.1f pattern predictions per pattern\n", 
               (float)total_pattern_predictions / patterns_with_pattern_predictions);
    }
    
    /* Test 5: Show specific pattern chains */
    printf("\n\nTEST 5: Pattern Chain Examples\n");
    printf("--------------------------------\n");
    
    int chain_count = 0;
    for (uint32_t p = 0; p < g->pattern_count && chain_count < 10; p++) {
        Pattern *pat = &g->patterns[p];
        if (pat->pattern_prediction_count > 0) {
            printf("Chain %d:\n", chain_count + 1);
            print_pattern_info(g, p);
            chain_count++;
        }
    }
    
    if (chain_count == 0) {
        printf("No pattern chains found.\n");
    }
    
    melvin_destroy(g);
    return 0;
}
