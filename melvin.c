/* ============================================================================
 * MELVIN O7: Pure Circular Self-Regulation
 * 
 * CORE PRINCIPLE: No hardcoded limits. Everything is ratios and feedback loops.
 * 
 * DESIGN PHILOSOPHY:
 * - Every variable is RELATIVE (not absolute)
 * - Every variable INFLUENCES others
 * - Every variable IS INFLUENCED by others
 * - Emergence from stable attractors (not from fighting limits)
 * 
 * NO STATIC THRESHOLDS. NO MAX VALUES. NO ARBITRARY CUTOFFS.
 * ============================================================================ */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdint.h>
#include <stdbool.h>

/* ============================================================================
 * UNIVERSAL CONSTANTS (Only physics/math, not behavior)
 * ============================================================================ */

#define BYTE_VALUES 256        /* Physical constraint: bytes are 0-255 */
#define BLANK_NODE 256         /* Wildcard node - matches any byte (for generalization) */
#define INITIAL_CAPACITY 10000  /* Starting memory allocation (grows as needed) */

#define IS_BLANK_NODE(id) ((id) == BLANK_NODE)
#define MATCHES_BLANK(node_id, pattern_id) (IS_BLANK_NODE(pattern_id) || (node_id == pattern_id))

/* ============================================================================
 * NODE: Universal byte-level primitive
 * 
 * All state is RELATIVE or PROPORTIONAL:
 * - activation: purely local, calculated per node during wave propagation
 * - threshold: relative to average activation in system
 * - energy: proportion of maximum metabolic capacity
 * ============================================================================ */

typedef struct {
    /* Identity */
    uint8_t payload;           /* The byte value (0-255) this node represents */
    bool exists;               /* Has this node been created? */
    
    /* PORT TRACKING: Which port (modality) did this node come from? */
    uint32_t source_port;      /* Port ID where this node originated */
    /* Port types: 0=text, 1=audio, 2=vision, 3=motor, 4+=custom */
    /* Nodes track their origin - prevents cross-modality confusion */
    
    /* Circular Dynamic State (all relative/proportional) */
    float activation;          /* Current activation [0,1] - purely local, calculated per node */
    float threshold;           /* Firing threshold [0,1] - relative to avg */
    
    /* History (for computing derivatives/rates) */
    float prev_activation;     /* Previous step activation */
    float activation_momentum; /* Rate of change (derivative) */
    
    /* Statistics (for computing ratios) */
    uint64_t fire_count;       /* Times this node fired */
    uint64_t receive_count;    /* Times this node received input */
    
} Node;

/* ============================================================================
 * EDGE: Learned association between nodes
 * 
 * Weight is PROPORTION of total outgoing weight (not absolute)
 * Strength emerges from USE, not manual setting
 * ============================================================================ */

typedef struct {
    uint32_t to_id;            /* Target node or pattern ID */
    
    /* Weight is proportion of parent node's output */
    float weight;              /* [0,1] - share of parent's activation */
    
    /* Usage tracking (for computing relative importance) */
    uint64_t use_count;        /* Times this edge was traversed */
    uint64_t success_count;    /* Times traversal led to correct output */
    
    bool active;               /* Is this edge currently in use? */
    bool is_pattern_edge;      /* If true, to_id refers to pattern, not node */
    
} Edge;

/* ============================================================================
 * EDGE LIST: Dynamic array of edges
 * 
 * No MAX_EDGES - grows as needed
 * Pruning happens through METABOLIC COST, not arbitrary limits
 * ============================================================================ */

typedef struct {
    Edge *edges;              /* Dynamic array of edges */
    uint32_t count;           /* Current number of edges */
    uint32_t capacity;        /* Allocated capacity */
    float total_weight;       /* Sum of all edge weights (for normalization) */
    float metabolic_load;     /* Cost of maintaining these edges */
    
} EdgeList;

/* ============================================================================
 * PATTERN: Discovered sequence chunk
 * 
 * Strength is RELATIVE to other patterns
 * Utility emerges from prediction accuracy
 * ============================================================================ */

typedef struct {
    /* Identity */
    uint32_t *node_ids;        /* Dynamic array of node IDs in sequence (can include BLANK_NODE) */
    uint32_t length;           /* Length of sequence */
    
    /* HIERARCHICAL: Patterns can contain other patterns */
    uint32_t *sub_pattern_ids; /* Patterns this pattern is built from (for hierarchy) */
    uint32_t sub_pattern_count; /* How many sub-patterns */
    
    /* Relative strength (proportion of pattern space) */
    float strength;            /* [0,1] - relative to all patterns */
    
    /* Prediction tracking (for computing utility) */
    uint64_t prediction_attempts;
    uint64_t prediction_successes;
    
    /* Activation state (like a node - pattern acts as micro neural net) */
    float activation;          /* Current pattern activation [0,1] - purely local */
    float threshold;           /* Pattern firing threshold */
    
    /* PATTERN FIRING STATE (prevent continuous firing) */
    bool has_fired;            /* Has this pattern already fired for current input? */
    uint32_t last_fired_step;  /* Last step this pattern fired */
    uint32_t fired_predictions; /* Bitmask of which predictions were already used */
    
    /* MICRO NEURAL NET: Pattern acts as a small neural network */
    uint32_t *predicted_nodes; /* Nodes this pattern predicts (outputs) */
    float *prediction_weights; /* Weights for each prediction */
    uint32_t prediction_count; /* How many predictions */
    
    /* PATTERN-LEVEL PREDICTIONS: Patterns predict other patterns (concept-level) */
    /* This enables hierarchical reasoning: small patterns → larger patterns → concepts */
    uint32_t *predicted_patterns; /* Patterns this pattern predicts (concept-level) */
    float *pattern_prediction_weights; /* Weights for pattern predictions */
    uint32_t pattern_prediction_count; /* How many pattern predictions */
    
    /* NEURAL NET COMPONENTS: Proper weights and bias */
    float *input_weights;       /* Weights for input nodes (like W in neural net) */
    float bias;                 /* Bias term (like b in neural net) */
    uint32_t input_size;        /* Number of input nodes this pattern processes */
    
    /* PORT TRACKING: Pattern learned from specific port relationships */
    uint32_t input_port;        /* Port where pattern's input nodes came from */
    uint32_t output_port;       /* Port where pattern's predictions go */
    /* Patterns learn port-to-port relationships (text→text, audio→audio) */
    /* Prevents confusion: same bytes mean different things in different ports */
    /* Example: byte 65 from TEXT port = 'A', from AUDIO port = frequency value */
    
    /* MODALITY CONTEXT: Also store context vector for fine-grained matching */
    float context_vector[16];   /* Context encoding when pattern was learned */
    
    /* PATTERN-TO-PATTERN CONNECTIONS: Patterns connect to other patterns (like nodes) */
    EdgeList outgoing_patterns; /* Edges to other patterns (dynamic array) */
    EdgeList incoming_patterns; /* Edges from other patterns */
    
} Pattern;

/* ============================================================================
 * SYSTEM STATE: Global statistics for computing ratios
 * 
 * Everything here is COMPUTED, not set
 * Used for normalizing node/edge/pattern states
 * ============================================================================ */

typedef struct {
    /* Averages (denominators for ratios) */
    float avg_activation;      /* Average node activation */
    float avg_threshold;       /* Average node threshold */
    
    /* Sums (for computing proportions) */
    float total_activation;    /* Sum of all activations */
    float total_edge_weight;   /* Sum of all edge weights */
    float total_pattern_strength; /* Sum of all pattern strengths */
    
    /* Counts (for computing densities) */
    uint32_t active_node_count;
    uint32_t total_edge_count;
    uint32_t active_pattern_count;
    
    /* Rates (derivatives - how fast things change) */
    float activation_rate;     /* Change in total activation */
    float learning_rate;       /* Change in edge weights */
    float error_rate;          /* Proportion of incorrect predictions */
    
    /* Pressures (emergent from ratios) */
    float competition_pressure; /* How much nodes compete vs cooperate */
    float exploration_pressure; /* How much system explores vs exploits */
    
    /* CONTEXT REPRESENTATION (for context-dependent output) */
    float context_vector[16];  /* 16D context encoding current task/mode */
    
    /* SELF-TUNING PRESSURES (emerge from state, not set by us) */
    float learning_pressure;      /* From error_rate² (quadratic feedback) */
    float metabolic_pressure;     /* From graph density (too many connections = prune) */
    float loop_pressure;          /* From repetition detection (stuck in loop = escape) */
    float pattern_confidence;     /* From avg pattern utility (patterns working = trust them) */
    float output_variance;        /* How much do outputs vary? (chaos vs convergence) */
    float avg_pattern_utility;    /* Average success rate of patterns */
    
    /* Output history for variance and loop detection */
    uint32_t recent_outputs[50];  /* Last 50 output bytes */
    uint32_t output_history_index;
    
    /* Time (for computing rates) */
    uint64_t step;             /* Global step counter */
    
} SystemState;

/* RICH ERROR TRACKING: Component contributions per output position */
typedef struct {
    uint32_t pattern_id;
    float contribution;  /* How much this pattern contributed */
    uint32_t predicted;  /* What this pattern predicted */
} PatternContribution;

typedef struct {
    uint32_t from_node;
    float contribution;  /* Edge weight × source activation */
} EdgeContribution;

typedef struct {
    PatternContribution *patterns;
    uint32_t pattern_count;
    EdgeContribution *edges;
    uint32_t edge_count;
    float total_contribution;
} OutputContribution;

/* ============================================================================
 * MELVIN GRAPH: The complete system
 * 
 * All arrays DYNAMIC - no hardcoded limits
 * ============================================================================ */

typedef struct {
    /* Nodes (fixed size - naturally limited to 256 by byte values) */
    Node nodes[BYTE_VALUES];
    
    /* Edges (dynamic - one list per node) */
    EdgeList outgoing[BYTE_VALUES];
    EdgeList incoming[BYTE_VALUES];
    
    /* Patterns (dynamic array - grows as needed) */
    Pattern *patterns;
    uint32_t pattern_count;
    uint32_t pattern_capacity;
    
    /* System state (computed each step) */
    SystemState state;
    
    /* Input/Output buffers (dynamic) */
    uint32_t *input_buffer;
    uint32_t input_length;
    uint32_t input_capacity;
    
    uint32_t *output_buffer;
    uint32_t output_length;
    uint32_t output_capacity;
    
    /* RICH ERROR TRACKING: Component contributions per output position */
    OutputContribution *output_contributions;
    uint32_t output_contrib_capacity;
    
    /* PORT TRACKING: Current input/output ports */
    uint32_t current_input_port;   /* Current input port (0=text, 1=audio, etc.) */
    uint32_t current_output_port;  /* Current output port */
    
} MelvinGraph;

/* ============================================================================
 * FORWARD DECLARATIONS
 * ============================================================================ */

void compute_system_state(MelvinGraph *g);
void normalize_edge_weights(MelvinGraph *g, uint32_t node_id);
void update_node_dynamics(MelvinGraph *g, uint32_t node_id);
float compute_firing_probability(MelvinGraph *g, uint32_t node_id);
float compute_node_relevance(MelvinGraph *g, uint32_t node_id);
float melvin_get_edge_weight(MelvinGraph *g, uint32_t from_id, uint32_t to_id);
void melvin_set_context(MelvinGraph *g, float *context);
void melvin_set_input_port(MelvinGraph *g, uint32_t port_id);
void melvin_set_output_port(MelvinGraph *g, uint32_t port_id);
void inject_input_from_port(MelvinGraph *g, const uint8_t *bytes, uint32_t length, uint32_t port_id);
int melvin_save_brain(MelvinGraph *g, const char *filename);
MelvinGraph* melvin_load_brain(const char *filename);
void melvin_destroy(MelvinGraph *g);
bool pattern_matches(MelvinGraph *g, uint32_t pattern_id, const uint32_t *sequence, uint32_t seq_len, uint32_t start_pos);
float pattern_forward_pass(MelvinGraph *g, uint32_t pattern_id, const uint32_t *input_nodes, uint32_t input_len);
void propagate_pattern_activation(MelvinGraph *g);
void detect_generalized_patterns(MelvinGraph *g);
void learn_pattern_predictions(MelvinGraph *g, const uint8_t *target, uint32_t target_len);
void learn_pattern_sequences_automatic(MelvinGraph *g);
void pattern_backprop(MelvinGraph *g, uint32_t pattern_id, float error, const uint32_t *input_nodes, uint32_t input_len);
void create_edges_from_coactivation(MelvinGraph *g);
void create_edges_from_patterns(MelvinGraph *g);
void create_pattern_edges_from_coactivation(MelvinGraph *g);
void create_or_strengthen_pattern_edge(MelvinGraph *g, uint32_t from_pattern_id, uint32_t to_pattern_id);

/* ============================================================================
 * INITIALIZATION
 * ============================================================================ */

MelvinGraph* melvin_create(void) {
    MelvinGraph *g = calloc(1, sizeof(MelvinGraph));
    if (!g) return NULL;
    
    /* Initialize nodes (start with minimal energy/activation) */
    for (int i = 0; i < BYTE_VALUES; i++) {
        g->nodes[i].payload = (uint8_t)i;
        g->nodes[i].exists = false;
        g->nodes[i].activation = 0.0f;
        g->nodes[i].threshold = 0.5f;    /* Start at midpoint */
        g->nodes[i].prev_activation = 0.0f;
        g->nodes[i].activation_momentum = 0.0f;
        g->nodes[i].fire_count = 0;
        g->nodes[i].receive_count = 0;
        g->nodes[i].source_port = 0;  /* Default port */
    }
    
    /* Initialize edge lists */
    for (int i = 0; i < BYTE_VALUES; i++) {
        g->outgoing[i].edges = malloc(sizeof(Edge) * INITIAL_CAPACITY);
        g->outgoing[i].count = 0;
        g->outgoing[i].capacity = INITIAL_CAPACITY;
        g->outgoing[i].total_weight = 0.0f;
        g->outgoing[i].metabolic_load = 0.0f;
        
        g->incoming[i].edges = malloc(sizeof(Edge) * INITIAL_CAPACITY);
        g->incoming[i].count = 0;
        g->incoming[i].capacity = INITIAL_CAPACITY;
        g->incoming[i].total_weight = 0.0f;
        g->incoming[i].metabolic_load = 0.0f;
    }
    
    /* Initialize patterns */
    g->patterns = malloc(sizeof(Pattern) * INITIAL_CAPACITY);
    g->pattern_count = 0;
    g->pattern_capacity = INITIAL_CAPACITY;
    
    /* Initialize buffers */
    g->input_buffer = malloc(sizeof(uint32_t) * INITIAL_CAPACITY);
    g->input_length = 0;
    g->input_capacity = INITIAL_CAPACITY;
    
    g->output_buffer = malloc(sizeof(uint32_t) * INITIAL_CAPACITY);
    g->output_length = 0;
    g->output_capacity = INITIAL_CAPACITY;
    
    /* Initialize contribution tracking */
    g->output_contributions = malloc(sizeof(OutputContribution) * INITIAL_CAPACITY);
    g->output_contrib_capacity = INITIAL_CAPACITY;
    for (uint32_t i = 0; i < INITIAL_CAPACITY; i++) {
        g->output_contributions[i].patterns = NULL;
        g->output_contributions[i].pattern_count = 0;
        g->output_contributions[i].edges = NULL;
        g->output_contributions[i].edge_count = 0;
        g->output_contributions[i].total_contribution = 0.0f;
    }
    
    /* Initialize system state */
    g->state.step = 0;
    g->state.avg_activation = 0.5f;      /* Start at neutral */
    g->state.avg_threshold = 0.5f;
    g->state.competition_pressure = 0.5f;
    g->state.exploration_pressure = 0.5f;
    g->state.error_rate = 0.5f;          /* Maximum uncertainty initially */
    
    /* Initialize context vector (default: no specific modality, matches all) */
    for (int i = 0; i < 16; i++) {
        g->state.context_vector[i] = 0.0f;
    }
    
    /* Initialize self-tuning pressures */
    g->state.learning_pressure = 0.25f;  /* error_rate² = 0.5² = 0.25 */
    g->state.metabolic_pressure = 0.0f;
    g->state.loop_pressure = 0.0f;
    g->state.pattern_confidence = 0.5f;  /* Start at chance */
    g->state.avg_pattern_utility = 0.5f;
    g->state.output_variance = 1.0f;     /* High variance initially (random) */
    g->state.output_history_index = 0;
    for (int i = 0; i < 50; i++) {
        g->state.recent_outputs[i] = 0;
    }
    
    return g;
}

/* ============================================================================
 * SYSTEM STATE COMPUTATION
 * 
 * Compute all averages, sums, rates, pressures
 * These are the denominators for all ratios in the system
 * ============================================================================ */

