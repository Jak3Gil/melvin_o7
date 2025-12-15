# PATH QUALITY DEFINITION

## What Makes a Path "Better"?

A **better path** is one that:
1. **Carries information from input to output** (information flow)
2. **Represents learned associations** (not random connections)
3. **Has predictive power** (predicts correct outputs)
4. **Forms coherent sequences** (makes sense contextually)

## Well-Defined Path Quality Measure

### Core Principle: **Information Flow Efficiency**

A path is "better" if it efficiently carries information from the current context (input + history) to a meaningful output.

### Path Quality = Information Flow Score

```
Path Quality = Information_Carried × Learning_Strength × Coherence × Predictive_Power
```

Where:

#### 1. Information_Carried
**Definition**: How much information this path carries from input/context to target
- **Input connection**: Is target reachable from input nodes? (yes = 1.0, no = 0.1)
- **Context match**: Does path match current input context? (pattern match = 1.0, no match = 0.5)
- **History coherence**: Does path follow from recent output? (yes = 1.0, no = 0.8)

```
Information_Carried = input_connection × context_match × history_coherence
```

#### 2. Learning_Strength
**Definition**: How well-learned is this path? (not random, but reinforced through training)
- **Edge weight**: Learned strength of connection (0.0 to 1.0)
- **Usage count**: How often this path has been used (log scale: log(1 + use_count))
- **Success rate**: How often this path led to correct outputs (success_count / use_count)

```
Learning_Strength = edge_weight × log(1 + use_count) × success_rate
```

#### 3. Coherence
**Definition**: Does this path form a coherent sequence? (makes sense contextually)
- **Pattern alignment**: Does path follow a learned pattern? (pattern match = 1.0, no = 0.7)
- **Sequential flow**: Does path continue naturally from previous output? (yes = 1.0, no = 0.8)
- **Context fit**: Does path fit the input context? (context match = 1.0, no = 0.6)

```
Coherence = pattern_alignment × sequential_flow × context_fit
```

#### 4. Predictive_Power
**Definition**: How well does this path predict correct outputs?
- **Pattern prediction**: Is target predicted by active patterns? (yes = pattern_confidence, no = 0.3)
- **Historical accuracy**: Has this path been accurate before? (success_rate)
- **Context prediction**: Does path predict what should come next? (context_match)

```
Predictive_Power = pattern_prediction × historical_accuracy × context_prediction
```

## Implementation

### For Wave Propagation (Edge Selection)

```c
float calculate_path_quality(MelvinGraph *g, uint32_t from, uint32_t to, Edge *edge) {
    // 1. Information_Carried
    float input_connection = is_reachable_from_input(g, from, to) ? 1.0f : 0.1f;
    float context_match = pattern_matches_context(g, from, to) ? 1.0f : 0.5f;
    float history_coherence = follows_from_output(g, from, to) ? 1.0f : 0.8f;
    float information = input_connection * context_match * history_coherence;
    
    // 2. Learning_Strength
    float edge_weight = edge->weight;
    float usage = logf(1.0f + edge->use_count) / 10.0f;  // Normalize
    float success_rate = edge->use_count > 0 ? 
        (float)edge->success_count / (float)edge->use_count : 0.5f;
    float learning = edge_weight * (1.0f + usage) * (0.5f + success_rate);
    
    // 3. Coherence
    float pattern_alignment = is_in_pattern(g, from, to) ? 1.0f : 0.7f;
    float sequential_flow = follows_from_output(g, from, to) ? 1.0f : 0.8f;
    float context_fit = pattern_matches_context(g, from, to) ? 1.0f : 0.6f;
    float coherence = pattern_alignment * sequential_flow * context_fit;
    
    // 4. Predictive_Power
    float pattern_prediction = is_predicted_by_pattern(g, to) ? get_pattern_confidence(g, to) : 0.3f;
    float historical_accuracy = success_rate;
    float context_prediction = context_match;
    float predictive = pattern_prediction * (0.5f + historical_accuracy) * context_prediction;
    
    // Combine: Path Quality = Information × Learning × Coherence × Predictive
    return information * learning * coherence * predictive;
}
```

### For Output Selection (Node Selection)

```c
float calculate_node_quality(MelvinGraph *g, uint32_t node_id) {
    // Same factors, but from node perspective
    // Node quality = how good is this node as an output choice?
    
    // 1. Information_Carried (from input to this node)
    float input_connection = is_reachable_from_input(g, node_id) ? 1.0f : 0.1f;
    float context_match = pattern_matches_context(g, node_id) ? 1.0f : 0.5f;
    float history_coherence = follows_from_output(g, node_id) ? 1.0f : 0.8f;
    float information = input_connection * context_match * history_coherence;
    
    // 2. Learning_Strength (how well-learned is this node?)
    float node_activation = g->nodes[node_id].activation;
    float usage = logf(1.0f + g->nodes[node_id].receive_count) / 10.0f;
    float learning = node_activation * (1.0f + usage);
    
    // 3. Coherence (does this node fit contextually?)
    float pattern_alignment = is_predicted_by_pattern(g, node_id) ? 1.0f : 0.7f;
    float sequential_flow = follows_from_output(g, node_id) ? 1.0f : 0.8f;
    float context_fit = pattern_matches_context(g, node_id) ? 1.0f : 0.6f;
    float coherence = pattern_alignment * sequential_flow * context_fit;
    
    // 4. Predictive_Power (how well does this node predict correct output?)
    float pattern_prediction = is_predicted_by_pattern(g, node_id) ? get_pattern_confidence(g, node_id) : 0.3f;
    float historical_accuracy = 0.5f;  // Would need to track node-level success
    float context_prediction = context_match;
    float predictive = pattern_prediction * (0.5f + historical_accuracy) * context_prediction;
    
    // Combine: Node Quality = Information × Learning × Coherence × Predictive
    return information * learning * coherence * predictive;
}
```

## Key Principles

1. **Information Flow**: Paths that carry information from input to output are better
2. **Learning Strength**: Well-learned paths (high weight, high usage, high success) are better
3. **Coherence**: Paths that form coherent sequences are better
4. **Predictive Power**: Paths that predict correct outputs are better

## Normalization

Path qualities are normalized so that:
- All paths get some activation (proportional to quality)
- Better paths get more activation
- No binary thresholds - continuous measure

## Example

**Path A**: "hello" → "world" (edge weight 0.8, used 100 times, 90% success, pattern predicts it)
- Information: 1.0 (from input) × 1.0 (pattern match) × 1.0 (coherent) = 1.0
- Learning: 0.8 × log(101)/10 × 0.9 = 0.8 × 0.46 × 0.9 = 0.33
- Coherence: 1.0 × 1.0 × 1.0 = 1.0
- Predictive: 0.9 × 0.9 × 1.0 = 0.81
- **Quality = 1.0 × 0.33 × 1.0 × 0.81 = 0.27**

**Path B**: "hello" → "x" (edge weight 0.1, used 2 times, 0% success, no pattern)
- Information: 0.1 × 0.5 × 0.8 = 0.04
- Learning: 0.1 × log(3)/10 × 0.0 = 0.1 × 0.11 × 0.0 = 0.0
- Coherence: 0.7 × 0.8 × 0.6 = 0.34
- Predictive: 0.3 × 0.0 × 0.5 = 0.0
- **Quality = 0.04 × 0.0 × 0.34 × 0.0 = 0.0**

Path A is clearly better (0.27 vs 0.0).
