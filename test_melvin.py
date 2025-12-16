#!/usr/bin/env python3
"""
MELVIN O7 Test Runner
Tests simple and complex tasks, tracks training samples needed
"""

import subprocess
import re
import sys

def parse_test_file(filename):
    """Parse test_input.txt into test cases"""
    tests = []
    with open(filename, 'r') as f:
        for line in f:
            line = line.strip()
            if not line or line.startswith('#'):
                continue
            
            # Parse: input -> output
            if '->' in line:
                parts = line.split('->')
                if len(parts) == 2:
                    input_text = parts[0].strip()
                    expected = parts[1].strip()
                    tests.append((input_text, expected))
    
    return tests

def run_melvin_test(input_text, expected_output, episode_num):
    """Run a single test through melvin"""
    # Create a simple C test program
    test_code = f'''
#include "melvin.c"

int main() {{
    MelvinGraph *g = melvin_create();
    if (!g) return 1;
    
    // Set text port
    melvin_set_input_port(g, 0);  // Port 0 = text
    melvin_set_output_port(g, 0);
    
    // Input
    const uint8_t *input = (const uint8_t*)"{input_text}";
    uint32_t input_len = {len(input_text)};
    const uint8_t *target = (const uint8_t*)"{expected_output}";
    uint32_t target_len = {len(expected_output)};
    
    // Run episode
    run_episode(g, input, input_len, target, target_len);
    
    // Get output
    uint32_t *output;
    uint32_t output_len;
    melvin_get_output(g, &output, &output_len);
    
    // Print result
    printf("Ep %d | Input: %s -> Output: ", {episode_num}, "{input_text}");
    for (uint32_t i = 0; i < output_len; i++) {{
        printf("%c", (char)output[i]);
    }}
    printf(" | Expected: %s | Error: %.3f\\n", 
           "{expected_output}", melvin_get_error_rate(g));
    
    melvin_destroy(g);
    return 0;
}}
'''
    
    # Write test program
    with open('test_single.c', 'w') as f:
        f.write(test_code)
    
    # Compile and run
    try:
        result = subprocess.run(
            ['gcc', '-o', 'test_single.exe', 'test_single.c', '-lm', '-DMELVIN_STANDALONE'],
            capture_output=True,
            text=True
        )
        if result.returncode != 0:
            print(f"Compile error: {result.stderr}")
            return None
        
        result = subprocess.run(
            ['./test_single.exe'],
            capture_output=True,
            text=True
        )
        return result.stdout
    except Exception as e:
        print(f"Error: {e}")
        return None

def main():
    print("MELVIN O7: Pattern Hierarchies & Wave Propagation Test")
    print("=" * 60)
    print()
    
    # Parse tests
    tests = parse_test_file('test_input.txt')
    print(f"Loaded {len(tests)} test cases")
    print()
    
    # Group tests by complexity
    simple_tests = tests[:5]  # First 5 are simple
    pattern_tests = tests[5:9]  # Pattern learning
    context_tests = tests[9:14]  # Context learning
    complex_tests = tests[14:]  # Complex sequences
    
    print("=== SIMPLE TESTS (Wire-like) ===")
    for i, (input_text, expected) in enumerate(simple_tests, 1):
        print(f"Test {i}: '{input_text}' -> '{expected}'")
        # Would need to run through melvin here
    
    print()
    print("=== PATTERN LEARNING TESTS ===")
    for i, (input_text, expected) in enumerate(pattern_tests, 1):
        print(f"Test {i}: '{input_text}' -> '{expected}'")
    
    print()
    print("=== CONTEXT LEARNING TESTS ===")
    for i, (input_text, expected) in enumerate(context_tests, 1):
        print(f"Test {i}: '{input_text}' -> '{expected}'")
    
    print()
    print("=== COMPLEX SEQUENCE TESTS ===")
    for i, (input_text, expected) in enumerate(complex_tests, 1):
        print(f"Test {i}: '{input_text}' -> '{expected}'")
    
    print()
    print("Note: To actually run these tests, integrate with melvin.c")
    print("This file shows the test structure and expected behavior")

if __name__ == '__main__':
    main()