void compute_system_state(MelvinGraph *g) {
    float total_act = 0.0f;
    float total_threshold = 0.0f;
    uint32_t active_count = 0;
    
    /* Compute sums and counts */
    for (int i = 0; i < BYTE_VALUES; i++) {
        if (!g->nodes[i].exists) continue;
        
        total_act += g->nodes[i].activation;
        total_threshold += g->nodes[i].threshold;
        
        if (g->nodes[i].activation > 0.0f) {
            active_count++;
        }
    }
    
    /* Compute averages (with safety for division by zero) */
    uint32_t existing_count = 0;
    for (int i = 0; i < BYTE_VALUES; i++) {
        if (g->nodes[i].exists) existing_count++;
    }
    
    if (existing_count > 0) {
        g->state.avg_activation = total_act / existing_count;
        g->state.avg_threshold = total_threshold / existing_count;
    }
    
    g->state.total_activation = total_act;
    g->state.active_node_count = active_count;
    
    /* Compute activation rate (change from last step) */
    static float prev_total_act = 0.0f;
    g->state.activation_rate = total_act - prev_total_act;
    prev_total_act = total_act;
    
    /* Compute competition pressure from activation distribution */
    /* High variance = high competition, low variance = cooperation */
    float variance = 0.0f;
    for (int i = 0; i < BYTE_VALUES; i++) {
        if (!g->nodes[i].exists) continue;
        float diff = g->nodes[i].activation - g->state.avg_activation;
        variance += diff * diff;
    }
    if (existing_count > 0) {
        variance /= existing_count;
    }
    
    /* Normalize variance to [0,1] pressure (sigmoid-like) */
    g->state.competition_pressure = 1.0f / (1.0f + expf(-10.0f * (variance - 0.5f)));
    
    /* SELF-TUNING FIX 2: Learning pressure = error_rate² (quadratic - aggressive when needed) */
    /* High error (0.9) → high pressure (0.81), Low error (0.1) → low pressure (0.01) */
    g->state.learning_pressure = g->state.error_rate * g->state.error_rate;
    
    /* Update learning rate based on learning pressure */
    g->state.learning_rate = 0.5f * g->state.learning_pressure;  /* Scales from 0.0 to 0.405 */
    
    /* Exploration pressure from error rate (high error = explore more) */
    g->state.exploration_pressure = g->state.error_rate;
    
    /* SELF-TUNING FIX 3: Metabolic pressure from graph density */
    /* Too many edges/patterns = high metabolic cost = pressure to prune */
    float edge_density = (g->state.total_edge_count > 0) ? 
        (float)g->state.total_edge_count / (BYTE_VALUES * 10.0f) : 0.0f;
    float pattern_density = (g->pattern_count > 0) ? 
        (float)g->pattern_count / 100.0f : 0.0f;
    g->state.metabolic_pressure = (edge_density + pattern_density) / 2.0f;
    if (g->state.metabolic_pressure > 1.0f) g->state.metabolic_pressure = 1.0f;
    
    g->state.step++;
}

/* ============================================================================
 * EDGE WEIGHT NORMALIZATION
 * 
 * All outgoing edges from a node sum to 1.0 (probability distribution)
 * This ensures weights can't explode - they're PROPORTIONS
 * ============================================================================ */

void normalize_edge_weights(MelvinGraph *g, uint32_t node_id) {
    EdgeList *out = &g->outgoing[node_id];
    
    /* Compute sum of all weights */
    float sum = 0.0f;
    for (uint32_t i = 0; i < out->count; i++) {
        if (out->edges[i].active) {
            sum += out->edges[i].weight;
        }
    }
    
    /* Normalize (divide by sum) */
    if (sum > 0.0f) {
        for (uint32_t i = 0; i < out->count; i++) {
            if (out->edges[i].active) {
                out->edges[i].weight /= sum;
            }
        }
    }
    
    out->total_weight = 1.0f; /* After normalization, always sums to 1 */
    
    /* Compute metabolic load (cost is quadratic in edge count) */
    /* Having many edges is expensive - encourages pruning */
    float density = (float)out->count / BYTE_VALUES;
    out->metabolic_load = density * density;
}

/* ============================================================================
 * NODE DYNAMICS UPDATE
 * 
 * Circular regulation:
 * - activation influenced by threshold and relative to average
 * - threshold adapts to activation history
 * - energy recovers based on activity
 * 
 * NO HARDCODED LIMITS - everything bounded by ratios
 * ============================================================================ */

void update_node_dynamics(MelvinGraph *g, uint32_t node_id) {
    Node *n = &g->nodes[node_id];
    if (!n->exists) return;
    
    /* ========================================================================
     * ACTIVATION UPDATE (PURELY LOCAL)
     * 
     * activation is calculated locally for this node
     * Wave prop: calculate activation → see where it goes → patterns activate → traverse
     * ======================================================================== */
    
    /* Relative activation (am I above or below average?) */
    float relative_activation = (g->state.avg_activation > 0.0f) ?
        n->activation / g->state.avg_activation : 1.0f;
    
    /* Compute activation momentum (derivative) */
    float activation_change = n->activation - n->prev_activation;
    n->activation_momentum = 0.9f * n->activation_momentum + 0.1f * activation_change;
    n->prev_activation = n->activation;
    
    /* Natural decay (prevents runaway activation) */
    /* But allow activation to accumulate along paths over multiple steps */
    float decay_rate = 0.95f + 0.05f * (1.0f - g->state.competition_pressure);
    n->activation *= decay_rate;
    
    /* ========================================================================
     * THRESHOLD ADAPTATION
     * 
     * threshold adapts to keep activation near average
     * If too active → increase threshold (less excitable)
     * If too quiet → decrease threshold (more excitable)
     * ======================================================================== */
    
    float target_activity_ratio = 1.0f; /* Want to be at average */
    float activity_error = relative_activation - target_activity_ratio;
    
    /* Adapt threshold proportional to error */
    float adaptation_rate = 0.01f * g->state.learning_rate;
    n->threshold += adaptation_rate * activity_error;
    
    /* Threshold naturally bounded [0,1] by sigmoid */
    n->threshold = 1.0f / (1.0f + expf(-5.0f * (n->threshold - 0.5f)));
    
    /* Activation bounded by threshold (must exceed threshold to be active) */
    /* No energy constraint - activation is purely local, calculated per node */
}

/* ============================================================================
 * FIRING PROBABILITY
 * 
 * Compute probability that this node should fire/output
 * Based on RELATIVE activation and threshold
 * 
 * Returns [0,1] probability (not a hard threshold)
 * ============================================================================ */

float compute_firing_probability(MelvinGraph *g, uint32_t node_id) {
    Node *n = &g->nodes[node_id];
    if (!n->exists) return 0.0f;
    
    /* Relative to average (above average = higher probability) */
    float relative_activation = (g->state.avg_activation > 0.0f) ?
        n->activation / g->state.avg_activation : 0.0f;
    
    /* Relative to threshold (must exceed threshold to fire) */
    float above_threshold = n->activation - n->threshold;
    
    /* Activation factor (activation itself determines firing) */
    float activation_factor = n->activation;
    
    /* Competition pressure (high competition = winner-take-all) */
    /* Low competition = multiple nodes can fire */
    float competition = g->state.competition_pressure;
    
    /* Combine factors with sigmoid (smooth probability) */
    float raw_probability = relative_activation * above_threshold * activation_factor;
    
    /* Apply competition sharpening */
    float sharpness = 5.0f * (1.0f + competition);
    float probability = 1.0f / (1.0f + expf(-sharpness * (raw_probability - 0.5f)));
    
    return probability;
}

/* ============================================================================
 * EDGE CREATION/STRENGTHENING
 * 
 * Create or strengthen edge from -> to
 * Weight starts small and grows with use
 * All weights normalized after creation (no explosion)
 * ============================================================================ */

void create_or_strengthen_edge(MelvinGraph *g, uint32_t from_id, uint32_t to_id) {
    /* PORT-AWARE EDGE CREATION: Only create edges within same port */
    /* Cross-port edges are weaker (prevents modality confusion) */
    uint32_t from_port = g->nodes[from_id].source_port;
    uint32_t to_port = g->nodes[to_id].source_port;
    
    /* If ports don't match, reduce edge strength (but still allow learning) */
    float port_penalty = (from_port == to_port) ? 1.0f : 0.3f;
    EdgeList *out = &g->outgoing[from_id];
    
    /* Find existing edge */
    for (uint32_t i = 0; i < out->count; i++) {
        if (out->edges[i].to_id == to_id && out->edges[i].active) {
            /* Strengthen existing edge (relative to current weight) */
            float growth_rate = 0.1f * g->state.learning_rate;
            out->edges[i].weight += growth_rate * (1.0f - out->edges[i].weight);
            out->edges[i].use_count++;
            
            /* Renormalize all weights */
            normalize_edge_weights(g, from_id);
            return;
        }
    }
    
    /* Create new edge (start with small weight) */
    if (out->count >= out->capacity) {
        /* Grow array */
        out->capacity *= 2;
        out->edges = realloc(out->edges, sizeof(Edge) * out->capacity);
    }
    
    Edge *e = &out->edges[out->count];
    e->to_id = to_id;
    e->weight = 0.01f; /* Start small */
    e->use_count = 1;
    e->success_count = 0;
    e->active = true;
    
    out->count++;
    
    /* Normalize (new edge takes proportion from existing edges) */
    normalize_edge_weights(g, from_id);
    
    /* Metabolic load increases with edge count */
    /* This creates natural pressure to prune weak edges */
    float density = (float)out->count / BYTE_VALUES;
    out->metabolic_load = density * density;
}

/* ============================================================================
 * EDGE PRUNING (METABOLIC)
 * 
 * Weak edges die when metabolic load is too high
 * No arbitrary MAX_EDGES - pruning emerges from cost/benefit
 * ============================================================================ */

void prune_weak_edges(MelvinGraph *g, uint32_t node_id) {
    EdgeList *out = &g->outgoing[node_id];
    
    /* Only prune if metabolic load is high */
    if (out->metabolic_load < 0.5f) return;
    
    /* Compute survival threshold (edges below this die) */
    /* Higher load = higher threshold = more aggressive pruning */
    float survival_threshold = out->metabolic_load * 0.1f;
    
    /* Mark weak edges as inactive */
    for (uint32_t i = 0; i < out->count; i++) {
        if (!out->edges[i].active) continue;
        
        /* Edge strength relative to metabolic cost */
        float strength = out->edges[i].weight;
        float cost = out->metabolic_load / out->count;
        float value = strength / (cost + 0.001f);
        
        if (value < survival_threshold) {
            out->edges[i].active = false;
        }
    }
    
    /* Renormalize remaining edges */
    normalize_edge_weights(g, node_id);
}

/* ============================================================================
 * PATTERN MATCHING (with blank node support)
 * 
 * Check if a pattern matches a sequence, handling blank nodes as wildcards
 * ============================================================================ */

/* Compute context similarity (dot product, normalized) */
float context_similarity(const float *ctx1, const float *ctx2) {
    float dot = 0.0f;
    float mag1 = 0.0f;
    float mag2 = 0.0f;
    
    for (int i = 0; i < 16; i++) {
        dot += ctx1[i] * ctx2[i];
        mag1 += ctx1[i] * ctx1[i];
        mag2 += ctx2[i] * ctx2[i];
    }
    
    if (mag1 < 0.001f || mag2 < 0.001f) return 0.0f;  /* No context = no match */
    return dot / (sqrtf(mag1) * sqrtf(mag2) + 0.001f);  /* Cosine similarity */
}

bool pattern_matches(MelvinGraph *g, uint32_t pattern_id, const uint32_t *sequence, uint32_t seq_len, uint32_t start_pos) {
    Pattern *pat = &g->patterns[pattern_id];
    
    if (start_pos + pat->length > seq_len) {
        return false;
    }
    
    /* AUTO-LEARNED PORT CHECK: Check ports from actual nodes in sequence */
    /* Ports are learned from node source_port - no manual setting needed */
    /* Pattern matches if sequence nodes have matching ports */
    if (start_pos < seq_len && sequence[start_pos] < BYTE_VALUES) {
        /* Check if sequence nodes match pattern's learned port */
        /* Get port from first node in sequence (most reliable) */
        uint32_t seq_port = g->nodes[sequence[start_pos]].source_port;
        /* Pattern matches if it learned from the same port as the sequence */
        /* This allows automatic port differentiation without manual setting */
        if (pat->input_port != seq_port) {
            return false;  /* Port mismatch - pattern learned from different port */
        }
    }
    
    /* Also check context similarity for fine-grained matching */
    float context_sim = context_similarity(pat->context_vector, g->state.context_vector);
    if (context_sim < 0.3f && context_sim > 0.001f) {  /* Allow zero context (no modality set) */
        return false;  /* Context mismatch - pattern doesn't apply to current modality */
    }
    
    /* Check each position, allowing blank nodes to match anything */
    for (uint32_t i = 0; i < pat->length; i++) {
        uint32_t seq_node = sequence[start_pos + i];
        uint32_t pat_node = pat->node_ids[i];
        
        if (!MATCHES_BLANK(seq_node, pat_node)) {
            return false;
        }
    }
    
    return true;
}

/* ============================================================================
 * PATTERN FORWARD PASS (Mini Neural Net)
 * 
 * Compute pattern activation using neural net formula:
 * output = sigmoid(sum(inputs × weights) + bias)
 * ============================================================================ */

float pattern_forward_pass(MelvinGraph *g, uint32_t pattern_id, const uint32_t *input_nodes, uint32_t input_len) {
    Pattern *pat = &g->patterns[pattern_id];
    
    /* Initialize weights if needed (first time pattern sees input) */
    if (pat->input_weights == NULL && input_len > 0) {
        pat->input_size = input_len;
        pat->input_weights = malloc(sizeof(float) * input_len);
        
        /* SMART INITIALIZATION: Use existing edge knowledge, not random! */
        /* Pattern represents a sequence - initialize from edge weights in that sequence */
        for (uint32_t i = 0; i < input_len; i++) {
            uint32_t node_id = input_nodes[i];
            
            if (node_id < BYTE_VALUES && g->nodes[node_id].exists) {
                /* Use average outgoing edge weight from this node */
                EdgeList *out = &g->outgoing[node_id];
                float avg_weight = 0.0f;
                uint32_t active_edges = 0;
                
                for (uint32_t e = 0; e < out->count; e++) {
                    if (out->edges[e].active) {
                        avg_weight += out->edges[e].weight;
                        active_edges++;
                    }
                }
                
                if (active_edges > 0) {
                    avg_weight /= active_edges;
                    /* Initialize weight proportional to existing edge strength */
                    pat->input_weights[i] = avg_weight - 0.5f;  /* Center around 0 */
                } else {
                    /* No edges yet - start at zero (learn from first example) */
                    pat->input_weights[i] = 0.0f;
                }
            } else {
                /* Blank node or new node - neutral */
                pat->input_weights[i] = 0.0f;
            }
        }
        
        /* Bias starts at 0 (neutral) */
        pat->bias = 0.0f;
    }
    
    if (pat->input_weights == NULL || input_len == 0) {
        return 0.0f;
    }
    
    /* Forward pass: sum(inputs × weights) + bias */
    float weighted_sum = pat->bias;
    for (uint32_t i = 0; i < input_len && i < pat->input_size; i++) {
        uint32_t node_id = input_nodes[i];
        if (node_id < BYTE_VALUES && g->nodes[node_id].exists) {
            /* Get input value (node activation) */
            float input_value = g->nodes[node_id].activation;
            /* Multiply by weight and add to sum */
            weighted_sum += input_value * pat->input_weights[i];
        }
    }
    
    /* Activation function: sigmoid */
    float output = 1.0f / (1.0f + expf(-weighted_sum));
    
    return output;
}

/* ============================================================================
 * PATTERN-BASED WAVE PROPAGATION
 * 
 * Patterns act as micro neural nets - when they match, they influence nodes
 * ============================================================================ */

