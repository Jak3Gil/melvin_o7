# The Relative Coherence Intelligence Equation

## Core Principle

**Everything is relative (right now, for me right now)**

The wave evaluates coherence as it propagates. Incoherent paths decay. Coherent paths survive. The wave itself is the decision.

---

## The Equation

### Step 1: Compute Relative Coherence (On-the-Fly)

For each edge `source → target` during wave propagation:

```c
float compute_relative_coherence(MelvinGraph *g, uint32_t source, uint32_t target, Edge *edge) {
    /* ========================================================================
     * RELATIVE COHERENCE: Everything relative to current context
     * Not historical averages, not absolute values
     * "What is this worth RIGHT NOW, in THIS context?"
     * ======================================================================== */
    
    /* 1. PATTERN SUPPORT (Relative to current active patterns) */
    float pattern_support = 0.0f;
    float max_pattern_activation = 0.0f;
    float total_active_pattern_strength = 0.0f;
    
    /* Find max active pattern activation (for relative comparison) */
    for (uint32_t p = 0; p < g->pattern_count; p++) {
        Pattern *pat = &g->patterns[p];
        if (pat->activation > pat->threshold) {
            if (pat->activation > max_pattern_activation) {
                max_pattern_activation = pat->activation;
            }
            total_active_pattern_strength += pat->strength;
        }
    }
    
    /* Check if patterns support this edge (relative to active patterns) */
    for (uint32_t p = 0; p < g->pattern_count; p++) {
        Pattern *pat = &g->patterns[p];
        
        /* Only consider ACTIVE patterns (right now) */
        if (pat->activation <= pat->threshold) continue;
        
        /* Check if pattern supports this edge */
        bool supports = false;
        
        /* Support 1: Edge is in pattern sequence */
        for (uint32_t idx = 0; idx + 1 < pat->length; idx++) {
            if (pat->node_ids[idx] == source && pat->node_ids[idx + 1] == target) {
                supports = true;
                break;
            }
        }
        
        /* Support 2: Pattern predicts target (and source is in pattern) */
        if (!supports) {
            bool source_in_pattern = false;
            for (uint32_t idx = 0; idx < pat->length; idx++) {
                if (pat->node_ids[idx] == source) {
                    source_in_pattern = true;
                    break;
                }
            }
            
            if (source_in_pattern) {
                for (uint32_t pred = 0; pred < pat->prediction_count; pred++) {
                    if (pat->predicted_nodes[pred] == target) {
                        supports = true;
                        break;
                    }
                }
            }
        }
        
        if (supports) {
            /* RELATIVE: Compare to max active pattern (not absolute) */
            float relative_activation = (max_pattern_activation > 0.001f) ?
                (pat->activation / max_pattern_activation) : 0.0f;
            
            /* Pattern support = relative strength × relative activation × meaning */
            float meaning_boost = 1.0f + (pat->accumulated_meaning * 0.5f);
            float hierarchy_boost = 1.0f + (1.0f / (1.0f + pat->chain_depth * 0.2f));
            
            pattern_support += pat->strength * relative_activation * meaning_boost * hierarchy_boost;
        }
    }
    
    /* Normalize pattern support relative to total active pattern strength */
    float relative_pattern_support = (total_active_pattern_strength > 0.001f) ?
        (pattern_support / total_active_pattern_strength) : 0.0f;
    
    
    /* 2. CONTEXT FIT (Relative to current input/output) */
    float context_fit = 0.0f;
    
    /* Input context: Is target reachable from current input? (right now) */
    bool target_in_input = false;
    bool source_in_input = false;
    for (uint32_t i = 0; i < g->input_length; i++) {
        if (g->input_buffer[i] == target) target_in_input = true;
        if (g->input_buffer[i] == source) source_in_input = true;
    }
    
    if (source_in_input) {
        context_fit += 1.0f;  /* Source is in input (strong context) */
    }
    
    if (target_in_input && g->output_length < g->input_length) {
        /* Target is next in input sequence (right now) */
        uint32_t next_input_pos = g->output_length;
        if (g->input_buffer[next_input_pos] == target) {
            context_fit += 1.5f;  /* Stronger - continues input sequence */
        }
    }
    
    /* Output context: Does target follow from last output? (right now) */
    if (g->output_length > 0) {
        uint32_t last_output = g->output_buffer[g->output_length - 1];
        if (last_output == source) {
            context_fit += 1.5f;  /* Stronger - continues output sequence */
        } else {
            /* Check if edge exists from last_output to source */
            EdgeList *last_edges = &g->outgoing[last_output];
            for (uint32_t e = 0; e < last_edges->count; e++) {
                if (last_edges->edges[e].to_id == source && last_edges->edges[e].active) {
                    context_fit += 0.5f;  /* Moderate - connected to last output */
                    break;
                }
            }
        }
    }
    
    /* Normalize context fit (relative to max possible) */
    float max_context_fit = 4.0f;  /* Max: input source (1.0) + input target (1.5) + output (1.5) */
    float relative_context_fit = context_fit / max_context_fit;
    
    
    /* 3. SEQUENCE COHERENCE (Relative to current sequence) */
    float sequence_coherence = 0.0f;
    
    /* Does this edge continue a logical sequence? (right now) */
    if (g->output_length > 0) {
        uint32_t last = g->output_buffer[g->output_length - 1];
        
        if (last == source) {
            /* Direct continuation - very coherent */
            sequence_coherence = 1.0f;
        } else {
            /* Check if there's a learned path from last to source */
            EdgeList *last_edges = &g->outgoing[last];
            for (uint32_t e = 0; e < last_edges->count; e++) {
                if (last_edges->edges[e].to_id == source) {
                    /* RELATIVE: Use edge's success rate (right now, not historical average) */
                    float success_rate = (edge->use_count > 0) ?
                        ((float)edge->success_count / (float)edge->use_count) : 0.5f;
                    sequence_coherence = success_rate;  /* Relative to this edge's performance */
                    break;
                }
            }
        }
    }
    
    
    /* 4. BLANK NODE HYPOTHESIS TEST (If applicable) */
    float generalization_coherence = 0.0f;
    
    /* For patterns with blank nodes, test if filling blank with target would be coherent */
    for (uint32_t p = 0; p < g->pattern_count; p++) {
        Pattern *pat = &g->patterns[p];
        if (pat->activation <= pat->threshold) continue;
        
        /* Check if pattern has blank nodes */
        bool has_blank = false;
        for (uint32_t i = 0; i < pat->length; i++) {
            if (pat->node_ids[i] == BLANK_NODE) {
                has_blank = true;
                break;
            }
        }
        
        if (has_blank) {
            /* Test hypothesis: "if blank = target, would pattern be coherent?" */
            bool would_match = pattern_would_match_if_filled(g, pat, source, target);
            
            if (would_match) {
                /* Compute coherence of filled pattern (relative to current context) */
                float filled_coherence = compute_filled_pattern_coherence(g, pat, source, target);
                
                /* If filled pattern is coherent, this edge is a good generalization */
                if (filled_coherence > 0.3f) {  /* Coherence threshold */
                    generalization_coherence += filled_coherence * pat->strength;
                }
            }
        }
    }
    
    /* Normalize generalization coherence */
    float relative_generalization = (total_active_pattern_strength > 0.001f) ?
        (generalization_coherence / total_active_pattern_strength) : 0.0f;
    
    
    /* ========================================================================
     * COMBINE: Relative Coherence Score
     * All components are relative (0-1 range)
     * ======================================================================== */
    
    float relative_coherence = 
        (relative_pattern_support * 0.5f) +      /* Patterns are primary (50%) */
        (relative_context_fit * 0.25f) +         /* Context matters (25%) */
        (sequence_coherence * 0.15f) +           /* Sequence matters (15%) */
        (relative_generalization * 0.1f);       /* Generalization matters (10%) */
    
    return relative_coherence;
}
```

