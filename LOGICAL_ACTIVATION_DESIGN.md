# Logical Activation: Pattern-Driven Intelligence

## The Core Problem

**Current:** System picks "highest activation" (dumb accumulation)

**Needed:** System picks "most logical activation" (pattern-driven, context-appropriate)

**Key Insight:** Logical ≠ Highest. Logical = Pattern-driven + Context-appropriate + Coherent.

---

## What is "Logical Activation"?

### Logical Activation = Pattern-Driven Relevance

```c
logical_relevance = 
    pattern_driven_score ×      // Patterns say this makes sense
    context_appropriateness ×   // Fits current context
    coherence_score ×           // Follows from what came before
    generalization_score        // Blank nodes test hypotheses
```

### Highest Activation = Raw Accumulation

```c
highest_activation = 
    SUM(all_transfers)          // Just accumulation
    // No pattern logic
    // No context check
    // No coherence validation
```

**Problem:** Highest activation might be illogical (random accumulation, wrong context, incoherent).

**Solution:** Use logical relevance, not raw activation.

---

## Architecture: Everything Relative, Pattern-Driven

### Principle 1: Everything is Relative (Right Now, For Me Right Now)

**NOT:** "What's the weight of this edge over all time?"

**YES:** "What is this edge worth RIGHT NOW in THIS context?"

```c
// BEFORE: Historical average
float edge_value = edge.weight;  // Fixed, historical

// AFTER: Context-relative, right now
float edge_value = compute_edge_relevance_right_now(g, edge, current_context);
```

### Principle 2: Patterns Make Decisions

**NOT:** "Highest activation wins"

**YES:** "Most logical pattern-driven path wins"

```c
// BEFORE: Highest activation
if (node.activation > best_activation) {
    selected = node;
}

// AFTER: Most logical (pattern-driven)
float logical_score = compute_logical_relevance(g, node, current_context);
if (logical_score > best_logical_score) {
    selected = node;
}
```

### Principle 3: Blank Nodes Test Hypotheses

**NOT:** "Blank nodes match anything"

**YES:** "Blank nodes test hypotheses - fill them and check if logical"

```c
// For each blank node in pattern:
// 1. Try filling with candidate bytes
// 2. Check if filled pattern would be logical
// 3. If logical, that candidate gets relevance boost

for each candidate_byte {
    // Fill blank with candidate
    filled_pattern = pattern_with_blank_filled(candidate_byte);
    
    // Check if this would be logical
    float logical_score = compute_logical_relevance(g, filled_pattern, context);
    
    if (logical_score > threshold) {
        // This candidate is logical - boost its relevance
        candidate_relevance += logical_score;
    }
}
```

---

## Logical Relevance Computation

### Step 1: Pattern-Driven Score

```c
float pattern_driven_score(MelvinGraph *g, uint32_t node_id) {
    float score = 0.0f;
    
    // For each pattern that predicts this node:
    for each pattern that predicts node_id {
        // Pattern says this node makes sense
        float pattern_confidence = 
            pattern.strength × 
            pattern.activation × 
            pattern.prediction_weight[node_id];
        
        // Meaning boost: patterns with meaning are more logical
        float meaning_boost = 1.0f + (pattern.accumulated_meaning × 2.0f);
        
        // Hierarchy boost: deeper patterns = more abstract = more logical
        float hierarchy_boost = 1.0f + (pattern.chain_depth × 0.3f);
        
        score += pattern_confidence × meaning_boost × hierarchy_boost;
    }
    
    return score;
}
```

### Step 2: Context Appropriateness

```c
float context_appropriateness(MelvinGraph *g, uint32_t node_id) {
    float appropriateness = 0.0f;
    
    // 1. Input context: Is node reachable from input?
    if (node_reachable_from_input(g, node_id)) {
        appropriateness += 1.0f;  // Strong context
    }
    
    // 2. Output context: Does node follow from last output?
    if (g->output_length > 0) {
        uint32_t last_output = g->output_buffer[g->output_length - 1];
        if (edge_exists(g, last_output, node_id)) {
            appropriateness += 1.5f;  // Stronger - follows sequence
        }
    }
    
    // 3. Pattern context: Do active patterns support this?
    for each active_pattern {
        if (pattern_supports_node(active_pattern, node_id)) {
            appropriateness += pattern.strength × 0.5f;
        }
    }
    
    return appropriateness;
}
```

### Step 3: Coherence Score

```c
float coherence_score(MelvinGraph *g, uint32_t node_id) {
    float coherence = 0.0f;
    
    // 1. Sequential coherence: Does node continue a sequence?
    if (g->output_length > 0) {
        uint32_t last = g->output_buffer[g->output_length - 1];
        
        // Check if last→node forms a learned sequence
        Edge *edge = find_edge(g, last, node_id);
        if (edge && edge->success_rate > 0.7f) {
            coherence += edge->success_rate;  // High success = coherent
        }
    }
    
    // 2. Pattern coherence: Does node complete a pattern?
    for each active_pattern {
        if (pattern_would_complete_with(pattern, node_id)) {
            coherence += pattern.strength × 0.5f;
        }
    }
    
    // 3. Input coherence: Does node continue input sequence?
    if (g->input_length > g->output_length) {
        uint32_t next_input = g->input_buffer[g->output_length];
        if (next_input == node_id) {
            coherence += 1.0f;  // Matches input sequence
        }
    }
    
    return coherence;
}
```

### Step 4: Generalization Score (Blank Node Hypothesis Testing)

