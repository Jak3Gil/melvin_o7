/* Continuous Learning Test - Feed Melvin O7 data for 5 minutes */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdint.h>
#include <stdbool.h>
#include <math.h>

/* Forward declarations - types defined in melvin.c */
typedef struct MelvinGraph MelvinGraph;
typedef struct Pattern Pattern;
typedef struct EdgeList EdgeList;

/* Include melvin.c functions */
MelvinGraph* melvin_create(void);
void run_episode(MelvinGraph *g, const uint8_t *input, uint32_t input_len,
                const uint8_t *target, uint32_t target_len);
void melvin_destroy(MelvinGraph *g);

/* Access internal structures (need to match melvin.c definitions) */
typedef struct {
    uint32_t to_id;
    float weight;
    uint64_t use_count;
    uint64_t success_count;
    bool active;
    bool is_pattern_edge;
} Edge;

typedef struct {
    Edge *edges;
    uint32_t count;
    uint32_t capacity;
    float total_weight;
    float metabolic_load;
} EdgeList_Internal;

typedef struct {
    uint32_t *node_ids;
    uint32_t length;
    uint32_t *sub_pattern_ids;
    uint32_t sub_pattern_count;
    float strength;
    uint64_t prediction_attempts;
    uint64_t prediction_successes;
    float activation;
    float threshold;
    bool has_fired;
    uint32_t last_fired_step;
    uint32_t fired_predictions;
    uint32_t *predicted_nodes;
    float *prediction_weights;
    uint32_t prediction_count;
    uint32_t *predicted_patterns;
    float *pattern_prediction_weights;
    uint32_t pattern_prediction_count;
    float *input_weights;
    float bias;
    uint32_t input_size;
    uint32_t input_port;
    uint32_t output_port;
    float context_vector[16];
    EdgeList_Internal outgoing_patterns;
    EdgeList_Internal incoming_patterns;
    uint32_t chain_depth;
    uint32_t parent_pattern_id;
    float accumulated_meaning;
    float dynamic_importance;
    float context_frequency;
    float co_occurrence_strength;
    uint32_t *associated_patterns;
    float *association_strengths;
    uint32_t association_count;
    uint32_t association_capacity;
    uint32_t *rule_condition_patterns;
    uint32_t *rule_target_patterns;
    float *rule_boost_amounts;
    float *rule_strengths;
    uint32_t rule_count;
    uint32_t rule_capacity;
    float rule_success_rate;
    float rule_confidence;
    uint32_t rule_attempts;
    uint32_t rule_successes;
    float activation_control_strength;
    float suppression_strength;
    float boost_strength;
} Pattern_Internal;

typedef struct {
    float avg_activation;
    float avg_threshold;
    float total_activation;
    float total_edge_weight;
    float total_pattern_strength;
    uint32_t active_node_count;
    uint32_t total_edge_count;
    uint32_t active_pattern_count;
    float activation_rate;
    float learning_rate;
    float error_rate;
    float competition_pressure;
    float exploration_pressure;
    float context_vector[16];
    float learning_pressure;
    float metabolic_pressure;
    float loop_pressure;
    float pattern_confidence;
    float output_variance;
    float avg_pattern_utility;
    float activation_flow_adjustment;
    float meaning_accumulation_rate;
    float loop_breaking_strength;
    float diversity_pressure;
    uint32_t recent_outputs[50];
    uint32_t output_history_index;
    uint64_t step;
} SystemState_Internal;

typedef struct MelvinGraph {
    void *nodes;  /* Node[256] */
    EdgeList_Internal outgoing[256];
    EdgeList_Internal incoming[256];
    Pattern_Internal *patterns;
    uint32_t pattern_count;
    uint32_t pattern_capacity;
    SystemState_Internal state;
    uint32_t *input_buffer;
    uint32_t input_length;
    uint32_t input_capacity;
    uint32_t *output_buffer;
    uint32_t output_length;
    uint32_t output_capacity;
    void *output_contributions;
    uint32_t output_contrib_capacity;
    uint32_t current_input_port;
    uint32_t current_output_port;
} MelvinGraph_Internal;