void propagate_pattern_activation(MelvinGraph *g) {
    /* Reset pattern firing state at start of each propagation step */
    /* (patterns can fire once per step, but tracking prevents continuous loops) */
    
    /* Check all patterns against current input/output state */
    for (uint32_t p = 0; p < g->pattern_count; p++) {
        Pattern *pat = &g->patterns[p];
        
        /* Reset firing state if pattern hasn't fired recently */
        /* OR: Reset if we're now generating output (allow patterns to fire for output matching) */
        if (g->state.step > pat->last_fired_step + 5 || 
            (g->output_length > 0 && g->state.step > pat->last_fired_step)) {
            /* Allow pattern to fire again if output has grown since last firing */
            /* This lets patterns activate when they match output sequence */
            if (g->output_length > 0 && g->output_length > pat->last_fired_step) {
                /* Reset firing state if output has changed */
                pat->has_fired = false;
                /* Keep fired_predictions to prevent repeating same predictions */
            } else if (g->state.step > pat->last_fired_step + 5) {
                /* Full reset if enough time has passed */
                pat->has_fired = false;
                pat->fired_predictions = 0;
            }
        }
        
        /* Patterns activate when they match INPUT (initial) or OUTPUT (continuation) */
        /* This allows patterns to participate in wave propagation from the start */
        
        bool can_fire = false;
        uint32_t *match_sequence = NULL;
        uint32_t match_len = 0;
        
        /* Priority 1: Match END of output (continuation during wave propagation) */
        if (g->output_length >= pat->length) {
            uint32_t start_pos = g->output_length - pat->length;
            if (pattern_matches(g, p, g->output_buffer, g->output_length, start_pos)) {
                can_fire = true;
                match_sequence = &g->output_buffer[start_pos];
                match_len = pat->length;
            }
        }
        
        /* Priority 2: Match ANYWHERE in input (context-aware matching) */
        /* Patterns should match longer sequences from input, not just the end */
        /* This allows patterns to use full question context, not just last few chars */
        if (!can_fire && g->input_length >= pat->length) {
            /* Try matching from end of input (most relevant) backwards */
            /* Longer matches = more context = stronger activation */
            float best_match_strength = 0.0f;
            uint32_t best_match_pos = 0;
            
            for (int pos = g->input_length - pat->length; pos >= 0; pos--) {
                if (pattern_matches(g, p, g->input_buffer, g->input_length, pos)) {
                    /* Match strength = position relevance (end is more relevant) + length bonus */
                    float position_relevance = (float)(pos + pat->length) / (float)g->input_length;
                    float length_bonus = (float)pat->length / 10.0f;  /* Longer patterns = more context */
                    float match_strength = position_relevance + length_bonus;
                    
                    if (match_strength > best_match_strength) {
                        best_match_strength = match_strength;
                        best_match_pos = pos;
                        can_fire = true;
                    }
                }
            }
            
            if (can_fire) {
                match_sequence = &g->input_buffer[best_match_pos];
                match_len = pat->length;
                /* Store match strength for context boost */
                pat->activation = best_match_strength * 0.5f;  /* Pre-boost based on context match */
            }
        }
        
        /* Pattern fires if it matches (can fire MULTIPLE times per episode at different positions) */
        /* Remove has_fired restriction - patterns should predict whenever they match */
        if (can_fire) {
            /* Use the matched sequence (from input or output) */
            uint32_t *input_nodes = match_sequence;
            uint32_t input_len = match_len;
            
            /* Forward pass: compute activation using neural net */
            float net_output = pattern_forward_pass(g, p, input_nodes, input_len);
            
            /* CONTEXT BOOST: Patterns that match longer input sequences get stronger */
            /* This makes patterns context-aware - "the" in "What is the capital" is different from "the" alone */
            float context_boost = 1.0f;
            if (match_sequence == g->input_buffer && g->input_length > pat->length) {
                /* Pattern matched input - boost based on how much input context it covers */
                float context_coverage = (float)pat->length / (float)g->input_length;
                context_boost = 1.0f + context_coverage * 0.5f;  /* Up to 1.5x boost for full context */
            }
            
            pat->activation = net_output * pat->strength * context_boost;
            
            /* Pattern activation bounded by threshold (no energy constraint) */
            
            /* Pattern predicts next nodes (micro neural net output) */
            if (pat->prediction_count > 0) {
                for (uint32_t pred = 0; pred < pat->prediction_count; pred++) {
                    uint32_t target_node = pat->predicted_nodes[pred];
                    float weight = pat->prediction_weights[pred];
                    
                    /* Create node if doesn't exist */
                    if (target_node < BYTE_VALUES && !g->nodes[target_node].exists) {
                        g->nodes[target_node].exists = true;
                        g->nodes[target_node].activation = 0.0f;
                        g->nodes[target_node].threshold = g->state.avg_threshold;
                    }
                    
                    /* Pattern activation spreads to predicted nodes */
                    /* BUT: Only if this prediction hasn't been used yet */
                    bool prediction_used = (pat->fired_predictions & (1u << pred)) != 0;
                    
                    if (target_node < BYTE_VALUES && !prediction_used) {
                        /* Track prediction attempts (for utility calculation) */
                        pat->prediction_attempts++;
                        
                        /* INTELLIGENT PATH: Patterns are learned paths - follow them STRONGLY */
                        /* Patterns are learned intelligence - they predict where activation should go */
                        /* Transfer based on pattern activation, prediction weight, and pattern strength */
                        float transfer = pat->activation * weight * pat->strength;
                        
                        /* STRONG BOOST: Patterns are learned intelligence, not random */
                        /* Active patterns = intelligent paths = strong activation boost */
                        /* This is the key: patterns guide activation to correct nodes */
                        float intelligent_path_boost = 3.0f;  /* Strong boost for pattern predictions */
                        transfer *= intelligent_path_boost;
                        
                        g->nodes[target_node].activation += transfer;
                        g->nodes[target_node].receive_count++;
                    }
                }
                
                /* PATTERN-LEVEL PREDICTIONS: Patterns predict other patterns (concept-level reasoning) */
                /* This is the key to scaling: patterns compose into concepts */
                if (pat->pattern_prediction_count > 0) {
                    for (uint32_t ppred = 0; ppred < pat->pattern_prediction_count; ppred++) {
                        uint32_t target_pattern_id = pat->predicted_patterns[ppred];
                        if (target_pattern_id >= g->pattern_count) continue;
                        
                        Pattern *target_pat = &g->patterns[target_pattern_id];
                        float pattern_pred_weight = pat->pattern_prediction_weights[ppred];
                        
                        /* Transfer activation from this pattern to predicted pattern */
                        /* This creates pattern chains: A → B → C = concept formation */
                        float pattern_transfer = pat->activation * pattern_pred_weight * pat->strength;
                        target_pat->activation += pattern_transfer;
                        
                        /* Pattern activation bounded by threshold (no energy constraint) */
                        
                        /* Pattern predictions are learned associations (like node predictions) */
                        /* Stronger than edges: direct learned predictions */
                    }
                }
                
                /* PATTERN-TO-PATTERN ACTIVATION: Patterns activate other patterns through edges */
                /* Edges are learned from co-activation, predictions are learned from sequences */
                EdgeList *out_patterns = &pat->outgoing_patterns;
                for (uint32_t pe = 0; pe < out_patterns->count; pe++) {
                    if (!out_patterns->edges[pe].active || !out_patterns->edges[pe].is_pattern_edge) continue;
                    
                    uint32_t target_pattern_id = out_patterns->edges[pe].to_id;
                    if (target_pattern_id >= g->pattern_count) continue;
                    
                    Pattern *target_pat = &g->patterns[target_pattern_id];
                    float pattern_weight = out_patterns->edges[pe].weight;
                    
                    /* Transfer activation from this pattern to target pattern */
                    float pattern_transfer = pat->activation * pattern_weight * pat->strength;
                    target_pat->activation += pattern_transfer;
                    
                    /* Pattern activation bounded by threshold (no energy constraint) */
                    
                    out_patterns->edges[pe].use_count++;
                }
                
                /* Track firing (for utility, not restriction) */
                pat->last_fired_step = g->state.step;
                /* Don't set has_fired=true - allow multiple fires per episode */
            }
        } else {
            /* Pattern doesn't match or already fired - decay activation */
            pat->activation *= 0.95f;
        }
        
        /* Pattern activation decays (like nodes) */
        /* Decay rate relative to system state */
        float decay_rate = 0.95f * (1.0f - g->state.competition_pressure * 0.1f);
        pat->activation *= decay_rate;
    }
}

/* ============================================================================
 * DETECT GENERALIZED PATTERNS (with blank nodes)
 * 
 * Find patterns like "_at" that match "cat", "bat", "rat"
 * ============================================================================ */

void detect_generalized_patterns(MelvinGraph *g) {
    if (g->input_length < 3) return;
    
    /* Look for patterns with one blank: _at, c_t, ca_ */
    for (uint32_t i = 0; i < g->input_length - 2; i++) {
        uint32_t b = g->input_buffer[i + 1];
        uint32_t c = g->input_buffer[i + 2];
        
        /* Try pattern: _bc (blank first) */
        uint32_t match_count = 0;
        
        for (uint32_t j = 0; j < g->input_length - 2; j++) {
            if (g->input_buffer[j + 1] == b && g->input_buffer[j + 2] == c) {
                match_count++;
            }
        }
        
        /* Pattern creation threshold: relative to system state */
        float pattern_threshold = 2.0f * (1.0f - g->state.error_rate);
        if (pattern_threshold < 1.5f) pattern_threshold = 1.5f;
        if (pattern_threshold > 3.0f) pattern_threshold = 3.0f;
        
        if (match_count >= (uint32_t)pattern_threshold) {
            /* Check if pattern already exists */
            bool exists = false;
            for (uint32_t p = 0; p < g->pattern_count; p++) {
                Pattern *pat = &g->patterns[p];
                if (pat->length == 3 &&
                    pat->node_ids[0] == BLANK_NODE &&
                    pat->node_ids[1] == b &&
                    pat->node_ids[2] == c) {
                    exists = true;
                    /* Strengthen relative to learning rate (not fixed increment) */
                    /* Also update utility if pattern made predictions */
                    if (pat->prediction_attempts > 0) {
                        float utility = (float)pat->prediction_successes / (float)pat->prediction_attempts;
                        pat->strength += 0.1f * g->state.learning_rate * utility;  /* Stronger if useful */
                    } else {
                        pat->strength += 0.1f * g->state.learning_rate;
                    }
                    break;
                }
            }
            
            if (!exists) {
                /* Grow pattern array if needed */
                if (g->pattern_count >= g->pattern_capacity) {
                    g->pattern_capacity *= 2;
                    g->patterns = realloc(g->patterns, sizeof(Pattern) * g->pattern_capacity);
                }
                
                /* Create generalized pattern */
                Pattern *pat = &g->patterns[g->pattern_count++];
                pat->node_ids = malloc(sizeof(uint32_t) * 3);
                pat->node_ids[0] = BLANK_NODE;
                pat->node_ids[1] = b;
                pat->node_ids[2] = c;
                pat->length = 3;
                
                /* Initialize fields */
                pat->sub_pattern_ids = NULL;
                pat->sub_pattern_count = 0;
                pat->predicted_nodes = NULL;
                pat->prediction_weights = NULL;
                pat->prediction_count = 0;
                pat->predicted_patterns = NULL;
                pat->pattern_prediction_weights = NULL;
                pat->pattern_prediction_count = 0;
                /* Initialize relative to system state */
                pat->threshold = g->state.avg_threshold;
                pat->input_weights = NULL;
                pat->bias = 0.0f;
                pat->input_size = 0;
                
                /* Strength from generalization benefit (blank nodes enable compression across variants) */
                float pattern_cost = 1.5f + (pat->prediction_count * 0.15f);
                float variants_compressed = match_count - 1;  /* Pattern replaces multiple variants */
                float generalization_benefit = (variants_compressed * 3.0f) - pattern_cost;
                
                /* Utility from actual prediction performance */
                float utility = 0.5f;
                if (pat->prediction_attempts > 0) {
                    utility = (float)pat->prediction_successes / (float)pat->prediction_attempts;
                }
                
                /* Generalized patterns are MORE valuable - they match multiple variants */
                float base_strength = utility;
                if (generalization_benefit > 1.0f) {
                    base_strength *= 1.8f;  /* Stronger boost for generalization */
                }
                if (base_strength > 1.0f) base_strength = 1.0f;
                if (base_strength < 0.1f) base_strength = 0.1f;
                pat->strength = base_strength * (1.0f + g->state.error_rate);
                pat->activation = g->state.avg_activation * 0.2f;
                pat->prediction_attempts = 0;
                pat->prediction_successes = 0;
                pat->has_fired = false;
                pat->last_fired_step = 0;
                pat->fired_predictions = 0;
                
                /* PORT AUTO-LEARNING: Learn ports from nodes (same as regular patterns) */
                uint32_t port_counts[256] = {0};
                for (uint32_t i = 0; i < pat->length; i++) {
                    if (pat->node_ids[i] < BYTE_VALUES && pat->node_ids[i] != BLANK_NODE) {
                        uint32_t node_port = g->nodes[pat->node_ids[i]].source_port;
                        port_counts[node_port]++;
                    }
                }
                uint32_t most_common_port = 0;
                uint32_t max_count = 0;
                for (uint32_t p = 0; p < 256; p++) {
                    if (port_counts[p] > max_count) {
                        max_count = port_counts[p];
                        most_common_port = p;
                    }
                }
                pat->input_port = most_common_port;
                pat->output_port = most_common_port;
                
                /* Initialize pattern-to-pattern edge lists */
                pat->outgoing_patterns.edges = malloc(sizeof(Edge) * INITIAL_CAPACITY);
                pat->outgoing_patterns.count = 0;
                pat->outgoing_patterns.capacity = INITIAL_CAPACITY;
                pat->outgoing_patterns.total_weight = 0.0f;
                pat->outgoing_patterns.metabolic_load = 0.0f;
                
                pat->incoming_patterns.edges = malloc(sizeof(Edge) * INITIAL_CAPACITY);
                pat->incoming_patterns.count = 0;
                pat->incoming_patterns.capacity = INITIAL_CAPACITY;
                pat->incoming_patterns.total_weight = 0.0f;
                pat->incoming_patterns.metabolic_load = 0.0f;
            }
        }
    }
}

/* ============================================================================
 * WAVE PROPAGATION STEP
 * 
 * Propagate activation through edges AND patterns (micro neural nets)
 * Competition emerges from firing probabilities (not hardcoded winner-take-all)
 * ============================================================================ */

