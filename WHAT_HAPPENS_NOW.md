# What Happens When We Feed Data Now

## Complete Flow (After Fixes)

### Training Example: "cat" → "cats"

#### 1. **Input Injection** (line 4159)
```c
inject_input(g, "cat", 3);
```
- Creates nodes: 'c'(99), 'a'(97), 't'(116)
- Sets activation: `node.activation = 0.5 + 0.5 * exploration_pressure` (~0.5 to 1.0)
- Creates sequential edges: 'c'→'a', 'a'→'t'
- Boosts to 0.8: `g->nodes[input_byte].activation = 0.8f` (line 4171)

**Result:** Input nodes have strong activation (0.8), sequential edges exist with initial weight ~0.1

---

#### 2. **Wave Propagation** (20-200 steps, line 4195-4227)

Each step:

**A. Compute System Statistics** (lines 2119-2248)
- Samples 50 nodes to compute:
  - `avg_input_connectivity` (how many edges from inputs)
  - `avg_context_match` (pattern support rates)
  - `avg_pattern_meaning` (pattern abstraction levels)
  - `avg_path_importance` (edge usage/success)

**B. For Each Active Node** (line 2256+):

**Step 1: Calculate Path Quality for Each Outgoing Edge**

For edge from 'a' → 't':

```c
// FACTOR 1: Information (edge weight from input)
float input_connection = 0.0f;
// Check: Is there edge from input 'c' to target 't'?
// NO → input_connection stays 0
// Check: Is there edge from input 'a' to target 't'?  
// YES → edge_weight = 0.1, usage_boost = log(1+1)/5 = 0.14
input_connection = 0.1 * (1 + 0.14) = 0.114
```

```c
// FACTOR 2: Learning (training signal)
float learning = edge->weight *           // 0.1 (new edge)
                (1.0 + success_rate * 2) * // 1.0 (no training yet)
                (1.0 + usage_boost);        // 1.14 (used once)
learning = 0.1 * 1.0 * 1.14 = 0.114
```

```c
// FACTOR 3: Coherence (pattern + history)
// No patterns match yet (first episode)
// No history yet (first output)
coherence = (0 + 0 + 0) / 3 = 0.0
```

```c
// FACTOR 4: Predictive (pattern predictions)
// No patterns yet
predictive = 0.0 * 1.0 * 1.0 * 0.5 * 0.0 = 0.0
```

```c
// PATH QUALITY (weighted sum)
base_quality = (0.114 * 0.4 +    // learning (training)
               0.114 * 0.3 +      // information
               0.0 * 0.2 +        // predictive
               0.0 * 0.1)         // coherence
             = 0.046 + 0.034 = 0.08
```

**Step 2: Transfer Activation**
```c
normalized_quality = 0.08 * soft_norm;  // Normalize across all edges
transfer = node['a'].activation * normalized_quality;
         = 0.8 * (0.08 / total_quality)
         
node['t'].activation += transfer;  // Accumulates
```

**Step 3: Decay Source**
```c
node['a'].activation *= 0.9;  // 90% retention
```

**Result After ~10 Steps:** 
- Activation spreads: 'c' → 'a' → 't' → ...
- Nodes near input have higher activation
- Random nodes have near-zero activation

---

#### 3. **Output Selection** (each step, line 4205)

```c
output_node = select_output_node(g);
```

**For each node, compute score:**
```c
// Same factors as path quality, but from node perspective
float information = input_connection * context_match * history_coherence;
float learning = node_activation * (1 + usage);
float coherence = pattern_alignment * sequential_flow * context_fit;
float predictive = pattern_prediction * historical_accuracy * context_prediction;

// Combine with relative measures
float score = node_activation * relative_info * relative_usage * 
             relative_coherence * relative_predictive;
```

**Winner:** Node with highest score

**First Episode Output (Untrained):**
- 'c', 'a', 't' all have activation (~0.5-0.8)
- Their neighbors also have some activation
- Random exploration → might output "cat", "caa", "ctt", etc.

**Output:** `ctc` (random/weak edges win initially)

---

#### 4. **Learning/Feedback** (line 4231-4258)

**A. Pattern Detection** (line 4234)
```c
detect_patterns(g);
```
- Finds bigrams in input: "ca", "at"  
- Finds bigrams in target: "ca", "at", "ts"
- Creates Pattern objects with `strength = 0.1`, `activation = 0.1`

**B. Learn Pattern Predictions** (line 4248)
```c
learn_pattern_predictions(g, "cats", 4);
```
For each pattern that matches:
- Pattern "ca" matches target[0:2] → predict target[2] = 't'
- Pattern "at" matches target[1:3] → predict target[3] = 's'
- Store: `pattern.predicted_nodes = ['t']`, `pattern.predicted_nodes = ['s']`

