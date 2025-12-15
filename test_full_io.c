/* Full I/O Test: Multiple scenarios showing input → output */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

typedef struct MelvinGraph MelvinGraph;
extern MelvinGraph* melvin_create(void);
extern void run_episode(MelvinGraph *g, const uint8_t *input, uint32_t input_len,
                       const uint8_t *target, uint32_t target_len);
extern void melvin_get_output(MelvinGraph *g, uint32_t **output, uint32_t *length);

void print_output(MelvinGraph *g, const char *label) {
    uint32_t *output;
    uint32_t output_len;
    melvin_get_output(g, &output, &output_len);
    
    printf("%s: ", label);
    for (uint32_t i = 0; i < output_len && i < 50; i++) {
        if (output[i] < 256) {
            printf("%c", (uint8_t)output[i]);
        }
    }
    printf("\n");
}

int main(void) {
    MelvinGraph *g = melvin_create();
    
    printf("╔═══════════════════════════════════════════════════════════╗\n");
    printf("║           MELVIN O7: Input → Output Demonstration         ║\n");
    printf("╚═══════════════════════════════════════════════════════════╝\n\n");
    
    /* Scenario 1: Simple learning task */
    printf("┌─ SCENARIO 1: Pluralization Learning ─────────────────────┐\n");
    printf("│ Training: 'cat' → 'cats' (50 episodes)                   │\n");
    for (int i = 0; i < 50; i++) {
        run_episode(g, (const uint8_t*)"cat", 3, (const uint8_t*)"cats", 4);
    }
    printf("│ Training complete.                                        │\n\n");
    
    printf("│ Test Inputs:                                              │\n");
    run_episode(g, (const uint8_t*)"cat", 3, NULL, 0);
    print_output(g, "│   'cat' → ");
    run_episode(g, (const uint8_t*)"bat", 3, NULL, 0);
    print_output(g, "│   'bat' → ");
    run_episode(g, (const uint8_t*)"mat", 3, NULL, 0);
    print_output(g, "│   'mat' → ");
    printf("└───────────────────────────────────────────────────────────┘\n\n");
    
    /* Scenario 2: Fresh graph, different pattern */
    printf("┌─ SCENARIO 2: Fresh Start, Different Pattern ─────────────┐\n");
    MelvinGraph *g2 = melvin_create();
    printf("│ Training: 'dog' → 'dogs' (50 episodes)                   │\n");
    for (int i = 0; i < 50; i++) {
        run_episode(g2, (const uint8_t*)"dog", 3, (const uint8_t*)"dogs", 4);
    }
    printf("│ Training complete.                                        │\n\n");
    
    printf("│ Test Inputs:                                              │\n");
    run_episode(g2, (const uint8_t*)"dog", 3, NULL, 0);
    print_output(g2, "│   'dog' → ");
    run_episode(g2, (const uint8_t*)"log", 3, NULL, 0);
    print_output(g2, "│   'log' → ");
    printf("└───────────────────────────────────────────────────────────┘\n\n");
    
    /* Scenario 3: Raw input (no training) */
    printf("┌─ SCENARIO 3: No Training (Pure Wave Propagation) ────────┐\n");
    MelvinGraph *g3 = melvin_create();
    printf("│ No training, just input → output:                        │\n\n");
    
    printf("│ Test Inputs:                                              │\n");
    run_episode(g3, (const uint8_t*)"hello", 5, NULL, 0);
    print_output(g3, "│   'hello' → ");
    run_episode(g3, (const uint8_t*)"world", 5, NULL, 0);
    print_output(g3, "│   'world' → ");
    printf("└───────────────────────────────────────────────────────────┘\n\n");
    
    printf("╔═══════════════════════════════════════════════════════════╗\n");
    printf("║  SUMMARY:                                                 ║\n");
    printf("║  - System accepts byte inputs                             ║\n");
    printf("║  - Wave propagation generates outputs                     ║\n");
    printf("║  - Patterns learn from training                           ║\n");
    printf("║  - Rich error tracking improves learning                  ║\n");
    printf("╚═══════════════════════════════════════════════════════════╝\n");
    
    return 0;
}

