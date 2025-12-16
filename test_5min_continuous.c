/* ============================================================================
 * MELVIN O7: 5-Minute Continuous Data Feed Test
 * 
 * Runs for 5 minutes, continuously feeding new data
 * Monitors: pattern growth, hierarchy depth, edge count, memory usage
 * ============================================================================ */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <time.h>
#include <math.h>

/* Include melvin.c but skip the main() function */
/* We need to undefine MELVIN_STANDALONE if it's set, then include */
#ifdef MELVIN_STANDALONE
#undef MELVIN_STANDALONE
#endif
#include "melvin.c"

/* Generate diverse test data */
const char* word_list[] = {
    "cat", "bat", "rat", "hat", "mat", "sat", "pat", "fat",
    "dog", "log", "fog", "bog", "jog", "cog", "hog", "frog",
    "the", "and", "for", "are", "but", "not", "you", "all",
    "can", "her", "was", "one", "our", "out", "day", "get",
    "has", "him", "his", "how", "man", "new", "now", "old",
    "see", "two", "way", "who", "boy", "did", "its", "let",
    "put", "say", "she", "too", "use", "what", "when", "where",
    "which", "who", "will", "with", "would", "your", "about", "after",
    "again", "before", "being", "below", "between", "during", "except",
    "hello", "world", "test", "data", "pattern", "system", "graph",
    "node", "edge", "learn", "train", "input", "output", "result"
};

const char* phrase_list[] = {
    "the cat", "the bat", "the rat", "the hat",
    "what is", "what are", "what was", "what were",
    "how many", "how much", "how long", "how far",
    "where is", "where are", "where was", "where were",
    "when is", "when are", "when was", "when were",
    "the quick", "brown fox", "jumps over", "lazy dog",
    "hello world", "test data", "pattern system", "graph node"
};

const char* question_list[] = {
    "what is the capital",
    "what is the name",
    "what is the answer",
    "what is the color",
    "how many people",
    "how many times",
    "how many years",
    "where is the",
    "when is the",
    "who is the"
};

uint32_t word_count = sizeof(word_list) / sizeof(word_list[0]);
uint32_t phrase_count = sizeof(phrase_list) / sizeof(phrase_list[0]);
uint32_t question_count = sizeof(question_list) / sizeof(question_list[0]);

/* Get random data item */
const char* get_random_data(int type) {
    switch(type % 3) {
        case 0: return word_list[rand() % word_count];
        case 1: return phrase_list[rand() % phrase_count];
        case 2: return question_list[rand() % question_count];
        default: return word_list[0];
    }
}

/* Calculate total edge count */
uint64_t count_total_edges(MelvinGraph *g) {
    uint64_t total = 0;
    for (int i = 0; i < BYTE_VALUES; i++) {
        if (g->nodes[i].exists) {
            total += g->outgoing[i].count;
        }
    }
    return total;
}

/* Calculate active edge count */
uint64_t count_active_edges(MelvinGraph *g) {
    uint64_t total = 0;
    for (int i = 0; i < BYTE_VALUES; i++) {
        if (g->nodes[i].exists) {
            EdgeList *out = &g->outgoing[i];
            for (uint32_t j = 0; j < out->count; j++) {
                if (out->edges[j].active) {
                    total++;
                }
            }
        }
    }
    return total;
}

/* Calculate max hierarchy depth */
uint32_t get_max_hierarchy_depth(MelvinGraph *g) {
    uint32_t max_depth = 0;
    for (uint32_t p = 0; p < g->pattern_count; p++) {
        Pattern *pat = &g->patterns[p];
        if (pat->chain_depth > max_depth) {
            max_depth = pat->chain_depth;
        }
    }
    return max_depth;
}

/* Calculate average hierarchy depth */
float get_avg_hierarchy_depth(MelvinGraph *g) {
    if (g->pattern_count == 0) return 0.0f;
    uint32_t total_depth = 0;
    for (uint32_t p = 0; p < g->pattern_count; p++) {
        total_depth += g->patterns[p].chain_depth;
    }
    return (float)total_depth / (float)g->pattern_count;
}

