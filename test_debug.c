/* ============================================================================
 * DEBUG TEST: Why is system repeating?
 * 
 * Trace step-by-step what's happening during output
 * ============================================================================ */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

typedef struct MelvinGraph MelvinGraph;
extern MelvinGraph* melvin_create(void);
extern void inject_input(MelvinGraph *g, const uint8_t *bytes, uint32_t length);
extern void propagate_activation(MelvinGraph *g);
extern uint32_t select_output_node(MelvinGraph *g);
extern void emit_output(MelvinGraph *g, uint32_t node_id);
extern void melvin_get_output(MelvinGraph *g, uint32_t **output, uint32_t *length);
extern float melvin_get_node_activation(MelvinGraph *g, uint32_t node_id);
extern float melvin_get_pattern_activation(MelvinGraph *g, uint32_t pattern_id);

/* Add accessor functions */
float melvin_get_node_activation(MelvinGraph *g, uint32_t node_id) {
    return g->nodes[node_id].activation;
}

float melvin_get_pattern_activation(MelvinGraph *g, uint32_t pattern_id) {
    if (pattern_id >= g->pattern_count) return 0.0f;
    return g->patterns[pattern_id].activation;
}

int main(void) {
    printf("=================================================================\n");
    printf("DEBUG: Why is System Repeating?\n");
    printf("=================================================================\n\n");
    
    MelvinGraph *g = melvin_create();
    if (!g) {
        fprintf(stderr, "Failed to create graph\n");
        return 1;
    }
    
    /* Train once to create patterns */
    printf("Training: 'cat' → 'cats'\n");
    uint8_t input[] = {'c', 'a', 't'};
    uint8_t target[] = {'c', 'a', 't', 's'};
    
    for (int ep = 0; ep < 10; ep++) {
        inject_input(g, input, 3);
        for (int step = 0; step < 6; step++) {
            propagate_activation(g);
        }
        /* Apply feedback manually */
        for (int i = 0; i < 3; i++) {
            create_or_strengthen_edge(g, target[i], target[i+1]);
        }
    }
    
    printf("\n=== TRACING OUTPUT SELECTION ===\n");
    printf("Testing on 'cat' - trace what gets selected and why\n\n");
    
    /* Reset and inject input */
    g->output_length = 0;
    inject_input(g, input, 3);
    
    printf("After input injection:\n");
    printf("  Node 'c': activation=%.3f\n", melvin_get_node_activation(g, 'c'));
    printf("  Node 'a': activation=%.3f\n", melvin_get_node_activation(g, 'a'));
    printf("  Node 't': activation=%.3f\n", melvin_get_node_activation(g, 't'));
    printf("  Node 's': activation=%.3f\n", melvin_get_node_activation(g, 's'));
    
    /* Trace first few outputs */
    for (int step = 0; step < 10; step++) {
        propagate_activation(g);
        
        uint32_t selected = select_output_node(g);
        
        printf("\nStep %d:\n", step);
        printf("  Node activations: c=%.3f a=%.3f t=%.3f s=%.3f\n",
               melvin_get_node_activation(g, 'c'),
               melvin_get_node_activation(g, 'a'),
               melvin_get_node_activation(g, 't'),
               melvin_get_node_activation(g, 's'));
        
        if (selected > 0) {
            printf("  Selected: '%c' (node %u)\n", (char)selected, selected);
            emit_output(g, selected);
            
            printf("  Output so far: ");
            uint32_t *output;
            uint32_t output_len;
            melvin_get_output(g, &output, &output_len);
            for (uint32_t i = 0; i < output_len; i++) {
                printf("%c", (uint8_t)output[i]);
            }
            printf("\n");
        } else {
            printf("  No node selected\n");
        }
        
        if (step > 5 && selected == 's') {
            printf("  WARNING: Stuck repeating 's'!\n");
            break;
        }
    }
    
    return 0;
}