---

## Step 2: Wave Propagation with Coherence

```c
void propagate_with_coherence(MelvinGraph *g) {
    /* ========================================================================
     * WAVE PROPAGATION WITH RELATIVE COHERENCE
     * The wave evaluates coherence as it propagates
     * Incoherent paths decay, coherent paths survive
     * ======================================================================== */
    
    /* For each active node */
    for (uint32_t source = 0; source < BYTE_VALUES; source++) {
        if (!g->nodes[source].exists) continue;
        if (g->nodes[source].activation < 0.001f) continue;  /* Not active */
        
        EdgeList *out = &g->outgoing[source];
        
        /* For each outgoing edge */
        for (uint32_t e = 0; e < out->count; e++) {
            Edge *edge = &out->edges[e];
            if (!edge->active) continue;
            
            uint32_t target = edge->to_id;
            
            /* Compute relative coherence (on-the-fly, right now) */
            float coherence = compute_relative_coherence(g, source, target, edge);
            
            /* ========================================================================
             * COHERENCE MULTIPLIER: Coherent paths strengthen, incoherent paths decay
             * ======================================================================== */
            
            /* Coherence multiplier: > 1.0 for coherent, < 1.0 for incoherent */
            float coherence_multiplier;
            
            if (coherence > 0.5f) {
                /* Coherent: Strengthen signal */
                /* Scale: 0.5 → 1.0, 1.0 → 2.0 */
                coherence_multiplier = 1.0f + ((coherence - 0.5f) * 2.0f);
            } else {
                /* Incoherent: Decay signal */
                /* Scale: 0.5 → 1.0, 0.0 → 0.1 (strong decay) */
                coherence_multiplier = 0.1f + (coherence * 1.8f);
            }
            
            /* Base signal strength (relative to edge weight, not absolute) */
            float base_signal = g->nodes[source].activation * edge->weight;
            
            /* Apply coherence multiplier */
            float coherent_signal = base_signal * coherence_multiplier;
            
            /* Transfer to target */
            if (!g->nodes[target].exists) {
                g->nodes[target].exists = true;
                g->nodes[target].activation = 0.0f;
            }
            
            g->nodes[target].activation += coherent_signal;
            
            /* ========================================================================
             * DECAY SOURCE (like energy consumption)
             * ======================================================================== */
            
            /* Source loses activation (proportional to transfer) */
            g->nodes[source].activation *= (1.0f - coherent_signal * 0.1f);
            if (g->nodes[source].activation < 0.0f) {
                g->nodes[source].activation = 0.0f;
            }
        }
    }
    
    /* ========================================================================
     * NATURAL DECAY (incoherent paths fade faster)
     * ======================================================================== */
    
    for (uint32_t i = 0; i < BYTE_VALUES; i++) {
        if (!g->nodes[i].exists) continue;
        
        /* Base decay rate */
        float decay_rate = 0.9f;  /* 10% decay per step */
        
        /* Check if node is coherent (has pattern support) */
        float node_coherence = 0.0f;
        for (uint32_t p = 0; p < g->pattern_count; p++) {
            Pattern *pat = &g->patterns[p];
            if (pat->activation > pat->threshold) {
                for (uint32_t pred = 0; pred < pat->prediction_count; pred++) {
                    if (pat->predicted_nodes[pred] == i) {
                        node_coherence += pat->strength * pat->activation;
                        break;
                    }
                }
            }
        }
        
        /* Coherent nodes decay slower */
        if (node_coherence > 0.1f) {
            decay_rate = 0.95f;  /* Only 5% decay for coherent nodes */
        }
        
        g->nodes[i].activation *= decay_rate;
    }
}
```

