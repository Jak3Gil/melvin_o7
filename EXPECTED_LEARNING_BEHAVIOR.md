# Expected Learning Behavior: What More Data Should Do

## Current Problem

**More data makes the system WORSE, not better:**
- Error rate increases (0.525 → 0.750)
- Outputs regress from "taccas" (with 's') back to "taccat" (loop)
- No convergence to correct behavior
- System gets stuck in stable loops instead of learning

## What SHOULD Happen with More Data

### 1. **Pattern Utility Should Increase**
```
Episode 1:  Pattern "at" predicts 's', success rate = 0/1 = 0%
Episode 10: Pattern "at" predicts 's', success rate = 3/10 = 30%
Episode 50: Pattern "at" predicts 's', success rate = 40/50 = 80%
Episode 100: Pattern "at" predicts 's', success rate = 90/100 = 90%
```
**Result:** Pattern strength increases with utility → predictions get stronger

### 2. **Error Rate Should Decrease**
```
Episode 1:  Error = 0.9 (system doesn't know anything)
Episode 10: Error = 0.7 (starting to learn)
Episode 50: Error = 0.3 (learned pattern, some mistakes)
Episode 100: Error = 0.1 (converged, mostly correct)
```
**Result:** Learning rate increases with error → faster correction

### 3. **Outputs Should Converge to Correct Behavior**
```
Episode 1:  Output = "taacta" (random)
Episode 10: Output = "tcaaac" (closer, but still wrong)
Episode 30: Output = "taccas" (CORRECT - ends with 's')
Episode 50: Output = "cats"   (PERFECT - exact match)
Episode 100: Output = "cats"   (STABLE - converged)
```
**Result:** Wave propagation settles into learned attractor states

### 4. **Weak Patterns Should Be Pruned**
```
Episode 1:  6 patterns (all new, utility unknown)
Episode 50: 4 patterns (pruned 2 weak ones, kept useful)
Episode 100: 3 patterns (only strong, useful patterns remain)
```
**Result:** Metabolic pressure removes noise, keeps signal

### 5. **Context Should Stabilize on Learned Paths**
```
Early: High exploration → try different outputs
Middle: Patterns emerging → some correct predictions
Late: Exploitation → follow learned patterns consistently
```
**Result:** Exploration pressure decreases as error decreases

## Why This Isn't Happening

### Hypothesis 1: Pattern Utility Not Updating
- Patterns track `prediction_attempts` and `prediction_successes`
- But utility might not be feeding back into strength correctly
- **Check:** Are successful predictions increasing pattern strength over time?

### Hypothesis 2: Feedback Loop Broken
- apply_feedback() updates edges and patterns
- But the feedback might not be strong enough to overcome wave chaos
- **Check:** Is learning_rate adapting properly to error_rate?

### Hypothesis 3: Context Logic Too Chaotic
- History penalty might be TOO strong (penalizing correct repetitions)
- Position context might not be stable enough
- **Check:** Are patterns matching consistently at the right positions?

### Hypothesis 4: Wave Dynamics Dominate Learned Structure
- Even with strong patterns, wave propagation might be too noisy
- Activation spreads chaotically, overwhelming pattern predictions
- **Check:** Is pattern activation transfer strong enough vs edge transfer?

### Hypothesis 5: Circular Regulation Not Finding Stable States
- System might be oscillating instead of converging
- Variables influencing each other create feedback loops without stable attractors
- **Check:** Are activation, energy, threshold settling or oscillating?

## What Needs to Be Fixed

For the system to be truly intelligent with more data:

1. **Strengthen utility feedback loop**
   - Successful pattern predictions → increase strength significantly
   - Failed predictions → decrease strength (not just track utility)

2. **Make learning proportional to error**
   - High error = aggressive learning (big weight updates)
   - Low error = fine-tuning (small weight updates)

3. **Context should favor learned patterns over noise**
   - When pattern matches and predicts, THAT should dominate
   - Wave activation should be secondary to learned structure

4. **Pruning should be aggressive**
   - Remove patterns that don't improve predictions
   - Keep only patterns with high utility (>50% success rate)

5. **Outputs should become DETERMINISTIC with learning**
   - Early: stochastic exploration
   - Late: deterministic exploitation of learned patterns
   - Temperature should decrease with error rate

## Test to Validate Fix

```c
for (int episode = 0; episode < 100; episode++) {
    train(g, "cat", "cats");
    test_output = infer(g, "cat");
    
    // Success criteria:
    assert(error_rate[episode] < error_rate[episode-1]); // Decreasing
    assert(pattern_utility[episode] > pattern_utility[episode-1]); // Increasing
    if (episode > 50) {
        assert(test_output == "cats"); // Converged
    }
}
```

If these assertions pass, the system is truly learning from more data.
If they fail, the learning mechanism is broken.

## Current Status

**BROKEN:** More data → worse performance
**NEEDED:** More data → better performance (true learning)

