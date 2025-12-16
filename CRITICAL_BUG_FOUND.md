# CRITICAL BUG FOUND

## Symptom
Output is always "ctc" regardless of:
- Input ("cat", "bat", "mat")
- Training (0 episodes or 30 episodes)
- Target ("cats")

## This Means:
1. ✓ Activation IS flowing (we get output)
2. ✗ Path quality calculation is BROKEN
3. ✗ Node selection is NOT distinguishing paths

## Root Cause Hypothesis:

### Issue 1: Learning Factor is TOO WEAK

```c
float learning = edge_weight *           // 0.01 to 0.1 (new edges)
                (1 + success_rate * 2) *  // 1.0 (no success yet)
                (1 + usage_boost);        // 1.1 (log(2)/5)

learning = 0.01 * 1.0 * 1.1 = 0.011  // TINY!
```

Even after training:
```c
edge_weight = 0.2 (strengthened)
success_rate = 0.9 (30/33 success)
usage_boost = log(34)/5 = 0.7

learning = 0.2 * 2.8 * 1.7 = 0.95  // Still small!
```

### Issue 2: Path Quality Formula

```c
base_quality = learning;  // Start with 0.011

// With bonuses (all near 0 initially):
if (information > 0.1) base_quality *= (1 + information * 0.5);  // No boost
if (predictive > 0.1) base_quality *= (1 + predictive * 0.3);    // No boost  
if (coherence > 0.1) base_quality *= (1 + coherence * 0.2);      // No boost

// Result: base_quality stays ~0.011 for all paths!
```

### Issue 3: Information Factor KILLS Everything

```c
float information = input_connection * context_match * history_coherence;
// All 0 initially → information = 0

// Fallback:
if (information < 0.01) {
    information = input_connection + context_match + history_coherence;
    if (information < 0.01) {
        information = 0.1;  // Baseline
    }
}

// So information = 0.1 for ALL paths (no distinction!)
```

## The Real Problem:

**ALL PATHS HAVE SAME QUALITY INITIALLY!**

- All edges: weight=0.01, use_count=1, success_count=0
- All learning factors: ~0.011
- All information factors: 0.1 (baseline)
- All path qualities: ~0.011

**No winner → random/first selection → "ctc"**

## Solution:

Need to make edge weight differences MORE DRAMATIC:

1. **Start edges stronger**: weight=0.1 instead of 0.01
2. **Strengthen faster**: growth_rate = 0.3 instead of 0.1
3. **Success boost stronger**: (1 + success_rate * 5) instead of * 2
4. **Or**: Use edge properties DIRECTLY without combining

The learning signal needs to be **orders of magnitude stronger** for trained vs untrained paths.