**C. Apply Feedback** (line 4251)
```c
apply_feedback(g, "cats", 4);
```

For each position:
```c
// Position 0: output='c', target='c' → CORRECT
if (output[0] == target[0]) {
    // Strengthen: input['c'] → 'c'
    edge->weight += learning_rate * (1.0 - edge->weight);
    edge->success_count++;
    // Edge weight: 0.1 → 0.2 (or higher)
}

// Position 1: output='t', target='a' → WRONG  
if (output[1] != target[1]) {
    // Weaken wrong edge: input → 't'
    edge->weight *= 0.95;  // Slight decay
    
    // Strengthen correct edge: input → 'a'
    create_or_strengthen_edge(g, prev_correct, target[1]);
    // Edge weight grows toward 1.0
}
```

**Result:**
- Correct edges: weight increases (0.1 → 0.3 → 0.5 → ...)
- Wrong edges: weight decreases or stays low
- `success_count` tracks correct predictions

---

#### 5. **After 30 Training Episodes**

**Edge Weights:**
```
'c' → 'a': weight=0.9, success_count=30, use_count=30
'a' → 't': weight=0.9, success_count=30, use_count=30
't' → 's': weight=0.9, success_count=30, use_count=30
(random edges): weight=0.1, success_count=0, use_count=0-5
```

**Path Quality Calculation (After Training):**
```c
// For edge 'a' → 't' (trained path)
learning = 0.9 *               // High weight (trained)
          (1 + 1.0 * 2) *      // Success rate 100% = 3x boost!
          (1 + log(31)/5) =    // usage_boost = 0.69
         = 0.9 * 3.0 * 1.69 = 4.56  // HIGH!

// For edge 'a' → 'x' (random path)
learning = 0.1 *               // Low weight (untrained)
          (1 + 0.0 * 2) *      // No success = 1x
          (1 + log(1)/5) =     // No usage = 1.0
         = 0.1 * 1.0 * 1.0 = 0.1  // LOW!

// Ratio: 4.56 / 0.1 = 45x difference!
```

**Output Generation:**
```
Input: "cat"
Wave propagation:
  'c'(0.8) → 'a'(quality=4.5) → 't'(quality=4.5) → 's'(quality=4.5)
         ↘ 'x'(quality=0.1)

Selection:
  'a' score: 0.6 * 4.5 (learning) * ... = HIGH
  'x' score: 0.1 * 0.1 (learning) * ... = TINY
  
Output: "cats" ✓
```

---

## Key Differences (Before vs After Fix)

### Before Fix:
```
Information: binary (0 or 1) / 0.01 = 0 or 100 (explosion)
Learning: 0.1 * 1.5 * 1.0 = 0.15 (weak, no training signal)
Path quality: geometric_mean(0, 0.15, 0, 0) = 0 (collapse)
Result: All paths quality ≈ 0, random selection
```

### After Fix:
```
Information: edge_weight * usage = 0.114 (continuous)
Learning: 0.9 * 3.0 * 1.69 = 4.56 (STRONG for trained paths!)
Path quality: weighted_sum = 0.08 (untrained) vs 2.5 (trained)
Result: Trained paths 30x better, clear winner
```

---

## Expected Behavior Now:

### Episode 1 (Untrained):
- Input: "cat"
- Path qualities: all ~0.08-0.15 (weak, similar)
- Output: Random, maybe "ctc", "caa", "cat"
- **No clear winner yet**

### Episode 10 (Partial Training):
- Correct edges: weight=0.5, success=10/15
- Learning factor: 0.5 * 1.67 * 1.3 ≈ 1.1
- Wrong edges: weight=0.1, success=0/5  
- Learning factor: 0.1 * 1.0 * 1.0 = 0.1
- **Ratio: 11x, starting to prefer correct paths**

### Episode 30 (Well-Trained):
- Correct edges: weight=0.9, success=30/30
- Learning factor: 0.9 * 3.0 * 1.69 ≈ 4.6
- Wrong edges: weight=0.1, success=0
- Learning factor: 0.1
- **Ratio: 46x, correct path dominates!**

### Generalization ("bat" → ?):
- Edges: 'b' → 'a', 'a' → 't', 't' → 's'
- 'a' → 't' edge: trained (weight=0.9, success=30)
- 't' → 's' edge: trained (weight=0.9, success=30)
- Path quality: HIGH for "at" → "ats"
- **Output: "bats" (generalizes!)**

---

## Why It Should Work Now:

1. ✅ **Learning signal visible**: `success_rate * 2.0` gives 3x boost to trained paths
2. ✅ **Continuous gradients**: Edge weights [0,1], not binary
3. ✅ **Proper selection**: 30-50x difference between trained/untrained
4. ✅ **Generalization**: Shared edges ("a"→"t") transfer learning
5. ✅ **Stable values**: No explosions, no collapse