void propagate_activation(MelvinGraph *g) {
    /* ========================================================================
     * INTELLIGENT WAVE PROPAGATION: Follow learned paths like fungi
     * 
     * Core Principles:
     * 1. PATH-AWARE: Only follow learned edges (strong connections)
     * 2. PATTERN-GUIDED: Active patterns boost their predicted nodes
     * 3. CONTEXT-AWARE: Activation influenced by input and history
     * 4. FORWARD-LOOKING: Activation anticipates next nodes (predictive)
     * 5. SELECTIVE: Prioritize strong paths over weak/random ones
     * ======================================================================== */
    
    /* PHASE 1: PATTERN-GUIDED PROPAGATION (Patterns predict next nodes) */
    /* Patterns are learned intelligence - they know where activation should go */
    /* This happens FIRST so patterns can guide edge-based propagation */
    
    /* PHASE 2: EDGE-BASED PROPAGATION (Follow learned edges) */
    /* For each active node, spread activation to neighbors via learned edges */
    for (int i = 0; i < BYTE_VALUES; i++) {
        if (!g->nodes[i].exists) continue;
        /* Skip nodes with negligible activation - threshold relative to system */
        float activation_floor = g->state.avg_activation * 0.1f;  /* 10% of average */
        if (g->nodes[i].activation < activation_floor) continue;
        
        EdgeList *out = &g->outgoing[i];
        
        /* ========================================================================
         * WELL-DEFINED PATH QUALITY: Information Flow Efficiency
         * 
         * Path Quality = Information_Carried × Learning_Strength × Coherence × Predictive_Power
         * 
         * A "better path" is one that efficiently carries information from input/context
         * to meaningful output, represents learned associations, and has predictive power.
         * ======================================================================== */
        
        /* First pass: Calculate well-defined path quality for each edge */
        float path_qualities[256];  /* Max edges per node */
        float total_path_quality = 0.0f;
        
        for (uint32_t j = 0; j < out->count && j < 256; j++) {
            if (!out->edges[j].active) {
                path_qualities[j] = 0.0f;
                continue;
            }
            
            uint32_t target = out->edges[j].to_id;
            Edge *edge = &out->edges[j];
            
            /* ========================================================================
             * FACTOR 1: Information_Carried
             * How much information does this path carry from input/context to target?
             * ======================================================================== */
            float input_connection = 0.1f;  /* Default: weak connection */
            for (uint32_t inp = 0; inp < g->input_length; inp++) {
                uint32_t input_node = g->input_buffer[inp];
                if (input_node < BYTE_VALUES && g->nodes[input_node].exists) {
                    EdgeList *input_out = &g->outgoing[input_node];
                    for (uint32_t e = 0; e < input_out->count; e++) {
                        if (input_out->edges[e].to_id == target && input_out->edges[e].active) {
                            input_connection = 1.0f;  /* Strong: reachable from input */
                            break;
                        }
                    }
                }
            }
            
            float context_match = 0.5f;  /* Default: no pattern match */
            for (uint32_t p = 0; p < g->pattern_count; p++) {
                Pattern *pat = &g->patterns[p];
                if (pat->activation > pat->threshold && pat->activation > 0.1f) {
                    for (uint32_t pred = 0; pred < pat->prediction_count; pred++) {
                        if (pat->predicted_nodes[pred] == target) {
                            context_match = 1.0f;  /* Strong: pattern matches context */
                            break;
                        }
                    }
                }
            }
            
            float history_coherence = 0.8f;  /* Default: doesn't follow from output */
            if (g->output_length > 0) {
                uint32_t last_output = g->output_buffer[g->output_length - 1];
                if (last_output < BYTE_VALUES && g->nodes[last_output].exists) {
                    EdgeList *last_out = &g->outgoing[last_output];
                    for (uint32_t e = 0; e < last_out->count; e++) {
                        if (last_out->edges[e].to_id == target && last_out->edges[e].active) {
                            history_coherence = 1.0f;  /* Strong: follows from output */
                            break;
                        }
                    }
                }
            }
            
            float information = input_connection * context_match * history_coherence;
            
            /* ========================================================================
             * FACTOR 2: Learning_Strength
             * How well-learned is this path? (not random, but reinforced through training)
             * ======================================================================== */
            float edge_weight = edge->weight;  /* Learned strength (0.0 to 1.0) */
            float usage = logf(1.0f + edge->use_count) / 10.0f;  /* Log scale, normalized */
            float success_rate = (edge->use_count > 0) ? 
                ((float)edge->success_count / (float)edge->use_count) : 0.5f;  /* Historical accuracy */
            
            float learning = edge_weight * (1.0f + usage) * (0.5f + success_rate);
            
            /* ========================================================================
             * FACTOR 3: Coherence
             * Does this path form a coherent sequence? (makes sense contextually)
             * ======================================================================== */
            float pattern_alignment = 0.7f;  /* Default: not in pattern */
            for (uint32_t p = 0; p < g->pattern_count; p++) {
                Pattern *pat = &g->patterns[p];
                if (pat->activation > pat->threshold && pat->activation > 0.1f) {
                    for (uint32_t pred = 0; pred < pat->prediction_count; pred++) {
                        if (pat->predicted_nodes[pred] == target) {
                            pattern_alignment = 1.0f;  /* Strong: follows pattern */
                            break;
                        }
                    }
                }
            }
            
            float sequential_flow = history_coherence;  /* Same as history_coherence */
            float context_fit = context_match;  /* Same as context_match */
            
            float coherence = pattern_alignment * sequential_flow * context_fit;
            
            /* ========================================================================
             * FACTOR 4: Predictive_Power
             * How well does this path predict correct outputs?
             * ======================================================================== */
            float pattern_prediction = 0.3f;  /* Default: not predicted */
            for (uint32_t p = 0; p < g->pattern_count; p++) {
                Pattern *pat = &g->patterns[p];
                if (pat->activation > pat->threshold && pat->activation > 0.1f) {
                    for (uint32_t pred = 0; pred < pat->prediction_count; pred++) {
                        if (pat->predicted_nodes[pred] == target) {
                            /* Pattern confidence = activation × strength */
                            pattern_prediction = pat->activation * pat->strength;
                            break;
                        }
                    }
                }
            }
            
            float historical_accuracy = success_rate;  /* From learning strength */
            float context_prediction = context_match;  /* From information */
            
            float predictive = pattern_prediction * (0.5f + historical_accuracy) * context_prediction;
            
            /* ========================================================================
             * COMBINE: Path Quality = Information × Learning × Coherence × Predictive
             * Well-defined measure: all factors must be good for high quality
             * ======================================================================== */
            path_qualities[j] = information * learning * coherence * predictive;
            total_path_quality += path_qualities[j];
        }
        
        /* Normalize path qualities (relative comparison) */
        /* Stronger paths get proportionally more activation, not just yes/no */
        float normalization = (total_path_quality > 0.0f) ? (1.0f / total_path_quality) : 1.0f;
        
        /* Second pass: Propagate activation based on path quality (continuous, not binary) */
        for (uint32_t j = 0; j < out->count && j < 256; j++) {
            if (!out->edges[j].active || path_qualities[j] <= 0.0f) continue;
            
            uint32_t target = out->edges[j].to_id;
            
            /* Create target node if doesn't exist */
            if (!g->nodes[target].exists) {
                g->nodes[target].exists = true;
                g->nodes[target].activation = 0.0f;
                g->nodes[target].threshold = g->state.avg_threshold;
            }
            
            /* Transfer activation proportional to path quality (normalized) */
            /* Better paths get more activation, but all paths get some if quality > 0 */
            float normalized_quality = path_qualities[j] * normalization;
            float transfer = g->nodes[i].activation * normalized_quality;
            
            /* PATH ACCUMULATION: Activation accumulates along paths */
            /* Multiple steps = activation builds up at end of good paths */
            /* This creates "brightest light" at end of best learned paths */
            g->nodes[target].activation += transfer;
            g->nodes[target].receive_count++;
            out->edges[j].use_count++;
            
            /* CREATE EDGE IF ACTIVATION TRANSFER IS STRONG (wave propagation creates connections) */
            /* If activation successfully transferred, strengthen the connection */
            /* Edge creation threshold relative to system state */
            float transfer_threshold = 0.05f * g->state.learning_rate;
            float activation_threshold = g->state.avg_activation * 0.2f;
            if (transfer > transfer_threshold && g->nodes[target].activation > activation_threshold) {
                /* Edge already exists (we're using it), but strengthen it */
                /* This happens automatically in create_or_strengthen_edge */
                /* But we can also create reverse edge (bidirectional learning) */
                /* PREVENT SELF-LOOPS: Don't create edge from node to itself */
                if (i != target && g->nodes[target].activation > g->nodes[i].activation * 0.5f) {
                    /* Strong activation transfer - create reverse edge too */
                    create_or_strengthen_edge(g, target, i);
                }
            }
        }
        
        /* Source node activation decays after propagating (like signal attenuation) */
        /* But don't decay too much - activation should accumulate along paths */
        g->nodes[i].activation *= 0.9f;  /* Slight decay (signal weakens as it propagates) */
        g->nodes[i].fire_count++;
    }
    
    /* PHASE 3: PATTERN-GUIDED PROPAGATION (Patterns boost predicted nodes) */
    /* Patterns are learned intelligence - they activate and boost their predictions */
    /* This happens AFTER edge propagation so patterns can reinforce learned paths */
    propagate_pattern_activation(g);
    
    /* CREATE EDGES FROM CO-ACTIVATION (Hebbian: fire together, wire together) */
    create_edges_from_coactivation(g);
    
    /* CREATE EDGES FROM PATTERN PREDICTIONS */
    create_edges_from_patterns(g);
    
    /* CREATE PATTERN-TO-PATTERN EDGES (Hebbian: patterns that fire together, wire together) */
    create_pattern_edges_from_coactivation(g);
    
    /* AUTOMATIC PATTERN-TO-PATTERN LEARNING: Learn from sequences in real-time */
    /* When patterns appear sequentially in input/output, learn those associations */
    learn_pattern_sequences_automatic(g);
    
    /* Update all node dynamics */
    for (int i = 0; i < BYTE_VALUES; i++) {
        update_node_dynamics(g, i);
    }
    
    /* Prune weak edges based on metabolic pressure */
    for (int i = 0; i < BYTE_VALUES; i++) {
        prune_weak_edges(g, i);
    }
    
    /* Update system state */
    compute_system_state(g);
}

/* ============================================================================
 * CREATE EDGES FROM CO-ACTIVATION
 * 
 * Brain principle: Neurons that fire together, wire together
 * If two nodes are active at the same time, create/strengthen edge between them
 * This creates connections INSIDE the graph, not just from input
 * ============================================================================ */

void create_edges_from_coactivation(MelvinGraph *g) {
    /* Find all currently active nodes */
    uint32_t active_nodes[BYTE_VALUES];
    uint32_t active_count = 0;
    
    for (int i = 0; i < BYTE_VALUES; i++) {
        /* Active node threshold relative to system average */
        float active_threshold = g->state.avg_activation * 0.2f;
        if (g->nodes[i].exists && g->nodes[i].activation > active_threshold) {
            active_nodes[active_count++] = i;
        }
    }
    
    /* Create edges between co-active nodes (within temporal window) */
    /* Only connect nodes that are both active RIGHT NOW */
    for (uint32_t i = 0; i < active_count; i++) {
        for (uint32_t j = i + 1; j < active_count; j++) {
            uint32_t node_a = active_nodes[i];
            uint32_t node_b = active_nodes[j];
            
            /* Create bidirectional edges (co-activation is symmetric) */
            /* Strength proportional to how active both nodes are */
            float coactivation_strength = g->nodes[node_a].activation * g->nodes[node_b].activation;
            
            /* Only create if co-activation is significant */
            /* AND: Prevent self-loops (don't create edges from node to itself) */
            /* Co-activation threshold relative to system state */
            float coactivation_threshold = 0.05f * g->state.learning_rate;
            if (coactivation_strength > coactivation_threshold && node_a != node_b) {
                /* Create or strengthen edge A→B */
                create_or_strengthen_edge(g, node_a, node_b);
                
                /* Create or strengthen edge B→A (bidirectional association) */
                create_or_strengthen_edge(g, node_b, node_a);
            }
        }
    }
}

/* ============================================================================
 * CREATE EDGES FROM PATTERN PREDICTIONS
 * 
 * When patterns predict nodes, create edges from pattern's input to predictions
 * This connects patterns to their learned associations
 * ============================================================================ */

void create_edges_from_patterns(MelvinGraph *g) {
    for (uint32_t p = 0; p < g->pattern_count; p++) {
        Pattern *pat = &g->patterns[p];
        
        /* If pattern is active and has predictions */
        if (pat->activation > pat->threshold && pat->prediction_count > 0) {
            /* Get pattern's input nodes (the sequence it matched) */
            uint32_t *pattern_inputs = NULL;
            uint32_t pattern_input_len = 0;
            
            /* Find where pattern matched in input */
            if (g->input_length >= pat->length) {
                for (uint32_t i = 0; i <= g->input_length - pat->length; i++) {
                    if (pattern_matches(g, p, g->input_buffer, g->input_length, i)) {
                        pattern_inputs = &g->input_buffer[i];
                        pattern_input_len = pat->length;
                        break;
                    }
                }
            }
            
            /* If pattern matched, create edges from pattern inputs to predictions */
            if (pattern_inputs != NULL) {
                for (uint32_t pred = 0; pred < pat->prediction_count; pred++) {
                    uint32_t predicted_node = pat->predicted_nodes[pred];
                    float prediction_weight = pat->prediction_weights[pred];
                    
                    if (predicted_node < BYTE_VALUES && prediction_weight > 0.3f) {
                        /* Create edge from last node in pattern to prediction */
                        uint32_t last_pattern_node = pattern_inputs[pattern_input_len - 1];
                        create_or_strengthen_edge(g, last_pattern_node, predicted_node);
                        
                        /* Also create edge from pattern's first node (for context) */
                        if (pattern_input_len > 1) {
                            uint32_t first_pattern_node = pattern_inputs[0];
                            create_or_strengthen_edge(g, first_pattern_node, predicted_node);
                        }
                    }
                }
            }
        }
    }
}

/* ============================================================================
 * AUTOMATIC PATTERN SEQUENCE LEARNING
 * 
 * Learn pattern-to-pattern associations from sequences automatically
 * System chunks and generalizes by detecting pattern chains in data
 * ============================================================================ */

