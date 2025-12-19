# Wave Propagation as Intelligence: Refactor Proposal

## Current Design (Activation as Follower)

```
Input → Inject activation → Wave prop transfers activation → Select max activation
```

**Problems:**
- Activation is "dumb accumulation" - just the brightest light
- Wave propagation follows activation (passive)
- Selection is separate step (just finds max)
- Activation stored in nodes (stateful, accumulates)

## Proposed Design (Wave Propagation as Intelligence)

```
Input → Wave prop computes relevance → Wave prop selects best node → Return
```

**Benefits:**
- Wave propagation IS the intelligence (active decision maker)
- No separate activation storage (compute relevance on-the-fly)
- Selection happens during propagation (not separate step)
- Cleaner: one function does everything

---

## Architecture Change

### Current:
```c
void propagate_activation(MelvinGraph *g) {
    // Transfers activation through edges
    // Activation accumulates in nodes
}

uint32_t select_output_node(MelvinGraph *g) {
    // Finds node with highest activation
    return max_activation_node;
}
```

### Proposed:
```c
uint32_t propagate_and_select(MelvinGraph *g) {
    // Wave propagation directly computes relevance for each node
    // Returns the node with highest relevance
    // No activation storage - relevance computed on-the-fly
}
```

---

## How It Works

### Step 1: Pattern Predictions (Compute Relevance)
```c
// For each pattern that matches context:
float pattern_relevance = 
    pattern_strength × 
    pattern_activation × 
    prediction_weight × 
    meaning_boost × 
    hierarchy_boost;

// Add to target node's relevance (not activation!)
node_relevance[target] += pattern_relevance;
```

### Step 2: Edge Propagation (Compute Relevance)
```c
// For each edge from source to target:
float path_quality = 
    edge_weight × 
    pattern_boost × 
    context_support × 
    input_boost × 
    success_rate;

// Compute relevance transfer (not activation!)
float source_relevance = compute_node_relevance(source);  // On-the-fly
float relevance_transfer = source_relevance × path_quality × normalization;

// Add to target's relevance
node_relevance[target] += relevance_transfer;
```

### Step 3: Selection (During Propagation)
```c
// Track best node as we compute relevance
uint32_t best_node = BYTE_VALUES;
float best_relevance = 0.0f;

for each node {
    float relevance = compute_node_relevance(node);  // On-the-fly calculation
    
    if (relevance > best_relevance) {
        best_relevance = relevance;
        best_node = node;
    }
}

return best_node;  // Wave prop directly returns decision
```

---

## Key Changes

### 1. Remove Activation Storage
**Before:**
```c
typedef struct {
    float activation;  // Stored state
} Node;
```

**After:**
```c
typedef struct {
    // No activation field!
    // Relevance computed on-the-fly during wave propagation
} Node;
```

### 2. Relevance Computation (On-the-Fly)
```c
float compute_node_relevance(MelvinGraph *g, uint32_t node_id) {
    float relevance = 0.0f;
    
    // 1. Pattern contributions (if patterns predict this node)
    for each pattern that predicts node_id {
        relevance += pattern_strength × pattern_activation × pred_weight;
    }
    
    // 2. Edge contributions (if edges connect to this node)
    for each edge to node_id {
        float source_relevance = compute_node_relevance(g, source);  // Recursive!
        relevance += source_relevance × path_quality;
    }
    
    // 3. Input boost (if node is in input)
    if (node_in_input(node_id)) {
        relevance *= 10.0f;  // Input boost
    }
    
    // 4. Context boost (if node follows from output)
    if (node_follows_output(node_id)) {
        relevance *= 1.5f;  // Context boost
    }
    
    return relevance;
}
```

### 3. Wave Propagation Returns Selection
```c
uint32_t propagate_and_select(MelvinGraph *g) {
    // Phase 1: Pattern predictions
    for each pattern {
        if (pattern_matches_context(pattern)) {
            for each predicted_node {
                // Pattern contributes relevance (not activation!)
                float pattern_relevance = compute_pattern_relevance(pattern, predicted_node);
                // No storage - just track for selection
            }
        }
    }
    
    // Phase 2: Edge propagation
    for each active_node {
        for each outgoing_edge {
            float source_relevance = compute_node_relevance(g, source);  // On-the-fly
            float path_quality = compute_path_quality(edge);
            float target_relevance = source_relevance × path_quality;
            // No storage - just track for selection
        }
    }
    
    // Phase 3: Selection (during computation)
    uint32_t best_node = BYTE_VALUES;
    float best_relevance = 0.0f;
    
    for each node {
        float relevance = compute_node_relevance(g, node);  // Compute on-the-fly
        if (relevance > best_relevance) {
            best_relevance = relevance;
            best_node = node;
        }
    }
    
    return best_node;  // Wave prop IS the decision
}
```

---

## Benefits

### 1. Cleaner Architecture
- One function does everything: `propagate_and_select()`
- No separate activation state
- No separate selection step

### 2. Wave Propagation IS Intelligence
- Wave propagation actively computes relevance
- Wave propagation actively selects best node
- Not a follower - it's the decision maker

### 3. No State Accumulation
- Relevance computed on-the-fly (no storage)
- No activation decay needed
- No activation normalization needed
- Simpler: compute → select → done

### 4. More Efficient
- No need to store activation in all nodes
- Compute only what's needed for selection
- Can early-exit if clear winner found

---

## Implementation Strategy

### Phase 1: Add Relevance Computation
1. Create `compute_node_relevance()` function
2. Compute relevance on-the-fly (no storage)
3. Test that relevance matches current activation

### Phase 2: Integrate Selection
1. Modify `propagate_activation()` to return `uint32_t` (selected node)
2. Selection happens during propagation
3. Remove separate `select_output_node()` call

### Phase 3: Remove Activation Storage
1. Remove `activation` field from Node struct
2. Remove all `activation +=` operations
3. Remove activation decay/normalization

### Phase 4: Cleanup
1. Remove `select_output_node()` function
2. Update `run_episode()` to use `propagate_and_select()`
3. Remove activation-related system state

---

## Example: "cat" → "cats"

### Current (Activation-Based):
```
Step 1: Inject activation
  → 'c', 'a', 't' activation = 1.0

Step 2: Wave propagation
  → Pattern "at" predicts 's'
  → 's' activation += 0.6
  → Edge t→s propagates
  → 's' activation += 0.4
  → 's' activation = 1.0

Step 3: Selection
  → Find max activation
  → 's' wins (activation = 1.0)
```

### Proposed (Relevance-Based):
```
Step 1: Wave propagation computes relevance
  → Pattern "at" matches, predicts 's'
  → 's' relevance = pattern_strength × pattern_act × pred_weight = 0.6
  
  → Edge t→s exists
  → t relevance = compute_node_relevance('t') = 0.8
  → path_quality = edge_weight × context × input_boost = 0.5
  → 's' relevance += 0.8 × 0.5 = 0.4
  → 's' relevance = 1.0

Step 2: Selection (during propagation)
  → Track best: 's' relevance = 1.0 (highest)
  → Return 's'
```

**Result:** Same outcome, but wave propagation IS the intelligence, not a follower.

---

## Key Insight

**Current:** Activation is dumb accumulation, wave prop follows it, selection finds max.

**Proposed:** Wave propagation directly computes relevance, wave propagation directly selects best, no separate steps.

**Wave propagation becomes the intelligence, not the follower.**