---

## Step 3: Selection (Wave Commits)

```c
uint32_t select_most_coherent(MelvinGraph *g) {
    /* ========================================================================
     * SELECTION: Wave commits to most coherent path
     * Not highest activation, but most coherent
     * ======================================================================== */
    
    uint32_t best_node = BYTE_VALUES;
    float best_coherence = 0.0f;
    
    for (uint32_t i = 0; i < BYTE_VALUES; i++) {
        if (!g->nodes[i].exists) continue;
        if (g->nodes[i].activation < 0.001f) continue;
        
        /* Compute coherence for this node (relative to current context) */
        float node_coherence = 0.0f;
        
        /* Pattern support */
        for (uint32_t p = 0; p < g->pattern_count; p++) {
            Pattern *pat = &g->patterns[p];
            if (pat->activation > pat->threshold) {
                for (uint32_t pred = 0; pred < pat->prediction_count; pred++) {
                    if (pat->predicted_nodes[pred] == i) {
                        float meaning_boost = 1.0f + (pat->accumulated_meaning * 0.5f);
                        node_coherence += pat->strength * pat->activation * meaning_boost;
                        break;
                    }
                }
            }
        }
        
        /* Context fit */
        bool in_input = false;
        for (uint32_t j = 0; j < g->input_length; j++) {
            if (g->input_buffer[j] == i) {
                in_input = true;
                break;
            }
        }
        if (in_input) node_coherence += 0.5f;
        
        /* Sequence coherence */
        if (g->output_length > 0) {
            uint32_t last = g->output_buffer[g->output_length - 1];
            EdgeList *last_edges = &g->outgoing[last];
            for (uint32_t e = 0; e < last_edges->count; e++) {
                if (last_edges->edges[e].to_id == i) {
                    node_coherence += 0.3f;
                    break;
                }
            }
        }
        
        /* Combined score: coherence × activation */
        /* Coherent nodes with high activation win */
        float score = node_coherence * g->nodes[i].activation;
        
        if (score > best_coherence) {
            best_coherence = score;
            best_node = i;
        }
    }
    
    return best_node;
}
```

