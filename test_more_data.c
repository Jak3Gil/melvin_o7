/* Test: What happens when we add MORE data?
 * 
 * Expected intelligent behavior:
 * 1. Patterns should strengthen with repetition
 * 2. Weak/useless patterns should be pruned
 * 3. Generalization should improve (blank node patterns stronger)
 * 4. Output quality should improve (cleaner sequences)
 * 5. Competing patterns should stabilize (best ones survive)
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

typedef struct MelvinGraph MelvinGraph;
extern MelvinGraph* melvin_create(void);
extern void run_episode(MelvinGraph *g, const uint8_t *input, uint32_t input_len,
                       const uint8_t *target, uint32_t target_len);
extern void melvin_get_output(MelvinGraph *g, uint32_t **output, uint32_t *length);
extern uint32_t melvin_get_pattern_count(MelvinGraph *g);
extern float melvin_get_error_rate(MelvinGraph *g);

void test_output(MelvinGraph *g, const char *input, int episode_num) {
    run_episode(g, (const uint8_t*)input, strlen(input), NULL, 0);
    
    uint32_t *output;
    uint32_t output_len;
    melvin_get_output(g, &output, &output_len);
    
    printf("  Episode %3d: ", episode_num);
    printf("'%s' → ", input);
    for (uint32_t i = 0; i < output_len && i < 15; i++) {
        printf("%c", (uint8_t)output[i]);
    }
    
    /* Check if it ends with 's' (intelligent) */
    if (output_len > 0 && output[output_len - 1] == 's') {
        printf(" ✓");
    }
    printf("\n");
}

int main(void) {
    MelvinGraph *g = melvin_create();
    
    printf("=================================================================\n");
    printf("EXPERIMENT: What does MORE DATA do to the graph?\n");
    printf("=================================================================\n\n");
    
    printf("HYPOTHESIS: More data should:\n");
    printf("  1. Strengthen useful patterns (higher utility)\n");
    printf("  2. Prune useless patterns (metabolic cost)\n");
    printf("  3. Improve generalization (blank node patterns)\n");
    printf("  4. Produce cleaner outputs (better predictions)\n");
    printf("  5. Reduce error rate (learning converges)\n\n");
    
    const char *training_words[] = {"cat", "dog", "pen", "cup", "box"};
    int num_words = 5;
    
    printf("TRAINING PHASE: Adding more and more data...\n");
    printf("------------------------------------------------\n");
    
    /* Train with increasing amounts of data */
    for (int episode = 1; episode <= 200; episode++) {
        /* Vary training examples */
        const char *word = training_words[episode % num_words];
        char target[10];
        sprintf(target, "%ss", word);
        
        run_episode(g, (const uint8_t*)word, strlen(word), 
                   (const uint8_t*)target, strlen(target));
        
        /* Test every 20 episodes to see progress */
        if (episode % 20 == 0) {
            uint32_t pattern_count = melvin_get_pattern_count(g);
            float error_rate = melvin_get_error_rate(g);
            
            printf("\n--- After %d episodes ---\n", episode);
            printf("  Patterns: %u, Error: %.3f\n", pattern_count, error_rate);
            
            /* Test on trained word */
            test_output(g, "cat", episode);
            
            /* Test on NOVEL word (generalization) */
            test_output(g, "bat", episode);
        }
    }
    
    printf("\n=================================================================\n");
    printf("ANALYSIS: What did more data do?\n");
    printf("=================================================================\n\n");
    
    printf("With MORE data, we expect:\n");
    printf("  - Error rate to DECREASE (convergence)\n");
    printf("  - Pattern count to STABILIZE (weak ones pruned)\n");
    printf("  - Outputs to get CLEANER (better predictions)\n");
    printf("  - Novel inputs to work BETTER (generalization)\n\n");
    
    printf("If outputs DON'T improve with more data, the problem is:\n");
    printf("  - Patterns not strengthening with utility\n");
    printf("  - Pruning too aggressive (killing useful patterns)\n");
    printf("  - Context logic not utilizing learned patterns\n");
    printf("  - Wave dynamics interfering with pattern predictions\n");
    
    return 0;
}

