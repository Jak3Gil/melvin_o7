# NODE ACTIVATION CALCULATION

## Complete Flow: How Activation is Calculated Per Node

### 1. INITIAL ACTIVATION (Input Injection)
**Location**: `run_episode()` → Input processing

```c
// For each input byte:
g->nodes[node_id].activation = 0.8f;  // Strong initial activation
```

**What happens**:
- Input nodes get `0.8f` activation directly
- This is the starting point - activation begins at input nodes

---

### 2. PATTERN PREDICTIONS (Pattern-Guided Boost)
**Location**: `propagate_pattern_activation()` → Pattern predictions

```c
// When pattern matches and predicts a node:
float transfer = pat->activation * weight * pat->strength;
float intelligent_path_boost = 3.0f;  // Strong boost for pattern predictions
transfer *= intelligent_path_boost;
g->nodes[target_node].activation += transfer;
```

**What happens**:
- Active patterns predict nodes they've learned
- Transfer = `pattern_activation × prediction_weight × pattern_strength × 3.0`
- This is the **primary source of intelligent activation** - patterns guide activation to correct nodes

**Example**:
- Pattern "hello" is active with `activation = 0.6`
- Pattern predicts "world" with `weight = 0.8`, `strength = 0.9`
- Transfer = `0.6 × 0.8 × 0.9 × 3.0 = 1.296`
- Node "world" gets `activation += 1.296`

---

### 3. EDGE-BASED PROPAGATION (Wave Propagation)
**Location**: `propagate_activation()` → Edge propagation

**Step 3a: Calculate Path Quality for Each Edge**

For each edge from node `i` to `target`:

```c
// FACTOR 1: Information_Carried
float information = input_connection * context_match * history_coherence;

// FACTOR 2: Learning_Strength
float learning = edge_weight * (1.0 + usage) * (0.5 + success_rate);

// FACTOR 3: Coherence
float coherence = pattern_alignment * sequential_flow * context_fit;

// FACTOR 4: Predictive_Power (with meaning and hierarchy boosts)
float pattern_prediction = pat->activation * pat->strength;
float pattern_meaning_boost = 1.0 + (pat->accumulated_meaning * 2.0);
float hierarchy_boost = 1.0 + (pat->chain_depth * 0.3);
float predictive = pattern_prediction * pattern_meaning_boost * hierarchy_boost * 
                   (0.5 + historical_accuracy) * context_prediction;

// Path Quality
float base_quality = information * learning * coherence * predictive;
float path_importance = (usage_importance + success_rate + edge_weight) / 3.0;
float pattern_connection_boost = 1.0 + (accumulated_meaning * 3.0) + (dynamic_importance * 2.0);
float path_quality = base_quality * (0.5 + path_importance) * pattern_connection_boost * 
                     quality_adjustment / activation_flow_adjustment;
```

**Step 3b: Normalize and Transfer**

```c
// Normalize path qualities
float soft_normalization = 1.0 / total_path_quality;
if (soft_normalization > 100.0f) soft_normalization = 100.0f;

// Transfer activation
float normalized_quality = path_quality * soft_normalization;
float transfer = source_node.activation * normalized_quality;
if (transfer > 10.0f) transfer = 10.0f;  // Cap transfer

g->nodes[target].activation += transfer;
if (g->nodes[target].activation > 100.0f) g->nodes[target].activation = 100.0f;  // Cap node activation
```

**What happens**:
- Activation flows from source node to target node through edges
- Transfer amount depends on **path quality** (how meaningful/intelligent the path is)
- Path quality considers:
  - **Information**: Is target reachable from input? Does it match context?
  - **Learning**: How well-learned is this edge? (weight, usage, success)
  - **Coherence**: Does it form a coherent sequence? (pattern alignment, sequential flow)
  - **Predictive Power**: Do patterns predict this? (with meaning and hierarchy boosts)
  - **Pattern Connections**: Is edge part of active patterns? (rich connections boost)

