/* ============================================================================
 * MELVIN O7 TEST RUNNER
 * 
 * Reads test_input.txt, runs tests, shows:
 * - Simple task performance
 * - Complex task attempts
 * - Training samples needed
 * ============================================================================ */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

/* Include melvin.c but skip the main() function */
#define MELVIN_STANDALONE 0
#include "melvin.c"
#undef MELVIN_STANDALONE

int main(void) {
    FILE *test_file = fopen("test_input.txt", "r");
    if (!test_file) {
        fprintf(stderr, "Error: Could not open test_input.txt\n");
        return 1;
    }
    
    MelvinGraph *g = melvin_create();
    if (!g) {
        fprintf(stderr, "Error: Failed to create graph\n");
        fclose(test_file);
        return 1;
    }
    
    /* Set text port */
    melvin_set_input_port(g, 0);  /* Port 0 = text */
    melvin_set_output_port(g, 0);
    
    printf("MELVIN O7: Pattern Hierarchies & Wave Propagation Test\n");
    printf("========================================================\n\n");
    
    char line[256];
    int test_num = 0;
    int simple_tests = 0;
    int complex_tests = 0;
    int simple_correct = 0;
    int complex_correct = 0;
    
    printf("FORMAT: Test# | Input -> Output | Expected | Correct | Error | Samples\n");
    printf("----------------------------------------------------------------------\n");
    
    while (fgets(line, sizeof(line), test_file)) {
        /* Skip comments and empty lines */
        if (line[0] == '#' || line[0] == '\n') continue;
        
        /* Parse: input -> expected */
        char *arrow = strstr(line, "->");
        if (!arrow) continue;
        
        *arrow = '\0';
        char *input_text = line;
        char *expected_text = arrow + 2;
        
        /* Trim whitespace */
        while (*input_text == ' ' || *input_text == '\t') input_text++;
        while (*expected_text == ' ' || *expected_text == '\t') expected_text++;
        
        /* Remove newline */
        char *nl = strchr(expected_text, '\n');
        if (nl) *nl = '\0';
        nl = strchr(input_text, '\n');
        if (nl) *nl = '\0';
        
        if (strlen(input_text) == 0 || strlen(expected_text) == 0) continue;
        
        test_num++;
        
        /* Determine complexity */
        int is_complex = (strlen(input_text) > 5 || strlen(expected_text) > 5);
        if (is_complex) {
            complex_tests++;
        } else {
            simple_tests++;
        }
        
        /* Convert to uint8_t arrays */
        uint8_t *input = (uint8_t*)input_text;
        uint8_t *expected = (uint8_t*)expected_text;
        uint32_t input_len = strlen(input_text);
        uint32_t expected_len = strlen(expected_text);
        
        /* Run episode */
        run_episode(g, input, input_len, expected, expected_len);
        
        /* Get output */
        uint32_t *output;
        uint32_t output_len;
        melvin_get_output(g, &output, &output_len);
        
        /* Check if correct */
        int correct = 1;
        if (output_len != expected_len) {
            correct = 0;
        } else {
            for (uint32_t i = 0; i < output_len; i++) {
                if (output[i] != expected[i]) {
                    correct = 0;
                    break;
                }
            }
        }
        
        if (correct) {
            if (is_complex) complex_correct++;
            else simple_correct++;
        }
        
        /* Print result */
        printf("Test %2d | Input: %-15s -> Output: ", test_num, input_text);
        for (uint32_t i = 0; i < output_len && i < 20; i++) {
            printf("%c", (char)output[i]);
        }
        printf(" | Expected: %-15s | %s | Error: %.3f | Samples: %llu\n",
               expected_text,
               correct ? "✓" : "✗",
               melvin_get_error_rate(g),
               (unsigned long long)test_num);
        
        /* Show pattern hierarchy growth every 5 tests */
        if (test_num % 5 == 0) {
            printf("  [Patterns: %u, Edges: ", g->pattern_count);
            uint32_t total_edges = 0;
            for (int i = 0; i < BYTE_VALUES; i++) {
                total_edges += g->outgoing[i].count;
            }
            printf("%u, Wave steps: %llu]\n", 
                   total_edges, (unsigned long long)g->state.step);
        }
    }
    
    fclose(test_file);
    
    /* Summary */
    printf("\n=== SUMMARY ===\n");
    printf("Simple tests: %d/%d correct (%.1f%%)\n", 
           simple_correct, simple_tests,
           simple_tests > 0 ? (100.0f * simple_correct / simple_tests) : 0.0f);
    printf("Complex tests: %d/%d correct (%.1f%%)\n",
           complex_correct, complex_tests,
           complex_tests > 0 ? (100.0f * complex_correct / complex_tests) : 0.0f);
    printf("Total tests: %d\n", test_num);
    printf("Final error rate: %.3f\n", melvin_get_error_rate(g));
    printf("Patterns learned: %u\n", g->pattern_count);
    
    /* Show pattern hierarchies */
    printf("\n=== PATTERN HIERARCHIES ===\n");
    uint32_t hierarchies_shown = 0;
    for (uint32_t p = 0; p < g->pattern_count && hierarchies_shown < 10; p++) {
        Pattern *pat = &g->patterns[p];
        if (pat->length == 0 || pat->length > 20) continue;
        
        printf("Pattern %u [depth:%u, meaning:%.2f]: \"", 
               p, pat->chain_depth, pat->accumulated_meaning);
        for (uint32_t i = 0; i < pat->length && i < 20; i++) {
            if (pat->node_ids[i] < 128) {
                printf("%c", (char)pat->node_ids[i]);
            } else {
                printf("_");
            }
        }
        printf("\"");
        
        if (pat->parent_pattern_id != INVALID_PATTERN_ID && pat->parent_pattern_id < g->pattern_count) {
            printf(" (child of %u)", pat->parent_pattern_id);
        } else {
            printf(" (root)");
        }
        
        uint32_t child_count = 0;
        for (uint32_t q = 0; q < g->pattern_count; q++) {
            if (g->patterns[q].parent_pattern_id == p) {
                child_count++;
            }
        }
        if (child_count > 0) {
            printf(" -> %u children", child_count);
        }
        printf("\n");
        hierarchies_shown++;
    }
    
    printf("\n=== WAVE PROPAGATION ===\n");
    printf("Total propagation steps: %llu\n", (unsigned long long)g->state.step);
    printf("Average steps per test: %.1f\n", 
           test_num > 0 ? (float)g->state.step / test_num : 0.0f);
    printf("Wave propagation: Multi-step (not single pass like standard NN)\n");
    
    melvin_destroy(g);
    return 0;
}