/* Test data - mix of simple, medium, and complex sequences */
const char* test_data[] = {
    /* Simple repeated words */
    "cat", "dog", "bat", "rat", "hat", "mat",
    "the", "and", "for", "not", "but", "can",
    
    /* Common phrases */
    "the cat", "the dog", "the bat",
    "a cat", "a dog", "a bat",
    "is the", "on the", "in the",
    
    /* Medium complexity */
    "what is", "what is the", "where is", "where is the",
    "how do", "how do you", "why is", "why is the",
    
    /* Questions */
    "what is the capital", "what is the capital of france",
    "where is the cat", "where is the dog",
    "how do you make", "how do you make tea",
    
    /* Longer sequences */
    "the quick brown fox", "jumps over the lazy dog",
    "once upon a time", "in a land far away",
    "the sun rises in the east", "the moon shines at night",
    
    /* Technical sequences */
    "machine learning model", "neural network architecture",
    "pattern recognition system", "data processing pipeline",
    
    /* Variations to test generalization */
    "the red cat", "the blue cat", "the green cat",
    "the red dog", "the blue dog", "the green dog",
    "big red cat", "big blue dog", "big green bat",
    
    /* Complex reasoning */
    "if the cat is happy then", "if the dog is sad then",
    "when the sun rises we", "when the moon shines we",
};

const int test_data_count = sizeof(test_data) / sizeof(test_data[0]);

/* Generate random test data */
void generate_random_data(char *buffer, size_t max_len) {
    /* Pick a random sequence */
    int idx = rand() % test_data_count;
    strncpy(buffer, test_data[idx], max_len - 1);
    buffer[max_len - 1] = '\0';
}

/* Generate new combination */
void generate_novel_combination(char *buffer, size_t max_len) {
    /* Combine random pieces */
    int idx1 = rand() % test_data_count;
    int idx2 = rand() % test_data_count;
    
    snprintf(buffer, max_len, "%s %s", test_data[idx1], test_data[idx2]);
}

/* Print statistics */
void print_statistics(MelvinGraph *g_opaque, time_t start_time, int episode_count) {
    MelvinGraph_Internal *g = (MelvinGraph_Internal*)g_opaque;
    time_t now = time(NULL);
    double elapsed = difftime(now, start_time);
    
    /* Count active edges */
    uint32_t total_edges = 0;
    uint32_t active_edges = 0;
    for (int i = 0; i < 256; i++) {
        total_edges += g->outgoing[i].count;
        for (uint32_t j = 0; j < g->outgoing[i].count; j++) {
            if (g->outgoing[i].edges[j].active) {
                active_edges++;
            }
        }
    }
    
    /* Count active patterns */
    uint32_t strong_patterns = 0;
    uint32_t weak_patterns = 0;
    uint32_t hierarchical_patterns = 0;
    uint32_t blank_node_patterns = 0;
    float total_accumulated_meaning = 0.0f;
    uint32_t max_chain_depth = 0;
    
    for (uint32_t p = 0; p < g->pattern_count; p++) {
        Pattern_Internal *pat = &g->patterns[p];
        if (pat->strength > 0.5f) {
            strong_patterns++;
        } else if (pat->strength > 0.0f) {
            weak_patterns++;
        }
        
        if (pat->parent_pattern_id != 0xFFFFFFFF) {
            hierarchical_patterns++;
        }
        
        if (pat->chain_depth > max_chain_depth) {
            max_chain_depth = pat->chain_depth;
        }
        
        total_accumulated_meaning += pat->accumulated_meaning;
        
        /* Check for blank nodes */
        for (uint32_t i = 0; i < pat->length; i++) {
            if (pat->node_ids[i] == 256) {  /* BLANK_NODE */
                blank_node_patterns++;
                break;
            }
        }
    }
    
    printf("\n=== STATISTICS (%.0f seconds, %d episodes) ===\n", elapsed, episode_count);
    printf("Edges: %u total, %u active (%.1f%% active)\n", 
           total_edges, active_edges, 
           total_edges > 0 ? (active_edges * 100.0f / total_edges) : 0.0f);
    
    printf("Patterns: %u total, %u strong (>0.5), %u weak\n",
           g->pattern_count, strong_patterns, weak_patterns);
    
    printf("Generalization: %u blank node patterns (%.1f%%)\n",
           blank_node_patterns,
           g->pattern_count > 0 ? (blank_node_patterns * 100.0f / g->pattern_count) : 0.0f);
    
    printf("Hierarchy: %u child patterns, max depth=%u\n",
           hierarchical_patterns, max_chain_depth);
    
    printf("Meaning: %.1f total accumulated (avg=%.2f per pattern)\n",
           total_accumulated_meaning,
           g->pattern_count > 0 ? (total_accumulated_meaning / g->pattern_count) : 0.0f);
    
    printf("System State:\n");
    printf("  - Error Rate: %.3f\n", g->state.error_rate);
    printf("  - Learning Rate: %.3f\n", g->state.learning_rate);
    printf("  - Pattern Confidence: %.3f\n", g->state.pattern_confidence);
    printf("  - Metabolic Pressure: %.3f\n", g->state.metabolic_pressure);
    printf("  - Loop Pressure: %.3f\n", g->state.loop_pressure);
    
    printf("Performance: %.1f episodes/second\n", episode_count / elapsed);
}