**Example**:
- Node "h" has `activation = 0.8` (from input)
- Edge "h" → "e" has:
  - `edge_weight = 0.9` (well-learned)
  - `path_quality = 0.85` (high quality path)
  - `normalized_quality = 0.85 / 2.5 = 0.34` (normalized)
- Transfer = `0.8 × 0.34 = 0.272`
- Node "e" gets `activation += 0.272`

---

### 4. PATTERN HIERARCHY & MEANING (Meaning Accumulation)
**Location**: `propagate_pattern_activation()` → Pattern-to-pattern predictions

```c
// When pattern A predicts pattern B:
float chain_meaning = parent_meaning * pattern_pred_weight * pat->strength;
target_pat->accumulated_meaning = max(target_pat->accumulated_meaning, chain_meaning);

// Meaning multiplier boosts activation
float meaning_multiplier = 1.0 + (target_pat->accumulated_meaning * 0.5);
float pattern_transfer = pat->activation * pattern_pred_weight * pat->strength * meaning_multiplier;
target_pat->activation += pattern_transfer;
```

**What happens**:
- Patterns accumulate meaning through hierarchy
- Patterns with more meaning boost their predictions more
- This creates **intelligent activation** - patterns with meaning guide activation better

---

### 5. DECAY (Signal Weakens Over Time)
**Location**: `propagate_activation()` → After propagation

```c
// After propagation, nodes decay slightly
g->nodes[i].activation *= 0.9f;  // 10% decay per step
```

**What happens**:
- Activation decays by 10% each propagation step
- This prevents activation from accumulating indefinitely
- Fresh activation (from input/patterns) is stronger than old activation

---

## Complete Formula Per Node

For a node `n` at step `t`:

```
activation[n][t] = 
    // 1. Previous activation (decayed)
    (activation[n][t-1] * 0.9)
    
    // 2. Pattern predictions (if any patterns predict this node)
    + SUM over all active patterns P:
        IF P predicts n:
            P.activation * P.prediction_weight[n] * P.strength * 3.0
    
    // 3. Edge propagation (from all source nodes)
    + SUM over all source nodes S with edges S → n:
        S.activation * path_quality(S → n) * normalization_factor
    
    // 4. Pattern meaning boosts (if node is predicted by meaningful patterns)
    * (1.0 + meaning_boost) * (1.0 + hierarchy_boost)
```

Where:
- `path_quality(S → n)` = `Information × Learning × Coherence × Predictive × Pattern_Connection_Boost`
- `meaning_boost` = `accumulated_meaning × 2.0` (if pattern has meaning)
- `hierarchy_boost` = `chain_depth × 0.3` (if pattern is deep in hierarchy)

---

## Key Insights

1. **Activation is NOT just edge weights** - it's a complex calculation considering:
   - Pattern predictions (primary intelligent source)
   - Path quality (meaning, learning, coherence, predictive power)
   - Pattern hierarchy and meaning
   - Context (input, history, patterns)

2. **Patterns are the intelligence** - they guide activation to correct nodes through:
   - Direct predictions (pattern → node)
   - Meaning accumulation (patterns with meaning boost more)
   - Hierarchy (deeper patterns = more abstract = more meaningful)

3. **Activation flows through meaningful paths** - not just any path, but paths that:
   - Are well-learned (high weight, usage, success)
   - Form coherent sequences (pattern alignment)
   - Are predicted by patterns (predictive power)
   - Are part of active patterns (rich connections)

4. **Self-regulation** - activation adjusts based on:
   - Error rate (high error = more selective)
   - Pattern success (successful patterns boost more)
   - Meaning (patterns with meaning guide better)

---

## Why This Creates Intelligence

Activation flows through **meaningful paths** determined by:
- **Patterns** (learned intelligence)
- **Hierarchy** (abstract concepts)
- **Meaning** (accumulated understanding)
- **Context** (input, history, coherence)

The node with highest activation is not random - it's the node at the end of the **most meaningful path** through the learned structure.