/* Count patterns with blank nodes */
uint32_t count_generalized_patterns(MelvinGraph *g) {
    uint32_t count = 0;
    for (uint32_t p = 0; p < g->pattern_count; p++) {
        Pattern *pat = &g->patterns[p];
        for (uint32_t i = 0; i < pat->length; i++) {
            if (pat->node_ids[i] == BLANK_NODE) {
                count++;
                break;
            }
        }
    }
    return count;
}

/* Count pattern-to-pattern edges */
uint64_t count_pattern_edges(MelvinGraph *g) {
    uint64_t total = 0;
    for (uint32_t p = 0; p < g->pattern_count; p++) {
        Pattern *pat = &g->patterns[p];
        total += pat->outgoing_patterns.count;
    }
    return total;
}

/* Print statistics */
void print_stats(MelvinGraph *g, int episode, time_t start_time, time_t current_time) {
    double elapsed = difftime(current_time, start_time);
    uint64_t total_edges = count_total_edges(g);
    uint64_t active_edges = count_active_edges(g);
    uint32_t max_depth = get_max_hierarchy_depth(g);
    float avg_depth = get_avg_hierarchy_depth(g);
    uint32_t generalized = count_generalized_patterns(g);
    uint64_t pattern_edges = count_pattern_edges(g);
    
    printf("\n");
    printf("═══════════════════════════════════════════════════════════════\n");
    printf("STATISTICS AT %.1f SECONDS (Episode %d)\n", elapsed, episode);
    printf("═══════════════════════════════════════════════════════════════\n");
    printf("Patterns:           %u\n", g->pattern_count);
    printf("  - Generalized:    %u (%.1f%%)\n", generalized, 
           g->pattern_count > 0 ? (100.0f * generalized / g->pattern_count) : 0.0f);
    printf("  - Max Depth:      %u\n", max_depth);
    printf("  - Avg Depth:      %.2f\n", avg_depth);
    printf("  - Pattern Edges:   %llu\n", (unsigned long long)pattern_edges);
    printf("Edges:              %llu total, %llu active\n", 
           (unsigned long long)total_edges, (unsigned long long)active_edges);
    printf("System State:\n");
    printf("  - Error Rate:     %.3f\n", g->state.error_rate);
    printf("  - Learning Rate:  %.3f\n", g->state.learning_rate);
    printf("  - Metabolic:      %.3f\n", g->state.metabolic_pressure);
    printf("  - Pattern Conf:   %.3f\n", g->state.pattern_confidence);
    printf("  - Loop Pressure:  %.3f\n", g->state.loop_pressure);
    printf("═══════════════════════════════════════════════════════════════\n");
}

/* Print sample patterns */
void print_sample_patterns(MelvinGraph *g, int count) {
    printf("\nSAMPLE PATTERNS (showing first %d):\n", count);
    printf("───────────────────────────────────────────────────────────────\n");
    
    int shown = 0;
    for (uint32_t p = 0; p < g->pattern_count && shown < count; p++) {
        Pattern *pat = &g->patterns[p];
        if (pat->strength < 0.1f) continue;  // Skip very weak patterns
        
        printf("Pattern %u: ", p);
        for (uint32_t i = 0; i < pat->length; i++) {
            if (pat->node_ids[i] == BLANK_NODE) {
                printf("_");
            } else if (pat->node_ids[i] < 256) {
                printf("%c", (char)pat->node_ids[i]);
            } else {
                printf("[%u]", pat->node_ids[i]);
            }
        }
        printf(" | Strength: %.3f | Depth: %u | Predictions: %u\n",
               pat->strength, pat->chain_depth, pat->prediction_count);
        shown++;
    }
    if (shown == 0) {
        printf("(No patterns with strength > 0.1)\n");
    }
}

