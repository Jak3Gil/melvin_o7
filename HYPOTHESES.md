# HYPOTHESES: Why Patterns Aren't Working

## Hypothesis A: Patterns Not Being Created
**Theory**: Pattern detection threshold is too high, so patterns like "hello" aren't being created even after 10 training runs.
**Evidence needed**: Pattern count after training, which patterns exist.

## Hypothesis B: Patterns Not Activating
**Theory**: Patterns exist but don't activate because matching fails or activation is too low.
**Evidence needed**: When input "hello" is given, does pattern "hello" match? What is its activation?

## Hypothesis C: Pattern Predictions Too Weak
**Theory**: Pattern predictions exist but give too little activation compared to edge-based propagation, so edge loops win.
**Evidence needed**: How much activation do pattern predictions give vs edge propagation? What are the transfer values?

## Hypothesis D: Edge Propagation Creating Loops
**Theory**: Edge-based propagation creates strong loops (e.g., l→l, e→e) that accumulate more activation than pattern predictions.
**Evidence needed**: Which nodes get activation from edges? Are there self-loops? What are the edge transfer values?

## Hypothesis E: Pattern Predictions Blocked
**Theory**: Pattern predictions are being blocked by `fired_predictions` flag or `prediction_used` check, preventing them from firing.
**Evidence needed**: Are predictions marked as used? Are they being skipped?