---

## The Complete Equation

### Relative Coherence Score:

```
coherence = 
    (pattern_support / total_active_patterns) × 0.5 +      /* Relative to active patterns */
    (context_fit / max_context) × 0.25 +                   /* Relative to current context */
    (sequence_coherence) × 0.15 +                          /* Relative to current sequence */
    (generalization / total_active_patterns) × 0.1          /* Relative to active patterns */
```

### Coherence Multiplier:

```
if coherence > 0.5:
    multiplier = 1.0 + ((coherence - 0.5) × 2.0)  /* Strengthen: 0.5→1.0, 1.0→2.0 */
else:
    multiplier = 0.1 + (coherence × 1.8)          /* Decay: 0.5→1.0, 0.0→0.1 */
```

### Signal Transfer:

```
coherent_signal = source_activation × edge_weight × coherence_multiplier
target_activation += coherent_signal
source_activation *= (1.0 - coherent_signal × 0.1)  /* Energy consumption */
```

### Selection:

```
score = node_coherence × node_activation
selected = argmax(score)  /* Most coherent, not just highest activation */
```

---

## Key Properties

1. **Everything Relative**: All values compared to current context, not historical averages
2. **On-the-Fly**: Coherence computed during propagation, not after
3. **Active Decay**: Incoherent paths decay (< 1.0 multiplier), not just weak
4. **Pattern-Driven**: Patterns determine coherence (primary factor: 50%)
5. **Context-Aware**: Current input/output context matters (25%)
6. **Sequence-Aware**: Follows from what came before (15%)
7. **Generalizes**: Blank nodes test hypotheses (10%)

---

## Result

The wave naturally converges on coherent paths because:
- Incoherent paths decay (multiplier < 1.0)
- Coherent paths strengthen (multiplier > 1.0)
- Selection picks most coherent (not just highest activation)

**The wave IS the decision. It evaluates, decays, strengthens, and commits.**