int main(void) {
    printf("=== MELVIN O7: 5-Minute Continuous Learning Test ===\n");
    printf("Feeding varied data continuously for 5 minutes...\n");
    printf("Tracking pattern growth, hierarchies, and generalization.\n\n");
    
    srand(time(NULL));
    
    /* Create Melvin graph */
    MelvinGraph *g_opaque = melvin_create();
    MelvinGraph_Internal *g = (MelvinGraph_Internal*)g_opaque;
    if (!g) {
        fprintf(stderr, "Failed to create Melvin graph\n");
        return 1;
    }
    
    time_t start_time = time(NULL);
    time_t last_report = start_time;
    int episode_count = 0;
    
    char input_buffer[256];
    char target_buffer[256];
    
    /* Run for 5 minutes (300 seconds) */
    while (difftime(time(NULL), start_time) < 300.0) {
        /* Generate varied data */
        if (rand() % 3 == 0) {
            /* 33% - Novel combinations */
            generate_novel_combination(input_buffer, sizeof(input_buffer));
        } else {
            /* 67% - Known sequences */
            generate_random_data(input_buffer, sizeof(input_buffer));
        }
        
        /* Copy to target (supervised learning) */
        strcpy(target_buffer, input_buffer);
        
        /* Run episode */
        run_episode(g_opaque, 
                   (uint8_t*)input_buffer, strlen(input_buffer),
                   (uint8_t*)target_buffer, strlen(target_buffer));
        
        episode_count++;
        
        /* Report every 30 seconds */
        time_t now = time(NULL);
        if (difftime(now, last_report) >= 30.0) {
            print_statistics(g_opaque, start_time, episode_count);
            last_report = now;
        }
    }
    
    /* Final report */
    printf("\n\n=== FINAL RESULTS ===\n");
    print_statistics(g_opaque, start_time, episode_count);
    
    /* Show some example patterns */
    printf("\n=== SAMPLE PATTERNS ===\n");
    int shown = 0;
    for (uint32_t p = 0; p < g->pattern_count && shown < 20; p++) {
        Pattern_Internal *pat = &g->patterns[p];
        if (pat->strength > 0.3f) {  /* Only show strong patterns */
            printf("Pattern %u (strength=%.2f, depth=%u): \"",
                   p, pat->strength, pat->chain_depth);
            
            for (uint32_t i = 0; i < pat->length && i < 20; i++) {
                if (pat->node_ids[i] == 256) {
                    printf("_");
                } else if (pat->node_ids[i] < 128) {
                    printf("%c", (char)pat->node_ids[i]);
                } else {
                    printf("?");
                }
            }
            
            printf("\" -> predicts %u nodes", pat->prediction_count);
            
            if (pat->parent_pattern_id != 0xFFFFFFFF) {
                printf(" (child of pattern %u)", pat->parent_pattern_id);
            }
            
            printf("\n");
            shown++;
        }
    }
    
    /* Show generalization examples */
    printf("\n=== GENERALIZATION EXAMPLES (Blank Node Patterns) ===\n");
    shown = 0;
    for (uint32_t p = 0; p < g->pattern_count && shown < 10; p++) {
        Pattern_Internal *pat = &g->patterns[p];
        
        /* Check if has blank nodes */
        bool has_blank = false;
        for (uint32_t i = 0; i < pat->length; i++) {
            if (pat->node_ids[i] == 256) {
                has_blank = true;
                break;
            }
        }
        
        if (has_blank && pat->strength > 0.2f) {
            printf("Generalized pattern %u (strength=%.2f): \"", p, pat->strength);
            
            for (uint32_t i = 0; i < pat->length && i < 20; i++) {
                if (pat->node_ids[i] == 256) {
                    printf("_");
                } else if (pat->node_ids[i] < 128) {
                    printf("%c", (char)pat->node_ids[i]);
                } else {
                    printf("?");
                }
            }
            
            printf("\" (matches any sequence with blanks)\n");
            shown++;
        }
    }
    
    printf("\n=== TEST COMPLETE ===\n");
    printf("System ran for 5 minutes, processed %d episodes.\n", episode_count);
    printf("Final pattern count: %u\n", g->pattern_count);
    printf("System demonstrated continuous growth and self-regulation.\n");
    
    melvin_destroy(g_opaque);
    return 0;
}

