# Save/Load Design & Modality Context

## The .m File Structure

The .m file should contain the COMPLETE learned graph state:
- All patterns (with node_ids, predictions, weights, context)
- All edges (with weights, success counts)
- Node states (energy, threshold baselines)
- Context encodings (which patterns learned in which contexts)

## Modality Context Solution

**Problem:** Byte 65 ('A') means different things in different modalities.

**Solution:** Patterns learn WITH context:
```c
Pattern {
    node_ids: [65, 66, 67],
    context_vector: [0.8, 0.0, 0.0, 0.0],  // TEXT=high, others=low
    predictions: [...]
}
```

When matching:
1. Check if sequence matches pattern
2. Check if current context matches pattern's context
3. Only fire if BOTH match

This way:
- "ABC" pattern in TEXT context → fires for text
- Different pattern in AUDIO context → fires for audio
- Same bytes, different patterns, no confusion

## Implementation Steps

1. Add context_vector[16] to Pattern struct
2. When creating patterns, capture current context
3. When matching patterns, check context similarity
4. Save/load functions write entire graph to .m file
5. Load function restores complete state

