/* ============================================================================
 * PATH QUALITY DIAGNOSTIC TEST: Probe how well path quality works
 * ============================================================================ */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "melvin.c"

int main(void) {
    printf("========================================\n");
    printf("PATH QUALITY DIAGNOSTIC TEST\n");
    printf("========================================\n\n");
    
    MelvinGraph *g = melvin_create();
    if (!g) {
        fprintf(stderr, "Failed to create graph\n");
        return 1;
    }
    
    /* Test 1: Single training, check if paths are learned */
    printf("Test 1: Single Training Episode\n");
    printf("--------------------------------\n");
    printf("Training 'h' -> 'e' -> 'l' -> 'l' -> 'o' -> 'w' -> 'o' -> 'r' -> 'l' -> 'd'\n");
    run_episode(g, (const uint8_t*)"hello", 5, (const uint8_t*)"world", 5);
    
    printf("\nAfter training:\n");
    printf("- Input length: %u\n", g->input_length);
    printf("- Output length: %u\n", g->output_length);
    printf("- Pattern count: %u\n", g->pattern_count);
    printf("- Avg activation: %.3f\n", g->state.avg_activation);
    printf("- Avg threshold: %.3f\n", g->state.avg_threshold);
    
    /* Check if edges were created */
    int edge_count = 0;
    for (int i = 0; i < BYTE_VALUES; i++) {
        if (g->nodes[i].exists && g->outgoing[i].count > 0) {
            edge_count += g->outgoing[i].count;
        }
    }
    printf("- Total edges: %d\n", edge_count);
    
    /* Test 2: Multiple training, then inference */
    printf("\nTest 2: Multiple Training Episodes\n");
    printf("-----------------------------------\n");
    for (int i = 0; i < 10; i++) {
        run_episode(g, (const uint8_t*)"hello", 5, (const uint8_t*)"world", 5);
    }
    
    printf("After 10 training episodes:\n");
    printf("- Pattern count: %u\n", g->pattern_count);
    printf("- Avg activation: %.3f\n", g->state.avg_activation);
    printf("- Total edges: %d\n", edge_count);
    
    /* Test 3: Inference (no target) */
    printf("\nTest 3: Inference (No Target)\n");
    printf("-----------------------------\n");
    printf("Input: 'hello'\n");
    run_episode(g, (const uint8_t*)"hello", 5, NULL, 0);
    
    printf("Output: \"");
    for (uint32_t i = 0; i < g->output_length && i < 50; i++) {
        uint8_t b = (uint8_t)g->output_buffer[i];
        if (b >= 32 && b < 127) {
            printf("%c", (char)b);
        } else {
            printf("[%u]", b);
        }
    }
    printf("\" (length: %u)\n", g->output_length);
    
    /* Check node activations */
    printf("\nNode activations (top 10):\n");
    float activations[BYTE_VALUES];
    uint32_t indices[BYTE_VALUES];
    for (int i = 0; i < BYTE_VALUES; i++) {
        activations[i] = g->nodes[i].activation;
        indices[i] = i;
    }
    
    /* Simple bubble sort (top 10) */
    for (int i = 0; i < 10 && i < BYTE_VALUES; i++) {
        for (int j = i + 1; j < BYTE_VALUES; j++) {
            if (activations[j] > activations[i]) {
                float temp_f = activations[i];
                uint32_t temp_i = indices[i];
                activations[i] = activations[j];
                indices[i] = indices[j];
                activations[j] = temp_f;
                indices[j] = temp_i;
            }
        }
    }
    
    for (int i = 0; i < 10; i++) {
        if (g->nodes[indices[i]].exists && activations[i] > 0.0f) {
            printf("  Node %u ('%c'): activation=%.4f, threshold=%.4f\n", 
                   indices[i], 
                   (indices[i] < 256 && indices[i] >= 32) ? (char)indices[i] : '?',
                   activations[i],
                   g->nodes[indices[i]].threshold);
        }
    }
    
    /* Test 4: Check pattern activations */
    printf("\nTest 4: Pattern Activations\n");
    printf("---------------------------\n");
    printf("Active patterns (top 5):\n");
    int active_count = 0;
    for (uint32_t p = 0; p < g->pattern_count && active_count < 5; p++) {
        if (g->patterns[p].activation > g->patterns[p].threshold) {
            printf("  Pattern %u: activation=%.4f, threshold=%.4f, strength=%.4f, length=%u\n",
                   p, g->patterns[p].activation, g->patterns[p].threshold,
                   g->patterns[p].strength, g->patterns[p].length);
            active_count++;
        }
    }
    if (active_count == 0) {
        printf("  No active patterns\n");
    }
    
    printf("\n========================================\n");
    printf("DIAGNOSTIC COMPLETE\n");
    printf("========================================\n");
    
    melvin_destroy(g);
    return 0;
}
