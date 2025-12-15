# MELVIN O7: Intelligence Test Suite Documentation

## Overview

The `test.c` file contains a comprehensive test suite designed to evaluate **TRUE intelligence**, not just memorization or echoing.

## Why These Tests?

### The Problem with Echo Tasks

If you only train a system to repeat what it sees:
- Input: "cat" → Output: "cat"

Then that's **all it will ever do**. It learns to be a parrot, not intelligent.

### What Tests Intelligence?

1. **Rule Extraction**: Can the system find abstract patterns? (e.g., "+s" for plural)
2. **Generalization**: Can it apply learned rules to new examples? (zero-shot)
3. **Context Discrimination**: Same input → different outputs based on task
4. **Pattern Reuse**: Transfer learned patterns across examples
5. **Composition**: Combine multiple learned rules

---

## Test Suite Breakdown

### TEST 1: Pluralization (Rule Extraction + Generalization)

**Training:**
- "cat" → "cats"
- "dog" → "dogs"  
- "pen" → "pens"

**Test (Zero-shot):**
- "bat" → "bats" (never seen "bat" before)

**What this tests:**
- Can system extract the abstract rule "add 's' for plural"?
- Can it apply that rule to a novel word?

**Success criteria:**
- ≥75% accuracy on "bats" output
- Shows the system learned a **generalizable pattern**, not just memorized examples

---

### TEST 2: Past Tense (Rule Extraction)

**Training:**
- "walk" → "walked"
- "jump" → "jumped"
- "play" → "played"

**Test (Zero-shot):**
- "talk" → "talked"

**What this tests:**
- Can system extract the abstract rule "add 'ed' for past tense"?
- Different rule than pluralization (tests multiple rule learning)

**Success criteria:**
- ≥75% accuracy on "talked" output

---

### TEST 3: Context Discrimination

**Training:**
- "test" → "tests" (plural task)
- "test" → "tested" (past tense task)
- Alternating between tasks

**Test:**
- Given "test", can system produce either transformation?

**What this tests:**
- Can system learn **multiple transformations** for the same input?
- This is hard - requires context signals or task representation
- Current system may not pass this yet (needs context mechanism)

**Success criteria:**
- Produces either "tests" OR "tested" (partial pass)
- Ideally: Can choose correct one based on context (full pass)

---

### TEST 4: Pattern Reuse (Transfer Learning)

**Training:**
- "cat" → "cats"
- "bat" → "bats"

**Test (Zero-shot):**
- "rat" → "rats" (shares "at" pattern with "cat" and "bat")

**What this tests:**
- Can system recognize that "rat" shares the "at" pattern?
- Can it transfer the learned "at" → "ats" transformation?

**Success criteria:**
- ≥75% accuracy on "rats" output
- Shows **pattern-based generalization**, not just word-by-word memorization

---

### TEST 5: Composition (Combining Rules)

**Training:**
- Plural: "cat" → "cats", "dog" → "dogs"
- Past tense: "walk" → "walked", "jump" → "jumped"
- Mixed training (both tasks)

**Test:**
- Plural: "bat" → "bats"
- Past tense: "talk" → "talked"

**What this tests:**
- Can system handle **multiple different rules** simultaneously?
- Can it compose/combine learned transformations?
- This is the hardest test - requires flexible rule system

**Success criteria:**
- ≥70% accuracy on both tests
- Shows system can handle multiple transformation types

---

## Current Results

**Status:** 0/5 tests passed (7% overall accuracy)

**Why?**
- System is still learning basic associations
- Patterns are detected but not yet used for prediction
- No context mechanism for discrimination
- Output selection is still mostly random

**What's Working:**
- ✓ System runs without crashing
- ✓ Patterns are being detected
- ✓ Edges are being learned
- ✓ Circular regulation is stable

**What Needs Work:**
- ✗ Patterns need to influence output (not just detection)
- ✗ Output selection needs to use learned patterns
- ✗ Context representation for task discrimination
- ✗ Better pattern matching/prediction

---

## How to Run

```bash
# Compile test suite
gcc -o test test.c melvin.c -lm -std=c99 -Wall

# Run tests
./test
```

**Output format:**
- Training progress (error rate over episodes)
- Test input → expected output → actual output
- Accuracy percentage for each test
- Overall summary with pass/fail

---

## Interpreting Results

### Good Signs:
- Accuracy increasing over episodes
- Patterns detected and strengthening
- Output shows some similarity to target
- System stability maintained

### Bad Signs:
- Accuracy stuck at random (16% for 4 chars = random)
- Patterns detected but output doesn't use them
- System instability (crashes, explosions)
- Output completely unrelated to input

### Metrics:
- **0-25% accuracy**: Random/exploration (expected early in training)
- **25-50% accuracy**: Partial learning (some pattern recognition)
- **50-75% accuracy**: Strong learning (majority correct)
- **75-100% accuracy**: Successful (generalization working)

---

## Next Steps for Improvement

### 1. Pattern-Based Prediction
Patterns are detected but not used for output. Need:
- Pattern matching during propagation
- Pattern predictions influence node activation
- Multi-head patterns for context-dependent predictions

### 2. Better Output Selection
Current: Random sampling from probabilities
Needed: Use pattern predictions to guide selection

### 3. Context Representation
For context discrimination, need:
- Task/mode representation (what transformation am I doing?)
- Context gates pattern activation
- Separate contexts for different tasks

### 4. Hierarchical Patterns
Patterns should compose:
- "at" + "s" → "ats" pattern
- Abstract rule patterns, not just concrete sequences

### 5. More Training
Current tests use 40-60 episodes. May need:
- 100-1000 episodes for better learning
- Better learning signals
- More diverse training examples

---

## The Goal

These tests measure **intelligence** because they require:

1. **Abstraction**: Learning general rules, not specific examples
2. **Generalization**: Applying rules to novel inputs
3. **Flexibility**: Handling multiple tasks/rules
4. **Composition**: Combining learned knowledge

If Melvin can pass these tests, it demonstrates **true learning**, not just memorization.

---

## Comparison to Melvin o6

**Melvin o6 results:**
- 0-19% zero-shot accuracy
- Mostly echo/random behavior
- Failed all generalization tests

**Melvin o7 current:**
- 7% overall accuracy
- Still early - architecture is cleaner
- Better foundation for improvement

**The difference:**
- o6: Complex, hard to debug, many failure points
- o7: Simple, clean, easier to improve
- Circular regulation prevents common failure modes

---

**Built:** December 15, 2025  
**Status:** Test suite complete, system needs improvement  
**Next:** Implement pattern-based prediction, better output selection