```c
float generalization_score(MelvinGraph *g, uint32_t node_id) {
    float gen_score = 0.0f;
    
    // For each pattern with blank nodes that could match:
    for each pattern with blank_nodes {
        // Try filling blank with node_id
        bool would_match = pattern_would_match_if_filled(pattern, node_id, current_context);
        
        if (would_match) {
            // This is a hypothesis: "if blank = node_id, would pattern be logical?"
            // Test the hypothesis: compute logical relevance of filled pattern
            float filled_pattern_relevance = compute_logical_relevance_for_filled_pattern(
                g, pattern, node_id, current_context
            );
            
            // If filled pattern is logical, this node is a good generalization
            if (filled_pattern_relevance > logical_threshold) {
                gen_score += filled_pattern_relevance × pattern.strength;
            }
        }
    }
    
    return gen_score;
}
```

### Step 5: Combined Logical Relevance

```c
float compute_logical_relevance(MelvinGraph *g, uint32_t node_id) {
    // Everything relative to current context, not historical averages
    
    float pattern_score = pattern_driven_score(g, node_id);
    float context_score = context_appropriateness(g, node_id);
    float coherence = coherence_score(g, node_id);
    float generalization = generalization_score(g, node_id);
    
    // Logical relevance = weighted combination
    // Patterns are primary (they encode logic)
    float logical_relevance = 
        (pattern_score × 0.5f) +      // Patterns are primary
        (context_score × 0.25f) +     // Context matters
        (coherence × 0.15f) +         // Coherence matters
        (generalization × 0.1f);     // Generalization matters
    
    return logical_relevance;
}
```

---

## Blank Node Hypothesis Testing

### How It Works

```c
// Pattern: "_at" (blank + "at")
// Current context: input="bat", output="b"

// Step 1: Pattern matches with blank at position 0
// Step 2: Try filling blank with candidate bytes
// Step 3: Check if filled pattern would be logical

for each candidate_byte in ['a', 'b', 'c', ...] {
    // Fill: candidate + "at"
    filled = [candidate, 'a', 't'];
    
    // Check if this would be logical in current context
    float logical = compute_logical_relevance_for_sequence(g, filled, context);
    
    if (logical > threshold) {
        // Hypothesis confirmed: candidate + "at" is logical
        // Boost candidate's relevance
        candidate_relevance += logical;
    }
}

// Result: 'b' gets high relevance (because "bat" is logical)
// System selects 'b' even if it doesn't have highest raw activation
```

---

## Wave Propagation as Logical Intelligence

### Current (Activation-Based):
```c
void propagate_activation(MelvinGraph *g) {
    // Transfer activation through edges
    // Activation accumulates (dumb)
}

uint32_t select_output_node(MelvinGraph *g) {
    // Find highest activation
    return max_activation_node;
}
```

### Proposed (Logical Relevance-Based):
```c
uint32_t propagate_and_select_logical(MelvinGraph *g) {
    uint32_t best_node = BYTE_VALUES;
    float best_logical_relevance = 0.0f;
    
    // For each candidate node:
    for each node {
        // Compute logical relevance (on-the-fly, relative to current context)
        float logical = compute_logical_relevance(g, node);
        
        // Track best logical choice
        if (logical > best_logical_relevance) {
            best_logical_relevance = logical;
            best_node = node;
        }
    }
    
    // Wave propagation directly returns most logical choice
    return best_node;
}
```

---

## Key Differences

### Historical vs. Relative

**BEFORE:**
```c
float edge_value = edge.weight;  // Historical average
```

**AFTER:**
```c
float edge_value = compute_edge_relevance_right_now(g, edge, context);
// Relative to current context, not historical average
```

### Highest vs. Logical

**BEFORE:**
```c
if (activation > max_activation) {
    selected = node;  // Just highest
}
```

**AFTER:**
```c
float logical = compute_logical_relevance(g, node);
if (logical > best_logical) {
    selected = node;  // Most logical
}
```

### Blank Nodes: Match vs. Hypothesis Testing

**BEFORE:**
```c
if (pattern.node_ids[i] == BLANK_NODE) {
    continue;  // Blank matches anything (passive)
}
```

**AFTER:**
```c
if (pattern.node_ids[i] == BLANK_NODE) {
    // Test hypothesis: try filling blank
    float logical = test_blank_hypothesis(g, pattern, candidate, context);
    if (logical > threshold) {
        candidate_relevance += logical;  // Active hypothesis testing
    }
}
```

---

## Example: "bat" → "bats"

### Current (Highest Activation):
```
Input: "bat"
  → 'b', 'a', 't' activation = 1.0
  → Pattern "_at" matches (blank matches 'b')
  → Pattern predicts 's'
  → 's' activation = 0.6
  → Edge t→s exists
  → 's' activation = 0.6 + 0.4 = 1.0
  → SELECTED: 's' (highest activation)
```

### Proposed (Logical Relevance):
```
Input: "bat"
  → Pattern "_at" matches (blank at position 0)
  
  → Hypothesis testing: Try filling blank
    - Fill with 'b': "bat" → logical_relevance = 0.9 ✓
    - Fill with 'c': "cat" → logical_relevance = 0.8 ✓
    - Fill with 'r': "rat" → logical_relevance = 0.7 ✓
  
  → Pattern "_at" predicts 's' (logical: "_at" → "s")
  → 's' logical_relevance = 
      pattern_driven (0.6) ×
      context_appropriate (1.0) ×
      coherent (0.9) ×
      generalization (0.8)
    = 0.43
  
  → SELECTED: 's' (most logical, not just highest)
```

---

## Summary

**Logical Activation = Pattern-Driven + Context-Appropriate + Coherent + Generalizes**

**Wave Propagation = Computes logical relevance, not raw activation**

**Blank Nodes = Test hypotheses, not just match anything**

**Everything Relative = Right now, for me right now, not historical averages**

**Result:** System makes logical decisions, not just picks highest activation.
