# PATTERN SELF-REGULATION & RULE-BASED GUIDANCE

## Overview
Patterns now serve **multiple purposes** and act as **self-regulating rules** that guide the system:

1. **Chunk Information**: Patterns compress sequences into reusable chunks
2. **Act Like Neural Nets**: Patterns predict next nodes/patterns (micro neural nets)
3. **Act Like If-Statements/Rules**: Patterns evaluate conditions and execute actions

## Pattern as Self-Regulating Rules

### IF-THEN Behavior
Patterns now act as **conditional rules**:
- **IF** condition pattern is active (above threshold)
- **THEN** boost/suppress target patterns
- Rules **self-regulate** their strength based on success/failure

### Self-Regulation Mechanisms

#### 1. Rule Success Tracking
- `rule_attempts`: How many times rules were evaluated
- `rule_successes`: How many times rules succeeded
- `rule_success_rate = successes / attempts`
- Patterns track whether their rules lead to correct outputs

#### 2. Rule Confidence
- `rule_confidence`: How reliable this pattern's rules are
- `rule_confidence = 0.5 + (rule_success_rate - 0.5) × 2.0`
- High success rate → high confidence → rules are reliable
- Low success rate → low confidence → rules are unreliable

#### 3. Activation Control Strength
- `activation_control_strength = rule_confidence × dynamic_importance`
- Successful patterns get **more control authority**
- Failed patterns get **less control authority**
- Patterns guide the system based on their reliability

#### 4. Boost/Suppression Strength
- `boost_strength = rule_confidence × 0.5`
- `suppression_strength = (1.0 - rule_confidence) × 0.3`
- **Successful patterns**: Boost more, suppress less
- **Failed patterns**: Boost less, suppress more (to prevent interference)

### Pattern-Guided Activation Control

When a pattern is active and has control authority (`activation_control_strength > 0.3`):

1. **Boost Associated Patterns**
   - Patterns boost patterns they're associated with
   - Boost strength = `activation × boost_strength × rule_confidence`
   - Successful patterns guide activation toward correct paths

2. **Suppress Competing Patterns**
   - Patterns suppress patterns with low success rate
   - Suppression strength = `activation × suppression_strength × rule_confidence`
   - Failed patterns are suppressed to prevent interference

### Self-Regulation Through Feedback

#### On Success:
- `rule_confidence` increases
- `rule_strengths` increase
- `rule_successes` increments
- Pattern gains more control authority

#### On Failure:
- `rule_confidence` decreases
- `rule_strengths` decrease
- Pattern loses control authority
- Pattern suppresses itself (less boost, more suppression)

## How Patterns Guide the System

### 1. Chunk Information
- Patterns compress sequences: "hello" → pattern P1
- Reduces graph complexity
- Enables generalization

### 2. Act Like Neural Nets
- Patterns predict: "IF P1 THEN predict 'world'"
- Predictions have weights (learned)
- Patterns learn what to predict

### 3. Act Like If-Statements/Rules
- Patterns evaluate: "IF condition THEN action"
- Rules self-regulate based on success
- Patterns guide activation flow

### Example: Pattern as Rule

```
Pattern P1: "hello" → "world"
- Rule: IF "hello" pattern active THEN boost "world" pattern
- Rule strength: 0.8 (learned from success)
- Rule confidence: 0.9 (high success rate)
- Control strength: 0.9 × 0.7 = 0.63 (high authority)

When "hello" is active:
- P1 evaluates: condition met (hello pattern active)
- P1 executes: boost "world" pattern by 0.8 × 0.9 = 0.72
- P1 guides: system activation flows toward "world"
```

## Self-Regulation Feedback Loop

1. **Pattern creates rule**: IF condition THEN action
2. **Rule is evaluated**: Condition checked, action executed
3. **Success/failure tracked**: Rule outcome recorded
4. **Rule strength adapts**: Successful rules get stronger, failed rules get weaker
5. **Pattern confidence updates**: High success → high confidence
6. **Control authority adjusts**: Confident patterns guide more, unconfident patterns guide less
7. **System improves**: Patterns that work get more control, patterns that fail get less control

## Key Insight

**Patterns are not just passive predictors - they are active controllers that guide the system.**

- Patterns **chunk** information (compression)
- Patterns **predict** (neural net behavior)
- Patterns **guide** (rule-based control)
- Patterns **self-regulate** (adapt based on success)

The system learns which patterns to trust and gives them more control authority. Failed patterns lose authority automatically. This creates a **self-organizing system** where successful patterns guide behavior and failed patterns are suppressed.