int main(void) {
    printf("╔═══════════════════════════════════════════════════════════════╗\n");
    printf("║     MELVIN O7: 5-MINUTE CONTINUOUS DATA FEED TEST            ║\n");
    printf("╚═══════════════════════════════════════════════════════════════╝\n\n");
    
    MelvinGraph *g = melvin_create();
    if (!g) {
        fprintf(stderr, "Error: Failed to create graph\n");
        return 1;
    }
    
    /* Set text port */
    melvin_set_input_port(g, 0);
    melvin_set_output_port(g, 0);
    
    printf("Starting 5-minute continuous data feed...\n");
    printf("Feeding random words, phrases, and questions\n");
    printf("Monitoring: patterns, edges, hierarchy, growth\n\n");
    
    time_t start_time = time(NULL);
    time_t end_time = start_time + 300;  // 5 minutes = 300 seconds
    time_t last_stats_time = start_time;
    time_t current_time;
    
    int episode = 0;
    int stats_interval = 30;  // Print stats every 30 seconds
    
    /* Track growth over time */
    uint32_t pattern_count_history[20] = {0};
    uint64_t edge_count_history[20] = {0};
    int history_index = 0;
    
    printf("Press Ctrl+C to stop early\n\n");
    
    while ((current_time = time(NULL)) < end_time) {
        /* Generate random data */
        const char* data = get_random_data(episode);
        uint32_t data_len = strlen(data);
        
        /* Run episode (use data as both input and target for learning) */
        run_episode(g, (const uint8_t*)data, data_len, (const uint8_t*)data, data_len);
        
        episode++;
        
        /* Print stats periodically */
        if (difftime(current_time, last_stats_time) >= stats_interval) {
            print_stats(g, episode, start_time, current_time);
            
            /* Store history */
            if (history_index < 20) {
                pattern_count_history[history_index] = g->pattern_count;
                edge_count_history[history_index] = count_total_edges(g);
                history_index++;
            }
            
            /* Show sample patterns every 60 seconds */
            if (episode % 2 == 0) {
                print_sample_patterns(g, 10);
            }
            
            last_stats_time = current_time;
        }
        
        /* Show progress every 10 episodes */
        if (episode % 10 == 0) {
            double elapsed = difftime(current_time, start_time);
            double remaining = 300.0 - elapsed;
            printf("Progress: %d episodes | %.1f seconds elapsed | %.1f seconds remaining | Patterns: %u\n",
                   episode, elapsed, remaining, g->pattern_count);
        }
    }
    
    /* Final statistics */
    current_time = time(NULL);
    double total_elapsed = difftime(current_time, start_time);
    
    printf("\n\n");
    printf("╔═══════════════════════════════════════════════════════════════╗\n");
    printf("║                    FINAL RESULTS                              ║\n");
    printf("╚═══════════════════════════════════════════════════════════════╝\n\n");
    
    print_stats(g, episode, start_time, current_time);
    
    printf("\nGROWTH OVER TIME:\n");
    printf("───────────────────────────────────────────────────────────────\n");
    for (int i = 0; i < history_index; i++) {
        printf("Time %d: Patterns: %u | Edges: %llu\n",
               i * stats_interval, 
               pattern_count_history[i],
               (unsigned long long)edge_count_history[i]);
    }
    
    print_sample_patterns(g, 20);
    
    printf("\n");
    printf("═══════════════════════════════════════════════════════════════\n");
    printf("TEST COMPLETE\n");
    printf("═══════════════════════════════════════════════════════════════\n");
    printf("Total Episodes:     %d\n", episode);
    printf("Total Time:         %.1f seconds\n", total_elapsed);
    printf("Episodes/Second:    %.2f\n", episode / total_elapsed);
    printf("Final Patterns:     %u\n", g->pattern_count);
    printf("Final Edges:        %llu\n", (unsigned long long)count_total_edges(g));
    printf("Max Hierarchy:      %u\n", get_max_hierarchy_depth(g));
    printf("═══════════════════════════════════════════════════════════════\n");
    
    melvin_destroy(g);
    return 0;
}

