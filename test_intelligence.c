/* Test that the system makes INTELLIGENT decisions, not just parroting */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

typedef struct MelvinGraph MelvinGraph;
extern MelvinGraph* melvin_create(void);
extern void run_episode(MelvinGraph *g, const uint8_t *input, uint32_t input_len,
                       const uint8_t *target, uint32_t target_len);
extern void melvin_get_output(MelvinGraph *g, uint32_t **output, uint32_t *length);

int main(void) {
    printf("=== Intelligence Test Suite ===\n\n");
    
    /* TEST 1: Different inputs → Different outputs */
    printf("TEST 1: Context-dependent outputs\n");
    printf("  Training: 'a' -> 'cat', 'b' -> 'dog'\n");
    {
        MelvinGraph *g = melvin_create();
        
        /* Train both mappings */
        for (int i = 0; i < 20; i++) {
            run_episode(g, (const uint8_t*)"a", 1, (const uint8_t*)"cat", 3);
            run_episode(g, (const uint8_t*)"b", 1, (const uint8_t*)"dog", 3);
        }
        
        /* Test 'a' */
        run_episode(g, (const uint8_t*)"a", 1, NULL, 0);
        uint32_t *out; uint32_t len;
        melvin_get_output(g, &out, &len);
        printf("  Input 'a' -> '");
        for (uint32_t i = 0; i < len && i < 10; i++) printf("%c", (char)out[i]);
        printf("' (expected 'cat')\n");
        int test1a = (len >= 3 && out[0] == 'c' && out[1] == 'a' && out[2] == 't');
        
        /* Test 'b' */
        run_episode(g, (const uint8_t*)"b", 1, NULL, 0);
        melvin_get_output(g, &out, &len);
        printf("  Input 'b' -> '");
        for (uint32_t i = 0; i < len && i < 10; i++) printf("%c", (char)out[i]);
        printf("' (expected 'dog')\n");
        int test1b = (len >= 3 && out[0] == 'd' && out[1] == 'o' && out[2] == 'g');
        
        printf("  Result: %s\n\n", (test1a && test1b) ? "PASS - Context matters!" : "FAIL");
    }
    
    /* TEST 2: Learned echo (when echo IS the right answer) */
    printf("TEST 2: Intelligent echo (when trained to echo)\n");
    printf("  Training: 'x' -> 'x', 'y' -> 'y'\n");
    {
        MelvinGraph *g = melvin_create();
        
        for (int i = 0; i < 20; i++) {
            run_episode(g, (const uint8_t*)"x", 1, (const uint8_t*)"x", 1);
            run_episode(g, (const uint8_t*)"y", 1, (const uint8_t*)"y", 1);
        }
        
        run_episode(g, (const uint8_t*)"x", 1, NULL, 0);
        uint32_t *out; uint32_t len;
        melvin_get_output(g, &out, &len);
        printf("  Input 'x' -> '");
        for (uint32_t i = 0; i < len && i < 10; i++) printf("%c", (char)out[i]);
        printf("' (expected 'x')\n");
        int test2a = (len >= 1 && out[0] == 'x');
        
        run_episode(g, (const uint8_t*)"y", 1, NULL, 0);
        melvin_get_output(g, &out, &len);
        printf("  Input 'y' -> '");
        for (uint32_t i = 0; i < len && i < 10; i++) printf("%c", (char)out[i]);
        printf("' (expected 'y')\n");
        int test2b = (len >= 1 && out[0] == 'y');
        
        printf("  Result: %s\n\n", (test2a && test2b) ? "PASS - Learned echo!" : "FAIL");
    }
    
    /* TEST 3: NO untrained echo (shouldn't echo what it wasn't trained on) */
    printf("TEST 3: No blind echo (untrained input)\n");
    printf("  Training: 'a' -> 'cat' only\n");
    {
        MelvinGraph *g = melvin_create();
        
        for (int i = 0; i < 20; i++) {
            run_episode(g, (const uint8_t*)"a", 1, (const uint8_t*)"cat", 3);
        }
        
        /* Test with untrained input 'z' */
        run_episode(g, (const uint8_t*)"z", 1, NULL, 0);
        uint32_t *out; uint32_t len;
        melvin_get_output(g, &out, &len);
        printf("  Input 'z' (untrained) -> '");
        for (uint32_t i = 0; i < len && i < 10; i++) printf("%c", (char)out[i]);
        printf("'\n");
        
        /* Should NOT blindly echo 'z' - that would be stupid */
        /* Might output nothing, or generalize, but not blind echo */
        int blind_echo = (len >= 1 && out[0] == 'z');
        printf("  Result: %s\n\n", !blind_echo ? "PASS - Not blind echo!" : "FAIL - Blind echo is not intelligence");
    }
    
    /* TEST 4: Sequence continuation */
    printf("TEST 4: Sequence learning\n");
    printf("  Training: 'hel' -> 'hello'\n");
    {
        MelvinGraph *g = melvin_create();
        
        for (int i = 0; i < 30; i++) {
            run_episode(g, (const uint8_t*)"hel", 3, (const uint8_t*)"hello", 5);
        }
        
        run_episode(g, (const uint8_t*)"hel", 3, NULL, 0);
        uint32_t *out; uint32_t len;
        melvin_get_output(g, &out, &len);
        printf("  Input 'hel' -> '");
        for (uint32_t i = 0; i < len && i < 10; i++) printf("%c", (char)out[i]);
        printf("' (expected 'hello')\n");
        
        int test4 = (len >= 5 && out[0] == 'h' && out[1] == 'e' && out[2] == 'l' && out[3] == 'l' && out[4] == 'o');
        printf("  Result: %s\n\n", test4 ? "PASS - Sequence completion!" : "PARTIAL");
    }
    
    printf("=== Summary ===\n");
    printf("Intelligence = making context-appropriate decisions\n");
    printf("Not intelligence = blind echo or random output\n");
    
    return 0;
}
