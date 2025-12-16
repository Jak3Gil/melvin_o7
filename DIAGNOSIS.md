# DIAGNOSIS: Why Patterns Aren't Working

## Hypothesis Evaluation

### Hypothesis A: Patterns Not Being Created
**Status**: ✅ CONFIRMED - Patterns ARE being created
**Evidence**: 
- Log shows patterns being created: "he" (104,101), "el" (101,108), "ll" (108,108), "lo" (108,111)
- Patterns are bigrams (2 characters), not full sequences
- Threshold is 1.50, patterns created with count=1

### Hypothesis B: Patterns Not Activating  
**Status**: ✅ CONFIRMED - Patterns ARE activating
**Evidence**:
- Pattern "he" activates with activation=0.3216, strength=0.7500
- Pattern "el" activates with activation=0.2946
- Pattern "ll" activates with activation=0.2103
- BUT: Pattern "lo" has predictionCount=0 (doesn't predict anything!)

### Hypothesis C: Pattern Predictions Too Weak
**Status**: ⚠️ PARTIALLY CONFIRMED
**Evidence**:
- Pattern predictions give transfers: 0.3-0.7 (decent)
- BUT: Edge transfers to "l" are 0.1-0.3, and some up to 2.4673!
- Edge propagation is overwhelming pattern predictions

### Hypothesis D: Edge Propagation Creating Loops
**Status**: ✅ CONFIRMED - ROOT CAUSE
**Evidence**:
- **257 self-loops detected!** (isSelfLoop=1)
- Self-loop example: "l"→"l" with transfer=0.1284, beforeActivation=0.9154, afterActivation=1.0438
- Many edges feeding into "l": "e"→"l" (transfer=2.4673!), "r"→"l", "d"→"l"
- Node "l" gets activation from multiple sources, accumulating in loops

### Hypothesis E: Pattern Predictions Blocked
**Status**: ✅ CONFIRMED
**Evidence**:
- Pattern predictions to node 108 ("l") are being blocked (predictionUsed=1)
- Multiple blocks: patternId 1, 4 repeatedly blocked

## CRITICAL FINDING

**ZERO predictions to "w" (119)!**
- No pattern predictions target "w" in the entire log
- Patterns only predict next characters in the SAME sequence ("hello" → "e", "l", "l", "o")
- There's no pattern that learns "hello" → "world" (cross-sequence prediction)

## Root Cause Analysis

1. **Patterns are only bigrams**: Patterns like "he", "el", "ll" only predict the next character in the same sequence, not cross-sequence predictions like "hello" → "world"

2. **Self-loops accumulating activation**: 257 self-loops create strong activation in nodes like "l", "e", "h", "o" that overwhelms pattern predictions

3. **Pattern predictions blocked**: The `predictionUsed` flag prevents patterns from firing multiple times, but this blocks legitimate predictions

4. **No cross-sequence learning**: The system learns "hello" → "hello" (same sequence) but not "hello" → "world" (different sequence). Patterns need to learn input→output mappings, not just next-char predictions.

5. **Edge propagation dominates**: Edge transfers (up to 2.4673) are much larger than pattern predictions (0.3-0.7), so loops win

## The Fix

1. **Prevent self-loops**: Don't create edges from node to itself
2. **Learn cross-sequence patterns**: Patterns should learn "hello" → "world", not just "he" → "e"
3. **Reduce prediction blocking**: Allow patterns to predict multiple times if needed
4. **Boost pattern predictions**: Make pattern predictions stronger than edge loops
5. **Learn input→output mappings**: When training "hello" → "world", create a pattern that maps input sequence to output sequence
