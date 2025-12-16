/* Simple growth test - just use the existing main in melvin.c with more data */
#include "melvin.c"

int main(void) {
    printf("=== 5-Minute Continuous Learning Test ===\n");
    printf("Starting...\n\n");
    
    MelvinGraph *g = melvin_create();
    if (!g) {
        printf("Failed to create graph\n");
        return 1;
    }
    
    const char* test_sequences[] = {
        "cat", "dog", "bat", "rat", "hat",
        "the cat", "the dog", "the bat",
        "what is", "where is", "how do",
        "what is the", "where is the",
        "the red cat", "the blue dog",
        "if the cat", "when the dog",
    };
    int num_sequences = sizeof(test_sequences) / sizeof(test_sequences[0]);
    
    time_t start_time = time(NULL);
    time_t last_report = start_time;
    int episode_count = 0;
    
    /* Run for 5 minutes */
    while (difftime(time(NULL), start_time) < 300.0) {
        /* Pick random sequence */
        const char *seq = test_sequences[rand() % num_sequences];
        
        /* Run episode */
        run_episode(g, (const uint8_t*)seq, strlen(seq),
                   (const uint8_t*)seq, strlen(seq));
        
        episode_count++;
        
        /* Report every 30 seconds */
        time_t now = time(NULL);
        if (difftime(now, last_report) >= 30.0) {
            printf("\n[%.0f seconds] Episodes: %d, Patterns: %u\n",
                   difftime(now, start_time), episode_count, g->pattern_count);
            printf("  Error Rate: %.3f, Learning Rate: %.3f\n",
                   g->state.error_rate, g->state.learning_rate);
            printf("  Pattern Confidence: %.3f, Metabolic Pressure: %.3f\n",
                   g->state.pattern_confidence, g->state.metabolic_pressure);
            
            /* Count edges */
            uint32_t total_edges = 0;
            for (int i = 0; i < 256; i++) {
                total_edges += g->outgoing[i].count;
            }
            printf("  Total Edges: %u\n", total_edges);
            
            last_report = now;
        }
    }
    
    printf("\n\n=== FINAL RESULTS ===\n");
    printf("Runtime: 5 minutes\n");
    printf("Episodes: %d (%.1f/sec)\n", episode_count, episode_count / 300.0);
    printf("Patterns: %u\n", g->pattern_count);
    
    /* Count edges */
    uint32_t total_edges = 0;
    for (int i = 0; i < 256; i++) {
        total_edges += g->outgoing[i].count;
    }
    printf("Edges: %u\n", total_edges);
    
    /* Show some patterns */
    printf("\n=== SAMPLE PATTERNS (first 10 strong ones) ===\n");
    int shown = 0;
    for (uint32_t p = 0; p < g->pattern_count && shown < 10; p++) {
        Pattern *pat = &g->patterns[p];
        if (pat->strength > 0.3f) {
            printf("Pattern %u (strength=%.2f, depth=%u): \"",
                   p, pat->strength, pat->chain_depth);
            for (uint32_t i = 0; i < pat->length && i < 20; i++) {
                if (pat->node_ids[i] == 256) {
                    printf("_");
                } else if (pat->node_ids[i] < 128) {
                    printf("%c", (char)pat->node_ids[i]);
                }
            }
            printf("\"\n");
            shown++;
        }
    }
    
    printf("\nTest complete!\n");
    
    melvin_destroy(g);
    return 0;
}

