# Melvin O7: Complete Research Document

## Table of Contents
1. [Executive Summary](#executive-summary)
2. [Core Philosophy](#core-philosophy)
3. [Fundamental Architecture](#fundamental-architecture)
4. [Data Structures](#data-structures)
5. [How It Works: Simple to Complex](#how-it-works-simple-to-complex)
6. [Calculation Locations](#calculation-locations)
7. [Learning Mechanisms](#learning-mechanisms)
8. [Wave Propagation](#wave-propagation)
9. [Pattern System](#pattern-system)
10. [Output Selection](#output-selection)
11. [Self-Regulation](#self-regulation)
12. [Complete Data Flow](#complete-data-flow)

---

## Executive Summary

**Melvin O7** is a self-regulating neural graph system that learns sequences and patterns without hardcoded limits. Unlike traditional neural networks that have fixed architectures, Melvin's structure grows and adapts dynamically based on experience.

### Key Differentiators:
- **No Static Limits**: Everything is relative and self-adjusting
- **Dynamic Structure**: Graph grows as needed (edges, patterns, connections)
- **Pattern Hierarchies**: Patterns form parent-child relationships, accumulating meaning
- **Wave Propagation**: Multi-step activation flow (not single-pass like neural nets)
- **Self-Regulation**: System detects problems and fixes them automatically
- **Generalization**: Blank nodes enable patterns to match unseen sequences

---

## Core Philosophy

### Principle 1: Everything is Relative
- No absolute thresholds or maximum values
- All values are computed relative to system state
- Example: Node activation threshold = `avg_threshold * 1.2` (relative to average)

### Principle 2: Circular Dependencies
- Every variable influences others
- Every variable is influenced by others
- Creates stable attractors (emergent behavior)

### Principle 3: Self-Regulation
- System computes its own problems (error rate, loop pressure, metabolic pressure)
- System fixes problems automatically (adjusts thresholds, breaks loops, prunes edges)
- No manual tuning required

### Principle 4: Emergence from Use
- Edge weights grow through usage, not manual setting
- Pattern strength emerges from prediction accuracy
- Strong paths get stronger (circular reinforcement)

---

## Fundamental Architecture

### The Graph Structure

```
MelvinGraph
├── Nodes[256]           # One node per byte value (0-255)
├── Edges                # Dynamic connections between nodes
│   ├── outgoing[256]    # Edges FROM each node
│   └── incoming[256]    # Edges TO each node
├── Patterns[]           # Discovered sequence chunks (dynamic array)
├── SystemState          # Computed statistics (averages, rates, pressures)
├── InputBuffer[]        # Current input sequence
└── OutputBuffer[]       # Generated output sequence
```

### Three Layers of Intelligence

1. **Node Layer**: Byte-level primitives (256 nodes, one per byte)
2. **Edge Layer**: Learned associations between nodes
3. **Pattern Layer**: Discovered sequences and concepts (hierarchical)

---

## Data Structures

### Node (Lines 44-66)
```c
typedef struct {
    uint8_t payload;           // Byte value (0-255)
    bool exists;                // Has this node been created?
    uint32_t source_port;      // Which modality (text/audio/vision)
    
    // Dynamic State (all relative)
    float activation;           // Current activation [0,1]
    float threshold;           // Firing threshold (relative to avg)
    
    // History (for rates/derivatives)
    float prev_activation;
    float activation_momentum;
    
    // Statistics
    uint64_t fire_count;       // Times fired
    uint64_t receive_count;    // Times received input
} Node;
```

**Key Point**: Activation is purely local - no global energy constraint. Each node's activation is independent.

### Edge (Lines 75-90)
```c
typedef struct {
    uint32_t to_id;            // Target node or pattern ID
    float weight;              // RELATIVE strength (not global)
    uint64_t use_count;        // Times traversed
    uint64_t success_count;    // Times led to correct output
    bool active;               // Currently in use?
    bool is_pattern_edge;       // Points to pattern (not node)?
} Edge;
```

**Key Point**: Weight is relative to other edges FROM THE SAME SOURCE NODE, not globally normalized.

### Pattern (Lines 115-215)
```c
typedef struct {
    uint32_t *node_ids;        // Sequence (can include BLANK_NODE)
    uint32_t length;           // Length of sequence
    
    // Hierarchical Structure
    uint32_t chain_depth;      // Depth in hierarchy
    uint32_t parent_pattern_id; // Parent pattern
    float accumulated_meaning;  // Meaning from hierarchy
    
    // Neural Net Components
    uint32_t *predicted_nodes; // What this pattern predicts
    float *prediction_weights; // Weights for predictions
    float *input_weights;     // Weights for input nodes
    float bias;                // Bias term
    
    // Activation State
    float activation;          // Current pattern activation
    float threshold;           // Firing threshold
    float strength;            // Relative strength [0,1]
    
    // Prediction Tracking
    uint64_t prediction_attempts;
    uint64_t prediction_successes;
    
    // Pattern-to-Pattern Connections
    EdgeList outgoing_patterns; // Edges to other patterns
    EdgeList incoming_patterns; // Edges from other patterns
    
    // Learned Rules (IF-THEN behavior)
    uint32_t *rule_condition_patterns;
    uint32_t *rule_target_patterns;
    float *rule_strengths;
} Pattern;
```

**Key Point**: Patterns are micro neural networks that predict next nodes based on matched sequences.

### SystemState (Lines 254-302)
```c
typedef struct {
    // Averages (denominators for ratios)
    float avg_activation;
    float avg_threshold;
    
    // Sums (for proportions)
    float total_activation;
    float total_edge_weight;
    float total_pattern_strength;
    
    // Rates (derivatives)
    float activation_rate;     // Change in activation
    float learning_rate;      // Change in edge weights
    float error_rate;         // Proportion of incorrect predictions
    
    // Pressures (emergent from ratios)
    float competition_pressure;  // Nodes compete vs cooperate
    float exploration_pressure;  // Explore vs exploit
    float metabolic_pressure;    // Too many connections = prune
    float loop_pressure;          // Stuck in loop = escape
    float pattern_confidence;     // Trust in patterns
    
    // Self-Tuning Adjustments
    float activation_flow_adjustment;
    float meaning_accumulation_rate;
    float loop_breaking_strength;
    float diversity_pressure;
    
    uint64_t step;             // Global step counter
} SystemState;
```

**Key Point**: All values are computed, not set. System state is recalculated each step.

---

## How It Works: Simple to Complex

### Level 1: Simple Echo (No Learning)

**Input**: "cat"  
**Output**: "cat"

**Process**:
1. Input bytes injected → nodes 'c', 'a', 't' created
2. Sequential edges created: c→a, a→t
3. Wave propagation: activation flows c→a→t
4. Output selection: follows strongest edge path
5. Result: Echoes input

**Where**: `run_episode()` → `propagate_activation()` → `select_output_node()`

### Level 2: Learning Sequences

**Input**: "cat" → "cat" (repeated)  
**Output**: "cat" (learned)

**Process**:
1. First episode: Creates edges c→a, a→t
2. Second episode: Strengthens existing edges (weight increases)
3. Pattern detection: Finds sequence "cat" → creates pattern
4. Pattern learns: "cat" predicts nothing (end of sequence)
5. Result: System remembers "cat"

**Where**: `detect_patterns()` → `learn_pattern_predictions()`

### Level 3: Pattern Generalization

**Input**: "cat", "bat", "rat"  
**Output**: Recognizes "_at" pattern

**Process**:
1. Sees "cat", "bat", "rat" multiple times
2. `detect_generalized_patterns()` finds common suffix "at"
3. Creates pattern with BLANK_NODE: `[BLANK, 'a', 't']`
4. Pattern "_at" matches all three words
5. Pattern learns: "_at" → predicts end of word
6. Result: Generalizes to unseen words ending in "at"

**Where**: `detect_generalized_patterns()` (lines 1847-1986)

### Level 4: Hierarchical Patterns

**Input**: "the cat", "the bat", "the rat"  
**Output**: Forms hierarchy

**Process**:
1. Pattern "_at" exists (from Level 3)
2. Pattern "the" exists
3. Pattern "the _" detected (blank after "the")
4. Hierarchy forms: "the _" → "_at" (parent-child)
5. Meaning accumulates: "the" + "_at" = "the [animal ending in at]"
6. Result: Hierarchical understanding

**Where**: `learn_pattern_sequences_automatic()` → pattern hierarchy tracking

### Level 5: Complex Reasoning

**Input**: "what is the capital of france"  
**Output**: "paris"

**Process**:
1. Pattern "what is" matches → predicts "the"
2. Pattern "what is the" matches → predicts "capital"
3. Pattern "capital of" matches → predicts country name
4. Pattern "france" matches → predicts "paris"
5. Wave propagation: Multiple patterns activate simultaneously
6. Pattern-guided selection: Follows pattern predictions
7. Result: Multi-step reasoning through pattern chain

**Where**: `propagate_pattern_activation()` → `select_output_node()` (pattern-guided)

---

## Calculation Locations

### System State Computation
**Location**: `compute_system_state()` (lines 505-632)

**Calculations**:
```c
// Average activation
avg_activation = total_activation / existing_node_count

// Competition pressure (from variance)
variance = sum((node_activation - avg_activation)²) / node_count
competition_pressure = 1 / (1 + exp(-10 * (variance - 0.5)))

// Learning rate (from usage + exploration)
usage_pressure = avg_edge_usage / 5.0
learning_rate = 0.3 + (usage_pressure * 0.3) + (exploration_pressure * 0.2)

// Metabolic pressure (from graph density)
edge_density = total_edges / (256 * 10)
pattern_density = pattern_count / 100
metabolic_pressure = (edge_density + pattern_density) / 2

// Self-tuning adjustments
activation_flow_adjustment = 1.0 + (error_rate * 2.0)
meaning_accumulation_rate = 1.0 - (error_rate * 0.5)
loop_breaking_strength = loop_pressure * 10.0
```

**Called**: Every step (or every N steps in chat mode)

### Wave Propagation
**Location**: `propagate_activation()` (lines 2289-2714)

**Calculations**:
```c
// PHASE 1: Pattern-guided propagation
propagate_pattern_activation(g);  // Patterns predict next nodes

// PHASE 2: Edge-based propagation
for each active node:
    for each outgoing edge:
        // Path quality calculation
        input_connectivity = (edge connected to input?) ? 1.0 : 0.0
        context_match = (pattern predicts target?) ? 1.0 : 0.0
        history_coherence = (edge follows output history?) ? 1.0 : 0.0
        pattern_meaning = (pattern hierarchy depth) * accumulated_meaning
        path_importance = (usage + success_rate + weight) / 3.0
        
        // Information flow efficiency
        path_quality = (input_connectivity * 0.3 + 
                      context_match * 0.2 + 
                      history_coherence * 0.2 + 
                      pattern_meaning * 0.15 + 
                      path_importance * 0.15)
        
        // Activation transfer
        relative_weight = edge.weight / max_weight_from_source
        transfer = source_activation * relative_weight * path_quality
        target_activation += transfer
```

**Key**: Activation flows through edges, weighted by path quality (not just edge weight).

### Pattern Activation
**Location**: `propagate_pattern_activation()` (lines 1156-1573)

**Calculations**:
```c
for each pattern:
    // Check if pattern matches input or output
    if pattern_matches(pattern, input_sequence):
        // Forward pass (neural net)
        weighted_sum = bias
        for each input_node:
            weighted_sum += node_activation * input_weights[i]
        pattern_activation = sigmoid(weighted_sum) * strength
        
        // Context boost
        context_coverage = pattern_length / input_length
        context_boost = 1.0 + (context_coverage * 0.5)
        pattern_activation *= context_boost
        
        // Predict next nodes
        for each predicted_node:
            transfer = pattern_activation * prediction_weight * strength * 3.0
            predicted_node.activation += transfer
```

**Key**: Patterns act as micro neural networks, predicting next nodes when they match.

### Node Relevance
**Location**: `compute_node_relevance()` (lines 3689-3817)

**Calculations**:
```c
// Context 1: Position context (pattern predictions)
position_context = sum(pattern_strength * prediction_weight)
    for all patterns that predict this node

// Context 2: History penalty (avoid loops)
history_penalty = sum(recency * 0.5)
    for all times node appeared in output

// Context 3: Wave activation
wave_activation = node.activation

// Context 4: Input context
input_context = 0.5 * (if node in input) + 
                0.3 * (if node connected to input via edges)

// Combined relevance
if position_context > 0.1:
    // Pattern-driven (intelligent)
    pattern_relevance = position_context * (1 - history_penalty) * (1 + wave_activation)
    wave_relevance = wave_activation * (1 - history_penalty) * (1 + input_context * 0.5)
    relevance = (pattern_weight * pattern_relevance) + (wave_weight * wave_relevance)
else:
    // Activation-driven (fallback)
    relevance = wave_activation * (1 - history_penalty) * (1 + input_context * 0.5)
```

**Key**: Relevance combines pattern predictions, wave activation, input context, and history.

### Output Selection
**Location**: `select_output_node()` (lines 3819-4014)

**Calculations**:
```c
// STEP 1: Pattern-guided selection (if patterns are confident)
if pattern_confidence > 0.5:
    for each pattern that matches output end:
        for each predicted_node:
            pattern_score = pattern_strength * pattern_activation * prediction_weight
            if pattern_confidence > 0.7:
                pattern_score *= 2.0  // Strong boost when reliable
            pattern_score += predicted_node.activation * 0.5
            pattern_score *= loop_penalty  // Suppress loops
            
            if pattern_score > best_score:
                best_score = pattern_score
                selected = predicted_node

// STEP 2: Greedy edge following (fallback)
if no pattern match:
    // Prioritize input sequence when output is empty
    if output_length == 0:
        selected = input_buffer[0]  // Start with first input
    else:
        // Follow strongest edge from last output
        last_node = output_buffer[output_length - 1]
        for each edge from last_node:
            edge_score = edge.weight * target_activation
            edge_score *= loop_penalty
            
            if edge_score > best_score:
                best_score = edge_score
                selected = target_node

// STEP 3: Relevance-based fallback
if still no selection:
    for each node:
        relevance = compute_node_relevance(node)
        if relevance > best_score:
            best_score = relevance
            selected = node
```

**Key**: Hybrid selection - patterns first (intelligent), edges second (greedy), relevance third (fallback).

---

## Learning Mechanisms

### 1. Sequential Edge Learning
**Location**: `create_or_strengthen_edge()` (lines 808-918)

**Process**:
```c
// When sequence "cat" is seen:
create_or_strengthen_edge('c', 'a')  // c→a
create_or_strengthen_edge('a', 't')  // a→t

// Edge weight grows with usage
edge.weight += learning_rate * (1.0 - edge.weight)
edge.use_count++
```

**Result**: Sequential associations learned automatically.

### 2. Pattern Detection
**Location**: `detect_patterns()` (lines 3282-3578)

**Process**:
```c
// Find repeated sequences
for each sequence of length 2 to 10:
    count = count_occurrences(sequence)
    if count >= threshold:
        create_pattern(sequence)
        
// Pattern learns predictions
for each pattern:
    if pattern matches sequence:
        next_node = sequence[pattern.length]
        add_prediction(pattern, next_node)
```

**Result**: Common sequences become patterns.

### 3. Pattern Generalization
**Location**: `detect_generalized_patterns()` (lines 1847-1986)

**Process**:
```c
// Find common suffixes/prefixes
// Example: "cat", "bat", "rat" → "_at"
for each position in input:
    suffix = input[pos+1:pos+3]  // e.g., "at"
    count = count_words_ending_in(suffix)
    
    if count >= threshold:
        create_pattern([BLANK_NODE, 'a', 't'])
```

**Result**: Blank nodes enable generalization to unseen sequences.

### 4. Pattern Hierarchy
**Location**: `learn_pattern_sequences_automatic()` (lines 3063-3150)

**Process**:
```c
// When pattern A followed by pattern B:
if pattern_A matches input[pos1:pos1+len_A]:
    if pattern_B matches input[pos1+len_A:pos1+len_A+len_B]:
        // Create pattern-to-pattern edge
        create_or_strengthen_pattern_edge(pattern_A, pattern_B)
        
        // Update hierarchy
        pattern_B.parent_pattern_id = pattern_A.id
        pattern_B.chain_depth = pattern_A.chain_depth + 1
        pattern_B.accumulated_meaning = pattern_A.accumulated_meaning * 1.2
```

**Result**: Patterns form parent-child relationships, accumulating meaning.

### 5. Co-activation Learning
**Location**: `create_edges_from_coactivation()` (lines 2858-2919)

**Process**:
```c
// When nodes activate simultaneously:
for each pair of active nodes (i, j):
    if i and j both active:
        // Create unidirectional edge (stable direction)
        if i < j:
            create_or_strengthen_edge(i, j)
```

**Result**: Nodes that fire together get connected.

### 6. Pattern-Based Edge Creation
**Location**: `create_edges_from_patterns()` (lines 3061-3054)

**Process**:
```c
// When pattern predicts node:
for each active pattern:
    for each predicted_node:
        last_pattern_node = pattern_inputs[pattern.length - 1]
        create_or_strengthen_edge(last_pattern_node, predicted_node)
```

**Result**: Pattern predictions create edges.

### 7. Feedback Learning
**Location**: `apply_feedback()` (lines 4518-4633)

**Process**:
```c
// Compare output to target
for each position:
    if output[pos] == target[pos]:
        // Correct - strengthen contributing components
        for each pattern that predicted correctly:
            pattern.prediction_weights[pred] += learning_rate * error_share
            pattern.prediction_successes++
        
        for each edge that contributed correctly:
            edge.weight += learning_rate
            edge.success_count++
    else:
        // Incorrect - weaken contributing components
        for each pattern that predicted incorrectly:
            pattern.prediction_weights[pred] -= learning_rate * error_share
        
        for each edge that contributed incorrectly:
            edge.weight -= learning_rate * 0.5
```

**Result**: Successful paths get stronger, failed paths get weaker.

---

## Wave Propagation

### Multi-Step vs Single-Pass

**Traditional Neural Net**:
```
Input → Layer1 → Layer2 → Output (1 pass)
```

**Melvin O7**:
```
Input → Step1 → Step2 → ... → StepN → Output (multi-step wave)
```

### Wave Propagation Steps

1. **Pattern Activation** (lines 1156-1573)
   - Patterns match input/output sequences
   - Patterns predict next nodes
   - Activation flows to predicted nodes

2. **Edge-Based Propagation** (lines 2435-2714)
   - Activation flows through learned edges
   - Path quality determines transfer amount
   - Multiple paths compete

3. **Node Dynamics Update** (lines 692-771)
   - Activation decays naturally
   - Threshold adjusts relative to average
   - Momentum computed (rate of change)

4. **Output Selection** (lines 3819-4014)
   - Selects node based on relevance
   - Suppresses loops
   - Prioritizes pattern predictions

### Path Quality Calculation

**Location**: `propagate_activation()` (lines 2445-2600)

```c
path_quality = (
    input_connectivity * 0.3 +      // Connected to input?
    context_match * 0.2 +            // Pattern predicts target?
    history_coherence * 0.2 +        // Follows output history?
    pattern_meaning * 0.15 +         // Pattern hierarchy depth
    path_importance * 0.15           // Edge usage + success
)
```

**Key**: Not just edge weight - considers information flow efficiency.

---

## Pattern System

### Pattern Matching
**Location**: `pattern_matches()` (lines 977-1017)

```c
bool pattern_matches(pattern, sequence, start_pos):
    // Port check
    if pattern.input_port != sequence[start_pos].port:
        return false
    
    // Context similarity
    context_sim = cosine_similarity(pattern.context_vector, system.context_vector)
    if context_sim < 0.3:
        return false
    
    // Byte-by-byte match (blank nodes match anything)
    for each position:
        if pattern.node_ids[i] == BLANK_NODE:
            continue  // Blank matches anything
        if pattern.node_ids[i] != sequence[start_pos + i]:
            return false
    
    return true
```

### Pattern Forward Pass (Neural Net)
**Location**: `pattern_forward_pass()` (lines 1047-1148)

```c
float pattern_forward_pass(pattern, input_nodes):
    weighted_sum = pattern.bias
    for each input_node:
        weighted_sum += node_activation * pattern.input_weights[i]
    
    output = sigmoid(weighted_sum)
    return output
```

**Key**: Patterns are micro neural networks with weights and bias.

### Pattern Hierarchy and Meaning

**Location**: Pattern hierarchy tracking (lines 173-175, 1224-1250)

```c
// When pattern generalizes:
generalized_pattern.chain_depth = original_pattern.chain_depth
generalized_pattern.accumulated_meaning = original_pattern.accumulated_meaning * 1.2

// When pattern becomes child:
child_pattern.parent_pattern_id = parent_pattern.id
child_pattern.chain_depth = parent_pattern.chain_depth + 1
child_pattern.accumulated_meaning = parent_pattern.accumulated_meaning + new_meaning
```

**Key**: Meaning accumulates through hierarchy - abstract patterns have more meaning.

---

## Output Selection

### Three-Tier Selection Strategy

1. **Pattern-Guided** (Intelligent)
   - Uses pattern predictions when patterns are confident
   - Follows learned sequences
   - Location: `select_output_node()` lines 3835-3907

2. **Greedy Edge Following** (Simple)
   - Follows strongest edge from last output
   - Prioritizes input sequence when output is empty
   - Location: `select_output_node()` lines 3908-4000

3. **Relevance-Based** (Fallback)
   - Computes relevance score for all nodes
   - Selects highest relevance
   - Location: `select_output_node()` lines 4001-4014

### Loop Suppression

**Location**: Multiple places (lines 3796-3800, 3881-3895, 4351-4356)

```c
// Check if node continues loop
if node == output_buffer[length - 2]:
    score *= 0.1  // Strong penalty

if node == output_buffer[length - 3]:
    score *= 0.2  // Moderate penalty

// System-wide loop pressure
if loop_pressure > 0.3:
    if node continues_loop:
        score *= (1.0 - loop_pressure)
```

**Key**: Multiple mechanisms prevent repetitive outputs.

---

## Self-Regulation

### Problem Detection

**Location**: `compute_system_state()` (lines 505-632)

```c
// Error rate (from feedback)
error_rate = incorrect_predictions / total_predictions

// Loop pressure (from repetition detection)
loop_pressure = detect_repetition(output_history)

// Metabolic pressure (from graph density)
metabolic_pressure = (edge_density + pattern_density) / 2

// Pattern confidence (from pattern utility)
pattern_confidence = avg_pattern_utility
```

### Automatic Fixes

**Location**: Various (self-tuning adjustments)

```c
// High error → be more selective
activation_flow_adjustment = 1.0 + (error_rate * 2.0)

// High loop pressure → break loops aggressively
loop_breaking_strength = loop_pressure * 10.0

// High metabolic pressure → prune weak edges
if metabolic_pressure > 0.5:
    prune_weak_edges(node)
```

**Key**: System fixes problems automatically, no manual tuning.

---

## Complete Data Flow

### Episode Execution

**Location**: `run_episode()` (lines 4811-5040)

```
1. Clear buffers
   └─> g->input_length = 0
   └─> g->output_length = 0

2. Inject input
   └─> inject_input(g, input, input_len)
       └─> Create nodes for each byte
       └─> Set node activation = 0.8

3. Generalization
   └─> connect_to_similar_patterns(g, input_buffer, input_length)
       └─> Find patterns with blank nodes that match
       └─> Create edges to pattern predictions

4. Compute system state
   └─> compute_system_state(g)
       └─> Calculate averages, rates, pressures

5. Wave propagation loop (N steps)
   ├─> compute_system_state(g)  [periodically]
   ├─> propagate_activation(g)
   │   ├─> propagate_pattern_activation(g)
   │   │   └─> Patterns match and predict
   │   └─> Edge-based propagation
   │       └─> Activation flows through edges
   ├─> select_output_node(g)
   │   ├─> Pattern-guided selection
   │   ├─> Greedy edge following
   │   └─> Relevance-based fallback
   └─> emit_output(g, selected_node)
       └─> Add to output buffer

6. Learning (if target provided)
   ├─> detect_patterns(g)
   ├─> learn_from_sequence(input)
   ├─> learn_from_sequence(output)
   ├─> learn_from_sequence(target)
   ├─> detect_generalized_patterns(g)
   ├─> actively_generalize_patterns(g)
   ├─> explore_pattern_connections(g)
   ├─> learn_pattern_predictions(g, target)
   └─> apply_feedback(g, target)
       └─> Strengthen correct paths
       └─> Weaken incorrect paths
```

### Activation Flow

```
Input Nodes (activation = 0.8)
    ↓
Pattern Matching
    ├─> Pattern "_at" matches
    │   └─> Predicts: end_of_word
    └─> Pattern "the" matches
        └─> Predicts: "_at"
    ↓
Edge Propagation
    ├─> c → a (weight = 0.9)
    ├─> a → t (weight = 0.9)
    └─> t → [pattern prediction]
    ↓
Node Activation
    ├─> 'c': activation = 0.8
    ├─> 'a': activation = 0.6 (from c→a)
    └─> 't': activation = 0.5 (from a→t)
    ↓
Output Selection
    ├─> Pattern predicts: 't'
    ├─> Edge suggests: 't'
    └─> Relevance: 't' = 0.8
    ↓
Output: 't'
```

---

## Key Insights

### 1. Relative, Not Absolute
- All thresholds are relative to system averages
- Edge weights are relative to source node's other edges
- Pattern strength is relative to other patterns

### 2. Self-Adjusting
- Learning rate adjusts based on usage and exploration pressure
- Activation flow adjusts based on error rate
- Loop breaking adjusts based on loop pressure

### 3. Multi-Layer Intelligence
- **Node Layer**: Byte-level primitives
- **Edge Layer**: Learned associations
- **Pattern Layer**: Sequence chunks and concepts

### 4. Pattern Hierarchies
- Patterns form parent-child relationships
- Meaning accumulates through hierarchy
- Abstract patterns (with blank nodes) have more meaning

### 5. Wave Propagation
- Multi-step activation flow (not single-pass)
- Path quality determines information flow
- Patterns guide activation to correct nodes

### 6. Generalization via Blank Nodes
- Blank nodes match any byte
- Pattern "_at" matches "cat", "bat", "rat"
- Enables generalization to unseen sequences

---

## Summary

Melvin O7 is a self-regulating neural graph system that:
- Learns sequences and patterns dynamically
- Forms hierarchical pattern structures
- Uses wave propagation for multi-step reasoning
- Self-adjusts based on performance
- Generalizes via blank nodes
- Requires no manual tuning

All calculations are relative to system state, creating stable attractors and emergent intelligence.

---

## File Locations Reference

- **System State**: `compute_system_state()` - lines 505-632
- **Wave Propagation**: `propagate_activation()` - lines 2289-2714
- **Pattern Activation**: `propagate_pattern_activation()` - lines 1156-1573
- **Pattern Matching**: `pattern_matches()` - lines 977-1017
- **Pattern Forward Pass**: `pattern_forward_pass()` - lines 1047-1148
- **Node Relevance**: `compute_node_relevance()` - lines 3689-3817
- **Output Selection**: `select_output_node()` - lines 3819-4014
- **Pattern Detection**: `detect_patterns()` - lines 3282-3578
- **Generalization**: `detect_generalized_patterns()` - lines 1847-1986
- **Edge Creation**: `create_or_strengthen_edge()` - lines 808-918
- **Feedback**: `apply_feedback()` - lines 4518-4633
- **Episode Execution**: `run_episode()` - lines 4811-5040

---

*Document generated from melvin.c analysis*
*Total lines: 6078*
*Last updated: Based on current codebase*

