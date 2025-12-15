# Self-Tuning Implementation Plan

## Current Architecture Already Has Self-Regulation

Looking at the code, Melvin o7 already has circular self-regulation:
- Variables are relative (ratios, not absolutes)
- Pressures emerge from state (competition_pressure, exploration_pressure)
- No hardcoded limits

## What's Missing: Proper Feedback Loops

The system has the RIGHT STRUCTURE but needs stronger feedback loops:

### 1. Pattern Utility → Strength (Direct Connection)

**Current:** Pattern strength calculated from compression benefit, then dampened
**Needed:** Pattern strength IS utility (direct, no intermediaries)

```c
// In detect_patterns(), after patterns are created:
for (uint32_t p = 0; p < g->pattern_count; p++) {
    Pattern *pat = &g->patterns[p];
    
    // Utility is raw success rate
    if (pat->prediction_attempts > 10) {  // Need enough data
        float utility = (float)pat->prediction_successes / (float)pat->prediction_attempts;
        
        // Strength IS utility (successful patterns = strong patterns)
        pat->strength = utility;
        
        // Weak patterns (below chance) get weakened further
        if (utility < 0.4) {
            pat->strength *= 0.5;  // Decay toward death
        }
    }
}
```

### 2. Error → Learning Pressure (Quadratic Feedback)

**Current:** Learning rate = 0.01 + 0.2 * error_rate (linear)
**Needed:** Learning pressure = error_rate² (quadratic - aggressive when needed)

```c
// In update_system_state():
g->state.learning_pressure = g->state.error_rate * g->state.error_rate;

// Use learning_pressure everywhere instead of fixed learning_rate
float learning_rate = 0.5f * g->state.learning_pressure;  // 0.9² = 0.405 (aggressive)
                                                            // 0.1² = 0.005 (gentle)
```

### 3. Pattern Success → Pattern Dominance

**Current:** Patterns and edges compete equally
**Needed:** When patterns work well, they DOMINATE output selection

```c
// Compute average pattern utility
float total_utility = 0.0f;
uint32_t utility_count = 0;
for (uint32_t p = 0; p < g->pattern_count; p++) {
    if (g->patterns[p].prediction_attempts > 5) {
        total_utility += g->patterns[p].utility;
        utility_count++;
    }
}
g->state.pattern_confidence = (utility_count > 0) ? 
    (total_utility / utility_count) : 0.0f;

// In compute_node_relevance():
if (g->state.pattern_confidence > 0.5) {
    // Patterns are working! Trust them over wave chaos
    if (position_context > 0.1) {
        return position_context * 2.0;  // Strong boost when patterns predict
    }
}
```

### 4. Output Variance → Exploration/Exploitation

**Current:** exploration_pressure = 0.5f (static)
**Needed:** Measure output variance, adjust exploration dynamically

```c
// Track recent outputs to compute variance
typedef struct {
    uint32_t recent_outputs[100];  // Last 100 output bytes
    uint32_t output_index;
} OutputHistory;

// Compute variance
float compute_output_variance(OutputHistory *hist) {
    // If outputs are all the same = low variance = converged
    // If outputs are random = high variance = exploring
    uint32_t unique_count = count_unique_in_window(hist, 20);
    return (float)unique_count / 20.0f;  // 0.0 = stuck, 1.0 = chaos
}

// Update exploration pressure
g->state.output_variance = compute_output_variance(&g->output_history);
g->state.exploration_pressure = g->state.output_variance * g->state.error_rate;
// High variance + high error = explore
// Low variance + low error = exploit (converged)
```

### 5. Loop Detection → Loop Escape Pressure

**Current:** History penalty based on any repetition
**Needed:** Detect actual LOOPS (repeating subsequences), create pressure

```c
// Detect if output is looping (e.g., "ababab")
bool detect_loop(uint32_t *output, uint32_t length) {
    if (length < 6) return false;
    
    // Check if last 3 chars repeat the previous 3
    for (uint32_t i = 0; i < 3 && length >= 6; i++) {
        if (output[length - 1 - i] != output[length - 4 - i]) {
            return false;
        }
    }
    return true;
}

// In compute_node_relevance():
if (detect_loop(g->output_buffer, g->output_length)) {
    g->state.loop_pressure = 0.9f;  // Strong pressure to break loop
    
    // If this node would continue the loop, suppress it
    if (node_id == g->output_buffer[g->output_length - 3]) {
        return relevance * 0.1f;  // Kill looping nodes
    }
} else {
    g->state.loop_pressure *= 0.9f;  // Decay when not looping
}
```

## Implementation Steps

### Phase 1: Strengthen Pattern Utility Feedback
1. Make pattern strength = utility (direct)
2. Prune patterns with utility < 0.3 (below chance + margin)
3. Test: Does pattern strength track success rate?

### Phase 2: Make Learning Pressure Adaptive
1. learning_pressure = error_rate²
2. Pattern updates scale with learning_pressure
3. Test: Does learning accelerate when error is high?

### Phase 3: Add Pattern Confidence
1. Compute avg_pattern_utility across all patterns
2. pattern_confidence drives output selection weighting
3. Test: Do successful patterns dominate output?

### Phase 4: Add Output Variance Tracking
1. Track recent outputs in circular buffer
2. Compute variance (unique count in window)
3. exploration_pressure = variance * error
4. Test: Does exploration decrease as system converges?

### Phase 5: Add Loop Detection
1. Detect repeating subsequences in output
2. loop_pressure suppresses loop-continuing nodes
3. Test: Does system break out of loops?

## Test Suite for Self-Tuning

```c
// Test 1: Pattern utility → strength
train(100 episodes);
assert(successful_pattern.strength > 0.7);
assert(failed_pattern.strength < 0.3);

// Test 2: Error → learning pressure
set_error(0.9);
assert(learning_pressure > 0.7);  // Aggressive
set_error(0.1);
assert(learning_pressure < 0.1);  // Gentle

// Test 3: Pattern confidence → dominance
train_until(pattern_confidence > 0.8);
assert(pattern_output_percentage > 0.8);  // Patterns drive output

// Test 4: Variance → exploration
assert(early_episodes.exploration > 0.7);  // High exploration
assert(late_episodes.exploration < 0.2);   // Low exploration (converged)

// Test 5: Loop detection → escape
create_loop_state();
assert(loop_pressure > 0.8);
assert(next_output != loop_continuation);
```

## The Key Insight

**Don't tune the parameters. Measure the problems, create pressures.**

Every problem the system faces creates measurable symptoms:
- Error too high → learning_pressure increases
- Patterns failing → their strength decreases
- Output chaotic → exploration_pressure high
- Output looping → loop_pressure increases

The system SELF-DIAGNOSES and SELF-CORRECTS through circular feedback.

**We design the diagnostic sensors and feedback loops.**
**The system discovers the solutions.**

