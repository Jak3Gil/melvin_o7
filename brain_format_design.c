/* Brain Format Design: .m file as executable brain

The .m file is NOT data - it's the active brain that does the work.
melvin.c just interprets/executes it.

.m file format (text-based, human-readable):
===========================================

# Pattern definitions
pattern "cat" -> "s" context:text strength:0.8
pattern "_at" -> "s" context:text strength:0.7  # generalized

# Edge definitions  
edge 'c' -> 'a' weight:0.5
edge 'a' -> 't' weight:0.6
edge 't' -> 's' weight:0.8

# Node states
node 'c' energy:0.7 threshold:0.5
node 'a' energy:0.6 threshold:0.5

# Execution rules (optional - defaults in melvin.c)
rule pattern_fire_threshold:0.3
rule wave_prop_steps:10

The .m file is:
- Written by the system as it learns
- Read by melvin.c to initialize the graph
- The active brain that processes inputs
- Evolves/grows as learning happens

melvin.c's job:
- Parse .m file
- Build graph structure from it
- Run wave propagation
- Execute patterns
- Update .m file as learning happens

*/