void learn_pattern_sequences_automatic(MelvinGraph *g) {
    /* Learn from input sequence: detect when patterns follow each other */
    if (g->input_length >= 2) {
        /* Check all pattern pairs in input */
        for (uint32_t p1 = 0; p1 < g->pattern_count; p1++) {
            Pattern *pat1 = &g->patterns[p1];
            if (pat1->length == 0) continue;
            
            /* Find all positions where pattern1 matches in input */
            for (uint32_t pos1 = 0; pos1 <= g->input_length - pat1->length; pos1++) {
                if (pattern_matches(g, p1, g->input_buffer, g->input_length, pos1)) {
                    uint32_t next_pos = pos1 + pat1->length;
                    
                    /* Check if another pattern matches right after */
                    if (next_pos < g->input_length) {
                        for (uint32_t p2 = 0; p2 < g->pattern_count; p2++) {
                            if (p1 == p2) continue;
                            
                            Pattern *pat2 = &g->patterns[p2];
                            if (pat2->length == 0) continue;
                            
                            if (g->input_length - next_pos >= pat2->length) {
                                if (pattern_matches(g, p2, g->input_buffer, g->input_length, next_pos)) {
                                    /* Pattern sequence found: p1 → p2 */
                                    /* Learn this association automatically */
                                    
                                    bool found = false;
                                    for (uint32_t ppred = 0; ppred < pat1->pattern_prediction_count; ppred++) {
                                        if (pat1->predicted_patterns[ppred] == p2) {
                                            pat1->pattern_prediction_weights[ppred] += 0.1f * g->state.learning_rate;
                                            if (pat1->pattern_prediction_weights[ppred] > 1.0f) {
                                                pat1->pattern_prediction_weights[ppred] = 1.0f;
                                            }
                                            found = true;
                                            break;
                                        }
                                    }
                                    
                                    if (!found) {
                                        if (pat1->pattern_prediction_count == 0) {
                                            pat1->predicted_patterns = malloc(sizeof(uint32_t) * 4);
                                            pat1->pattern_prediction_weights = malloc(sizeof(float) * 4);
                                            pat1->pattern_prediction_count = 0;
                                        } else if (pat1->pattern_prediction_count % 4 == 0) {
                                            pat1->predicted_patterns = realloc(pat1->predicted_patterns,
                                                                               sizeof(uint32_t) * (pat1->pattern_prediction_count + 4));
                                            pat1->pattern_prediction_weights = realloc(pat1->pattern_prediction_weights,
                                                                                       sizeof(float) * (pat1->pattern_prediction_count + 4));
                                        }
                                        
                                        pat1->predicted_patterns[pat1->pattern_prediction_count] = p2;
                                        pat1->pattern_prediction_weights[pat1->pattern_prediction_count] = 0.5f;
                                        pat1->pattern_prediction_count++;
                                    }
                                    
                                    break;  /* Found match, move to next position */
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    
    /* Normalize pattern prediction weights for all patterns */
    for (uint32_t p = 0; p < g->pattern_count; p++) {
        Pattern *pat = &g->patterns[p];
        if (pat->pattern_prediction_count > 0) {
            float sum = 0.0f;
            for (uint32_t ppred = 0; ppred < pat->pattern_prediction_count; ppred++) {
                sum += pat->pattern_prediction_weights[ppred];
            }
            if (sum > 0.0f) {
                for (uint32_t ppred = 0; ppred < pat->pattern_prediction_count; ppred++) {
                    pat->pattern_prediction_weights[ppred] /= sum;
                }
            }
        }
    }
}

/* ============================================================================
 * CREATE PATTERN-TO-PATTERN EDGES (Patterns connect like nodes)
 * 
 * When patterns fire together, create edges between them (Hebbian learning)
 * This allows hierarchical/composite patterns to emerge
 * Patterns can influence each other's activations
 * ============================================================================ */

void create_or_strengthen_pattern_edge(MelvinGraph *g, uint32_t from_pattern_id, uint32_t to_pattern_id) {
    if (from_pattern_id >= g->pattern_count || to_pattern_id >= g->pattern_count) return;
    
    Pattern *from_pat = &g->patterns[from_pattern_id];
    EdgeList *out = &from_pat->outgoing_patterns;
    
    /* Find existing edge */
    for (uint32_t i = 0; i < out->count; i++) {
        if (out->edges[i].to_id == to_pattern_id && out->edges[i].active && out->edges[i].is_pattern_edge) {
            /* Strengthen existing edge */
            float learning_rate = g->state.learning_rate;
            out->edges[i].weight += learning_rate * (1.0f - out->edges[i].weight);
            out->edges[i].use_count++;
            return;
        }
    }
    
    /* Create new edge */
    if (out->count >= out->capacity) {
        out->capacity *= 2;
        out->edges = realloc(out->edges, sizeof(Edge) * out->capacity);
    }
    
    Edge *e = &out->edges[out->count++];
    e->to_id = to_pattern_id;
    e->weight = 0.1f;  /* Start small */
    e->use_count = 1;
    e->success_count = 0;
    e->active = true;
    e->is_pattern_edge = true;
}

void create_pattern_edges_from_coactivation(MelvinGraph *g) {
    /* Find all currently active patterns */
    uint32_t active_patterns[256];  /* Max patterns */
    uint32_t active_count = 0;
    
    for (uint32_t p = 0; p < g->pattern_count; p++) {
        Pattern *pat = &g->patterns[p];
        if (pat->activation > pat->threshold && pat->activation > 0.1f) {
            active_patterns[active_count++] = p;
        }
    }
    
    /* Create edges between co-active patterns (Hebbian: fire together, wire together) */
    for (uint32_t i = 0; i < active_count; i++) {
        for (uint32_t j = i + 1; j < active_count; j++) {
            uint32_t pat_a_id = active_patterns[i];
            uint32_t pat_b_id = active_patterns[j];
            
            Pattern *pat_a = &g->patterns[pat_a_id];
            Pattern *pat_b = &g->patterns[pat_b_id];
            
            /* PORT CHECK: Patterns from different ports get weaker connections */
            float port_penalty = (pat_a->input_port == pat_b->input_port) ? 1.0f : 0.3f;
            
            /* Co-activation strength */
            float coactivation_strength = pat_a->activation * pat_b->activation * port_penalty;
            
            /* Only create if significant co-activation */
            float threshold = 0.05f * g->state.learning_rate;
            if (coactivation_strength > threshold) {
                /* Create bidirectional edges (patterns influence each other) */
                create_or_strengthen_pattern_edge(g, pat_a_id, pat_b_id);
                create_or_strengthen_pattern_edge(g, pat_b_id, pat_a_id);
            }
        }
    }
}

/* ============================================================================
 * 1. INPUT INJECTION
 * 
 * Inject byte sequence into the graph
 * Activation injected proportional to system state (not fixed amount)
 * ============================================================================ */

void inject_input(MelvinGraph *g, const uint8_t *bytes, uint32_t length) {
    /* AUTO-LEARNED PORTS: Use current input port (learned from last injection) */
    /* Default: port 0, but automatically tracks from injection context */
    inject_input_from_port(g, bytes, length, g->current_input_port);
}

/* AUTO-LEARNING: Ports are automatically set when input is injected */
/* This is the simplest way - ports come with the data, no separate function needed */

void inject_input_from_port(MelvinGraph *g, const uint8_t *bytes, uint32_t length, uint32_t port_id) {
    /* AUTO-LEARNED PORTS: Port is automatically set from injection */
    /* This updates the current port context for wave prop to learn from */
    g->current_input_port = port_id;
    g->current_output_port = port_id;  /* Default: same port (can be overridden if needed) */
    
    /* Grow input buffer if needed */
    while (g->input_length + length > g->input_capacity) {
        g->input_capacity *= 2;
        g->input_buffer = realloc(g->input_buffer, sizeof(uint32_t) * g->input_capacity);
    }
    
    /* Add to input buffer */
    for (uint32_t i = 0; i < length; i++) {
        g->input_buffer[g->input_length++] = bytes[i];
    }
    
    /* Inject activation into corresponding nodes */
    for (uint32_t i = 0; i < length; i++) {
        uint8_t byte = bytes[i];
        
        /* Create node if doesn't exist */
        if (!g->nodes[byte].exists) {
            g->nodes[byte].exists = true;
            g->nodes[byte].activation = 0.0f;
            g->nodes[byte].threshold = g->state.avg_threshold;
            /* AUTO-LEARNED PORT: Node learns its port from injection */
            g->nodes[byte].source_port = port_id;
        } else {
            /* Existing node: port might change if reused across modalities */
            /* Keep original port (first port it was seen with) */
            /* This allows nodes to have a "primary" port but still work across ports */
        }
        
        /* Injection strength relative to exploration pressure */
        /* High exploration = stronger injection (force attention) */
        float injection_strength = 0.5f + 0.5f * g->state.exploration_pressure;
        
        /* Add activation (purely local) */
        g->nodes[byte].activation += injection_strength;
        
        /* Create sequential edges (Hebbian: this byte followed that byte) */
        if (i > 0) {
            uint8_t prev_byte = bytes[i - 1];
            create_or_strengthen_edge(g, prev_byte, byte);
        }
    }
}

/* ============================================================================
 * 2. PATTERN DETECTION
 * 
 * Find repeated sequences in input/output history
 * Patterns kept only if they reduce complexity (compression benefit)
 * No arbitrary limits - metabolic cost determines survival
 * ============================================================================ */

void detect_patterns(MelvinGraph *g) {
    /* Only detect patterns if we have enough data */
    if (g->input_length < 2) return;
    
    /* Look for repeated bigrams (length 2) */
    for (uint32_t i = 0; i < g->input_length - 1; i++) {
        uint32_t a = g->input_buffer[i];
        uint32_t b = g->input_buffer[i + 1];
        
        /* Check if pattern already exists */
        bool found = false;
        for (uint32_t p = 0; p < g->pattern_count; p++) {
            Pattern *pat = &g->patterns[p];
            if (pat->length == 2 && 
                pat->node_ids[0] == a && 
                pat->node_ids[1] == b) {
                /* Pattern exists - strengthen it */
                pat->activation += 0.1f;
                if (pat->activation > 1.0f) pat->activation = 1.0f;
                found = true;
                break;
            }
        }
        
        if (!found) {
            /* Count occurrences of this bigram */
            uint32_t count = 0;
            for (uint32_t j = 0; j < g->input_length - 1; j++) {
                if (g->input_buffer[j] == a && g->input_buffer[j + 1] == b) {
                    count++;
                }
            }
            
            /* HIERARCHICAL: Check if this bigram can be built from existing patterns */
            /* If a pattern ends with 'a' and another starts with 'b', we can compose them */
            uint32_t *sub_pattern_ids = NULL;
            uint32_t sub_pattern_count = 0;
            
            for (uint32_t p = 0; p < g->pattern_count; p++) {
                Pattern *existing_pat = &g->patterns[p];
                if (existing_pat->length > 0 && existing_pat->strength > 0.0f) {
                    uint32_t last_node = existing_pat->node_ids[existing_pat->length - 1];
                    if (last_node == a) {
                        /* Pattern ends with 'a' - could be first sub-pattern */
                        /* Check if 'b' is a pattern starting node */
                        for (uint32_t p2 = 0; p2 < g->pattern_count; p2++) {
                            Pattern *existing_pat2 = &g->patterns[p2];
                            if (existing_pat2->length > 0 && existing_pat2->strength > 0.0f) {
                                uint32_t first_node = existing_pat2->node_ids[0];
                                if (first_node == b) {
                                    /* Found hierarchical composition: pattern ending in 'a' + pattern starting with 'b' */
                                    /* This is more efficient than raw bigram - build from sub-patterns */
                                    if (sub_pattern_count == 0) {
                                        sub_pattern_ids = malloc(sizeof(uint32_t) * 2);
                                    } else {
                                        sub_pattern_ids = realloc(sub_pattern_ids, sizeof(uint32_t) * (sub_pattern_count + 2));
                                    }
                                    sub_pattern_ids[sub_pattern_count++] = p;
                                    sub_pattern_ids[sub_pattern_count++] = p2;
                                    break;
                                }
                            }
                        }
                        if (sub_pattern_count > 0) break;
                    }
                }
            }
            
            /* Pattern creation threshold: relative to system state */
            /* More patterns when system has high error (needs learning) */
            /* Fewer patterns when system is stable (metabolic cost) */
            float pattern_threshold = 2.0f * (1.0f - g->state.error_rate);  /* High error = lower threshold */
            if (pattern_threshold < 1.5f) pattern_threshold = 1.5f;  /* Minimum: still need repetition */
            if (pattern_threshold > 3.0f) pattern_threshold = 3.0f;  /* Maximum: prevent noise patterns */
            
            if (count >= (uint32_t)pattern_threshold) {
                /* Grow pattern array if needed */
                if (g->pattern_count >= g->pattern_capacity) {
                    g->pattern_capacity *= 2;
                    g->patterns = realloc(g->patterns, sizeof(Pattern) * g->pattern_capacity);
                }
                
                /* Create new pattern */
                Pattern *pat = &g->patterns[g->pattern_count++];
                pat->node_ids = malloc(sizeof(uint32_t) * 2);
                pat->node_ids[0] = a;
                pat->node_ids[1] = b;
                pat->length = 2;
                
                /* Initialize hierarchical fields */
                if (sub_pattern_count > 0) {
                    /* Pattern is built from sub-patterns (hierarchical composition) */
                    pat->sub_pattern_ids = sub_pattern_ids;
                    pat->sub_pattern_count = sub_pattern_count;
                } else {
                    pat->sub_pattern_ids = NULL;
                    pat->sub_pattern_count = 0;
                }
                
                /* Initialize micro neural net fields */
                pat->predicted_nodes = NULL;
                pat->prediction_weights = NULL;
                pat->prediction_count = 0;
                pat->predicted_patterns = NULL;
                pat->pattern_prediction_weights = NULL;
                pat->pattern_prediction_count = 0;
                /* Initialize relative to system state */
                pat->threshold = g->state.avg_threshold;
                
                /* Initialize neural net components */
                pat->input_weights = NULL;
                pat->bias = 0.0f;  /* Start at 0, like simple neural net */
                pat->input_size = 0;
                
                /* Strength from COMPRESSION BENEFIT - patterns reduce graph complexity */
                /* True compression: pattern represents multiple sequences, reducing edge count */
                /* Cost: pattern takes resources (memory, activation overhead) */
                /* Benefit: replaces (count - 1) * length edges with 1 pattern + pattern→node edges */
                float pattern_cost = 1.0f + (pat->prediction_count * 0.1f);  /* Cost scales with predictions */
                float edges_saved = (count - 1) * 2.0f;  /* Each repetition saves 2 edges */
                float compression_benefit = edges_saved - pattern_cost;
                
                /* Strength emerges from actual utility (prediction success), not just compression */
                /* If pattern already exists and has predictions, use its utility */
                float utility = 0.5f;  /* Default utility for new patterns */
                if (pat->prediction_attempts > 0) {
                    utility = (float)pat->prediction_successes / (float)pat->prediction_attempts;
                }
                
                /* Strength = pattern utility as a "virtual edge weight" */
                /* Pattern acts like a strong edge that compresses multiple paths */
                /* Strength should be 0.5-1.0 range (like strong edges) to compete effectively */
                float base_strength = utility;  /* Start at utility level (0.5 default, up to 1.0) */
                if (compression_benefit > 1.0f) {
                    base_strength *= 1.5f;  /* Boost for good compression */
                }
                if (base_strength > 1.0f) base_strength = 1.0f;  /* Cap at 1.0 */
                if (base_strength < 0.1f) base_strength = 0.1f;  /* Floor at 0.1 */
                
                /* HIERARCHICAL BOOST: Patterns built from other patterns are more efficient */
                if (pat->sub_pattern_count > 0) {
                    /* Hierarchical patterns inherit strength from sub-patterns (composition benefit) */
                    float sub_strength_sum = 0.0f;
                    for (uint32_t s = 0; s < pat->sub_pattern_count; s++) {
                        if (pat->sub_pattern_ids[s] < g->pattern_count) {
                            sub_strength_sum += g->patterns[pat->sub_pattern_ids[s]].strength;
                        }
                    }
                    /* Hierarchical pattern gets boost from composing useful sub-patterns */
                    base_strength *= (1.0f + 0.3f * sub_strength_sum);
                }
                
                /* Boost strength when error is high (system needs help learning) */
                pat->strength = base_strength * (1.0f + g->state.error_rate);
                
                /* Activation starts relative to system's average activation */
                pat->activation = g->state.avg_activation * 0.2f;
                pat->prediction_attempts = 0;
                pat->prediction_successes = 0;
                pat->has_fired = false;
                pat->last_fired_step = 0;
                pat->fired_predictions = 0;
                
                /* PORT AUTO-LEARNING: Pattern learns ports from the nodes it's built from */
                /* Ports are automatically derived from node source_port (no manual setting needed) */
                /* Find most common port among pattern's nodes */
                uint32_t port_counts[256] = {0};  /* Count ports (assume max 256 ports) */
                for (uint32_t i = 0; i < pat->length; i++) {
                    if (pat->node_ids[i] < BYTE_VALUES && pat->node_ids[i] != BLANK_NODE) {
                        uint32_t node_port = g->nodes[pat->node_ids[i]].source_port;
                        port_counts[node_port]++;
                    }
                }
                /* Find most common port */
                uint32_t most_common_port = 0;
                uint32_t max_count = 0;
                for (uint32_t p = 0; p < 256; p++) {
                    if (port_counts[p] > max_count) {
                        max_count = port_counts[p];
                        most_common_port = p;
                    }
                }
                pat->input_port = most_common_port;
                
                /* Output port: learn from predicted nodes (if any) or default to input port */
                pat->output_port = most_common_port;  /* Default: same as input */
                
                /* Also store context for backward compatibility */
                for (int i = 0; i < 16; i++) {
                    pat->context_vector[i] = g->state.context_vector[i];
                }
                
                /* Initialize pattern-to-pattern edge lists */
                pat->outgoing_patterns.edges = malloc(sizeof(Edge) * INITIAL_CAPACITY);
                pat->outgoing_patterns.count = 0;
                pat->outgoing_patterns.capacity = INITIAL_CAPACITY;
                pat->outgoing_patterns.total_weight = 0.0f;
                pat->outgoing_patterns.metabolic_load = 0.0f;
                
                pat->incoming_patterns.edges = malloc(sizeof(Edge) * INITIAL_CAPACITY);
                pat->incoming_patterns.count = 0;
                pat->incoming_patterns.capacity = INITIAL_CAPACITY;
                pat->incoming_patterns.total_weight = 0.0f;
                pat->incoming_patterns.metabolic_load = 0.0f;
            }
        }
    }
    
    /* SELF-TUNING: Pattern strength IS utility (direct connection, no intermediaries) */
    /* Successful patterns automatically become strong. Failed patterns automatically weaken. */
    float total_utility = 0.0f;
    uint32_t utility_count = 0;
    
    for (uint32_t p = 0; p < g->pattern_count; p++) {
        Pattern *pat = &g->patterns[p];
        
        /* Update utility from actual usage */
        if (pat->prediction_attempts > 10) {  /* Need enough data for reliable utility */
            float current_utility = (float)pat->prediction_successes / (float)pat->prediction_attempts;
            
            /* SELF-TUNING FIX 1: Strength = Utility (direct, not dampened) */
            pat->strength = current_utility;  /* 90% success = 0.9 strength, 10% success = 0.1 strength */
            
            /* Weak patterns (below chance) decay toward death */
            if (current_utility < 0.4f) {
                pat->strength *= 0.95f;  /* Slow decay for patterns that don't work */
            }
            
            /* Track for system-level confidence */
            total_utility += current_utility;
            utility_count++;
            
            /* Reset counters (exponential moving average) */
            pat->prediction_attempts = (uint64_t)(pat->prediction_attempts * 0.9f);
            pat->prediction_successes = (uint64_t)(pat->prediction_successes * 0.9f);
        }
    }
    
    /* Compute average pattern utility (pattern confidence) */
    g->state.avg_pattern_utility = (utility_count > 0) ? (total_utility / utility_count) : 0.5f;
    g->state.pattern_confidence = g->state.avg_pattern_utility;  /* System trusts patterns when they work */
    
    /* DON'T normalize pattern strengths - they need to compete with EDGES, not each other */
    /* Pattern strength should reflect their actual compression benefit and utility */
    /* Normalizing makes them too weak to influence output vs edges */
    
    /* Instead, compute total strength for system state tracking */
    float total_strength = 0.0f;
    for (uint32_t p = 0; p < g->pattern_count; p++) {
        total_strength += g->patterns[p].strength;
    }
    g->state.total_pattern_strength = total_strength;
    
    /* PRUNE WEAK PATTERNS (metabolic cost - like edge pruning) */
    /* Patterns with very low strength and low utility get removed */
    for (int p = g->pattern_count - 1; p >= 0; p--) {
        Pattern *pat = &g->patterns[p];
        
        /* Pattern is weak if strength is very low */
        float strength_threshold = 0.01f / g->pattern_count;  /* Relative to pattern count */
        
        /* Pattern is useless if it has low utility after many attempts */
        bool low_utility = (pat->prediction_attempts > 50 && 
                           (float)pat->prediction_successes / (float)pat->prediction_attempts < 0.2f);
        
        if (pat->strength < strength_threshold && low_utility) {
            /* Remove weak pattern (free resources for better patterns) */
            /* TODO: Free pattern memory */
            /* For now, just mark as inactive by setting strength to 0 */
            pat->strength = 0.0f;
        }
    }
}

/* ============================================================================
 * 3. OUTPUT SELECTION
 * 
 * Select next output node based on firing probabilities
 * No winner-take-all threshold - probabilistic sampling
 * ============================================================================ */

/* ============================================================================
 * COMPUTE NODE RELEVANCE SCORE
 * 
 * Multi-factor scoring (like attention mechanism):
 * 1. Base activation (how active is the node?)
 * 2. Pattern support (do patterns predict this node?)
 * 3. Context relevance (does this node fit current context?)
 * 4. Sequential coherence (does this follow from previous output?)
 * 
 * Returns relevance score [0,1]
 * ============================================================================ */

float compute_node_relevance(MelvinGraph *g, uint32_t node_id) {
    Node *n = &g->nodes[node_id];
    if (!n->exists) return 0.0f;
    
    /* ========================================================================
     * INTELLIGENCE: Context determines what fires, not just static weights
     * ======================================================================== */
    
    /* CONTEXT 1: Where are we in the sequence? (Position in output) */
    /* Output position determines what makes sense - not just "highest weight" */
    float position_context = 0.0f;
    
    /* Check if output sequence matches any pattern's INPUT part */
    /* If pattern matches current output, its PREDICTIONS are contextually relevant */
    for (uint32_t p = 0; p < g->pattern_count; p++) {
        Pattern *pat = &g->patterns[p];
        
        /* Does pattern match END of current output? */
        if (g->output_length >= pat->length && pat->prediction_count > 0) {
            uint32_t start_pos = g->output_length - pat->length;
            if (pattern_matches(g, p, g->output_buffer, g->output_length, start_pos)) {
                /* Pattern matches! Check if it predicts THIS node */
                for (uint32_t pred = 0; pred < pat->prediction_count; pred++) {
                    if (pat->predicted_nodes[pred] == node_id) {
                        /* This node is contextually relevant (pattern says "this should come next") */
                        /* Weight by pattern strength AND prediction confidence */
                        position_context += pat->strength * pat->prediction_weights[pred];
                    }
                }
            }
        }
    }
    
    /* CONTEXT 2: Where did we come from? (Activation history) */
    /* Nodes that were recently active are LESS likely to repeat (avoid loops) */
    float history_penalty = 0.0f;
    for (uint32_t i = 0; i < g->output_length; i++) {
        if (g->output_buffer[i] == node_id) {
            /* Node already appeared - penalize based on recency */
            float recency = (float)(g->output_length - i) / (g->output_length + 1.0f);
            history_penalty += recency * 0.5f;  /* Recent repetition = strong penalty */
        }
    }
    if (history_penalty > 0.9f) history_penalty = 0.9f;  /* Cap penalty */
    
    /* CONTEXT 3: Current wave activation (what's hot right now) */
    /* Activation represents the CURRENT state of wave propagation */
    float wave_activation = n->activation;  /* Activation is purely local */
    
    /* CONTEXT 4: Input context (what's the task?) */
    /* Nodes from input are more likely to be relevant */
    /* STRONGER: Check if node is connected to input through wave propagation */
    float input_context = 0.0f;
    
    /* Direct input match (node appears in input) */
    for (uint32_t i = 0; i < g->input_length; i++) {
        if (g->input_buffer[i] == node_id) {
            /* Position matters: later in input = more relevant */
            float position_weight = (float)(i + 1) / (float)g->input_length;
            input_context += 0.5f * position_weight;  /* Stronger boost */
        }
    }
    
    /* Indirect input connection (node is reachable from input through edges) */
    /* This captures wave propagation context - nodes connected to input get boost */
    for (uint32_t i = 0; i < g->input_length; i++) {
        uint32_t input_node = g->input_buffer[i];
        if (input_node < BYTE_VALUES && g->nodes[input_node].exists) {
            /* Check if there's a path from input_node to node_id */
            EdgeList *out = &g->outgoing[input_node];
            for (uint32_t j = 0; j < out->count; j++) {
                if (out->edges[j].to_id == node_id && out->edges[j].active) {
                    /* Connected to input through learned edge */
                    float edge_weight = out->edges[j].weight;
                    float position_weight = (float)(i + 1) / (float)g->input_length;
                    input_context += 0.3f * edge_weight * position_weight;
                }
            }
        }
    }
    
    if (input_context > 1.0f) input_context = 1.0f;
    
    /* ========================================================================
     * COMPUTE LIVE RELEVANCE: No static weights, all dynamic context
     * ======================================================================== */
    
    /* SELF-TUNING FIX 6: Pattern confidence drives output selection */
    /* When patterns work well (high confidence), trust them over wave chaos */
    float pattern_weight = g->state.pattern_confidence;
    float wave_weight = 1.0f - pattern_weight;
    
    /* If node has strong position context (patterns predict it), prioritize it */
    if (position_context > 0.1f) {
        /* Pattern-driven selection (INTELLIGENT) - boost when patterns are confident */
        float pattern_relevance = position_context * (1.0f - history_penalty) * (1.0f + wave_activation);
        float wave_relevance = wave_activation * (1.0f - history_penalty) * (1.0f + input_context * 0.5f);
        
        /* Weight by pattern confidence */
        float relevance = (pattern_weight * pattern_relevance) + (wave_weight * wave_relevance);
        
        /* If patterns are confident, boost their predictions */
        if (pattern_weight > 0.7f) {
            relevance *= 2.0f;  /* Strong boost when patterns are reliable */
        }
        
        /* Apply loop pressure - suppress nodes that continue loops */
        if (g->state.loop_pressure > 0.5f && g->output_length > 0) {
            if (node_id == g->output_buffer[g->output_length - 3]) {
                relevance *= 0.1f;  /* Kill looping nodes */
            }
        }
        
        return relevance;
    } else {
        /* Activation-driven selection (fallback when no pattern matches) */
        /* Use wave activation, moderated by history and input context */
        float relevance = wave_activation * (1.0f - history_penalty) * (1.0f + input_context * 0.5f);
        
        /* Apply loop pressure */
        if (g->state.loop_pressure > 0.5f && g->output_length > 0) {
            if (node_id == g->output_buffer[g->output_length - 3]) {
                relevance *= 0.1f;  /* Kill looping nodes */
            }
        }
        
        return relevance;
    }
}

uint32_t select_output_node(MelvinGraph *g) {
    /* ========================================================================
     * INTELLIGENT PATH SELECTION: Pick node at end of best learned path
     * 
     * Wave propagation creates "brightest light" at end of intelligent paths
     * Intelligence is in how activation travels through learned structure
     * 
     * Selection: Find the brightest light (highest activation)
     * But that light should be at the end of an intelligent path
     * ======================================================================== */
    
    float max_activation = 0.0f;
    uint32_t winner_node = 0;
    
    /* Find node with highest activation (brightest light) */
    /* This light should be at end of intelligent path from wave propagation */
    for (int i = 0; i < BYTE_VALUES; i++) {
        if (!g->nodes[i].exists) continue;
        
        /* ========================================================================
         * WELL-DEFINED NODE QUALITY: Information Flow Efficiency
         * 
         * Node Quality = Information_Carried × Learning_Strength × Coherence × Predictive_Power
         * 
         * Same well-defined measure as path quality, but from node perspective.
         * ======================================================================== */
        
        /* FACTOR 1: Information_Carried (from input to this node) */
        float input_connection = 0.1f;  /* Default: weak connection */
        for (uint32_t inp = 0; inp < g->input_length; inp++) {
            uint32_t input_node = g->input_buffer[inp];
            if (input_node < BYTE_VALUES && g->nodes[input_node].exists) {
                EdgeList *input_out = &g->outgoing[input_node];
                for (uint32_t e = 0; e < input_out->count; e++) {
                    if (input_out->edges[e].to_id == i && input_out->edges[e].active) {
                        input_connection = 1.0f;  /* Strong: reachable from input */
                        break;
                    }
                }
            }
        }
        
        float context_match = 0.5f;  /* Default: no pattern match */
        for (uint32_t p = 0; p < g->pattern_count; p++) {
            Pattern *pat = &g->patterns[p];
            if (pat->activation > pat->threshold && pat->activation > 0.1f) {
                for (uint32_t pred = 0; pred < pat->prediction_count; pred++) {
                    if (pat->predicted_nodes[pred] == i) {
                        context_match = 1.0f;  /* Strong: pattern matches context */
                        break;
                    }
                }
            }
        }
        
        float history_coherence = 0.8f;  /* Default: doesn't follow from output */
        if (g->output_length > 0) {
            uint32_t last_output = g->output_buffer[g->output_length - 1];
            if (last_output < BYTE_VALUES && g->nodes[last_output].exists) {
                EdgeList *last_out = &g->outgoing[last_output];
                for (uint32_t e = 0; e < last_out->count; e++) {
                    if (last_out->edges[e].to_id == i && last_out->edges[e].active) {
                        history_coherence = 1.0f;  /* Strong: follows from output */
                        break;
                    }
                }
            }
        }
        
        float information = input_connection * context_match * history_coherence;
        
        /* FACTOR 2: Learning_Strength (how well-learned is this node?) */
        float node_activation = g->nodes[i].activation;
        float usage = logf(1.0f + g->nodes[i].receive_count) / 10.0f;  /* Log scale, normalized */
        float learning = node_activation * (1.0f + usage);
        
        /* FACTOR 3: Coherence (does this node fit contextually?) */
        float pattern_alignment = context_match;  /* Same as context_match */
        float sequential_flow = history_coherence;  /* Same as history_coherence */
        float context_fit = context_match;  /* Same as context_match */
        float coherence = pattern_alignment * sequential_flow * context_fit;
        
        /* FACTOR 4: Predictive_Power (how well does this node predict correct output?) */
        float pattern_prediction = 0.3f;  /* Default: not predicted */
        for (uint32_t p = 0; p < g->pattern_count; p++) {
            Pattern *pat = &g->patterns[p];
            if (pat->activation > pat->threshold && pat->activation > 0.1f) {
                for (uint32_t pred = 0; pred < pat->prediction_count; pred++) {
                    if (pat->predicted_nodes[pred] == i) {
                        pattern_prediction = pat->activation * pat->strength;  /* Pattern confidence */
                        break;
                    }
                }
            }
        }
        
        float historical_accuracy = 0.5f;  /* Would need node-level success tracking */
        float context_prediction = context_match;
        float predictive = pattern_prediction * (0.5f + historical_accuracy) * context_prediction;
        
        /* COMBINE: Node Quality = Information × Learning × Coherence × Predictive */
        /* Well-defined measure: all factors must be good for high quality */
        float score = information * learning * coherence * predictive;
        
        /* Skip nodes with negligible quality (relative to system) */
        float min_quality = g->state.avg_activation * 0.01f;  /* Very low threshold - let quality measure decide */
        if (score < min_quality) continue;
        
        /* Apply loop pressure (suppress recent repeats to prevent stuck loops) */
        if (g->state.loop_pressure > 0.5f && g->output_length > 2) {
            if (i == g->output_buffer[g->output_length - 1] ||
                i == g->output_buffer[g->output_length - 2] ||
                i == g->output_buffer[g->output_length - 3]) {
                score *= 0.1f;  /* Strongly suppress looping nodes */
            }
        }
        
        /* Slight history penalty (avoid immediate repetition) */
        if (g->output_length > 0 && i == g->output_buffer[g->output_length - 1]) {
            score *= 0.3f;  /* Don't repeat immediately */
        }
        
        if (score > max_activation) {
            max_activation = score;
            winner_node = i;
        }
    }
    
    /* Return winner (brightest light at end of intelligent path) */
    return winner_node;
}

void emit_output(MelvinGraph *g, uint32_t node_id) {
    /* Grow output buffer if needed */
    if (g->output_length >= g->output_capacity) {
        g->output_capacity *= 2;
        g->output_buffer = realloc(g->output_buffer, sizeof(uint32_t) * g->output_capacity);
    }
    
    /* Add to output */
    g->output_buffer[g->output_length++] = node_id;
    
    /* PORT TRACKING: Output nodes inherit output port */
    /* This allows patterns to learn which port their predictions go to */
    if (g->nodes[node_id].exists) {
        /* Output node should match output port (for pattern learning) */
        g->nodes[node_id].source_port = g->current_output_port;
    }
    
    /* SELF-TUNING FIX 4: Track output history for variance and loop detection */
    g->state.recent_outputs[g->state.output_history_index % 50] = node_id;
    g->state.output_history_index++;
    
    /* Compute output variance (unique count in recent window) */
    uint32_t window_size = (g->state.output_history_index > 20) ? 20 : g->state.output_history_index;
    if (window_size > 0) {
        uint32_t unique_count = 0;
        bool seen[BYTE_VALUES] = {0};
        uint32_t start = (g->state.output_history_index > 50) ? 
            (g->state.output_history_index - window_size) % 50 : 
            (50 + g->state.output_history_index - window_size) % 50;
        
        for (uint32_t i = 0; i < window_size; i++) {
            uint32_t idx = (start + i) % 50;
            uint32_t val = g->state.recent_outputs[idx];
            if (val < BYTE_VALUES && !seen[val]) {
                seen[val] = true;
                unique_count++;
            }
        }
        g->state.output_variance = (float)unique_count / window_size;  /* 0.0 = stuck, 1.0 = chaos */
        
        /* Update exploration pressure based on variance and error */
        g->state.exploration_pressure = g->state.output_variance * g->state.error_rate;
    }
    
    /* SELF-TUNING FIX 5: Detect loops and create escape pressure */
    bool is_looping = false;
    if (g->output_length >= 6) {
        /* Check if last 3 chars repeat the previous 3 */
        is_looping = true;
        for (uint32_t i = 0; i < 3; i++) {
            if (g->output_buffer[g->output_length - 1 - i] != 
                g->output_buffer[g->output_length - 4 - i]) {
                is_looping = false;
                break;
            }
        }
    }
    
    if (is_looping) {
        g->state.loop_pressure = 0.9f;  /* Strong pressure to break loop */
    } else {
        g->state.loop_pressure *= 0.95f;  /* Decay when not looping */
        if (g->state.loop_pressure < 0.01f) g->state.loop_pressure = 0.0f;
    }
    
    /* Reduce activation (refractory period) */
    g->nodes[node_id].activation *= 0.3f;
    /* Node activation decays after output (refractory period) */
    g->nodes[node_id].activation *= 0.3f;
    
    /* MARK PREDICTION AS USED in all patterns that predicted this node */
    /* This prevents patterns from repeatedly boosting the same node */
    for (uint32_t p = 0; p < g->pattern_count; p++) {
        Pattern *pat = &g->patterns[p];
        
        if (pat->prediction_count > 0) {
            for (uint32_t pred = 0; pred < pat->prediction_count; pred++) {
                if (pat->predicted_nodes[pred] == node_id) {
                    /* Mark this prediction as used (bitmask) */
                    pat->fired_predictions |= (1u << pred);
                    
                    /* Track successful prediction (node was actually output) */
                    /* Utility emerges from prediction accuracy */
                    pat->prediction_successes++;
                    
                    /* Decay pattern activation after firing */
                    pat->activation *= 0.5f;  /* Strong reduction after prediction used */
                }
            }
        }
    }
    
    /* PREVENT SELF-LOOPS: Don't create edges from node to itself */
    /* Co-activation was creating s→s edges, causing loops */
    /* Skip co-activation edges if they would be self-loops */
}

/* ============================================================================
 * 4. LEARNING FROM FEEDBACK
 * 
 * Strengthen successful paths, weaken unsuccessful ones
 * Learning rate emerges from error rate (not fixed)
 * ============================================================================ */

void apply_feedback(MelvinGraph *g, const uint8_t *target, uint32_t target_length) {
    /* Compare output to target */
    uint32_t correct = 0;
    uint32_t min_len = (g->output_length < target_length) ? g->output_length : target_length;
    
    /* RICH ERROR SIGNAL: Compute positional error and attribute to components */
    for (uint32_t i = 0; i < min_len; i++) {
        uint32_t predicted = g->output_buffer[i];
        uint32_t expected = target[i];
        
        if (predicted == expected) {
            correct++;
            
            /* Correct prediction - strengthen contributing components */
            OutputContribution *contrib = &g->output_contributions[i];
            
            /* Strengthen patterns that contributed correctly */
            for (uint32_t pc = 0; pc < contrib->pattern_count; pc++) {
                if (contrib->patterns[pc].predicted == predicted) {
                    uint32_t p = contrib->patterns[pc].pattern_id;
                    if (p < g->pattern_count) {
                        Pattern *pat = &g->patterns[p];
                        pat->prediction_successes++;
                        
                        /* Strengthen prediction weight for this node */
                        for (uint32_t pred = 0; pred < pat->prediction_count; pred++) {
                            if (pat->predicted_nodes[pred] == predicted) {
                                float error_share = contrib->patterns[pc].contribution / 
                                                   (contrib->total_contribution + 0.001f);
                                pat->prediction_weights[pred] += 
                                    g->state.learning_rate * error_share * 0.5f;
                                if (pat->prediction_weights[pred] > 1.0f) {
                                    pat->prediction_weights[pred] = 1.0f;
                                }
                                break;
                            }
                        }
                    }
                }
            }
            
            /* Strengthen edges that contributed correctly */
            for (uint32_t ec = 0; ec < contrib->edge_count; ec++) {
                uint32_t from = contrib->edges[ec].from_node;
                create_or_strengthen_edge(g, from, predicted);
            }
        } else {
            /* INCORRECT prediction - compute error share and weaken contributors */
            OutputContribution *contrib = &g->output_contributions[i];
            float error_magnitude = 1.0f;  /* Wrong = full error */
            
            /* Weaken patterns that contributed incorrectly */
            for (uint32_t pc = 0; pc < contrib->pattern_count; pc++) {
                uint32_t p = contrib->patterns[pc].pattern_id;
                if (p < g->pattern_count) {
                    Pattern *pat = &g->patterns[p];
                    pat->prediction_attempts++;
                    
                    /* Error share = (pattern contribution / total) × error */
                    float error_share = (contrib->patterns[pc].contribution / 
                                        (contrib->total_contribution + 0.001f)) * error_magnitude;
                    
                    /* Weaken prediction weights that led to wrong prediction */
                    for (uint32_t pred = 0; pred < pat->prediction_count; pred++) {
                        if (pat->predicted_nodes[pred] == predicted) {
                            pat->prediction_weights[pred] -= 
                                g->state.learning_rate * error_share * 0.3f;
                            if (pat->prediction_weights[pred] < 0.0f) {
                                pat->prediction_weights[pred] = 0.0f;
                            }
                            break;
                        }
                    }
                    
                    /* If pattern predicted wrong node, learn to predict correct one instead */
                    bool has_correct_prediction = false;
                    for (uint32_t pred = 0; pred < pat->prediction_count; pred++) {
                        if (pat->predicted_nodes[pred] == expected) {
                            has_correct_prediction = true;
                            /* Strengthen this prediction */
                            pat->prediction_weights[pred] += 
                                g->state.learning_rate * error_share * 0.2f;
                            if (pat->prediction_weights[pred] > 1.0f) {
                                pat->prediction_weights[pred] = 1.0f;
                            }
                            break;
                        }
                    }
                    
                    /* If pattern doesn't predict correct node, add it */
                    if (!has_correct_prediction && contrib->patterns[pc].contribution > 0.1f) {
                        /* Add new prediction */
                        if (pat->prediction_count == 0) {
                            pat->predicted_nodes = malloc(sizeof(uint32_t) * 4);
                            pat->prediction_weights = malloc(sizeof(float) * 4);
                            pat->prediction_count = 0;
                        } else if (pat->prediction_count % 4 == 0) {
                            pat->predicted_nodes = realloc(pat->predicted_nodes,
                                                           sizeof(uint32_t) * (pat->prediction_count + 4));
                            pat->prediction_weights = realloc(pat->prediction_weights,
                                                             sizeof(float) * (pat->prediction_count + 4));
                        }
                        pat->predicted_nodes[pat->prediction_count] = expected;
                        pat->prediction_weights[pat->prediction_count] = 
                            g->state.learning_rate * error_share;
                        pat->prediction_count++;
                    }
                }
            }
        }
    }
    
    /* Compute error rate */
    float accuracy = (target_length > 0) ? (float)correct / target_length : 0.0f;
    float current_error = 1.0f - accuracy;
    
    /* Update system error rate (smoothed) */
    float smoothing = 0.9f;
    g->state.error_rate = smoothing * g->state.error_rate + (1.0f - smoothing) * current_error;
    
    /* Learning rate adapts to error (high error = learn faster) */
    /* SELF-TUNING: Learning rate already computed in compute_system_state() */
    /* It's based on learning_pressure = error_rate² (quadratic feedback) */
    /* learning_rate = 0.5 * learning_pressure (already set in compute_system_state) */
    
    /* Strengthen sequential edges in target (correct path) */
    for (uint32_t i = 0; i < target_length - 1; i++) {
        create_or_strengthen_edge(g, target[i], target[i + 1]);
    }
    
    /* Pattern backpropagation (preserves neural net learning) */
    /* This still runs to maintain pattern hierarchy and internal weight updates */
    for (uint32_t p = 0; p < g->pattern_count; p++) {
        Pattern *pat = &g->patterns[p];
        
        if (pat->activation > 0.0f && g->input_length >= pat->length) {
            /* Compute pattern error from contribution history */
            float pattern_error = 0.0f;
            bool pattern_contributed = false;
            
            for (uint32_t i = 0; i < min_len; i++) {
                OutputContribution *contrib = &g->output_contributions[i];
                for (uint32_t pc = 0; pc < contrib->pattern_count; pc++) {
                    if (contrib->patterns[pc].pattern_id == p) {
                        pattern_contributed = true;
                        if (g->output_buffer[i] != target[i]) {
                            pattern_error += contrib->patterns[pc].contribution / 
                                            (contrib->total_contribution + 0.001f);
                        }
                    }
                }
            }
            
            if (pattern_contributed && pattern_error > 0.0f) {
                uint32_t *input_nodes = &g->input_buffer[g->input_length - pat->length];
                pattern_backprop(g, p, pattern_error, input_nodes, pat->length);
            }
        }
    }
}

/* ============================================================================
 * PATTERN BACKPROPAGATION (Neural Net Learning)
 * 
 * Update weights and bias: weight += learning_rate × error × input
 * ============================================================================ */

void pattern_backprop(MelvinGraph *g, uint32_t pattern_id, float error, const uint32_t *input_nodes, uint32_t input_len) {
    Pattern *pat = &g->patterns[pattern_id];
    
    if (pat->input_weights == NULL || input_len == 0) return;
    
    /* SELF-TUNING: Use learning_pressure (scales with error_rate²) */
    float learning_rate = g->state.learning_rate;  /* Already computed from learning_pressure */
    
    /* Update weights: weight += learning_rate × error × input */
    for (uint32_t i = 0; i < input_len && i < pat->input_size; i++) {
        uint32_t node_id = input_nodes[i];
        if (node_id < BYTE_VALUES && g->nodes[node_id].exists) {
            float input_value = g->nodes[node_id].activation;
            float weight_delta = learning_rate * error * input_value;
            pat->input_weights[i] += weight_delta;
            
            /* Clip weights to reasonable range */
            if (pat->input_weights[i] > 1.0f) pat->input_weights[i] = 1.0f;
            if (pat->input_weights[i] < -1.0f) pat->input_weights[i] = -1.0f;
        }
    }
    
    /* Update bias: bias += learning_rate × error */
    pat->bias += learning_rate * error;
    
    /* Clip bias to reasonable range */
    if (pat->bias > 1.0f) pat->bias = 1.0f;
    if (pat->bias < -1.0f) pat->bias = -1.0f;
}

/* ============================================================================
 * 5. EPISODE EXECUTION
 * 
 * Complete input -> process -> output -> feedback cycle
 * Output length emerges from activation dynamics (not predetermined)
 * ============================================================================ */

void run_episode(MelvinGraph *g, const uint8_t *input, uint32_t input_len,
                 const uint8_t *target, uint32_t target_len) {
    
    /* Reset output and contribution tracking */
    g->output_length = 0;
    
    /* Clear old contributions */
    for (uint32_t i = 0; i < g->output_contrib_capacity; i++) {
        if (g->output_contributions[i].patterns) {
            free(g->output_contributions[i].patterns);
            g->output_contributions[i].patterns = NULL;
        }
        if (g->output_contributions[i].edges) {
            free(g->output_contributions[i].edges);
            g->output_contributions[i].edges = NULL;
        }
        g->output_contributions[i].pattern_count = 0;
        g->output_contributions[i].edge_count = 0;
        g->output_contributions[i].total_contribution = 0.0f;
    }
    
    /* Reset pattern firing states at start of episode */
    /* This allows patterns to fire again for this new episode */
    for (uint32_t p = 0; p < g->pattern_count; p++) {
        g->patterns[p].has_fired = false;
        g->patterns[p].fired_predictions = 0;
    }
    
    /* Inject input */
    inject_input(g, input, input_len);
    
    /* CRITICAL: Compute system state BEFORE propagation */
    /* Wave prop needs current system state (avg_activation, etc.) */
    compute_system_state(g);
    
    /* Ensure input nodes have strong activation to start wave */
    /* Input nodes should be the source of activation flow */
    for (uint32_t i = 0; i < input_len && i < g->input_length; i++) {
        uint32_t node_id = g->input_buffer[i];
        if (node_id < BYTE_VALUES && g->nodes[node_id].exists) {
            /* Boost input node activation - these are the wave sources */
            g->nodes[node_id].activation = 0.8f;  /* Strong initial activation */
            /* Activation is purely local, no energy constraint */
        }
    }
    
    /* Propagate activation for several steps */
    /* Number of steps proportional to input length (not fixed) */
    uint32_t num_steps = input_len * 3;  /* More steps for better propagation */
    if (num_steps < 20) num_steps = 20;  /* Minimum steps */
    if (num_steps > 200) num_steps = 200;  /* Maximum steps */
    
    for (uint32_t step = 0; step < num_steps; step++) {
        /* Compute system state each step (for dynamic thresholds) */
        compute_system_state(g);
        
        /* Propagate */
        propagate_activation(g);
        
        /* Try to output */
        uint32_t output_node = select_output_node(g);
        
        if (output_node > 0) {
            emit_output(g, output_node);
        }
        
        /* Stop if output length matches expected range */
        /* Range is relative to input length */
        float expected_ratio = 1.0f + 0.2f * g->state.error_rate;
        uint32_t max_output = (uint32_t)(input_len * expected_ratio + 5);
        
        if (g->output_length >= max_output) {
            break;
        }
    }
    
    /* Detect patterns in the data we just saw */
    detect_patterns(g);
    
    /* Detect generalized patterns (with blank nodes for generalization) */
    detect_generalized_patterns(g);
    
    /* Learn pattern predictions (what comes after each pattern) */
    learn_pattern_predictions(g, target, target_len);
    
    /* Apply feedback if target provided */
    if (target != NULL && target_len > 0) {
        apply_feedback(g, target, target_len);
    }
}

/* ============================================================================
 * LEARN PATTERN PREDICTIONS
 * 
 * When a pattern matches and is followed by a node, learn that association
 * This makes patterns into micro neural nets that predict outputs
 * ============================================================================ */

void learn_pattern_predictions(MelvinGraph *g, const uint8_t *target, uint32_t target_len) {
    if (target == NULL || target_len == 0) return;
    
    /* AUTOMATIC PATTERN-TO-PATTERN LEARNING: System learns pattern chains from raw data */
    /* This enables automatic chunking and generalization - patterns compose into concepts */
    
    /* First, learn pattern-to-pattern associations (concept-level) */
    /* Check all pattern pairs: does pattern A followed by pattern B appear in target? */
    for (uint32_t p1 = 0; p1 < g->pattern_count; p1++) {
        Pattern *pat1 = &g->patterns[p1];
        
        /* Check if pattern1 matches ANY position in target */
        if (target_len >= pat1->length) {
            for (uint32_t pos1 = 0; pos1 <= target_len - pat1->length; pos1++) {
                /* Convert target bytes to node IDs for pattern matching */
                uint32_t target_nodes[256];
                uint32_t target_node_len = (target_len < 256) ? target_len : 256;
                for (uint32_t i = 0; i < target_node_len; i++) {
                    target_nodes[i] = target[i];
                }
                
                if (pattern_matches(g, p1, target_nodes, target_node_len, pos1)) {
                    /* Pattern1 matched! Check if another pattern follows it */
                    uint32_t next_pos = pos1 + pat1->length;
                    
                    if (next_pos < target_len) {
                        /* Check all other patterns to see if they match at next_pos */
                        for (uint32_t p2 = 0; p2 < g->pattern_count; p2++) {
                            if (p1 == p2) continue;  /* Don't predict self */
                            
                            Pattern *pat2 = &g->patterns[p2];
                            
                            if (target_len - next_pos >= pat2->length) {
                                if (pattern_matches(g, p2, target_nodes, target_node_len, next_pos)) {
                                    /* PATTERN-TO-PATTERN ASSOCIATION FOUND! */
                                    /* Pattern1 → Pattern2 (automatic chunking) */
                                    
                                    /* Find or add pattern prediction */
                                    bool found = false;
                                    for (uint32_t ppred = 0; ppred < pat1->pattern_prediction_count; ppred++) {
                                        if (pat1->predicted_patterns[ppred] == p2) {
                                            /* Strengthen existing pattern prediction */
                                            pat1->pattern_prediction_weights[ppred] += 0.2f * g->state.learning_rate;
                                            if (pat1->pattern_prediction_weights[ppred] > 1.0f) {
                                                pat1->pattern_prediction_weights[ppred] = 1.0f;
                                            }
                                            found = true;
                                            break;
                                        }
                                    }
                                    
                                    if (!found) {
                                        /* Add new pattern prediction */
                                        if (pat1->pattern_prediction_count == 0) {
                                            pat1->predicted_patterns = malloc(sizeof(uint32_t) * 4);
                                            pat1->pattern_prediction_weights = malloc(sizeof(float) * 4);
                                            pat1->pattern_prediction_count = 0;
                                        } else if (pat1->pattern_prediction_count % 4 == 0) {
                                            pat1->predicted_patterns = realloc(pat1->predicted_patterns,
                                                                               sizeof(uint32_t) * (pat1->pattern_prediction_count + 4));
                                            pat1->pattern_prediction_weights = realloc(pat1->pattern_prediction_weights,
                                                                                       sizeof(float) * (pat1->pattern_prediction_count + 4));
                                        }
                                        
                                        pat1->predicted_patterns[pat1->pattern_prediction_count] = p2;
                                        pat1->pattern_prediction_weights[pat1->pattern_prediction_count] = 0.7f;
                                        pat1->pattern_prediction_count++;
                                    }
                                    
                                    /* Normalize pattern prediction weights */
                                    float pattern_sum = 0.0f;
                                    for (uint32_t ppred = 0; ppred < pat1->pattern_prediction_count; ppred++) {
                                        pattern_sum += pat1->pattern_prediction_weights[ppred];
                                    }
                                    if (pattern_sum > 0.0f) {
                                        for (uint32_t ppred = 0; ppred < pat1->pattern_prediction_count; ppred++) {
                                            pat1->pattern_prediction_weights[ppred] /= pattern_sum;
                                        }
                                    }
                                    
                                    break;  /* Found pattern match, move to next position */
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    
    /* Also learn pattern-to-node predictions (for output generation) */
    /* For each pattern, check if it matches input and what comes next in target */
    for (uint32_t p = 0; p < g->pattern_count; p++) {
        Pattern *pat = &g->patterns[p];
        
        /* Check if pattern matches ANY position in input (not just end) */
        if (g->input_length >= pat->length) {
            /* Try all positions where pattern could match */
            bool matched = false;
            uint32_t match_pos = 0;
            
            for (uint32_t start_pos = 0; start_pos <= g->input_length - pat->length; start_pos++) {
                if (pattern_matches(g, p, g->input_buffer, g->input_length, start_pos)) {
                    matched = true;
                    match_pos = start_pos;
                    break;  /* Found a match, use it */
                }
            }
            
            if (matched) {
                /* Pattern matched at position match_pos! Learn what comes next in target */
                /* Next node should be at target position: match_pos + pat->length */
                uint32_t next_target_pos = match_pos + pat->length;
                
                if (next_target_pos < target_len) {
                    uint32_t next_node = target[next_target_pos];
                    
                    /* Find or add prediction */
                    bool found = false;
                    for (uint32_t pred = 0; pred < pat->prediction_count; pred++) {
                        if (pat->predicted_nodes[pred] == next_node) {
                            /* Strengthen existing prediction (STRONGER learning) */
                            pat->prediction_weights[pred] += 0.2f * g->state.learning_rate;  /* 2x learning rate */
                            if (pat->prediction_weights[pred] > 1.0f) {
                                pat->prediction_weights[pred] = 1.0f;
                            }
                            found = true;
                            break;
                        }
                    }
                    
                    if (!found) {
                        /* Add new prediction */
                        if (pat->prediction_count == 0) {
                            pat->predicted_nodes = malloc(sizeof(uint32_t) * 4);
                            pat->prediction_weights = malloc(sizeof(float) * 4);
                            pat->prediction_count = 0;
                        } else if (pat->prediction_count % 4 == 0) {
                            pat->predicted_nodes = realloc(pat->predicted_nodes, 
                                                           sizeof(uint32_t) * (pat->prediction_count + 4));
                            pat->prediction_weights = realloc(pat->prediction_weights,
                                                             sizeof(float) * (pat->prediction_count + 4));
                        }
                        
                        pat->predicted_nodes[pat->prediction_count] = next_node;
                        pat->prediction_weights[pat->prediction_count] = 0.7f;  /* Start stronger */
                        pat->prediction_count++;
                    }
                    
                    /* Normalize prediction weights (proportions) */
                    float sum = 0.0f;
                    for (uint32_t pred = 0; pred < pat->prediction_count; pred++) {
                        sum += pat->prediction_weights[pred];
                    }
                    if (sum > 0.0f) {
                        for (uint32_t pred = 0; pred < pat->prediction_count; pred++) {
                            pat->prediction_weights[pred] /= sum;
                        }
                    }
                }
            }
        }
    }
}

/* Set modality context (for preventing confusion between text/audio/vision/motor) */
void melvin_set_context(MelvinGraph *g, float *context) {
    if (!context) return;
    for (int i = 0; i < 16; i++) {
        g->state.context_vector[i] = context[i];
    }
}

/* Set input port (0=text, 1=audio, 2=vision, 3=motor, etc.) */
void melvin_set_input_port(MelvinGraph *g, uint32_t port_id) {
    g->current_input_port = port_id;
}

/* Set output port (0=text, 1=audio, 2=vision, 3=motor, etc.) */
void melvin_set_output_port(MelvinGraph *g, uint32_t port_id) {
    g->current_output_port = port_id;
}

/* ============================================================================
 * BRAIN I/O: .m file IS the brain (not just data)
 * 
 * melvin.c = interpreter/VM that executes the brain
 * .m file = the active brain (patterns, edges, execution state)
 * ============================================================================ */

/* Save brain to .m file (the brain writes itself) */
int melvin_save_brain(MelvinGraph *g, const char *filename) {
    FILE *f = fopen(filename, "w");
    if (!f) return -1;
    
    fprintf(f, "# Melvin o7 Brain File\n");
    fprintf(f, "# This file IS the brain - patterns, edges, and learned state\n");
    fprintf(f, "# Generated by melvin.c interpreter\n\n");
    
    /* Save patterns (the learned knowledge) */
    fprintf(f, "# Patterns (learned sequences with predictions)\n");
    for (uint32_t p = 0; p < g->pattern_count; p++) {
        Pattern *pat = &g->patterns[p];
        if (pat->strength < 0.01f) continue;  /* Skip weak patterns */
        
        fprintf(f, "pattern \"");
        for (uint32_t i = 0; i < pat->length; i++) {
            if (pat->node_ids[i] == 256) {
                fprintf(f, "_");  /* Blank node */
            } else {
                fprintf(f, "%c", (uint8_t)pat->node_ids[i]);
            }
        }
        fprintf(f, "\"");
        
        /* Predictions */
        if (pat->prediction_count > 0) {
            fprintf(f, " -> \"");
            for (uint32_t pred = 0; pred < pat->prediction_count && pred < 5; pred++) {
                if (pat->prediction_weights[pred] > 0.2f) {
                    fprintf(f, "%c", (uint8_t)pat->predicted_nodes[pred]);
                }
            }
            fprintf(f, "\"");
        }
        
        /* Context (modality) */
        fprintf(f, " context:[");
        for (int i = 0; i < 16; i++) {
            fprintf(f, "%.3f", pat->context_vector[i]);
            if (i < 15) fprintf(f, ",");
        }
        fprintf(f, "]");
        
        fprintf(f, " strength:%.4f", pat->strength);
        if (pat->prediction_attempts > 0) {
            float utility = (float)pat->prediction_successes / (float)pat->prediction_attempts;
            if (utility > 1.0f) utility = 1.0f;  /* Cap utility at 1.0 */
            fprintf(f, " utility:%.4f", utility);
        }
        fprintf(f, " port_in:%u port_out:%u", pat->input_port, pat->output_port);
        fprintf(f, "\n");
    }
    
    /* Save pattern-to-pattern edges */
    fprintf(f, "\n# Pattern-to-pattern edges\n");
    for (uint32_t p = 0; p < g->pattern_count; p++) {
        Pattern *pat = &g->patterns[p];
        EdgeList *out = &pat->outgoing_patterns;
        for (uint32_t e = 0; e < out->count; e++) {
            if (out->edges[e].active && out->edges[e].is_pattern_edge && out->edges[e].weight > 0.1f) {
                fprintf(f, "pat_edge %u -> %u weight:%.4f\n", p, out->edges[e].to_id, out->edges[e].weight);
            }
        }
    }
    
    fprintf(f, "\n# Node edges (learned connections)\n");
    /* Save significant edges */
    for (uint32_t i = 0; i < BYTE_VALUES; i++) {
        if (!g->nodes[i].exists) continue;
        EdgeList *out = &g->outgoing[i];
        for (uint32_t e = 0; e < out->count; e++) {
            if (out->edges[e].active && out->edges[e].weight > 0.1f) {
                fprintf(f, "edge '%c' -> '%c' weight:%.4f\n",
                        (uint8_t)i,
                        (uint8_t)out->edges[e].to_id,
                        out->edges[e].weight);
            }
        }
    }
    
    fprintf(f, "\n# System state\n");
    fprintf(f, "state error_rate:%.4f learning_rate:%.4f pattern_count:%u\n",
            g->state.error_rate, g->state.learning_rate, g->pattern_count);
    
    fclose(f);
    return 0;
}

/* Load brain from .m file (the brain initializes itself) */
MelvinGraph* melvin_load_brain(const char *filename) {
    FILE *f = fopen(filename, "r");
    if (!f) return NULL;
    
    MelvinGraph *g = melvin_create();
    if (!g) {
        fclose(f);
        return NULL;
    }
    
    char line[1024];
    while (fgets(line, sizeof(line), f)) {
        /* Skip comments and empty lines */
        if (line[0] == '#' || line[0] == '\n') continue;
        
        /* Parse pattern line: pattern "cat" -> "s" context:[...] strength:0.8 port_in:0 port_out:0 */
        if (strncmp(line, "pattern ", 8) == 0) {
            /* Find pattern sequence in quotes */
            char *seq_start = strchr(line, '"');
            if (!seq_start) continue;
            seq_start++;  /* Skip opening quote */
            char *seq_end = strchr(seq_start, '"');
            if (!seq_end) continue;
            
            uint32_t seq_len = seq_end - seq_start;
            if (seq_len == 0 || seq_len > 100) continue;
            
            /* Grow pattern array if needed */
            if (g->pattern_count >= g->pattern_capacity) {
                g->pattern_capacity = (g->pattern_capacity == 0) ? 16 : g->pattern_capacity * 2;
                g->patterns = realloc(g->patterns, sizeof(Pattern) * g->pattern_capacity);
            }
            
            Pattern *pat = &g->patterns[g->pattern_count++];
            pat->node_ids = malloc(sizeof(uint32_t) * seq_len);
            pat->length = 0;
            
            /* Parse sequence (can contain '_' for blank nodes) */
            for (uint32_t i = 0; i < seq_len; i++) {
                if (seq_start[i] == '_') {
                    pat->node_ids[pat->length++] = BLANK_NODE;
                } else {
                    pat->node_ids[pat->length++] = (uint8_t)seq_start[i];
                }
            }
            
            /* Parse predictions (after ->) */
            char *pred_start = strstr(seq_end + 1, "-> \"");
            pat->predicted_nodes = NULL;
            pat->prediction_weights = NULL;
            pat->prediction_count = 0;
            if (pred_start) {
                pred_start += 4;  /* Skip '-> "' */
                char *pred_end = strchr(pred_start, '"');
                if (pred_end) {
                    uint32_t pred_len = pred_end - pred_start;
                    if (pred_len > 0 && pred_len <= 100) {
                        pat->predicted_nodes = malloc(sizeof(uint32_t) * pred_len);
                        pat->prediction_weights = malloc(sizeof(float) * pred_len);
                        pat->prediction_count = pred_len;
                        for (uint32_t i = 0; i < pred_len; i++) {
                            pat->predicted_nodes[i] = (uint8_t)pred_start[i];
                            pat->prediction_weights[i] = 1.0f / pred_len;  /* Equal weights initially */
                        }
                    }
                }
            }
            
            /* Parse context vector */
            char *ctx_start = strstr(line, "context:[");
            if (ctx_start) {
                ctx_start += 9;  /* Skip "context:[" */
                for (int i = 0; i < 16; i++) {
                    pat->context_vector[i] = 0.0f;
                    if (ctx_start && *ctx_start != ']') {
                        sscanf(ctx_start, "%f", &pat->context_vector[i]);
                        ctx_start = strchr(ctx_start, ',');
                        if (ctx_start) ctx_start++;
                    }
                }
            } else {
                for (int i = 0; i < 16; i++) pat->context_vector[i] = 0.0f;
            }
            
            /* Parse strength */
            char *strength_str = strstr(line, "strength:");
            if (strength_str) {
                sscanf(strength_str, "strength:%f", &pat->strength);
            } else {
                pat->strength = 0.5f;
            }
            
            /* Parse utility */
            char *util_str = strstr(line, "utility:");
            if (util_str) {
                float utility;
                sscanf(util_str, "utility:%f", &utility);
                if (utility > 1.0f) utility = 1.0f;
                /* Reconstruct attempts/successes from utility (approximate) */
                pat->prediction_attempts = 100;  /* Assume 100 attempts */
                pat->prediction_successes = (uint64_t)(utility * 100.0f);
            } else {
                pat->prediction_attempts = 0;
                pat->prediction_successes = 0;
            }
            
            /* Parse ports */
            char *port_in_str = strstr(line, "port_in:");
            char *port_out_str = strstr(line, "port_out:");
            if (port_in_str) {
                sscanf(port_in_str, "port_in:%u", &pat->input_port);
            } else {
                pat->input_port = 0;  /* Default */
            }
            if (port_out_str) {
                sscanf(port_out_str, "port_out:%u", &pat->output_port);
            } else {
                pat->output_port = 0;  /* Default */
            }
            
            /* Initialize other fields */
            pat->sub_pattern_ids = NULL;
            pat->sub_pattern_count = 0;
            pat->activation = 0.0f;
            pat->threshold = 0.5f;
            pat->has_fired = false;
            pat->last_fired_step = 0;
            pat->fired_predictions = 0;
            pat->input_weights = NULL;
            pat->bias = 0.0f;
            pat->input_size = 0;
            
            /* Initialize pattern-to-pattern edge lists */
            pat->outgoing_patterns.edges = malloc(sizeof(Edge) * INITIAL_CAPACITY);
            pat->outgoing_patterns.count = 0;
            pat->outgoing_patterns.capacity = INITIAL_CAPACITY;
            pat->outgoing_patterns.total_weight = 0.0f;
            pat->outgoing_patterns.metabolic_load = 0.0f;
            
            pat->incoming_patterns.edges = malloc(sizeof(Edge) * INITIAL_CAPACITY);
            pat->incoming_patterns.count = 0;
            pat->incoming_patterns.capacity = INITIAL_CAPACITY;
            pat->incoming_patterns.total_weight = 0.0f;
            pat->incoming_patterns.metabolic_load = 0.0f;
        }
        
        /* Parse edge line: edge 'c' -> 'a' weight:0.5 */
        if (strncmp(line, "edge ", 5) == 0) {
            char from, to;
            float weight;
            if (sscanf(line, "edge '%c' -> '%c' weight:%f", &from, &to, &weight) == 3) {
                create_or_strengthen_edge(g, (uint8_t)from, (uint8_t)to);
                /* Set weight directly */
                EdgeList *out = &g->outgoing[(uint8_t)from];
                for (uint32_t e = 0; e < out->count; e++) {
                    if (out->edges[e].to_id == (uint8_t)to) {
                        out->edges[e].weight = weight;
                        break;
                    }
                }
            }
        }
        
        /* Parse pattern-to-pattern edge line: pat_edge 0 -> 3 weight:0.5 */
        if (strncmp(line, "pat_edge ", 9) == 0) {
            uint32_t from_pat, to_pat;
            float weight;
            if (sscanf(line, "pat_edge %u -> %u weight:%f", &from_pat, &to_pat, &weight) == 3) {
                if (from_pat < g->pattern_count && to_pat < g->pattern_count) {
                    Pattern *from = &g->patterns[from_pat];
                    EdgeList *out = &from->outgoing_patterns;
                    
                    /* Find or create edge */
                    bool found = false;
                    for (uint32_t e = 0; e < out->count; e++) {
                        if (out->edges[e].to_id == to_pat && out->edges[e].is_pattern_edge) {
                            out->edges[e].weight = weight;
                            out->edges[e].active = true;
                            found = true;
                            break;
                        }
                    }
                    
                    if (!found) {
                        if (out->count >= out->capacity) {
                            out->capacity *= 2;
                            out->edges = realloc(out->edges, sizeof(Edge) * out->capacity);
                        }
                        Edge *e = &out->edges[out->count++];
                        e->to_id = to_pat;
                        e->weight = weight;
                        e->use_count = 1;
                        e->success_count = 0;
                        e->active = true;
                        e->is_pattern_edge = true;
                    }
                }
            }
        }
        
        /* Parse state line: state error_rate:0.5 learning_rate:0.1 pattern_count:18 */
        if (strncmp(line, "state ", 6) == 0) {
            sscanf(line, "state error_rate:%f learning_rate:%f", 
                   &g->state.error_rate, &g->state.learning_rate);
        }
    }
    
    fclose(f);
    return g;
}

/* Destroy graph and free all memory */
void melvin_destroy(MelvinGraph *g) {
    if (!g) return;
    
    /* Free pattern memory */
    for (uint32_t p = 0; p < g->pattern_count; p++) {
        Pattern *pat = &g->patterns[p];
        if (pat->node_ids) free(pat->node_ids);
        if (pat->sub_pattern_ids) free(pat->sub_pattern_ids);
        if (pat->predicted_nodes) free(pat->predicted_nodes);
        if (pat->prediction_weights) free(pat->prediction_weights);
        if (pat->input_weights) free(pat->input_weights);
        if (pat->outgoing_patterns.edges) free(pat->outgoing_patterns.edges);
        if (pat->incoming_patterns.edges) free(pat->incoming_patterns.edges);
    }
    if (g->patterns) free(g->patterns);
    
    /* Free edge lists */
    for (int i = 0; i < BYTE_VALUES; i++) {
        if (g->outgoing[i].edges) free(g->outgoing[i].edges);
        if (g->incoming[i].edges) free(g->incoming[i].edges);
    }
    
    /* Free buffers */
    if (g->input_buffer) free(g->input_buffer);
    if (g->output_buffer) free(g->output_buffer);
    
    /* Free output contributions */
    if (g->output_contributions) {
        for (uint32_t i = 0; i < g->output_contrib_capacity; i++) {
            if (g->output_contributions[i].patterns) free(g->output_contributions[i].patterns);
            if (g->output_contributions[i].edges) free(g->output_contributions[i].edges);
        }
        free(g->output_contributions);
    }
    
    free(g);
}

/* Get output buffer (for testing) */
void melvin_get_output(MelvinGraph *g, uint32_t **output, uint32_t *length) {
    *output = g->output_buffer;
    *length = g->output_length;
}

/* Get error rate (for testing) */
float melvin_get_error_rate(MelvinGraph *g) {
    return g->state.error_rate;
}

/* Get pattern count (for testing) */
uint32_t melvin_get_pattern_count(MelvinGraph *g) {
    return g->pattern_count;
}

/* Get pattern details (for testing) */
void melvin_get_pattern_info(MelvinGraph *g, uint32_t pattern_id, 
                             uint32_t **node_ids, uint32_t *length, float *strength) {
    if (pattern_id >= g->pattern_count) {
        *node_ids = NULL;
        *length = 0;
        *strength = 0.0f;
        return;
    }
    
    Pattern *pat = &g->patterns[pattern_id];
    *node_ids = pat->node_ids;
    *length = pat->length;
    *strength = pat->strength;
}

/* Debug: Get pattern predictions */
void melvin_get_pattern_predictions(MelvinGraph *g, uint32_t pattern_id,
                                    uint32_t **predicted_nodes, float **prediction_weights,
                                    uint32_t *prediction_count) {
    if (pattern_id >= g->pattern_count) {
        *predicted_nodes = NULL;
        *prediction_weights = NULL;
        *prediction_count = 0;
        return;
    }
    
    Pattern *pat = &g->patterns[pattern_id];
    *predicted_nodes = pat->predicted_nodes;
    *prediction_weights = pat->prediction_weights;
    *prediction_count = pat->prediction_count;
}

/* Get edge weight (for testing) */
float melvin_get_edge_weight(MelvinGraph *g, uint32_t from_id, uint32_t to_id) {
    EdgeList *out = &g->outgoing[from_id];
    
    for (uint32_t i = 0; i < out->count; i++) {
        if (out->edges[i].to_id == to_id && out->edges[i].active) {
            return out->edges[i].weight;
        }
    }
    
    return 0.0f;
}

/* ============================================================================
 * MAIN: Input/Output Test - Watch the system learn
 * (Comment out when compiling with test.c)
 * ============================================================================ */

#ifdef MELVIN_STANDALONE
int main(void) {
    printf("MELVIN O7: Input/Output Test\n");
    printf("============================\n\n");
    
    MelvinGraph *g = melvin_create();
    if (!g) {
        fprintf(stderr, "Failed to create graph\n");
        return 1;
    }
    
    /* No randomness - system is deterministic */
    
    printf("System initialized. Starting training...\n\n");
    printf("FORMAT: Episode | Input → Output | Error | Learning Rate\n");
    printf("----------------------------------------------------------\n");
    
    /* Training sequences */
    uint8_t training_inputs[][10] = {
        {'c', 'a', 't'},
        {'d', 'o', 'g'},
        {'c', 'a', 't'},  /* Repeat to form patterns */
        {'d', 'o', 'g'}
    };
    uint32_t training_lengths[] = {3, 3, 3, 3};
    
    /* Run training episodes - show EVERY input/output */
    for (int episode = 0; episode < 30; episode++) {
        int idx = episode % 4;
        uint8_t *input = training_inputs[idx];
        uint32_t len = training_lengths[idx];
        
        /* Run episode (target = input for echo task) */
        run_episode(g, input, len, input, len);
        
        /* Show input and output */
        printf("Ep %2d | Input:  ", episode + 1);
        for (uint32_t i = 0; i < len; i++) {
            printf("%c", input[i]);
        }
        printf(" → Output: ");
        for (uint32_t i = 0; i < g->output_length; i++) {
            printf("%c", (uint8_t)g->output_buffer[i]);
        }
        printf(" | Error: %.3f | LR: %.3f\n", 
               g->state.error_rate, g->state.learning_rate);
    }
    
    printf("\n=== NOVEL INPUT TEST ===\n");
    printf("Testing on unseen inputs...\n\n");
    
    /* Test on novel inputs */
    uint8_t test_cases[][10] = {
        {'b', 'a', 't'},
        {'r', 'a', 't'},
        {'h', 'a', 't'},
        {'c', 'a', 'r'}
    };
    const char* test_names[] = {"bat", "rat", "hat", "car"};
    
    for (int t = 0; t < 4; t++) {
        run_episode(g, test_cases[t], 3, NULL, 0);
        printf("Input:  %s → Output: ", test_names[t]);
        for (uint32_t i = 0; i < g->output_length; i++) {
            printf("%c", (uint8_t)g->output_buffer[i]);
        }
        printf("\n");
    }
    
    printf("\n=== SYSTEM STATE ===\n");
    printf("Error Rate: %.3f\n", g->state.error_rate);
    printf("Learning Rate: %.3f\n", g->state.learning_rate);
    printf("Competition Pressure: %.3f\n", g->state.competition_pressure);
    printf("Patterns: %u\n", g->pattern_count);
    
    uint32_t total_edges = 0;
    for (int i = 0; i < BYTE_VALUES; i++) {
        total_edges += g->outgoing[i].count;
    }
    printf("Total Edges: %u\n", total_edges);
    
    return 0;
}
#endif /* MELVIN_STANDALONE */


