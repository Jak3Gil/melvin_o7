/* Simple test: Read from test_input.txt, input to port 0, watch port 0 output */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

/* Compile with: gcc -o simple_test.exe simple_test.c melvin.c -lm */
/* But we need to avoid main() conflict - let's use a different approach */

/* Just create a wrapper that calls melvin functions */
/* We'll need to compile melvin.c without MELVIN_STANDALONE */

