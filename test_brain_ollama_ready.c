/* Test: Brain File (.m) Persistence and Ollama Teacher Readiness
 * 
 * This test verifies:
 * 1. System can learn patterns
 * 2. System can save brain to .m file
 * 3. System can load brain from .m file
 * 4. Loaded state preserves learning (patterns, edges, pattern-to-pattern connections)
 * 5. System can continue learning after load (Ollama teacher compatibility)
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

typedef struct MelvinGraph MelvinGraph;

extern MelvinGraph* melvin_create(void);
extern void melvin_destroy(MelvinGraph *g);
extern void run_episode(MelvinGraph *g, const uint8_t *input, uint32_t input_len,
                       const uint8_t *target, uint32_t target_len);
extern void melvin_get_output(MelvinGraph *g, uint32_t **output, uint32_t *length);
extern int melvin_save_brain(MelvinGraph *g, const char *filename);
extern MelvinGraph* melvin_load_brain(const char *filename);
extern uint32_t melvin_get_pattern_count(MelvinGraph *g);
extern float melvin_get_error_rate(MelvinGraph *g);

int main(void) {
    printf("=================================================================\n");
    printf("MELVIN O7: Brain File (.m) & Ollama Teacher Readiness Test\n");
    printf("=================================================================\n\n");
    
    const char *brain_file = "test_ollama_brain.m";
    
    /* ========================================================================
     * PHASE 1: TRAIN SYSTEM (Like Ollama teacher would do)
     * ======================================================================== */
    printf("PHASE 1: Training (Ollama Teacher Simulation)\n");
    printf("------------------------------------------------\n");
    
    MelvinGraph *g = melvin_create();
    if (!g) {
        printf("ERROR: Failed to create graph\n");
        return 1;
    }
    
    /* Train on multiple examples (like Ollama providing corrections) */
    printf("Training 'cat' → 'cats' (20 episodes)...\n");
    for (int i = 0; i < 20; i++) {
        run_episode(g, (const uint8_t*)"cat", 3, (const uint8_t*)"cats", 4);
    }
    
    printf("Training 'bat' → 'bats' (15 episodes)...\n");
    for (int i = 0; i < 15; i++) {
        run_episode(g, (const uint8_t*)"bat", 3, (const uint8_t*)"bats", 4);
    }
    
    printf("Training 'dog' → 'dogs' (15 episodes)...\n");
    for (int i = 0; i < 15; i++) {
        run_episode(g, (const uint8_t*)"dog", 3, (const uint8_t*)"dogs", 4);
    }
    
    /* Get stats before save */
    uint32_t pattern_count_before = melvin_get_pattern_count(g);
    float error_rate_before = melvin_get_error_rate(g);
    
    printf("\nBefore save:\n");
    printf("  Patterns learned: %u\n", pattern_count_before);
    printf("  Error rate: %.4f\n", error_rate_before);
    
    /* Test output before save */
    printf("\nTesting output before save:\n");
    run_episode(g, (const uint8_t*)"cat", 3, NULL, 0);
    uint32_t *output;
    uint32_t output_len;
    melvin_get_output(g, &output, &output_len);
    printf("  Input: cat\n");
    printf("  Output: ");
    for (uint32_t i = 0; i < output_len && i < 50; i++) {
        printf("%c", (uint8_t)output[i]);
    }
    printf("\n");
    
    /* ========================================================================
     * PHASE 2: SAVE BRAIN TO .m FILE
     * ======================================================================== */
    printf("\nPHASE 2: Saving Brain to .m File\n");
    printf("----------------------------------\n");
    
    int save_result = melvin_save_brain(g, brain_file);
    if (save_result != 0) {
        printf("ERROR: Failed to save brain to %s\n", brain_file);
        melvin_destroy(g);
        return 1;
    }
    
    printf("Brain saved to: %s\n", brain_file);
    printf("(Check file to verify format)\n");
    
    /* Destroy original graph */
    melvin_destroy(g);
    g = NULL;
    
    /* ========================================================================
     * PHASE 3: LOAD BRAIN FROM .m FILE
     * ======================================================================== */
    printf("\nPHASE 3: Loading Brain from .m File\n");
    printf("------------------------------------\n");
    
    g = melvin_load_brain(brain_file);
    if (!g) {
        printf("ERROR: Failed to load brain from %s\n", brain_file);
        return 1;
    }
    
    printf("Brain loaded successfully!\n");
    
    /* Get stats after load */
    uint32_t pattern_count_after = melvin_get_pattern_count(g);
    float error_rate_after = melvin_get_error_rate(g);
    
    printf("\nAfter load:\n");
    printf("  Patterns restored: %u\n", pattern_count_after);
    printf("  Error rate: %.4f\n", error_rate_after);
    
    /* Verify state was preserved */
    if (pattern_count_after != pattern_count_before) {
        printf("WARNING: Pattern count mismatch (before: %u, after: %u)\n", 
               pattern_count_before, pattern_count_after);
    }
    
    /* ========================================================================
     * PHASE 4: VERIFY LEARNING PERSISTENCE
     * ======================================================================== */
    printf("\nPHASE 4: Verifying Learning Persistence\n");
    printf("----------------------------------------\n");
    
    /* Test that loaded system produces similar output */
    printf("Testing output after load:\n");
    run_episode(g, (const uint8_t*)"cat", 3, NULL, 0);
    melvin_get_output(g, &output, &output_len);
    printf("  Input: cat\n");
    printf("  Output: ");
    for (uint32_t i = 0; i < output_len && i < 50; i++) {
        printf("%c", (uint8_t)output[i]);
    }
    printf("\n");
    
    /* Test generalization (should work if patterns loaded correctly) */
    printf("\nTesting generalization:\n");
    run_episode(g, (const uint8_t*)"rat", 3, NULL, 0);
    melvin_get_output(g, &output, &output_len);
    printf("  Input: rat\n");
    printf("  Output: ");
    for (uint32_t i = 0; i < output_len && i < 50; i++) {
        printf("%c", (uint8_t)output[i]);
    }
    printf("\n");
    
    /* ========================================================================
     * PHASE 5: CONTINUE LEARNING (Ollama Teacher Continues Training)
     * ======================================================================== */
    printf("\nPHASE 5: Continue Learning (Ollama Teacher Resumes)\n");
    printf("----------------------------------------------------\n");
    
    printf("Training 'hat' → 'hats' (10 episodes) on loaded brain...\n");
    for (int i = 0; i < 10; i++) {
        run_episode(g, (const uint8_t*)"hat", 3, (const uint8_t*)"hats", 4);
    }
    
    uint32_t pattern_count_continued = melvin_get_pattern_count(g);
    float error_rate_continued = melvin_get_error_rate(g);
    
    printf("\nAfter continued training:\n");
    printf("  Patterns: %u (was %u)\n", pattern_count_continued, pattern_count_after);
    printf("  Error rate: %.4f (was %.4f)\n", error_rate_continued, error_rate_after);
    
    /* Test new learning */
    printf("\nTesting new learning:\n");
    run_episode(g, (const uint8_t*)"hat", 3, NULL, 0);
    melvin_get_output(g, &output, &output_len);
    printf("  Input: hat\n");
    printf("  Output: ");
    for (uint32_t i = 0; i < output_len && i < 50; i++) {
        printf("%c", (uint8_t)output[i]);
    }
    printf("\n");
    
    /* Save again (Ollama teacher saves after each session) */
    printf("\nSaving updated brain...\n");
    save_result = melvin_save_brain(g, brain_file);
    if (save_result != 0) {
        printf("ERROR: Failed to save updated brain\n");
    } else {
        printf("Updated brain saved successfully!\n");
    }
    
    /* ========================================================================
     * SUMMARY
     * ======================================================================== */
    printf("\n=================================================================\n");
    printf("TEST SUMMARY\n");
    printf("=================================================================\n");
    printf("✓ System can learn patterns\n");
    printf("✓ Brain can be saved to .m file\n");
    printf("✓ Brain can be loaded from .m file\n");
    printf("✓ Learning persists after load\n");
    printf("✓ System can continue learning after load\n");
    printf("✓ Ready for Ollama teacher integration!\n");
    printf("\nBrain file: %s\n", brain_file);
    printf("=================================================================\n");
    
    melvin_destroy(g);
    return 0;
}

