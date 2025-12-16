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
#include <time.h>

/* ============================================================================
 * UNIVERSAL CONSTANTS (Only physics/math, not behavior)
 * ============================================================================ */

#define BYTE_VALUES 256        /* Physical constraint: bytes are 0-255 */
#define BLANK_NODE 256         /* Wildcard node - matches any byte (for generalization) */
#define INITIAL_CAPACITY 10000  /* Starting memory allocation (grows as needed) */
#define INVALID_PATTERN_ID 0xFFFFFFFF  /* Invalid pattern ID (for parent tracking) */

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
    
    /* Weight is RELATIVE STRENGTH of this edge FROM this source node */
    /* NOT a global proportion - relative to other edges from same source */
    /* Grows with usage and success - no global normalization */
    float weight;              /* Absolute strength - relative to source node's other edges */
    
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
    float total_weight;       /* Max weight from this node (for relative comparison, not normalization) */
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
    
    /* ========================================================================
     * PHASE 1: PATTERN HIERARCHY TRACKING
     * ======================================================================== */
    uint32_t chain_depth;          /* How deep in hierarchy (0 = root, 1 = child, etc.) */
    uint32_t parent_pattern_id;     /* Parent pattern in hierarchy (INVALID = root) */
    float accumulated_meaning;      /* Meaning accumulated through chain */
    
    /* ========================================================================
     * PHASE 2: DYNAMIC IMPORTANCE
     * ======================================================================== */
    float dynamic_importance;      /* Learned importance (changes with experience) */
    float context_frequency;       /* How often pattern appears in current context */
    float co_occurrence_strength;  /* Strength of co-occurrence with other patterns */
    
    /* ========================================================================
     * PHASE 2: PATTERN ASSOCIATION NETWORKS
     * ======================================================================== */
    uint32_t *associated_patterns; /* Patterns that co-occur with this pattern */
    float *association_strengths;  /* Strength of each association */
    uint32_t association_count;    /* Number of associations */
    uint32_t association_capacity; /* Capacity of association arrays */
    
    /* ========================================================================
     * PHASE 3: LEARNED ACTIVATION RULES (IF-THEN BEHAVIOR)
     * ======================================================================== */
    uint32_t *rule_condition_patterns;  /* Patterns that trigger rules (IF condition) */
    uint32_t *rule_target_patterns;     /* Patterns boosted by rules (THEN action) */
    float *rule_boost_amounts;           /* Boost amount for each rule */
    float *rule_strengths;               /* Reliability of each rule (self-regulated) */
    uint32_t rule_count;                 /* Number of learned rules */
    uint32_t rule_capacity;              /* Capacity of rule arrays */
    
    /* ========================================================================
     * SELF-REGULATING PATTERN RULES (GUIDE SYSTEM BEHAVIOR)
     * ======================================================================== */
    float rule_success_rate;            /* How often this pattern's rules succeed (self-regulated) */
    float rule_confidence;              /* Confidence in pattern's rules (self-regulated) */
    uint32_t rule_attempts;             /* How many times rules were evaluated */
    uint32_t rule_successes;            /* How many times rules succeeded */
    
    /* Pattern-guided activation control */
    float activation_control_strength;  /* How strongly this pattern controls activation flow */
    float suppression_strength;         /* How strongly this pattern suppresses others */
    float boost_strength;                /* How strongly this pattern boosts others */
    
    /* ========================================================================
     * LEARNED PROPAGATION & SELECTION PARAMETERS (DATA-DRIVEN)
     * Patterns learn HOW to propagate and HOW to select from raw data
     * Like transistors - complexity comes from connections and activation patterns
     * ======================================================================== */
    
    /* PROPAGATION PARAMETERS: Learned from data, control how activation transfers */
    float propagation_transfer_rate;    /* How much activation to transfer (learned from data) */
    float propagation_decay_rate;        /* How fast activation decays (learned from data) */
    float propagation_threshold;         /* Minimum activation to propagate (learned from data) */
    float propagation_boost_factor;      /* Boost for pattern-predicted paths (learned from data) */
    
    /* SELECTION PARAMETERS: Learned from data, control how edges are selected */
    float selection_weight_factor;       /* How much edge weight matters (learned from data) */
    float selection_activation_factor;   /* How much node activation matters (learned from data) */
    float selection_context_factor;      /* How much context matters (learned from data) */
    float selection_pattern_factor;      /* How much pattern predictions matter (learned from data) */
    
    /* LEARNING TRACKING: Track what works for this pattern */
    uint32_t propagation_attempts;      /* How many times pattern tried to propagate */
    uint32_t propagation_successes;     /* How many times propagation led to correct output */
    uint32_t selection_attempts;         /* How many times pattern tried to select */
    uint32_t selection_successes;       /* How many times selection led to correct output */
    
} Pattern;

/* ============================================================================
 * HELPER: Initialize new pattern fields (all phases)
 * ============================================================================ */

void initialize_pattern_enhancements(Pattern *pat) {
    /* PHASE 1: Initialize hierarchy tracking */
    pat->chain_depth = 0;
    pat->parent_pattern_id = INVALID_PATTERN_ID;
    pat->accumulated_meaning = 0.0f;
    
    /* PHASE 2: Initialize dynamic importance */
    pat->dynamic_importance = 0.5f;
    pat->context_frequency = 0.0f;
    pat->co_occurrence_strength = 0.0f;
    
    /* PHASE 2: Initialize pattern associations */
    pat->associated_patterns = NULL;
    pat->association_strengths = NULL;
    pat->association_count = 0;
    pat->association_capacity = 0;
    
    /* PHASE 3: Initialize learned rules */
    pat->rule_condition_patterns = NULL;
    pat->rule_target_patterns = NULL;
    pat->rule_boost_amounts = NULL;
    pat->rule_strengths = NULL;
    pat->rule_count = 0;
    pat->rule_capacity = 0;
    
    /* Initialize learned propagation & selection parameters */
    /* Start with reasonable defaults, learn from data */
    pat->propagation_transfer_rate = 0.5f;      /* Start: transfer 50% of activation */
    pat->propagation_decay_rate = 0.9f;        /* Start: 90% retention */
    pat->propagation_threshold = 0.1f;          /* Start: propagate if activation > 0.1 */
    pat->propagation_boost_factor = 1.0f;       /* Start: no boost */
    
    pat->selection_weight_factor = 0.4f;       /* Start: weight matters 40% */
    pat->selection_activation_factor = 0.3f;    /* Start: activation matters 30% */
    pat->selection_context_factor = 0.2f;       /* Start: context matters 20% */
    pat->selection_pattern_factor = 0.1f;       /* Start: patterns matter 10% */
    
    pat->propagation_attempts = 0;
    pat->propagation_successes = 0;
    pat->selection_attempts = 0;
    pat->selection_successes = 0;
}

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
    
    /* SELF-TUNING: Dynamic adjustments based on performance */
    float activation_flow_adjustment;  /* Adjust path quality thresholds based on error */
    float meaning_accumulation_rate;    /* Adjust meaning accumulation speed based on error */
    float loop_breaking_strength;      /* How aggressively to break loops */
    float diversity_pressure;          /* Pressure to increase output diversity */
    
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
void learn_propagation_selection_parameters(MelvinGraph *g, const uint8_t *target, uint32_t target_len);
void detect_generalized_patterns(MelvinGraph *g);
void learn_pattern_predictions(MelvinGraph *g, const uint8_t *target, uint32_t target_len);
void learn_pattern_sequences_automatic(MelvinGraph *g);
void connect_to_similar_patterns(MelvinGraph *g, const uint32_t *sequence, uint32_t seq_len);
void pattern_backprop(MelvinGraph *g, uint32_t pattern_id, float error, const uint32_t *input_nodes, uint32_t input_len);
void create_edges_from_coactivation(MelvinGraph *g);
void create_edges_from_patterns(MelvinGraph *g);
void create_pattern_edges_from_coactivation(MelvinGraph *g);
void create_or_strengthen_pattern_edge(MelvinGraph *g, uint32_t from_pattern_id, uint32_t to_pattern_id);
void update_pattern_context_frequency(MelvinGraph *g);
void learn_pattern_association(MelvinGraph *g, uint32_t pattern_a_id, uint32_t pattern_b_id);
void learn_activation_rule(MelvinGraph *g, uint32_t condition_pattern_id, uint32_t target_pattern_id, 
                          float boost_amount, float success_rate);
float compute_semantic_distance(MelvinGraph *g, uint32_t pattern_a_id, uint32_t pattern_b_id);
void propagate_semantic_activation(MelvinGraph *g);
void actively_generalize_patterns(MelvinGraph *g);
void explore_pattern_connections(MelvinGraph *g);

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
    
    /* Initialize self-tuning adjustments */
    g->state.activation_flow_adjustment = 1.0f;
    g->state.meaning_accumulation_rate = 1.0f;
    g->state.loop_breaking_strength = 0.0f;
    g->state.diversity_pressure = 0.0f;
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
    
    /* SELF-ADJUSTING: Learning rate based on USAGE, not error_rate
     * 
     * Without feedback, we can't use error_rate. Instead:
     * - High usage = system is active = learn faster
     * - Low usage = system is inactive = learn slower
     * - Exploration pressure = need to explore = learn faster
     * 
     * This creates natural self-regulation: active paths learn faster
     */
    
    /* Compute average edge usage (how active is the system?) */
    float total_usage = 0.0f;
    uint32_t usage_count = 0;
    for (int i = 0; i < BYTE_VALUES && i < 50; i++) {  /* Sample edges */
        if (!g->nodes[i].exists) continue;
        EdgeList *out = &g->outgoing[i];
        for (uint32_t j = 0; j < out->count && j < 5; j++) {
            if (out->edges[j].active) {
                total_usage += logf(1.0f + out->edges[j].use_count);
                usage_count++;
            }
        }
    }
    float avg_usage = (usage_count > 0) ? (total_usage / usage_count) : 0.0f;
    float usage_pressure = fmin(avg_usage / 5.0f, 1.0f);  /* Normalize to [0,1] */
    
    /* Learning rate = usage pressure + exploration pressure */
    /* Active system (high usage) + need to explore = learn faster */
    g->state.learning_rate = 0.3f + (usage_pressure * 0.3f) + (g->state.exploration_pressure * 0.2f);
    if (g->state.learning_rate > 1.0f) g->state.learning_rate = 1.0f;
    
    /* Learning pressure for other uses (keep for compatibility) */
    g->state.learning_pressure = g->state.learning_rate;  /* Use learning_rate as pressure */
    
    /* Exploration pressure from error rate (high error = explore more) */
    g->state.exploration_pressure = g->state.error_rate;
    
    /* ========================================================================
     * SELF-TUNING: Control emergence through feedback loops
     * 
     * System detects its own problems and fixes them automatically:
     * - High error rate → adjust activation flow, slow meaning accumulation
     * - High loop pressure → break loops aggressively, increase diversity
     * - Low output variance → stuck → increase exploration
     * - Wrong outputs → weaken failed paths, strengthen successful ones
     * ======================================================================== */
    
    /* SELF-TUNING: Activation flow control based on error rate */
    /* High error = activation flowing wrong → be more selective */
    /* Low error = activation flowing right → keep current settings */
    g->state.activation_flow_adjustment = 1.0f + (g->state.error_rate * 2.0f);  /* High error = more aggressive filtering */
    
    /* SELF-TUNING: Meaning accumulation rate based on error */
    /* High error = meaning accumulation too fast → slow it down */
    /* Low error = meaning accumulation working → keep it */
    g->state.meaning_accumulation_rate = 1.0f - (g->state.error_rate * 0.5f);  /* High error = slower accumulation */
    
    /* SELF-TUNING: Loop breaking strength based on loop pressure */
    /* High loop pressure = stuck in loops → break them aggressively */
    g->state.loop_breaking_strength = g->state.loop_pressure * 10.0f;  /* Strong loop breaking when needed */
    
    /* SELF-TUNING: Output diversity pressure */
    /* Low variance = stuck in loops → increase diversity */
    /* High variance = too chaotic → reduce diversity */
    g->state.diversity_pressure = (1.0f - g->state.output_variance) * g->state.error_rate;  /* Low variance + high error = need diversity */
    
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
    /* NO NORMALIZATION: Weights are RELATIVE to source node, not globally normalized
     * 
     * Weight represents: "How strong is this edge relative to other edges FROM THIS NODE?"
     * 
     * Self-adjustment through:
     * - Usage: More use = stronger (natural growth)
     * - Success: Correct predictions = stronger (when feedback exists)
     * - Competition: Strong edges get more activation = stronger (circular)
     * - Pruning: Weak edges get pruned (metabolic pressure)
     * 
     * When selecting from a node, compare weights RELATIVELY:
     *   relative_weight = edge.weight / max_weight_from_node
     * 
     * This makes weights relative to the node's context, not globally normalized!
     */
    
    EdgeList *out = &g->outgoing[node_id];
    
    /* Compute max weight from this node (for relative comparison) */
    float max_weight = 0.0f;
    float sum = 0.0f;
    for (uint32_t i = 0; i < out->count; i++) {
        if (out->edges[i].active) {
            if (out->edges[i].weight > max_weight) {
                max_weight = out->edges[i].weight;
            }
            sum += out->edges[i].weight;
        }
    }
    
    /* Store max for relative comparison (not normalization) */
    out->total_weight = max_weight;  /* Max weight, not sum - for relative comparison */
    
    /* Compute metabolic load (cost is quadratic in edge count) */
    float density = (float)out->count / BYTE_VALUES;
    out->metabolic_load = density * density;
    
    /* NO NORMALIZATION: Weights grow independently, compared relatively when needed */
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
     * SELF-REGULATED THRESHOLD ADAPTATION
     * 
     * Thresholds adapt based on MEANING/IMPORTANCE, not fixed averages
     * 
     * Key insight: "cat" = low importance = low activation = easily handled
     *              "your girlfriend cheated on you" = high importance = high activation = rings for weeks
     * 
     * The system decides what's important based on:
     * - How often it's used (usage = importance)
     * - How much activation it receives (high activation = important)
     * - How well it predicts (successful predictions = important)
     * 
     * Thresholds self-regulate to ALLOW important things to stay active
     * Not force everything to average - let meaning determine activation
     * ======================================================================== */
    
    /* IMPORTANCE = learned by the system through experience */
    /* High usage + high activation + high success = important */
    float usage_importance = logf(1.0f + n->receive_count) / 10.0f;  /* More usage = more important */
    float activation_importance = (n->activation > g->state.avg_activation) ? 
        (n->activation / (g->state.avg_activation + 0.1f)) : 0.5f;  /* High activation = important */
    float success_importance = (n->receive_count > 0) ? 
        ((float)n->fire_count / (float)n->receive_count) : 0.5f;  /* High success rate = important */
    
    /* Combined importance (what the system has learned is important) */
    float importance = (usage_importance + activation_importance + success_importance) / 3.0f;
    
    /* THRESHOLD ADAPTATION: Allow important things to stay active */
    /* Important things → lower threshold (easier to activate, stay active longer) */
    /* Unimportant things → higher threshold (harder to activate, decay faster) */
    float target_threshold = 1.0f - importance;  /* Important = low threshold, unimportant = high threshold */
    float threshold_error = n->threshold - target_threshold;
    
    /* Adapt threshold toward target (self-regulation) */
    float adaptation_rate = 0.01f * g->state.learning_rate;
    n->threshold -= adaptation_rate * threshold_error;  /* Move toward target */
    
    /* Threshold naturally bounded [0,1] by sigmoid */
    n->threshold = 1.0f / (1.0f + expf(-5.0f * (n->threshold - 0.5f)));
    
    /* ACTIVATION REFLECTS MEANING: Important things can have high activation */
    /* Don't force activation to average - let meaning determine it */
    /* High activation for important things is CORRECT, not a bug */
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
    /* CRITICAL FIX: Prevent self-loops (root cause of chaotic outputs) */
    if (from_id == to_id) {
        return;  /* Never create edge from node to itself */
    }
    
    /* UNIDIRECTIONAL ENFORCEMENT: Prevent creating reverse edge if forward exists */
    /* Check if reverse edge (to→from) already exists */
    EdgeList *reverse_check = &g->outgoing[to_id];
    for (uint32_t i = 0; i < reverse_check->count; i++) {
        if (reverse_check->edges[i].to_id == from_id && reverse_check->edges[i].active) {
            /* Reverse edge exists! This would create bidirectional pair */
            /* STRICT RULE: Never allow bidirectional edges */
            /* Strengthen the existing reverse edge instead */
            reverse_check->edges[i].use_count++;
            return;  /* Don't create forward edge - keep unidirectional */
        }
    }
    
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
            /* SELF-ADJUSTING: Strengthen edge based on usage and success */
            /* Growth rate depends on:
             * - Learning rate (how fast system learns)
             * - Port penalty (same port = faster)
             * - Usage (more use = faster growth)
             * - Success rate (correct predictions = faster growth)
             */
            float old_weight = out->edges[i].weight;
            out->edges[i].use_count++;
            
            /* Base growth rate */
            float base_growth = 0.1f * g->state.learning_rate * port_penalty;
            
            /* Usage boost: More use = faster growth (log scale) */
            float usage_boost = logf(1.0f + out->edges[i].use_count) / 10.0f;
            
            /* Success boost: Correct predictions = faster growth */
            float success_rate = (out->edges[i].use_count > 0) ? 
                ((float)out->edges[i].success_count / (float)out->edges[i].use_count) : 0.0f;
            float success_boost = 1.0f + (success_rate * 2.0f);  /* 100% success = 3x growth */
            
            /* Total growth */
            float growth_rate = base_growth * (1.0f + usage_boost) * success_boost;
            
            /* Self-adjusting growth: Stronger edges grow faster (rich get richer) */
            /* But cap growth to prevent explosion */
            float max_growth = 0.5f;  /* Max 50% growth per use */
            if (growth_rate > max_growth) growth_rate = max_growth;
            
            /* Grow weight (no normalization - self-adjusting) */
            out->edges[i].weight += growth_rate;
            
            /* NO CAP: Let edges grow naturally - competition will regulate */
            /* Strong edges (high usage, high success) will grow large */
            /* Weak edges (low usage, low success) will stay small */
            /* Pruning will remove truly weak edges */
            
            // #region agent log
            FILE *f = fopen("f:\\Melvin_Research\\Melvin_o7\\melvin_o7\\.cursor\\debug.log", "a");
            if (f) {
                fprintf(f, "{\"location\":\"melvin.c:785\",\"message\":\"edge strengthened\",\"data\":{\"from\":%u,\"to\":%u,\"old_weight\":%.3f,\"new_weight\":%.3f,\"use\":%llu,\"success_rate\":%.3f},\"timestamp\":%lld,\"sessionId\":\"debug-session\",\"hypothesisId\":\"A\"}\n",
                        from_id, to_id, old_weight, out->edges[i].weight, 
                        (unsigned long long)out->edges[i].use_count,
                        success_rate,
                        (long long)time(NULL) * 1000);
                fclose(f);
            }
            // #endregion
            
            /* Update total weight (for tracking) */
            normalize_edge_weights(g, from_id);
            return;
        }
    }
    
    /* Create new edge (start with reasonable weight for exploration) */
    if (out->count >= out->capacity) {
        /* Grow array */
        out->capacity *= 2;
        out->edges = realloc(out->edges, sizeof(Edge) * out->capacity);
    }
    
    Edge *e = &out->edges[out->count];
    e->to_id = to_id;
    /* SELF-ADJUSTING: Start with base weight, grows through usage */
    e->weight = 0.5f * port_penalty; /* Start at 0.5 (or 0.15 for cross-port) */
    e->use_count = 1;
    e->success_count = 0;
    e->active = true;
    
    out->count++;
    
    /* Update total weight (for tracking, not normalization) */
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
    
    /* Update total weight (for tracking, not normalization) */
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
 * PATTERN SIMILARITY (for generalization)
 * 
 * Compute how similar a pattern is to a sequence
 * Returns similarity score [0,1] where 1.0 = exact match, 0.0 = no match
 * Used to connect new words to similar patterns for generalization
 * ============================================================================ */

/* REMOVED: pattern_similarity() - redundant with blank nodes
 * 
 * Blank nodes ARE the generalization mechanism:
 * - Pattern "_at" (blank + "at") matches "cat", "bat", "rat", "quokka" (if ends in "at")
 * - Blank nodes match any byte = automatic generalization
 * - No need for similarity scores - blank nodes handle it!
 * 
 * When new words are seen:
 * 1. Existing patterns with blank nodes automatically match (via pattern_matches)
 * 2. detect_generalized_patterns() creates new blank node patterns for similar sequences
 * 3. connect_to_similar_patterns() connects new words to matching blank node patterns
 */

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
                /* Try exact match first */
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
                /* BLANK NODES HANDLE GENERALIZATION - no need for similarity matching */
                /* If pattern has blank nodes, pattern_matches() already handles generalization */
                /* Blank nodes match any byte, so patterns like "_at" automatically match "cat", "bat", "quokka" */
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
                    /* CRITICAL FIX: Reduce prediction blocking - allow patterns to fire multiple times */
                    /* Only block if prediction was used in LAST output (not all time) */
                    bool prediction_used = (pat->fired_predictions & (1u << pred)) != 0;
                    
                    /* Allow prediction if it hasn't been used OR if enough time has passed */
                    bool can_predict = !prediction_used || (g->state.step > pat->last_fired_step + 3);
                    
                    if (target_node < BYTE_VALUES && can_predict) {
                        /* Track prediction attempts (for utility calculation) */
                        pat->prediction_attempts++;
                        
                        /* INTELLIGENT PATH: Patterns are learned paths - follow them STRONGLY */
                        /* Patterns are learned intelligence - they predict where activation should go */
                        /* Transfer based on pattern activation, prediction weight, and pattern strength */
                        /* Patterns start at 1.0 weight, so no artificial boost needed */
                        float transfer = pat->activation * weight * pat->strength;
                        /* No boost - pattern weight is already 1.0 (full strength) */
                        
                        g->nodes[target_node].activation += transfer;
                        g->nodes[target_node].receive_count++;
                    }
                }
                
                /* ========================================================================
                 * PHASE 1: PATTERN HIERARCHY ACTIVATION WITH MEANING ACCUMULATION
                 * ======================================================================== */
                if (pat->pattern_prediction_count > 0) {
                    for (uint32_t ppred = 0; ppred < pat->pattern_prediction_count; ppred++) {
                        uint32_t target_pattern_id = pat->predicted_patterns[ppred];
                        if (target_pattern_id >= g->pattern_count) continue;
                        
                        Pattern *target_pat = &g->patterns[target_pattern_id];
                        float pattern_pred_weight = pat->pattern_prediction_weights[ppred];
                        
                        /* PHASE 1: Update chain depth (child is one level deeper than parent) */
                        /* Only update if this is a better parent (closer in chain) */
                        if (target_pat->parent_pattern_id == INVALID_PATTERN_ID) {
                            target_pat->parent_pattern_id = p;
                            target_pat->chain_depth = pat->chain_depth + 1;
                        } else if (pat->chain_depth < g->patterns[target_pat->parent_pattern_id].chain_depth) {
                            /* This parent is closer to root - update */
                            target_pat->parent_pattern_id = p;
                            target_pat->chain_depth = pat->chain_depth + 1;
                        }
                        
                        /* PHASE 1: Accumulate meaning through chain */
                        /* CONNECTIONS ARE UNDERSTANDING: When patterns connect, they build meaning */
                        float parent_meaning = pat->accumulated_meaning;
                        float chain_meaning = parent_meaning * pattern_pred_weight * pat->strength;
                        /* If this pattern has no meaning yet, start with its activation */
                        if (parent_meaning < 0.1f) {
                            parent_meaning = pat->activation;
                            chain_meaning = parent_meaning * pattern_pred_weight * pat->strength;
                        }
                        
                        /* CONNECTION BOOST: Patterns that connect to many others have more meaning */
                        /* This reflects that understanding comes from connections */
                        float connection_boost = 1.0f + (logf(1.0f + pat->outgoing_patterns.count + pat->association_count) / 5.0f);
                        chain_meaning *= connection_boost;
                        
                        /* HIERARCHY BOOST: Higher in hierarchy = more abstract = more understanding */
                        /* Deeper patterns (closer to root) represent more abstract concepts */
                        float hierarchy_boost = 1.0f + (1.0f / (1.0f + pat->chain_depth * 0.3f));
                        chain_meaning *= hierarchy_boost;
                        
                        /* SELF-TUNING: Meaning accumulation rate adjusts based on error */
                        /* High error = slow accumulation (system is making mistakes) */
                        /* Low error = fast accumulation (system is working) */
                        chain_meaning *= g->state.meaning_accumulation_rate;
                        
                        /* FIX: Cap meaning to prevent overflow (inf/nan) */
                        /* Meaning should be bounded - use log scale for very high values */
                        if (chain_meaning > 100.0f) {
                            /* Very high meaning - use log scale to prevent overflow */
                            chain_meaning = 100.0f + logf(chain_meaning / 100.0f);
                        }
                        if (chain_meaning > 1000.0f) chain_meaning = 1000.0f;  /* Hard cap */
                        
                        target_pat->accumulated_meaning = fmax(target_pat->accumulated_meaning, chain_meaning);
                        
                        /* FIX: Cap accumulated_meaning to prevent overflow */
                        if (target_pat->accumulated_meaning > 1000.0f) {
                            target_pat->accumulated_meaning = 1000.0f;
                        }
                        /* Check for NaN/Inf */
                        if (target_pat->accumulated_meaning != target_pat->accumulated_meaning || 
                            target_pat->accumulated_meaning > 1e6f) {
                            target_pat->accumulated_meaning = 1.0f;  /* Reset to safe value */
                        }
                        
                        /* PHASE 1: Meaning multiplier boosts activation for complex concepts */
                        /* SELF-TUNING: Adjust multiplier based on error rate */
                        /* FIX: Use bounded meaning to prevent overflow */
                        float bounded_meaning = target_pat->accumulated_meaning;
                        if (bounded_meaning > 100.0f) {
                            /* Use log scale for very high meaning */
                            bounded_meaning = 100.0f + logf(bounded_meaning / 100.0f) * 10.0f;
                        }
                        if (bounded_meaning > 200.0f) bounded_meaning = 200.0f;  /* Cap for multiplier */
                        float base_multiplier = 1.0f + (bounded_meaning * 0.5f);
                        /* High error = reduce meaning boost (system is over-weighting complex concepts) */
                        float meaning_multiplier = base_multiplier * (1.0f - g->state.error_rate * 0.3f);
                        /* Cap multiplier to prevent explosion */
                        if (meaning_multiplier > 50.0f) meaning_multiplier = 50.0f;
                        
                        /* Transfer activation with meaning accumulation */
                        float pattern_transfer = pat->activation * pattern_pred_weight * pat->strength * meaning_multiplier;
                        target_pat->activation += pattern_transfer;
                        
                        /* Cap activation to prevent explosion */
                        if (target_pat->activation > 10.0f) target_pat->activation = 10.0f;
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
                
                /* ========================================================================
                 * PHASE 2: UPDATE DYNAMIC IMPORTANCE
                 * ======================================================================== */
                /* Importance = usage + success + hierarchy + co-occurrence */
                float usage_importance = logf(1.0f + pat->prediction_attempts) / 10.0f;
                float success_importance = (pat->prediction_attempts > 0) ? 
                    ((float)pat->prediction_successes / (float)pat->prediction_attempts) : 0.5f;
                float hierarchy_importance = 1.0f / (1.0f + pat->chain_depth * 0.5f);  /* Deeper = more abstract = more important */
                float co_occurrence_importance = pat->co_occurrence_strength;
                
                pat->dynamic_importance = (usage_importance + success_importance + 
                                          hierarchy_importance + co_occurrence_importance) / 4.0f;
                
                /* ========================================================================
                 * SELF-REGULATING: Update pattern rule success rate and confidence
                 * ======================================================================== */
                /* Patterns self-regulate their rule behavior based on success */
                if (pat->rule_attempts > 0) {
                    pat->rule_success_rate = (float)pat->rule_successes / (float)pat->rule_attempts;
                }
                
                /* Rule confidence = how reliable this pattern's rules are */
                /* High success rate = high confidence = rules are reliable */
                /* Start with moderate confidence (0.6) for new patterns, adapt based on success */
                if (pat->rule_attempts == 0) {
                    pat->rule_confidence = 0.6f;  /* Start with moderate confidence for new patterns */
                } else {
                    pat->rule_confidence = 0.5f + (pat->rule_success_rate - 0.5f) * 2.0f;  /* Map [0,1] to [0,1] */
                }
                if (pat->rule_confidence < 0.1f) pat->rule_confidence = 0.1f;
                if (pat->rule_confidence > 1.0f) pat->rule_confidence = 1.0f;
                
                /* Activation control strength = how much this pattern guides the system */
                /* Successful patterns get more control authority */
                pat->activation_control_strength = pat->rule_confidence * pat->dynamic_importance;
                
                /* Boost/suppression strength adapts based on success */
                /* Successful patterns boost more, suppress less */
                /* Increase boost strength to make pattern guidance more effective */
                pat->boost_strength = pat->rule_confidence * 0.8f;  /* Increased from 0.5f */
                pat->suppression_strength = (1.0f - pat->rule_confidence) * 0.2f;  /* Reduced from 0.3f */
                
                /* PHASE 2: Important patterns get activation boost */
                /* SELF-TUNING: Adjust boost based on error rate and pattern success */
                /* High error = reduce importance boost (system is over-weighting wrong patterns) */
                /* Low error = keep importance boost (system is working) */
                float importance_boost_base = 1.0f + (pat->dynamic_importance * 2.0f);
                
                /* SELF-TUNING: Patterns with low success rate get less boost (they're failing) */
                float pattern_success_rate = (pat->prediction_attempts > 0) ?
                    ((float)pat->prediction_successes / (float)pat->prediction_attempts) : 0.5f;
                float success_adjustment = 0.5f + pattern_success_rate;  /* Failed patterns get 0.5x, successful get 1.0x */
                
                float importance_boost = importance_boost_base * (1.0f - g->state.error_rate * 0.4f) * success_adjustment;
                pat->activation *= importance_boost;
                
                /* ========================================================================
                 * PHASE 2: PATTERN ASSOCIATION NETWORKS
                 * ======================================================================== */
                /* When pattern activates, boost associated patterns (co-occurrence) */
                /* NATURAL SELF-REGULATION: Patterns naturally boost similar patterns */
                /* High-confidence patterns boost other high-confidence patterns (they "know" together) */
                /* Low-confidence patterns boost other low-confidence patterns (they're "confused" together) */
                /* System state emerges from these natural interactions */
                for (uint32_t assoc = 0; assoc < pat->association_count; assoc++) {
                    uint32_t assoc_pattern_id = pat->associated_patterns[assoc];
                    if (assoc_pattern_id >= g->pattern_count) continue;
                    
                    Pattern *assoc_pat = &g->patterns[assoc_pattern_id];
                    float assoc_strength = pat->association_strengths[assoc];
                    
                    /* CONFIDENCE SIMILARITY: Similar patterns boost each other more */
                    float confidence_pat = (pat->prediction_attempts > 0) ?
                        ((float)pat->prediction_successes / (float)pat->prediction_attempts) : 0.5f;
                    float confidence_assoc = (assoc_pat->prediction_attempts > 0) ?
                        ((float)assoc_pat->prediction_successes / (float)assoc_pat->prediction_attempts) : 0.5f;
                    float confidence_similarity = 1.0f - fabsf(confidence_pat - confidence_assoc);
                    
                    /* HIERARCHY SIMILARITY: Patterns at similar hierarchy levels boost each other more */
                    float hierarchy_similarity = 1.0f / (1.0f + fabsf((float)pat->chain_depth - (float)assoc_pat->chain_depth));
                    
                    /* Similarity boost: similar patterns boost each other more strongly */
                    float similarity_boost = (confidence_similarity * 0.6f + hierarchy_similarity * 0.4f);
                    
                    /* Boost associated pattern activation - stronger if similar */
                    float assoc_activation = pat->activation * assoc_strength * 0.5f * (0.7f + similarity_boost * 0.3f);
                    assoc_pat->activation += assoc_activation;
                    if (assoc_pat->activation > 10.0f) assoc_pat->activation = 10.0f;
                }
                
                /* ========================================================================
                 * PHASE 3: HIERARCHICAL PROPAGATION (Bidirectional)
                 * ======================================================================== */
                /* Bottom-up: Boost parent pattern (if exists) */
                /* HIERARCHY BUILDS UNDERSTANDING: Children contribute meaning to parents */
                /* Higher in hierarchy = more abstract = more understanding */
                if (pat->parent_pattern_id != INVALID_PATTERN_ID && pat->parent_pattern_id < g->pattern_count) {
                    Pattern *parent_pat = &g->patterns[pat->parent_pattern_id];
                    float child_meaning = pat->accumulated_meaning;
                    if (child_meaning < 0.1f) child_meaning = pat->activation;
                    
                    /* CONNECTION BOOST: Child's connections contribute to parent's understanding */
                    float child_connections = pat->outgoing_patterns.count + pat->association_count;
                    float connection_contribution = logf(1.0f + child_connections) / 3.0f;
                    child_meaning += connection_contribution;
                    
                    /* FIX: Cap child_meaning to prevent overflow */
                    if (child_meaning > 100.0f) {
                        child_meaning = 100.0f + logf(child_meaning / 100.0f) * 10.0f;
                    }
                    if (child_meaning > 200.0f) child_meaning = 200.0f;
                    
                    parent_pat->activation += child_meaning * 0.3f;  /* Boost parent */
                    parent_pat->accumulated_meaning += child_meaning * 0.2f;
                    
                    /* FIX: Cap accumulated_meaning */
                    if (parent_pat->accumulated_meaning > 1000.0f) {
                        parent_pat->accumulated_meaning = 1000.0f;
                    }
                    if (parent_pat->accumulated_meaning != parent_pat->accumulated_meaning || 
                        parent_pat->accumulated_meaning > 1e6f) {
                        parent_pat->accumulated_meaning = 1.0f;  /* Reset if NaN/Inf */
                    }
                    
                    if (parent_pat->activation > 10.0f) parent_pat->activation = 10.0f;
                }
                
                /* Top-down: Boost child patterns (through predictions) */
                /* This is already handled in pattern predictions above */
                
                /* ========================================================================
                 * PHASE 3: SELF-REGULATING PATTERN RULES (IF-THEN BEHAVIOR)
                 * ======================================================================== */
                /* Patterns act as rules: IF condition THEN action */
                /* Rules self-regulate their strength based on success/failure */
                for (uint32_t rule = 0; rule < pat->rule_count; rule++) {
                    uint32_t condition_id = pat->rule_condition_patterns[rule];
                    if (condition_id >= g->pattern_count) continue;
                    
                    Pattern *condition_pat = &g->patterns[condition_id];
                    bool condition_met = (condition_pat->activation > condition_pat->threshold);
                    
                    if (condition_met) {
                        /* THEN execute action: boost target pattern */
                        uint32_t target_id = pat->rule_target_patterns[rule];
                        if (target_id < g->pattern_count) {
                            Pattern *target_pat = &g->patterns[target_id];
                            
                            /* SELF-REGULATING: Rule strength adapts based on success */
                            /* Successful rules get stronger, failed rules get weaker */
                            float base_boost = pat->rule_boost_amounts[rule];
                            float rule_strength = pat->rule_strengths[rule];
                            
                            /* Apply rule with self-regulated strength and confidence */
                            float boost = base_boost * rule_strength * pat->rule_confidence;
                            target_pat->activation += condition_pat->activation * boost;
                            if (target_pat->activation > 10.0f) target_pat->activation = 10.0f;
                            
                            /* Track rule evaluation (for self-regulation) */
                            pat->rule_attempts++;
                        }
                    }
                }
                
                /* ========================================================================
                 * PATTERN-GUIDED ACTIVATION CONTROL
                 * ======================================================================== */
                /* Patterns guide system by controlling activation flow */
                /* High control strength = pattern actively guides system behavior */
                /* Lower threshold to allow more patterns to guide (0.2 instead of 0.3) */
                if (pat->activation > pat->threshold && pat->activation_control_strength > 0.2f) {
                    /* Pattern is active and has control authority */
                    
                    /* Boost patterns this pattern wants to activate */
                    if (pat->boost_strength > 0.1f) {
                        /* Boost associated patterns (patterns this pattern guides) */
                        for (uint32_t assoc = 0; assoc < pat->association_count; assoc++) {
                            uint32_t assoc_id = pat->associated_patterns[assoc];
                            if (assoc_id < g->pattern_count) {
                                Pattern *assoc_pat = &g->patterns[assoc_id];
                                float boost = pat->activation * pat->boost_strength * pat->rule_confidence;
                                assoc_pat->activation += boost;
                                if (assoc_pat->activation > 10.0f) assoc_pat->activation = 10.0f;
                            }
                        }
                    }
                    
                    /* Suppress patterns this pattern wants to suppress */
                    if (pat->suppression_strength > 0.1f) {
                        /* Suppress competing patterns (patterns that conflict with this one) */
                        /* This is learned - patterns learn what to suppress based on failure */
                        /* For now, suppress patterns with low success rate when this pattern is active */
                        for (uint32_t p2 = 0; p2 < g->pattern_count; p2++) {
                            if (p2 == p) continue;  /* Don't suppress self */
                            Pattern *other_pat = &g->patterns[p2];
                            if (other_pat->activation > other_pat->threshold) {
                                float other_success = (other_pat->prediction_attempts > 0) ?
                                    ((float)other_pat->prediction_successes / (float)other_pat->prediction_attempts) : 0.5f;
                                /* Suppress patterns with low success when this pattern is active */
                                if (other_success < 0.3f) {
                                    float suppression = pat->activation * pat->suppression_strength * pat->rule_confidence;
                                    other_pat->activation *= (1.0f - suppression);
                                }
                            }
                        }
                    }
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
 * PHASE 2: LEARN PATTERN ASSOCIATIONS (Co-occurrence)
 * ============================================================================ */

void learn_pattern_association(MelvinGraph *g, uint32_t pattern_a_id, uint32_t pattern_b_id) {
    if (pattern_a_id >= g->pattern_count || pattern_b_id >= g->pattern_count) return;
    if (pattern_a_id == pattern_b_id) return;  /* No self-association */
    
    Pattern *pat_a = &g->patterns[pattern_a_id];
    Pattern *pat_b = &g->patterns[pattern_b_id];
    
    /* NATURAL SELF-REGULATION: Patterns connect based on confidence similarity */
    /* High-confidence patterns naturally connect to other high-confidence patterns (they "know" together) */
    /* Low-confidence patterns naturally connect to other low-confidence patterns (they're "confused" together) */
    float confidence_a = (pat_a->prediction_attempts > 0) ?
        ((float)pat_a->prediction_successes / (float)pat_a->prediction_attempts) : 0.5f;
    float confidence_b = (pat_b->prediction_attempts > 0) ?
        ((float)pat_b->prediction_successes / (float)pat_b->prediction_attempts) : 0.5f;
    
    /* Confidence similarity: patterns with similar confidence connect more strongly */
    float confidence_similarity = 1.0f - fabsf(confidence_a - confidence_b);
    
    /* HIERARCHY SIMILARITY: Patterns at similar hierarchy levels connect more strongly */
    /* Higher in hierarchy = more abstract = patterns about patterns = self-understanding */
    float hierarchy_similarity = 1.0f / (1.0f + fabsf((float)pat_a->chain_depth - (float)pat_b->chain_depth));
    
    /* Combined similarity: patterns that are similar in confidence AND hierarchy connect strongly */
    float similarity_boost = (confidence_similarity * 0.6f + hierarchy_similarity * 0.4f);
    
    /* Check if association already exists */
    bool found = false;
    for (uint32_t i = 0; i < pat_a->association_count; i++) {
        if (pat_a->associated_patterns[i] == pattern_b_id) {
            /* Strengthen existing association - stronger if similar */
            float base_strength = 0.1f * g->state.learning_rate;
            pat_a->association_strengths[i] += base_strength * similarity_boost;
            if (pat_a->association_strengths[i] > 1.0f) pat_a->association_strengths[i] = 1.0f;
            found = true;
            break;
        }
    }
    
    if (!found) {
        /* Create new association - stronger if similar */
        if (pat_a->association_count >= pat_a->association_capacity) {
            pat_a->association_capacity = (pat_a->association_capacity == 0) ? 4 : pat_a->association_capacity * 2;
            pat_a->associated_patterns = realloc(pat_a->associated_patterns, 
                                                sizeof(uint32_t) * pat_a->association_capacity);
            pat_a->association_strengths = realloc(pat_a->association_strengths, 
                                                   sizeof(float) * pat_a->association_capacity);
        }
        
        pat_a->associated_patterns[pat_a->association_count] = pattern_b_id;
        pat_a->association_strengths[pat_a->association_count] = 0.1f * g->state.learning_rate * similarity_boost;
        pat_a->association_count++;
        
        /* Update co-occurrence strength - stronger if similar */
        float co_occurrence_update = 0.1f * similarity_boost;
        pat_a->co_occurrence_strength = (pat_a->co_occurrence_strength + co_occurrence_update) / 2.0f;
        pat_b->co_occurrence_strength = (pat_b->co_occurrence_strength + co_occurrence_update) / 2.0f;
    }
}

/* ============================================================================
 * PHASE 3: LEARN ACTIVATION RULES
 * ============================================================================ */

void learn_activation_rule(MelvinGraph *g, uint32_t condition_pattern_id, uint32_t target_pattern_id, 
                          float boost_amount, float success_rate) {
    if (condition_pattern_id >= g->pattern_count || target_pattern_id >= g->pattern_count) return;
    if (condition_pattern_id == target_pattern_id) return;  /* No self-rule */
    
    Pattern *condition_pat = &g->patterns[condition_pattern_id];
    
    /* Check if rule already exists */
    bool found = false;
    for (uint32_t i = 0; i < condition_pat->rule_count; i++) {
        if (condition_pat->rule_condition_patterns[i] == condition_pattern_id &&
            condition_pat->rule_target_patterns[i] == target_pattern_id) {
            /* Strengthen existing rule */
            condition_pat->rule_boost_amounts[i] = (condition_pat->rule_boost_amounts[i] + boost_amount) / 2.0f;
            condition_pat->rule_strengths[i] = (condition_pat->rule_strengths[i] + success_rate) / 2.0f;
            found = true;
            break;
        }
    }
    
    if (!found) {
        /* Create new rule */
        if (condition_pat->rule_count >= condition_pat->rule_capacity) {
            condition_pat->rule_capacity = (condition_pat->rule_capacity == 0) ? 4 : condition_pat->rule_capacity * 2;
            condition_pat->rule_condition_patterns = realloc(condition_pat->rule_condition_patterns,
                                                             sizeof(uint32_t) * condition_pat->rule_capacity);
            condition_pat->rule_target_patterns = realloc(condition_pat->rule_target_patterns,
                                                          sizeof(uint32_t) * condition_pat->rule_capacity);
            condition_pat->rule_boost_amounts = realloc(condition_pat->rule_boost_amounts,
                                                        sizeof(float) * condition_pat->rule_capacity);
            condition_pat->rule_strengths = realloc(condition_pat->rule_strengths,
                                                   sizeof(float) * condition_pat->rule_capacity);
        }
        
        condition_pat->rule_condition_patterns[condition_pat->rule_count] = condition_pattern_id;
        condition_pat->rule_target_patterns[condition_pat->rule_count] = target_pattern_id;
        condition_pat->rule_boost_amounts[condition_pat->rule_count] = boost_amount;
        condition_pat->rule_strengths[condition_pat->rule_count] = success_rate;
        condition_pat->rule_count++;
        
        /* SELF-REGULATING: Track rule creation for self-regulation */
        /* Rules start with moderate confidence, adapt based on success */
    }
}

/* ============================================================================
 * PHASE 3: COMPUTE SEMANTIC DISTANCE
 * ============================================================================ */

float compute_semantic_distance(MelvinGraph *g, uint32_t pattern_a_id, uint32_t pattern_b_id) {
    if (pattern_a_id >= g->pattern_count || pattern_b_id >= g->pattern_count) return 1.0f;
    if (pattern_a_id == pattern_b_id) return 0.0f;
    
    Pattern *pat_a = &g->patterns[pattern_a_id];
    Pattern *pat_b = &g->patterns[pattern_b_id];
    
    /* Factor 1: Co-occurrence distance */
    float co_occurrence_dist = 1.0f;
    for (uint32_t i = 0; i < pat_a->association_count; i++) {
        if (pat_a->associated_patterns[i] == pattern_b_id) {
            co_occurrence_dist = 1.0f - pat_a->association_strengths[i];
            break;
        }
    }
    
    /* Factor 2: Shared predictions */
    float shared_pred_ratio = 0.0f;
    uint32_t shared_count = 0;
    uint32_t total_pred = pat_a->prediction_count + pat_b->prediction_count;
    if (total_pred > 0) {
        for (uint32_t i = 0; i < pat_a->prediction_count; i++) {
            for (uint32_t j = 0; j < pat_b->prediction_count; j++) {
                if (pat_a->predicted_nodes[i] == pat_b->predicted_nodes[j]) {
                    shared_count++;
                    break;
                }
            }
        }
        shared_pred_ratio = (float)shared_count / (float)total_pred;
    }
    float shared_pred_dist = 1.0f - shared_pred_ratio;
    
    /* Factor 3: Hierarchy distance */
    float hierarchy_dist = fabs((float)pat_a->chain_depth - (float)pat_b->chain_depth) / 10.0f;
    if (hierarchy_dist > 1.0f) hierarchy_dist = 1.0f;
    
    /* Combined semantic distance */
    return (co_occurrence_dist + shared_pred_dist + hierarchy_dist) / 3.0f;
}

/* ============================================================================
 * PHASE 3: SEMANTIC DISTANCE ACTIVATION
 * ============================================================================ */

void propagate_semantic_activation(MelvinGraph *g) {
    /* For each active pattern, boost semantically close patterns */
    for (uint32_t p = 0; p < g->pattern_count; p++) {
        Pattern *pat = &g->patterns[p];
        if (pat->activation < pat->threshold || pat->activation < 0.1f) continue;
        
        /* Find semantically close patterns */
        for (uint32_t q = 0; q < g->pattern_count; q++) {
            if (p == q) continue;
            
            Pattern *other_pat = &g->patterns[q];
            float distance = compute_semantic_distance(g, p, q);
            
            /* Close patterns get activation boost */
            if (distance < 0.5f) {
                float distance_factor = 1.0f / (1.0f + distance);
                float semantic_activation = pat->activation * distance_factor * 0.2f;
                other_pat->activation += semantic_activation;
                if (other_pat->activation > 10.0f) other_pat->activation = 10.0f;
            }
        }
    }
}

/* ============================================================================
 * DETECT GENERALIZED PATTERNS (with blank nodes)
 * 
 * Find patterns like "_at" that match "cat", "bat", "rat"
 * ============================================================================ */

void detect_generalized_patterns(MelvinGraph *g) {
    /* GENERALIZATION: Create blank node patterns for similar sequences
     * 
     * When similar sequences are found, create patterns with blank nodes.
     * Example: "cat", "bat", "rat" → pattern "_at" (blank matches c, b, r)
     * 
     * This is how the system generalizes - blank nodes match any byte,
     * so patterns like "_at" automatically match new words ending in "at"
     */
    
    if (g->input_length < 3) return;
    
    /* Look for patterns with one blank: _at, c_t, ca_ */
    for (uint32_t i = 0; i < g->input_length - 2; i++) {
        uint32_t b = g->input_buffer[i + 1];
        uint32_t c = g->input_buffer[i + 2];
        
        /* Try pattern: _bc (blank first) - matches any Xbc sequence */
        uint32_t match_count = 0;
        
        for (uint32_t j = 0; j < g->input_length - 2; j++) {
            /* Check if sequence has same bc ending (blank matches any first char) */
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
                
                /* Initialize all enhancement fields */
                initialize_pattern_enhancements(pat);
                
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
 * ACTIVE GENERALIZATION: Patterns try to create blank node variants
 * 
 * Intelligence: Patterns actively explore connections by trying blank node
 * substitutions. This is how patterns "try out connections" - they generalize
 * themselves to see if they can connect to other patterns.
 * 
 * Hierarchy builds understanding: When a pattern generalizes, it moves up
 * the hierarchy (becomes more abstract). Higher = more understanding.
 * ============================================================================ */

void actively_generalize_patterns(MelvinGraph *g) {
    if (g->pattern_count == 0) return;
    
    /* Only generalize patterns that are active and have some connections */
    /* But also try patterns with high activation but few connections (they need exploration) */
    for (uint32_t p = 0; p < g->pattern_count; p++) {
        Pattern *pat = &g->patterns[p];
        
        /* Skip if pattern is too short or too long */
        if (pat->length < 2 || pat->length > 10) continue;
        
        /* Skip if pattern already has many blank nodes (already generalized) */
        uint32_t blank_count = 0;
        for (uint32_t i = 0; i < pat->length; i++) {
            if (pat->node_ids[i] == BLANK_NODE) blank_count++;
        }
        if (blank_count >= pat->length / 2) continue;  /* Already too generalized */
        
        /* Generalization pressure: based on activation and connection need */
        float connection_need = 0.0f;
        if (pat->outgoing_patterns.count < 3 && pat->association_count < 3) {
            connection_need = 1.0f;  /* Needs more connections */
        }
        
        float generalization_pressure = (pat->activation * 0.5f) + (connection_need * 0.5f);
        
        /* Only generalize if pressure is high enough */
        if (generalization_pressure < 0.3f) continue;
        
        /* Try creating blank node variants at each position */
        for (uint32_t pos = 0; pos < pat->length; pos++) {
            if (pat->node_ids[pos] == BLANK_NODE) continue;  /* Already blank */
            
            /* Create variant with blank at this position */
            uint32_t variant_length = pat->length;
            uint32_t *variant_nodes = malloc(sizeof(uint32_t) * variant_length);
            if (!variant_nodes) continue;
            
            for (uint32_t i = 0; i < variant_length; i++) {
                variant_nodes[i] = (i == pos) ? BLANK_NODE : pat->node_ids[i];
            }
            
            /* Check if this variant already exists */
            bool variant_exists = false;
            for (uint32_t q = 0; q < g->pattern_count; q++) {
                if (q == p) continue;
                Pattern *other = &g->patterns[q];
                if (other->length != variant_length) continue;
                
                bool matches = true;
                for (uint32_t i = 0; i < variant_length; i++) {
                    if (variant_nodes[i] != other->node_ids[i]) {
                        matches = false;
                        break;
                    }
                }
                if (matches) {
                    variant_exists = true;
                    /* Strengthen existing variant if it matches */
                    other->strength += 0.05f * generalization_pressure;
                    if (other->strength > 1.0f) other->strength = 1.0f;
                    break;
                }
            }
            
            if (!variant_exists) {
                /* Check how many patterns this variant would match */
                uint32_t match_count = 0;
                for (uint32_t q = 0; q < g->pattern_count; q++) {
                    if (q == p) continue;
                    Pattern *other = &g->patterns[q];
                    if (other->length != variant_length) continue;
                    
                    bool matches = true;
                    for (uint32_t i = 0; i < variant_length; i++) {
                        if (variant_nodes[i] != BLANK_NODE && 
                            variant_nodes[i] != other->node_ids[i]) {
                            matches = false;
                            break;
                        }
                    }
                    if (matches) match_count++;
                }
                
                /* Only create if it would match at least 2 other patterns */
                if (match_count >= 2) {
                    /* Grow pattern array if needed */
                    if (g->pattern_count >= g->pattern_capacity) {
                        g->pattern_capacity *= 2;
                        g->patterns = realloc(g->patterns, sizeof(Pattern) * g->pattern_capacity);
                    }
                    
                    /* Create generalized pattern */
                    Pattern *generalized = &g->patterns[g->pattern_count++];
                    generalized->node_ids = variant_nodes;
                    generalized->length = variant_length;
                    
                    /* Initialize fields */
                    generalized->sub_pattern_ids = NULL;
                    generalized->sub_pattern_count = 0;
                    generalized->predicted_nodes = NULL;
                    generalized->prediction_weights = NULL;
                    generalized->prediction_count = 0;
                    generalized->predicted_patterns = NULL;
                    generalized->pattern_prediction_weights = NULL;
                    generalized->pattern_prediction_count = 0;
                    initialize_pattern_enhancements(generalized);
                    
                    /* Initialize relative to parent pattern */
                    generalized->threshold = pat->threshold;
                    generalized->input_weights = NULL;
                    generalized->bias = 0.0f;
                    generalized->input_size = 0;
                    
                    /* Strength from generalization: matches multiple patterns = more valuable */
                    float generalization_benefit = (float)match_count * 0.3f;
                    generalized->strength = pat->strength * 0.8f + generalization_benefit;
                    if (generalized->strength > 1.0f) generalized->strength = 1.0f;
                    generalized->activation = pat->activation * 0.5f;
                    generalized->prediction_attempts = 0;
                    generalized->prediction_successes = 0;
                    generalized->has_fired = false;
                    generalized->last_fired_step = 0;
                    generalized->fired_predictions = 0;
                    
                    /* Set hierarchy: generalized pattern is parent of original */
                    generalized->chain_depth = pat->chain_depth;
                    generalized->parent_pattern_id = pat->parent_pattern_id;
                    generalized->accumulated_meaning = pat->accumulated_meaning * 1.2f;  /* More abstract = more meaning */
                    
                    /* Original pattern becomes child of generalized */
                    pat->parent_pattern_id = g->pattern_count - 1;
                    pat->chain_depth = generalized->chain_depth + 1;
                    
                    /* Copy port info from parent */
                    generalized->input_port = pat->input_port;
                    generalized->output_port = pat->output_port;
                    
                    /* Initialize pattern-to-pattern edge lists */
                    generalized->outgoing_patterns.edges = malloc(sizeof(Edge) * INITIAL_CAPACITY);
                    generalized->outgoing_patterns.count = 0;
                    generalized->outgoing_patterns.capacity = INITIAL_CAPACITY;
                    generalized->outgoing_patterns.total_weight = 0.0f;
                    generalized->outgoing_patterns.metabolic_load = 0.0f;
                    
                    generalized->incoming_patterns.edges = malloc(sizeof(Edge) * INITIAL_CAPACITY);
                    generalized->incoming_patterns.count = 0;
                    generalized->incoming_patterns.capacity = INITIAL_CAPACITY;
                    generalized->incoming_patterns.total_weight = 0.0f;
                    generalized->incoming_patterns.metabolic_load = 0.0f;
                    
                    /* Create edge from generalized to original (parent→child) */
                    create_or_strengthen_pattern_edge(g, g->pattern_count - 1, p);
                } else {
                    free(variant_nodes);
                }
            } else {
                free(variant_nodes);
            }
        }
    }
}

/* ============================================================================
 * EXPLORE PATTERN CONNECTIONS: Patterns try blank node substitutions to find connections
 * 
 * Intelligence: When a pattern has high activation but few connections, it
 * tries substituting blank nodes to see if it can match and connect to other patterns.
 * This is how patterns "try out connections" - active exploration.
 * ============================================================================ */

void explore_pattern_connections(MelvinGraph *g) {
    if (g->pattern_count < 2) return;
    
    /* Find patterns with high activation but few connections (they need exploration) */
    for (uint32_t p = 0; p < g->pattern_count; p++) {
        Pattern *pat = &g->patterns[p];
        
        /* Skip if already well-connected */
        if (pat->outgoing_patterns.count >= 5 && pat->association_count >= 5) continue;
        
        /* Skip if activation is too low */
        if (pat->activation < 0.3f) continue;
        
        /* Skip if pattern is too short */
        if (pat->length < 2) continue;
        
        /* Try to find patterns that could connect if we use blank nodes */
        for (uint32_t q = 0; q < g->pattern_count; q++) {
            if (q == p) continue;
            Pattern *other = &g->patterns[q];
            
            /* Skip if already connected */
            bool already_connected = false;
            for (uint32_t e = 0; e < pat->outgoing_patterns.count; e++) {
                if (pat->outgoing_patterns.edges[e].to_id == q) {
                    already_connected = true;
                    break;
                }
            }
            if (already_connected) continue;
            
            /* Check if patterns could match with blank node substitutions */
            if (pat->length == other->length) {
                /* Count differences */
                uint32_t differences = 0;
                uint32_t diff_positions[10];
                for (uint32_t i = 0; i < pat->length && differences < 10; i++) {
                    if (pat->node_ids[i] != other->node_ids[i] &&
                        pat->node_ids[i] != BLANK_NODE &&
                        other->node_ids[i] != BLANK_NODE) {
                        diff_positions[differences++] = i;
                    }
                }
                
                /* If only 1-2 differences, create a generalized pattern that matches both */
                if (differences >= 1 && differences <= 2) {
                    /* Create generalized pattern with blanks at difference positions */
                    uint32_t *generalized_nodes = malloc(sizeof(uint32_t) * pat->length);
                    if (!generalized_nodes) continue;
                    
                    for (uint32_t i = 0; i < pat->length; i++) {
                        bool is_diff = false;
                        for (uint32_t d = 0; d < differences; d++) {
                            if (diff_positions[d] == i) {
                                is_diff = true;
                                break;
                            }
                        }
                        generalized_nodes[i] = is_diff ? BLANK_NODE : pat->node_ids[i];
                    }
                    
                    /* Check if this generalized pattern already exists */
                    bool exists = false;
                    for (uint32_t r = 0; r < g->pattern_count; r++) {
                        Pattern *existing = &g->patterns[r];
                        if (existing->length != pat->length) continue;
                        
                        bool matches = true;
                        for (uint32_t i = 0; i < pat->length; i++) {
                            if (generalized_nodes[i] != existing->node_ids[i]) {
                                matches = false;
                                break;
                            }
                        }
                        if (matches) {
                            exists = true;
                            /* Create connections through existing generalized pattern */
                            create_or_strengthen_pattern_edge(g, p, r);
                            create_or_strengthen_pattern_edge(g, r, q);
                            break;
                        }
                    }
                    
                    if (!exists) {
                        /* Create new generalized pattern */
                        if (g->pattern_count >= g->pattern_capacity) {
                            g->pattern_capacity *= 2;
                            g->patterns = realloc(g->patterns, sizeof(Pattern) * g->pattern_capacity);
                        }
                        
                        Pattern *generalized = &g->patterns[g->pattern_count++];
                        generalized->node_ids = generalized_nodes;
                        generalized->length = pat->length;
                        
                        /* Initialize */
                        generalized->sub_pattern_ids = NULL;
                        generalized->sub_pattern_count = 0;
                        generalized->predicted_nodes = NULL;
                        generalized->prediction_weights = NULL;
                        generalized->prediction_count = 0;
                        generalized->predicted_patterns = NULL;
                        generalized->pattern_prediction_weights = NULL;
                        generalized->pattern_prediction_count = 0;
                        initialize_pattern_enhancements(generalized);
                        
                        generalized->threshold = (pat->threshold + other->threshold) / 2.0f;
                        generalized->input_weights = NULL;
                        generalized->bias = 0.0f;
                        generalized->input_size = 0;
                        
                        /* Strength from connecting two patterns */
                        generalized->strength = (pat->strength + other->strength) / 2.0f * 1.2f;
                        if (generalized->strength > 1.0f) generalized->strength = 1.0f;
                        generalized->activation = (pat->activation + other->activation) / 2.0f;
                        generalized->prediction_attempts = 0;
                        generalized->prediction_successes = 0;
                        generalized->has_fired = false;
                        generalized->last_fired_step = 0;
                        generalized->fired_predictions = 0;
                        
                        /* Hierarchy: generalized is parent of both */
                        generalized->chain_depth = (pat->chain_depth < other->chain_depth) ? pat->chain_depth : other->chain_depth;
                        generalized->parent_pattern_id = INVALID_PATTERN_ID;
                        generalized->accumulated_meaning = (pat->accumulated_meaning + other->accumulated_meaning) / 2.0f * 1.3f;
                        
                        /* Update parents */
                        if (pat->parent_pattern_id == INVALID_PATTERN_ID || 
                            generalized->chain_depth < g->patterns[pat->parent_pattern_id].chain_depth) {
                            pat->parent_pattern_id = g->pattern_count - 1;
                            pat->chain_depth = generalized->chain_depth + 1;
                        }
                        if (other->parent_pattern_id == INVALID_PATTERN_ID ||
                            generalized->chain_depth < g->patterns[other->parent_pattern_id].chain_depth) {
                            other->parent_pattern_id = g->pattern_count - 1;
                            other->chain_depth = generalized->chain_depth + 1;
                        }
                        
                        generalized->input_port = pat->input_port;
                        generalized->output_port = pat->output_port;
                        
                        /* Initialize edge lists */
                        generalized->outgoing_patterns.edges = malloc(sizeof(Edge) * INITIAL_CAPACITY);
                        generalized->outgoing_patterns.count = 0;
                        generalized->outgoing_patterns.capacity = INITIAL_CAPACITY;
                        generalized->outgoing_patterns.total_weight = 0.0f;
                        generalized->outgoing_patterns.metabolic_load = 0.0f;
                        
                        generalized->incoming_patterns.edges = malloc(sizeof(Edge) * INITIAL_CAPACITY);
                        generalized->incoming_patterns.count = 0;
                        generalized->incoming_patterns.capacity = INITIAL_CAPACITY;
                        generalized->incoming_patterns.total_weight = 0.0f;
                        generalized->incoming_patterns.metabolic_load = 0.0f;
                        
                        /* Create connections: p → generalized → q */
                        create_or_strengthen_pattern_edge(g, p, g->pattern_count - 1);
                        create_or_strengthen_pattern_edge(g, g->pattern_count - 1, q);
                    }
                }
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
    
    /* ========================================================================
     * COMPUTE SYSTEM-WIDE STATISTICS (for relative measures)
     * Everything must be relative to system state, not arbitrary constants
     * ======================================================================== */
    
    /* Statistics for information factors */
    float total_input_connectivity = 0.0f;
    float total_context_match = 0.0f;
    float total_history_coherence = 0.0f;
    float total_pattern_prediction = 0.0f;
    float total_pattern_meaning = 0.0f;
    float total_path_importance = 0.0f;
    uint32_t connectivity_samples = 0;
    uint32_t pattern_samples = 0;
    uint32_t importance_samples = 0;
    
    /* Sample edges to compute average connectivity and match rates */
    for (int sample_i = 0; sample_i < BYTE_VALUES && sample_i < 50; sample_i++) {
        if (!g->nodes[sample_i].exists) continue;
        EdgeList *sample_out = &g->outgoing[sample_i];
        if (sample_out->count == 0) continue;
        
        /* Sample first edge from this node */
        if (sample_out->count > 0 && sample_out->edges[0].active) {
            uint32_t sample_target = sample_out->edges[0].to_id;
            
            /* Check input connectivity */
            float sample_input_conn = 0.0f;
            for (uint32_t inp = 0; inp < g->input_length && inp < 10; inp++) {
                uint32_t input_node = g->input_buffer[inp];
                if (input_node < BYTE_VALUES && g->nodes[input_node].exists) {
                    EdgeList *input_out = &g->outgoing[input_node];
                    for (uint32_t e = 0; e < input_out->count; e++) {
                        if (input_out->edges[e].to_id == sample_target && input_out->edges[e].active) {
                            sample_input_conn = 1.0f;
                            break;
                        }
                    }
                    if (sample_input_conn > 0.0f) break;
                }
            }
            total_input_connectivity += sample_input_conn;
            
            /* Check context match */
            float sample_context = 0.0f;
            for (uint32_t p = 0; p < g->pattern_count; p++) {
                Pattern *pat = &g->patterns[p];
                if (pat->activation > pat->threshold && pat->activation > 0.1f) {
                    for (uint32_t pred = 0; pred < pat->prediction_count; pred++) {
                        if (pat->predicted_nodes[pred] == sample_target) {
                            sample_context = 1.0f;
                            break;
                        }
                    }
                    if (sample_context > 0.0f) break;
                }
            }
            total_context_match += sample_context;
            
            /* Check history coherence */
            float sample_history = 0.0f;
            if (g->output_length > 0) {
                uint32_t last_output = g->output_buffer[g->output_length - 1];
                if (last_output < BYTE_VALUES && g->nodes[last_output].exists) {
                    EdgeList *last_out = &g->outgoing[last_output];
                    for (uint32_t e = 0; e < last_out->count; e++) {
                        if (last_out->edges[e].to_id == sample_target && last_out->edges[e].active) {
                            sample_history = 1.0f;
                            break;
                        }
                    }
                }
            }
            total_history_coherence += sample_history;
            
            connectivity_samples++;
        }
    }
    
    /* Compute averages (defaults relative to system) */
    float avg_input_connectivity = (connectivity_samples > 0) ? 
        (total_input_connectivity / connectivity_samples) : 0.0f;
    float avg_context_match = (connectivity_samples > 0) ? 
        (total_context_match / connectivity_samples) : 0.0f;
    float avg_history_coherence = (connectivity_samples > 0) ? 
        (total_history_coherence / connectivity_samples) : 0.0f;
    
    /* Compute pattern statistics */
    float total_active_pattern_strength = 0.0f;
    uint32_t active_pattern_count = 0;
    for (uint32_t p = 0; p < g->pattern_count; p++) {
        Pattern *pat = &g->patterns[p];
        if (pat->activation > pat->threshold && pat->activation > 0.1f) {
            total_pattern_meaning += pat->accumulated_meaning;
            total_active_pattern_strength += pat->strength;
            active_pattern_count++;
        }
    }
    float avg_pattern_meaning = (active_pattern_count > 0) ? 
        (total_pattern_meaning / active_pattern_count) : 0.0f;
    float avg_pattern_strength = (active_pattern_count > 0) ? 
        (total_active_pattern_strength / active_pattern_count) : 0.0f;
    float avg_pattern_prediction = avg_pattern_strength;  /* Use strength as proxy */
    
    /* Compute importance statistics from edge weights and usage */
    for (int sample_i = 0; sample_i < BYTE_VALUES && sample_i < 50; sample_i++) {
        if (!g->nodes[sample_i].exists) continue;
        EdgeList *sample_out = &g->outgoing[sample_i];
        for (uint32_t j = 0; j < sample_out->count && j < 5; j++) {
            Edge *edge = &sample_out->edges[j];
            if (!edge->active) continue;
            float usage_importance = logf(1.0f + edge->use_count) / 10.0f;
            float success_rate = (edge->use_count > 0) ? 
                ((float)edge->success_count / (float)edge->use_count) : 0.0f;
            float path_imp = (usage_importance + success_rate + edge->weight) / 3.0f;
            total_path_importance += path_imp;
            importance_samples++;
        }
    }
    float avg_path_importance = (importance_samples > 0) ? 
        (total_path_importance / importance_samples) : 0.0f;
    
    /* Ensure minimum baselines (to avoid division by zero, use small positive values) */
    if (avg_input_connectivity < 0.01f) avg_input_connectivity = 0.01f;
    if (avg_context_match < 0.01f) avg_context_match = 0.01f;
    if (avg_history_coherence < 0.01f) avg_history_coherence = 0.01f;
    if (avg_pattern_meaning < 0.01f) avg_pattern_meaning = 0.01f;
    if (avg_path_importance < 0.01f) avg_path_importance = 0.01f;
    
    /* PHASE 1: PATTERN-GUIDED PROPAGATION (Patterns predict next nodes) */
    /* Patterns are learned intelligence - they know where activation should go */
    /* This happens FIRST so patterns can guide edge-based propagation */
    propagate_pattern_activation(g);
    
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
             * FACTOR 1: Information_Carried (SEQUENTIAL STRUCTURE AWARENESS)
             * How much information does this path carry from input/context to target?
             * 
             * KEY INSIGHT: Input has SEQUENTIAL structure (cat = c→a→t)
             * Edges that follow the input sequence should have HIGH information
             * This is how the system learns from data structure itself
             * ======================================================================== */
            float input_connection = 0.0f;
            
            /* Check if this edge follows the input sequence */
            /* If we're at input[i] and edge goes to input[i+1], that's sequential = high info */
            for (uint32_t inp = 0; inp < g->input_length; inp++) {
                uint32_t input_node = g->input_buffer[inp];
                if (input_node == i) {  /* Current node matches input position */
                    /* Check if target is the NEXT node in input sequence */
                    if (inp + 1 < g->input_length) {
                        uint32_t next_input = g->input_buffer[inp + 1];
                        if (target == next_input) {
                            /* SEQUENTIAL PATH: This edge follows input structure! */
                            /* High information because it matches data structure */
                            Edge *edge_to_check = NULL;
                            EdgeList *input_out = &g->outgoing[input_node];
                            for (uint32_t e = 0; e < input_out->count; e++) {
                                if (input_out->edges[e].to_id == target && input_out->edges[e].active) {
                                    edge_to_check = &input_out->edges[e];
                                    break;
                                }
                            }
                            if (edge_to_check) {
                                float edge_strength = edge_to_check->weight;
                                float usage_boost = logf(1.0f + edge_to_check->use_count) / 5.0f;
                                /* Sequential paths get 10x boost - they match data structure! */
                                input_connection = fmax(input_connection, edge_strength * (1.0f + usage_boost) * 10.0f);
                            } else {
                                /* Edge doesn't exist yet, but sequence structure still gives info */
                                input_connection = fmax(input_connection, 5.0f);  /* High baseline for sequential */
                            }
                            break;
                        }
                    }
                }
            }
            
            /* Fallback: Check if target is reachable from any input (non-sequential) */
            if (input_connection < 0.1f) {
                for (uint32_t inp = 0; inp < g->input_length; inp++) {
                    uint32_t input_node = g->input_buffer[inp];
                    if (input_node < BYTE_VALUES && g->nodes[input_node].exists) {
                        EdgeList *input_out = &g->outgoing[input_node];
                        for (uint32_t e = 0; e < input_out->count; e++) {
                            if (input_out->edges[e].to_id == target && input_out->edges[e].active) {
                                Edge *input_edge = &input_out->edges[e];
                                float edge_strength = input_edge->weight;
                                float usage_boost = logf(1.0f + input_edge->use_count) / 5.0f;
                                float edge_info = edge_strength * (1.0f + usage_boost);
                                input_connection = fmax(input_connection, edge_info);
                                break;
                            }
                        }
                    }
                }
            }
            
            /* Pattern support: use pattern strength × activation (not binary) */
            float context_match = 0.0f;
            for (uint32_t p = 0; p < g->pattern_count; p++) {
                Pattern *pat = &g->patterns[p];
                if (pat->activation > pat->threshold && pat->activation > 0.1f) {
                    for (uint32_t pred = 0; pred < pat->prediction_count; pred++) {
                        if (pat->predicted_nodes[pred] == target) {
                            /* Pattern strength × activation = support for this target */
                            float pattern_support = pat->strength * pat->activation;
                            context_match = fmax(context_match, pattern_support);
                            break;
                        }
                    }
                }
            }
            
            /* History coherence: edge weight from last output (not binary) */
            float history_coherence = 0.0f;
            if (g->output_length > 0) {
                uint32_t last_output = g->output_buffer[g->output_length - 1];
                if (last_output < BYTE_VALUES && g->nodes[last_output].exists) {
                    EdgeList *last_out = &g->outgoing[last_output];
                    for (uint32_t e = 0; e < last_out->count; e++) {
                        if (last_out->edges[e].to_id == target && last_out->edges[e].active) {
                            Edge *hist_edge = &last_out->edges[e];
                            /* Edge weight = sequential strength */
                            float edge_strength = hist_edge->weight;
                            float usage_boost = logf(1.0f + hist_edge->use_count) / 5.0f;
                            history_coherence = edge_strength * (1.0f + usage_boost);
                            break;
                        }
                    }
                }
            }
            
            /* Information = edge strength from input × pattern support × sequential flow */
            /* All factors now [0, ~2], naturally normalized, no division needed */
            float information = input_connection * context_match * history_coherence;
            /* Early exploration: if all factors are weak, use edge existence as baseline */
            if (information < 0.01f) {
                /* Allow some information flow based on any connection */
                information = input_connection + context_match + history_coherence;
                if (information < 0.01f) {
                    /* Absolute baseline: edge exists = some information */
                    information = 0.1f;  /* Minimum for exploration */
                }
            }
            
            /* ========================================================================
             * FACTOR 2: Learning_Strength (EDGE WEIGHT × SUCCESS RATE × USAGE)
             * How well-learned is this path? Use actual training results
             * ======================================================================== */
            float edge_weight = edge->weight;  /* Learned strength [0,1] */
            float usage_boost = logf(1.0f + edge->use_count) / 5.0f;  /* Log scale usage */
            float success_rate = (edge->use_count > 0) ? 
                ((float)edge->success_count / (float)edge->use_count) : 0.0f;  /* Training success */
            
            /* Learning = base weight × success rate × usage */
            /* Trained paths (high success_rate) get HUGE boost */
            float learning = edge_weight * (1.0f + success_rate * 10.0f) * (1.0f + usage_boost);
            /* Range: [0.1, ~20] for well-trained paths, ~0.1 for untrained */
            /* Success rate × 10 means 100% success = 11x boost! */
            
            /* ========================================================================
             * FACTOR 3: Coherence (PATTERN ALIGNMENT × SEQUENTIAL FLOW)
             * Does this path form a coherent sequence? Use pattern support
             * ======================================================================== */
            /* Pattern alignment = strongest pattern supporting this target */
            float pattern_alignment = context_match;  /* Already computed as pattern support */
            
            /* Sequential flow = history coherence */
            float sequential_flow = history_coherence;  /* Already computed as edge weight */
            
            /* Context fit = pattern support */
            float context_fit = context_match;  /* Pattern strength × activation */
            
            /* Coherence = pattern support × sequential flow */
            float coherence = (pattern_alignment + sequential_flow + context_fit) / 3.0f;
            /* Range: [0, ~2] for paths matching patterns and history */
            
            /* ========================================================================
             * FACTOR 4: Predictive_Power
             * How well does this path predict correct outputs?
             * ======================================================================== */
            /* ========================================================================
             * INTELLIGENT ACTIVATION: Pattern hierarchy and meaning guide flow (RELATIVE)
             * ======================================================================== */
            float pattern_prediction = avg_pattern_prediction;  /* Default: relative to system average */
            float pattern_meaning_boost = 1.0f;  /* Start neutral, boost relative to system */
            float hierarchy_boost = 1.0f;  /* Start neutral, boost relative to system */
            
            /* Check if this edge is part of an active pattern */
            for (uint32_t p = 0; p < g->pattern_count; p++) {
                Pattern *pat = &g->patterns[p];
                if (pat->activation > pat->threshold && pat->activation > 0.1f) {
                    /* Pattern is active - check if it predicts this target */
                    for (uint32_t pred = 0; pred < pat->prediction_count; pred++) {
                        if (pat->predicted_nodes[pred] == target) {
                            /* Pattern confidence = activation × strength (relative to system) */
                            float raw_prediction = pat->activation * pat->strength;
                            pattern_prediction = (avg_pattern_prediction > 0.0f) ? 
                                (raw_prediction / avg_pattern_prediction) : raw_prediction;
                            
                            /* INTELLIGENT ACTIVATION: Meaning boost (RELATIVE to system average) */
                            /* Patterns with accumulated meaning carry more information */
                            /* FIX: Use bounded meaning to prevent overflow in ratio */
                            float bounded_meaning = pat->accumulated_meaning;
                            if (bounded_meaning > 1000.0f) bounded_meaning = 1000.0f;
                            if (bounded_meaning != bounded_meaning || bounded_meaning > 1e6f) {
                                bounded_meaning = 1.0f;  /* Reset if NaN/Inf */
                            }
                            float meaning_ratio = (avg_pattern_meaning > 0.01f) ? 
                                (bounded_meaning / avg_pattern_meaning) : 1.0f;
                            if (meaning_ratio > 100.0f) meaning_ratio = 100.0f;  /* Cap ratio */
                            pattern_meaning_boost = 1.0f + meaning_ratio;  /* Relative boost */
                            if (pattern_meaning_boost > 200.0f) pattern_meaning_boost = 200.0f;  /* Cap boost */
                            
                            /* INTELLIGENT ACTIVATION: Hierarchy depth (RELATIVE - use pattern depth relative to max) */
                            /* Deeper patterns (more abstract) = more meaningful */
                            float max_chain_depth = 10.0f;  /* Reasonable max (could be computed from system) */
                            float relative_depth = (pat->chain_depth / max_chain_depth);
                            hierarchy_boost = 1.0f + relative_depth;  /* Relative boost */
                            
                            break;
                        }
                    }
                    
                    /* Check if this edge (i → target) is part of the pattern sequence */
                    /* If pattern contains this edge sequence, boost activation flow */
                    for (uint32_t pat_idx = 0; pat_idx < pat->length - 1; pat_idx++) {
                        if (pat->node_ids[pat_idx] == i && 
                            pat->node_ids[pat_idx + 1] == target) {
                            /* This edge is part of an active pattern! */
                            /* INTELLIGENT ACTIVATION: Rich pattern connections boost flow */
                            float pattern_edge_boost = pat->activation * pat->strength * 
                                                      pat->dynamic_importance * 
                                                      pattern_meaning_boost * hierarchy_boost;
                            pattern_prediction = fmax(pattern_prediction, pattern_edge_boost);
                            break;
                        }
                    }
                }
            }
            
            /* Edge's historical success rate (training signal) */
            float historical_accuracy = success_rate;  /* [0,1] from edge success */
            float context_prediction = context_match;  /* Pattern support */
            
            /* INTELLIGENT ACTIVATION: Pattern predictions × training success */
            /* Predictive = how well this path predicts correct outputs */
            float predictive = pattern_prediction * pattern_meaning_boost * hierarchy_boost * 
                             (0.5f + historical_accuracy * 0.5f) * context_prediction;
            /* Range: [0, ~4] for well-predicted paths */
            
            /* ========================================================================
             * SELF-REGULATED PATH QUALITY: Importance determines quality
             * 
             * Path quality reflects MEANING/IMPORTANCE (learned by system)
             * Not fixed formulas - the system decides what's important
             * 
             * Important paths (high usage, high success, high activation) = high quality
             * Unimportant paths = low quality
             * 
             * The system self-regulates: learns what's important through experience
             * ======================================================================== */
            
            /* ========================================================================
             * PATH QUALITY = Learning + Bonuses
             * 
             * Core principle: Edge weight (learning) is BASE quality
             * Other factors are BONUSES that enhance it
             * 
             * This ensures:
             * - Even untrained edges have some quality (exploration)
             * - Trained edges get major boosts (exploitation)
             * - Information/patterns enhance, not replace
             * ======================================================================== */
            
            /* BASE: Edge weight is the foundation (always > 0 for active edges) */
            float base_quality = learning;  /* [0, ~6] for trained, ~0.1 for new */
            
            /* BONUS 1: Information boost (relevant to input) */
            if (information > 0.1f) {
                base_quality *= (1.0f + information * 0.5f);  /* Up to 2x boost */
            }
            
            /* BONUS 2: Pattern prediction boost */
            if (predictive > 0.1f) {
                base_quality *= (1.0f + predictive * 0.3f);  /* Up to 2.2x boost */
            }
            
            /* BONUS 3: Coherence boost */
            if (coherence > 0.1f) {
                base_quality *= (1.0f + coherence * 0.2f);  /* Up to 1.4x boost */
            }
            
            /* Result: Untrained edges ~0.1, trained edges 0.1 → 6.0 → 18.0 with all bonuses */
            
            /* INTELLIGENT ACTIVATION: Rich pattern connections boost path quality (RELATIVE) */
            /* If this edge is part of active patterns with meaning, boost quality */
            float pattern_connection_boost = 1.0f;
            for (uint32_t p = 0; p < g->pattern_count; p++) {
                Pattern *pat = &g->patterns[p];
                if (pat->activation > pat->threshold && pat->activation > 0.1f) {
                    /* Check if edge is in pattern sequence */
                    for (uint32_t pat_idx = 0; pat_idx < pat->length - 1; pat_idx++) {
                        if (pat->node_ids[pat_idx] == i && 
                            pat->node_ids[pat_idx + 1] == target) {
                            /* Edge is part of active pattern - boost based on pattern's meaning (relative) */
                            /* Meaning boost: relative to system average */
                            /* FIX: Use bounded meaning to prevent overflow */
                            float bounded_meaning = pat->accumulated_meaning;
                            if (bounded_meaning > 1000.0f) bounded_meaning = 1000.0f;
                            if (bounded_meaning != bounded_meaning || bounded_meaning > 1e6f) {
                                bounded_meaning = 1.0f;  /* Reset if NaN/Inf */
                            }
                            float meaning_ratio = (avg_pattern_meaning > 0.01f) ? 
                                (bounded_meaning / avg_pattern_meaning) : 1.0f;
                            if (meaning_ratio > 100.0f) meaning_ratio = 100.0f;  /* Cap ratio */
                            /* Dynamic importance is already relative (0.0 to 1.0 range) */
                            float pattern_boost = 1.0f + meaning_ratio + pat->dynamic_importance;
                            if (pattern_boost > 200.0f) pattern_boost = 200.0f;  /* Cap boost */
                            pattern_connection_boost = fmax(pattern_connection_boost, pattern_boost);
                            break;
                        }
                    }
                }
            }
            
            /* SELF-TUNING: Adjust path quality based on error rate and pattern support */
            /* High error = reduce quality, low error = keep quality */
            float quality_adjustment = 1.0f - (g->state.error_rate * 0.5f);
            
            /* Final path quality: base × pattern boost × error adjustment */
            path_qualities[j] = base_quality * pattern_connection_boost * quality_adjustment;
            
            /* Ensure minimum quality for exploration (prevent zero-sum) */
            if (path_qualities[j] < 0.001f) path_qualities[j] = 0.001f;
            
            // #region agent log
            if (j < 5) {  // Log first 5 edges only
                FILE *f = fopen("f:\\Melvin_Research\\Melvin_o7\\melvin_o7\\.cursor\\debug.log", "a");
                if (f) {
                    fprintf(f, "{\"location\":\"melvin.c:2450\",\"message\":\"path quality calculated\",\"data\":{\"from\":%d,\"to\":%u,\"learning\":%.3f,\"info\":%.3f,\"quality\":%.3f,\"edge_weight\":%.3f,\"success_rate\":%.3f},\"timestamp\":%lld,\"sessionId\":\"debug-session\",\"hypothesisId\":\"C\"}\n",
                            i, target, learning, information, path_qualities[j], edge_weight, success_rate,
                            (long long)time(NULL) * 1000);
                    fclose(f);
                }
            }
            // #endregion
            total_path_quality += path_qualities[j];
        }
        
        /* SELF-REGULATED NORMALIZATION: Prevent explosion while allowing importance */
        /* If total quality is very small, don't explode - use soft normalization */
        float soft_normalization = (total_path_quality > 0.001f) ? 
            (1.0f / total_path_quality) : 
            (1.0f / (total_path_quality + 0.001f));  /* Soft floor prevents explosion */
        
        /* Cap normalization to prevent activation explosion */
        if (soft_normalization > 100.0f) soft_normalization = 100.0f;
        
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
            float normalized_quality = path_qualities[j] * soft_normalization;
            
            /* DATA-DRIVEN PROPAGATION: Use pattern-learned transfer rate */
            /* Check if any pattern controls this edge */
            float learned_transfer_rate = 1.0f;  /* Default: full transfer */
            uint32_t controlling_pattern = INVALID_PATTERN_ID;
            
            for (uint32_t p = 0; p < g->pattern_count; p++) {
                Pattern *pat = &g->patterns[p];
                if (pat->activation > pat->threshold && pat->activation_control_strength > 0.2f) {
                    /* Check if this edge is part of pattern */
                    for (uint32_t pat_idx = 0; pat_idx < pat->length - 1; pat_idx++) {
                        if (pat->node_ids[pat_idx] == i && pat->node_ids[pat_idx + 1] == target) {
                            /* Pattern controls this edge - use its learned transfer rate */
                            learned_transfer_rate = pat->propagation_transfer_rate;
                            controlling_pattern = p;
                            break;
                        }
                    }
                    if (controlling_pattern != INVALID_PATTERN_ID) break;
                }
            }
            
            float transfer = g->nodes[i].activation * normalized_quality * learned_transfer_rate;
            
            /* SELF-REGULATION: Cap transfer to prevent explosion, but allow high activation for important things */
            if (transfer > 10.0f) transfer = 10.0f;  /* Cap to prevent explosion */
            
            /* PATH ACCUMULATION: Activation accumulates along paths */
            /* Important paths get more activation - this is CORRECT (meaning determines activation) */
            /* "your girlfriend cheated on you" SHOULD have high activation - it's important! */
            g->nodes[target].activation += transfer;
            
            /* SELF-REGULATION: Cap node activation to prevent unbounded growth */
            /* But allow high activation for important things (up to reasonable limit) */
            if (g->nodes[target].activation > 100.0f) {
                g->nodes[target].activation = 100.0f;  /* Cap to prevent explosion */
            }
            g->nodes[target].receive_count++;
            out->edges[j].use_count++;
            
            /* CREATE EDGE IF ACTIVATION TRANSFER IS STRONG (wave propagation creates connections) */
            /* If activation successfully transferred, strengthen the connection */
            /* Edge creation threshold relative to system state */
            float transfer_threshold = 0.05f * g->state.learning_rate;
            float activation_threshold = g->state.avg_activation * 0.2f;
            if (transfer > transfer_threshold && g->nodes[target].activation > activation_threshold) {
                /* Edge already exists (we're using it), strengthen it */
                /* This happens automatically in create_or_strengthen_edge */
                /* EDGES ARE UNIDIRECTIONAL - no reverse edges (one-way valves) */
                /* Forward edge i→target already exists and was just used */
            }
        }
        
        /* Source node activation decays after propagating (like signal attenuation) */
        /* DATA-DRIVEN DECAY: Use pattern-learned decay rate */
        float learned_decay_rate = 0.9f;  /* Default decay */
        
        /* Find pattern that best matches this node's context */
        for (uint32_t p = 0; p < g->pattern_count; p++) {
            Pattern *pat = &g->patterns[p];
            if (pat->activation > pat->threshold) {
                /* Check if pattern contains this node */
                for (uint32_t pat_idx = 0; pat_idx < pat->length; pat_idx++) {
                    if (pat->node_ids[pat_idx] == i) {
                        /* Use this pattern's learned decay rate */
                        learned_decay_rate = pat->propagation_decay_rate;
                        break;
                    }
                }
            }
        }
        
        g->nodes[i].activation *= learned_decay_rate;  /* Pattern-learned decay */
        g->nodes[i].fire_count++;
    }
    
    /* PHASE 3: PATTERN REINFORCEMENT (Patterns boost predicted nodes again) */
    /* Patterns already guided initial flow in PHASE 1, now reinforce */
    /* This second pass helps patterns that matched during edge propagation */
    propagate_pattern_activation(g);
    
    /* PHASE 2: UPDATE CONTEXT FREQUENCY */
    /* Track how often patterns appear in current context */
    update_pattern_context_frequency(g);
    
    /* PHASE 3: SEMANTIC DISTANCE ACTIVATION */
    /* Boost semantically close patterns (meaning similarity) */
    propagate_semantic_activation(g);
    
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
    /* RE-ENABLED: Coactivation enables scaling potential
     * 
     * At scale, coactivation creates rich cross-connections that enable:
     * - Concept formation (related ideas connect)
     * - Contextual reasoning (multiple concepts active together)
     * - Emergent intelligence (connections beyond simple sequences)
     * 
     * This is how brains work: neurons that fire together, wire together.
     * The system can be "as smart as a human brain" (rich connections)
     * or "as dumb as a wire" (simple sequential paths).
     * 
     * Intelligence emerges from the density and quality of connections.
     */
    
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
            
            /* Strength proportional to how active both nodes are */
            float coactivation_strength = g->nodes[node_a].activation * g->nodes[node_b].activation;
            
            /* Only create if co-activation is significant */
            /* AND: Prevent self-loops (don't create edges from node to itself) */
            /* Co-activation threshold relative to system state */
            float coactivation_threshold = 0.05f * g->state.learning_rate;
            if (coactivation_strength > coactivation_threshold && node_a != node_b) {
                /* Create or strengthen edge A→B (UNIDIRECTIONAL - one-way valve) */
                /* Direction: STABLE rule based on node ID (lower ID → higher ID) */
                /* This ensures consistent directionality across all episodes */
                if (node_a < node_b) {
                    create_or_strengthen_edge(g, node_a, node_b);
                } else {
                    create_or_strengthen_edge(g, node_b, node_a);
                }
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

/* ============================================================================
 * GENERALIZATION: Connect new words to similar patterns
 * 
 * When a new word is seen, find similar patterns based on:
 * - Similar byte values (character overlap, edit distance)
 * - Similar contexts (context vector similarity)
 * 
 * This allows generalization: "quokka" connects to similar patterns like "cat", "bat"
 * ============================================================================ */

void connect_to_similar_patterns(MelvinGraph *g, const uint32_t *sequence, uint32_t seq_len) {
    /* GENERALIZATION USING BLANK NODES
     * 
     * Blank nodes ARE the generalization mechanism - they match any byte.
     * When a new word is seen, patterns with blank nodes automatically match it.
     * 
     * Example: Pattern "_at" (blank + "at") matches:
     *   - "cat" (c matches blank)
     *   - "bat" (b matches blank)
     *   - "rat" (r matches blank)
     *   - "quokka" (if it has "at" ending, the blank matches "quokk")
     * 
     * We don't need similarity scores - blank nodes already handle generalization!
     * 
     * What we DO need: When a new word matches a pattern with blank nodes,
     * create edges from the new word to the pattern's predictions.
     */
    
    if (seq_len < 2) return;
    
    /* Find patterns with blank nodes that match this sequence */
    for (uint32_t p = 0; p < g->pattern_count; p++) {
        Pattern *pat = &g->patterns[p];
        if (pat->length == 0 || pat->length > seq_len) continue;
        
        /* Check if pattern matches (blank nodes will match any byte) */
        for (uint32_t pos = 0; pos <= seq_len - pat->length; pos++) {
            if (pattern_matches(g, p, sequence, seq_len, pos)) {
                /* Pattern matches! This is generalization - blank nodes matched new word */
                /* Create edges from sequence to pattern's predicted nodes */
                if (pat->prediction_count > 0) {
                    for (uint32_t pred = 0; pred < pat->prediction_count; pred++) {
                        uint32_t predicted_node = pat->predicted_nodes[pred];
                        float pred_weight = pat->prediction_weights[pred];
                        
                        if (predicted_node < BYTE_VALUES && pred_weight > 0.3f) {
                            /* Create edge from last node in sequence to predicted node */
                            uint32_t last_seq_node = sequence[seq_len - 1];
                            create_or_strengthen_edge(g, last_seq_node, predicted_node);
                            
                            /* Pattern matched via blank nodes = generalization connection */
                            /* Boost edge weight to reflect generalization benefit */
                            EdgeList *out = &g->outgoing[last_seq_node];
                            for (uint32_t i = 0; i < out->count; i++) {
                                if (out->edges[i].to_id == predicted_node && out->edges[i].active) {
                                    /* Generalization boost: blank node match = strong connection */
                                    float generalization_boost = pat->strength * 0.2f;
                                    out->edges[i].weight += generalization_boost;
                                    break;
                                }
                            }
                        }
                    }
                }
                
                /* Learn pattern association (new word associated with this generalized pattern) */
                /* This strengthens the pattern's ability to generalize */
                pat->prediction_attempts++;
                /* Pattern successfully generalized to new word */
            }
        }
    }
}

void create_edges_from_patterns(MelvinGraph *g) {
    /* RE-ENABLED: Pattern-based edges enable concept-level connections
     * 
     * Patterns represent learned concepts and sequences. When patterns
     * predict nodes, creating edges enables:
     * - Hierarchical reasoning (patterns → concepts → predictions)
     * - Abstraction (patterns generalize beyond raw sequences)
     * - Predictive intelligence (learned patterns guide future connections)
     * 
     * At scale, this creates a rich knowledge graph where:
     * - Simple sequences = "dumb wire" behavior
     * - Pattern networks = "smart brain" behavior
     * 
     * Intelligence scales with pattern complexity and connections.
     */
    
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
                    
                    /* Only create edges for confident predictions (prevent noise) */
                    if (predicted_node < BYTE_VALUES && prediction_weight > 0.3f) {
                        /* Create edge from last node in pattern to prediction */
                        uint32_t last_pattern_node = pattern_inputs[pattern_input_len - 1];
                        create_or_strengthen_edge(g, last_pattern_node, predicted_node);
                        
                        /* Also create edge from pattern's first node (for context) */
                        /* This enables richer contextual connections at scale */
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
                                        
                                        /* PHASE 3: Learn activation rule */
                                        /* If pattern A predicts pattern B successfully, learn rule */
                                        float success_rate = (pat1->prediction_attempts > 0) ?
                                            ((float)pat1->prediction_successes / (float)pat1->prediction_attempts) : 0.5f;
                                        float boost_amount = pat1->pattern_prediction_weights[pat1->pattern_prediction_count - 1];
                                        learn_activation_rule(g, p1, p2, boost_amount, success_rate);
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

/* ============================================================================
 * PHASE 2: UPDATE PATTERN CONTEXT FREQUENCY
 * ============================================================================ */

void update_pattern_context_frequency(MelvinGraph *g) {
    /* Update context frequency for all patterns based on current input */
    for (uint32_t p = 0; p < g->pattern_count; p++) {
        Pattern *pat = &g->patterns[p];
        
        /* Check if pattern matches current input */
        bool matches = false;
        for (uint32_t pos = 0; pos <= g->input_length - pat->length; pos++) {
            if (pattern_matches(g, p, g->input_buffer, g->input_length, pos)) {
                matches = true;
                break;
            }
        }
        
        /* Update context frequency (exponential moving average) */
        if (matches) {
            pat->context_frequency = pat->context_frequency * 0.9f + 1.0f * 0.1f;
        } else {
            pat->context_frequency = pat->context_frequency * 0.9f + 0.0f * 0.1f;
        }
    }
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
    /* NATURAL SELF-REGULATION: Patterns naturally connect based on similarity */
    /* This creates clusters: high-confidence patterns connect to high-confidence patterns */
    /* Low-confidence patterns connect to low-confidence patterns */
    /* System state emerges from these natural connections */
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
            
            /* CONFIDENCE SIMILARITY: Patterns with similar confidence connect more strongly */
            /* High-confidence patterns naturally cluster together (they "know" together) */
            /* Low-confidence patterns naturally cluster together (they're "confused" together) */
            float confidence_a = (pat_a->prediction_attempts > 0) ?
                ((float)pat_a->prediction_successes / (float)pat_a->prediction_attempts) : 0.5f;
            float confidence_b = (pat_b->prediction_attempts > 0) ?
                ((float)pat_b->prediction_successes / (float)pat_b->prediction_attempts) : 0.5f;
            float confidence_similarity = 1.0f - fabsf(confidence_a - confidence_b);
            
            /* HIERARCHY SIMILARITY: Patterns at similar hierarchy levels connect more strongly */
            /* Higher in hierarchy = more abstract = patterns about patterns = self-understanding */
            float hierarchy_similarity = 1.0f / (1.0f + fabsf((float)pat_a->chain_depth - (float)pat_b->chain_depth));
            
            /* Combined similarity boost */
            float similarity_boost = (confidence_similarity * 0.6f + hierarchy_similarity * 0.4f);
            coactivation_strength *= (0.5f + similarity_boost * 0.5f);  /* Boost by up to 50% */
            
            /* Only create if significant co-activation */
            float threshold = 0.05f * g->state.learning_rate;
            if (coactivation_strength > threshold) {
                /* Create bidirectional edges (patterns influence each other) */
                /* Similar patterns get stronger edges - natural clustering */
                create_or_strengthen_pattern_edge(g, pat_a_id, pat_b_id);
                create_or_strengthen_pattern_edge(g, pat_b_id, pat_a_id);
                
                /* PHASE 2: Learn pattern associations (co-occurrence) */
                /* This will also use similarity boost internally */
                learn_pattern_association(g, pat_a_id, pat_b_id);
                learn_pattern_association(g, pat_b_id, pat_a_id);
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
                
                /* Initialize all enhancement fields */
                initialize_pattern_enhancements(pat);
                
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
     * HYBRID SELECTION: Pattern-guided + Greedy edge following
     * 
     * Strategy: 
     * 1. Check if patterns predict next node (pattern-guided intelligence)
     * 2. If no pattern match, follow strongest edge (greedy fallback)
     * 3. Suppress looping nodes aggressively
     * 
     * This combines pattern intelligence with edge-based paths
     * ======================================================================== */
    
    uint32_t selected_node = BYTE_VALUES;
    uint32_t source_node = BYTE_VALUES;
    float best_score = 0.0f;
    
    /* STEP 1: Check pattern predictions (pattern-guided intelligence) */
    /* Patterns find meaning from highest level (deeper hierarchy = more meaning) */
    /* Input is just a spark - patterns guide the wave based on accumulated meaning */
    /* Prioritize patterns with higher accumulated_meaning (they have more understanding) */
    
    /* CRITICAL FIX: Check INPUT context when output is empty, OUTPUT context when output exists */
    bool check_patterns = (g->output_length > 0) || (g->input_length > 0);
    
    if (check_patterns) {
        /* Use pattern-guided selection - match INPUT when output empty, OUTPUT when output exists */
        for (uint32_t p = 0; p < g->pattern_count; p++) {
            Pattern *pat = &g->patterns[p];
            
            /* Pattern must be strong enough to trust */
            if (pat->strength < 0.3f) continue;  /* Skip weak patterns */
            
            bool pattern_matches_context = false;
            uint32_t *match_sequence = NULL;
            uint32_t match_start_pos = 0;
            
            if (g->output_length > 0 && g->output_length >= pat->length) {
                /* Match OUTPUT context (continuation) */
                uint32_t start_pos = g->output_length - pat->length;
                if (pattern_matches(g, p, g->output_buffer, g->output_length, start_pos)) {
                    pattern_matches_context = true;
                    match_sequence = g->output_buffer;
                    match_start_pos = start_pos;
                }
            } else if (g->output_length == 0 && g->input_length >= pat->length) {
                /* Match INPUT context (first selection) - CRITICAL FIX */
                /* Try matching from end of input (most relevant) */
                for (int pos = g->input_length - pat->length; pos >= 0; pos--) {
                    if (pattern_matches(g, p, g->input_buffer, g->input_length, pos)) {
                        pattern_matches_context = true;
                        match_sequence = g->input_buffer;
                        match_start_pos = pos;
                        break;  /* Use most recent match */
                    }
                }
            }
            
            if (pattern_matches_context && pat->prediction_count > 0) {
                /* Pattern matches! Check its predictions */
                for (uint32_t pred = 0; pred < pat->prediction_count; pred++) {
                        uint32_t predicted_node = pat->predicted_nodes[pred];
                        float pred_weight = pat->prediction_weights[pred];
                        
                        /* Prediction must be confident enough */
                        if (pred_weight < 0.4f) continue;  /* Skip weak predictions */
                        
                        if (predicted_node < BYTE_VALUES && g->nodes[predicted_node].exists) {
                            /* Calculate pattern-guided score */
                            float pattern_score = pat->strength * pat->activation * pred_weight;
                            
                            /* MEANING FROM HIGHEST LEVEL: Patterns with accumulated_meaning guide output */
                            /* Deeper hierarchy = more meaning = stronger influence */
                            /* This is how meaning emerges - not from input, but from pattern hierarchies */
                            float meaning_boost = 1.0f;
                            if (pat->accumulated_meaning > 0.1f) {
                                float bounded_meaning = pat->accumulated_meaning;
                                if (bounded_meaning > 100.0f) {
                                    bounded_meaning = 100.0f + logf(bounded_meaning / 100.0f) * 10.0f;
                                }
                                if (bounded_meaning > 200.0f) bounded_meaning = 200.0f;
                                meaning_boost = 1.0f + (bounded_meaning * 0.5f);
                                if (meaning_boost > 30.0f) meaning_boost = 30.0f;
                            }
                            
                            /* HIERARCHY BOOST: Deeper patterns (more abstract) have more meaning */
                            float hierarchy_boost = 1.0f + (1.0f / (1.0f + pat->chain_depth * 0.2f));
                            
                            pattern_score *= meaning_boost * hierarchy_boost;
                            
                            /* Add node activation (wave propagation support) */
                            pattern_score += g->nodes[predicted_node].activation * 0.5f;
                            
                            /* Suppress looping nodes */
                            float loop_penalty = 1.0f;
                            if (g->output_length >= 2 && predicted_node == g->output_buffer[g->output_length - 2]) {
                                loop_penalty = 0.1f;  /* Strong penalty for immediate repetition */
                            }
                            if (g->output_length >= 3 && predicted_node == g->output_buffer[g->output_length - 3]) {
                                loop_penalty = 0.2f;  /* Penalty for near repetition */
                            }
                            pattern_score *= loop_penalty;
                            
                            /* Apply loop pressure (system-wide loop suppression) */
                            if (g->state.loop_pressure > 0.3f) {
                                /* Check if this node continues a loop */
                                bool continues_loop = false;
                                if (g->output_length >= 3) {
                                    for (uint32_t i = 0; i < 3 && i < g->output_length; i++) {
                                        if (predicted_node == g->output_buffer[g->output_length - 1 - i]) {
                                            continues_loop = true;
                                            break;
                                        }
                                    }
                                }
                                if (continues_loop) {
                                    pattern_score *= (1.0f - g->state.loop_pressure);  /* Suppress loops */
                                }
                            }
                            
                            if (pattern_score > best_score) {
                                best_score = pattern_score;
                                selected_node = predicted_node;
                                source_node = BYTE_VALUES;  /* Pattern prediction, no edge source */
                            }
                        }
                    }
                }
            }
        }
    
    /* STEP 3: Pattern-driven edge selection (patterns generate rules based on context) */
    /* Patterns = edges + nodes that create meaning - they generate if-statements dynamically */
    if (selected_node >= BYTE_VALUES && g->output_length > 0) {
        /* Follow from previous output */
        uint32_t prev_output = g->output_buffer[g->output_length - 1];
        if (prev_output < BYTE_VALUES) {
            EdgeList *edges = &g->outgoing[prev_output];
            
            /* Get max weight from this node for relative comparison */
            float max_weight_from_node = 0.0f;
            for (uint32_t j = 0; j < edges->count; j++) {
                if (edges->edges[j].active && edges->edges[j].weight > max_weight_from_node) {
                    max_weight_from_node = edges->edges[j].weight;
                }
            }
            if (max_weight_from_node < 0.001f) max_weight_from_node = 1.0f;  /* Avoid division by zero */
            
            for (uint32_t i = 0; i < edges->count; i++) {
                if (!edges->edges[i].active) continue;
                
                Edge *e = &edges->edges[i];
                uint32_t candidate = e->to_id;
                
                /* ========================================================================
                 * PATTERN-DRIVEN EDGE SCORING: Patterns generate rules based on context
                 * Patterns evaluate: node activations, edge states, context → edge score
                 * This replaces hardcoded if-statements with dynamic pattern rules
                 * ======================================================================== */
                
                float edge_score = 0.0f;
                float pattern_contributions = 0.0f;
                uint32_t pattern_count_contributing = 0;
                
                /* Let patterns evaluate this edge based on context */
                for (uint32_t p = 0; p < g->pattern_count; p++) {
                    Pattern *pat = &g->patterns[p];
                    
                    /* Pattern must be active and have control authority */
                    if (pat->activation <= pat->threshold || pat->activation_control_strength < 0.2f) {
                        continue;
                    }
                    
                    /* Pattern evaluates edge based on context:
                     * - Does pattern predict this candidate node?
                     * - Does pattern match current output context?
                     * - What is the pattern's confidence in this edge?
                     */
                    
                    bool pattern_predicts_candidate = false;
                    float prediction_weight = 0.0f;
                    
                    /* Check if pattern predicts this candidate */
                    for (uint32_t pred = 0; pred < pat->prediction_count; pred++) {
                        if (pat->predicted_nodes[pred] == candidate) {
                            pattern_predicts_candidate = true;
                            prediction_weight = pat->prediction_weights[pred];
                            break;
                        }
                    }
                    
                    /* Check if pattern matches current context (output end) */
                    bool pattern_matches_context = false;
                    if (g->output_length >= pat->length) {
                        uint32_t start_pos = g->output_length - pat->length;
                        if (pattern_matches(g, p, g->output_buffer, g->output_length, start_pos)) {
                            pattern_matches_context = true;
                        }
                    }
                    
                    /* Pattern generates rule: IF context matches AND predicts candidate THEN boost */
                    if (pattern_matches_context && pattern_predicts_candidate) {
                        /* Pattern rule: boost this edge based on pattern confidence */
                        float pattern_rule_score = pat->activation * prediction_weight * 
                                                   pat->strength * pat->rule_confidence;
                        pattern_contributions += pattern_rule_score;
                        pattern_count_contributing++;
                    }
                }
                
                /* DATA-DRIVEN SELECTION: Use pattern-learned selection factors */
                /* Find pattern that best controls this edge selection */
                float weight_factor = 0.4f;  /* Default factors */
                float activation_factor = 0.3f;
                float context_factor = 0.2f;
                float pattern_factor = 0.1f;
                uint32_t controlling_pattern = INVALID_PATTERN_ID;
                
                for (uint32_t p = 0; p < g->pattern_count; p++) {
                    Pattern *pat = &g->patterns[p];
                    if (pat->activation > pat->threshold && pat->activation_control_strength > 0.2f) {
                        /* Check if pattern matches context and predicts candidate */
                        bool matches = false;
                        if (g->output_length >= pat->length) {
                            uint32_t start_pos = g->output_length - pat->length;
                            if (pattern_matches(g, p, g->output_buffer, g->output_length, start_pos)) {
                                matches = true;
                            }
                        }
                        for (uint32_t pred = 0; pred < pat->prediction_count; pred++) {
                            if (pat->predicted_nodes[pred] == candidate && matches) {
                                /* Pattern controls this selection - use its learned factors */
                                weight_factor = pat->selection_weight_factor;
                                activation_factor = pat->selection_activation_factor;
                                context_factor = pat->selection_context_factor;
                                pattern_factor = pat->selection_pattern_factor;
                                controlling_pattern = p;
                                break;
                            }
                        }
                        if (controlling_pattern != INVALID_PATTERN_ID) break;
                    }
                }
                
                /* Base edge score from edge properties (using learned factors) */
                float relative_weight = e->weight / max_weight_from_node;
                float usage_boost = logf(1.0f + e->use_count) / 5.0f;
                float success_rate = (e->use_count > 0) ? 
                    ((float)e->success_count / (float)e->use_count) : 0.0f;
                float success_boost = 1.0f + success_rate;
                
                /* Base score = weighted combination using learned factors */
                float base_score = (relative_weight * weight_factor) + 
                                  ((1.0f + usage_boost) * weight_factor * 0.5f) +
                                  (success_boost * weight_factor * 0.5f);
                
                /* Add node activation support (using learned activation factor) */
                float activation_contribution = 0.0f;
                if (candidate < BYTE_VALUES && g->nodes[candidate].exists) {
                    activation_contribution = g->nodes[candidate].activation * activation_factor;
                }
                
                /* Pattern contributions (using learned pattern factor) */
                float pattern_contribution = 0.0f;
                if (pattern_count_contributing > 0) {
                    float avg_pattern_contribution = pattern_contributions / pattern_count_contributing;
                    pattern_contribution = avg_pattern_contribution * pattern_factor;
                }
                
                /* Context contribution (using learned context factor) */
                /* Context = how well this edge fits current input/output context */
                float context_contribution = 0.0f;
                
                if (g->output_length == 0 && g->input_length > 0) {
                    /* FIRST SELECTION: Check if candidate is next in INPUT sequence */
                    /* This is critical - different inputs should select different first nodes */
                    for (uint32_t i = 0; i < g->input_length; i++) {
                        if (g->input_buffer[i] == candidate) {
                            /* Candidate is in input - check if it's the RIGHT position */
                            float position_relevance = 1.0f;
                            
                            /* If we're at position i in input, candidate should be at position i */
                            /* Boost if candidate matches input sequence position */
                            if (i == 0) {
                                /* First character - strong boost */
                                context_contribution = context_factor * 2.0f;
                            } else {
                                /* Later in sequence - check if it follows from previous */
                                uint32_t prev_input = g->input_buffer[i - 1];
                                if (prev_output == prev_input) {
                                    /* Follows input sequence - strong boost */
                                    context_contribution = context_factor * 1.5f;
                                } else {
                                    /* In input but not sequential - moderate boost */
                                    context_contribution = context_factor * 0.5f;
                                }
                            }
                            break;
                        }
                    }
                } else if (g->output_length > 0) {
                    /* CONTINUATION: Check if candidate follows output sequence */
                    /* Check if candidate is next in input sequence (for completion) */
                    if (g->input_length > g->output_length) {
                        uint32_t next_input_pos = g->output_length;
                        if (g->input_buffer[next_input_pos] == candidate) {
                            /* Candidate is next in input sequence - strong boost */
                            context_contribution = context_factor * 1.5f;
                        }
                    }
                    
                    /* Also check if candidate appears in input (contextual relevance) */
                    bool in_input = false;
                    for (uint32_t i = 0; i < g->input_length; i++) {
                        if (g->input_buffer[i] == candidate) {
                            in_input = true;
                            break;
                        }
                    }
                    if (in_input && context_contribution < 0.1f) {
                        context_contribution = context_factor * 0.3f;  /* Moderate boost */
                    }
                }
                
                /* Final edge score = weighted combination of all factors */
                edge_score = base_score + activation_contribution + pattern_contribution + context_contribution;
                
                /* Pattern-generated loop suppression (patterns learn to avoid loops) */
                float loop_penalty = 1.0f;
                bool pattern_suppresses_loop = false;
                
                /* Check if any pattern suppresses this candidate (patterns learn loop avoidance) */
                for (uint32_t p = 0; p < g->pattern_count; p++) {
                    Pattern *pat = &g->patterns[p];
                    if (pat->activation > pat->threshold && pat->suppression_strength > 0.1f) {
                        /* Pattern has learned to suppress loops - check if this is a loop */
                        if (g->output_length >= 2 && candidate == g->output_buffer[g->output_length - 2]) {
                            loop_penalty *= (1.0f - pat->suppression_strength * pat->rule_confidence);
                            pattern_suppresses_loop = true;
                        }
                    }
                }
                
                /* Fallback loop penalty if no pattern handles it */
                if (!pattern_suppresses_loop) {
                    if (g->output_length >= 2 && candidate == g->output_buffer[g->output_length - 2]) {
                        loop_penalty = 0.1f;
                    }
                    if (g->output_length >= 3 && candidate == g->output_buffer[g->output_length - 3]) {
                        loop_penalty = 0.2f;
                    }
                }
                edge_score *= loop_penalty;
                
                /* System-wide loop pressure (if patterns haven't learned to handle it) */
                if (g->state.loop_pressure > 0.3f && !pattern_suppresses_loop) {
                    bool continues_loop = false;
                    if (g->output_length >= 3) {
                        for (uint32_t j = 0; j < 3 && j < g->output_length; j++) {
                            if (candidate == g->output_buffer[g->output_length - 1 - j]) {
                                continues_loop = true;
                                break;
                            }
                        }
                    }
                    if (continues_loop) {
                        edge_score *= (1.0f - g->state.loop_pressure);
                    }
                }
                
                if (edge_score > best_score) {
                    best_score = edge_score;
                    selected_node = candidate;
                    source_node = prev_output;
                }
            }
        }
    }
    
    /* STEP 4: Final fallback: Use context-aware selection */
    /* CRITICAL FIX: When output is empty, use INPUT context, not just highest activation */
    if (selected_node >= BYTE_VALUES) {
        if (g->output_length == 0 && g->input_length > 0) {
            /* FIRST SELECTION: Use INPUT context - select first character of input */
            /* Different inputs should produce different first outputs */
            selected_node = g->input_buffer[0];
            if (selected_node >= BYTE_VALUES || !g->nodes[selected_node].exists) {
                /* Fallback: find node with highest activation that's in input */
                float max_activation = 0.0f;
                for (uint32_t i = 0; i < g->input_length; i++) {
                    uint32_t input_node = g->input_buffer[i];
                    if (input_node < BYTE_VALUES && g->nodes[input_node].exists) {
                        if (g->nodes[input_node].activation > max_activation) {
                            max_activation = g->nodes[input_node].activation;
                            selected_node = input_node;
                        }
                    }
                }
            }
        } else {
            /* CONTINUATION: Use highest activation from wave propagation */
            float max_activation = 0.0f;
            for (int i = 0; i < BYTE_VALUES; i++) {
                if (g->nodes[i].exists && g->nodes[i].activation > max_activation) {
                    max_activation = g->nodes[i].activation;
                    selected_node = i;
                }
            }
        }
    }
    
    /* TRACK CONTRIBUTION: Record which edge led to this selection */
    if (selected_node < BYTE_VALUES && source_node < BYTE_VALUES) {
        /* Grow contribution array if needed */
        if (g->output_length >= g->output_contrib_capacity) {
            g->output_contrib_capacity *= 2;
            g->output_contributions = realloc(g->output_contributions, 
                sizeof(OutputContribution) * g->output_contrib_capacity);
        }
        
        /* Record edge contribution */
        OutputContribution *contrib = &g->output_contributions[g->output_length];
        if (contrib->edges == NULL) {
            contrib->edges = malloc(sizeof(EdgeContribution) * 1);
            contrib->edge_count = 0;
        } else if (contrib->edge_count == 0) {
            contrib->edges = realloc(contrib->edges, sizeof(EdgeContribution) * 1);
        }
        
        contrib->edges[0].from_node = source_node;
        contrib->edges[0].contribution = 1.0f;
        contrib->edge_count = 1;
        contrib->total_contribution = 1.0f;
        contrib->pattern_count = 0;
    }
    
    return selected_node;
    
    /* ========================================================================
     * OLD COMPLEX CODE BELOW (DISABLED)
     * ======================================================================== */
    #if 0
    
    /* ========================================================================
     * COMPUTE SYSTEM-WIDE STATISTICS (for relative measures)
     * Same statistics as in propagate_activation for consistency
     * ======================================================================== */
    
    /* Quick statistics computation for node selection */
    float sample_input_conn = 0.0f;
    float sample_context_match = 0.0f;
    float sample_history = 0.0f;
    float sample_pattern_pred = 0.0f;
    uint32_t samples = 0;
    
    for (int sample_i = 0; sample_i < BYTE_VALUES && sample_i < 30; sample_i++) {
        if (!g->nodes[sample_i].exists) continue;
        EdgeList *sample_out = &g->outgoing[sample_i];
        if (sample_out->count == 0) continue;
        
        uint32_t sample_target = sample_out->edges[0].to_id;
        
        /* Sample input connectivity */
        for (uint32_t inp = 0; inp < g->input_length && inp < 5; inp++) {
            uint32_t input_node = g->input_buffer[inp];
            if (input_node < BYTE_VALUES && g->nodes[input_node].exists) {
                EdgeList *input_out = &g->outgoing[input_node];
                for (uint32_t e = 0; e < input_out->count; e++) {
                    if (input_out->edges[e].to_id == sample_target && input_out->edges[e].active) {
                        sample_input_conn += 1.0f;
                        break;
                    }
                }
            }
        }
        
        /* Sample context match */
        for (uint32_t p = 0; p < g->pattern_count; p++) {
            Pattern *pat = &g->patterns[p];
            if (pat->activation > pat->threshold && pat->activation > 0.1f) {
                for (uint32_t pred = 0; pred < pat->prediction_count; pred++) {
                    if (pat->predicted_nodes[pred] == sample_target) {
                        sample_context_match += 1.0f;
                        break;
                    }
                }
            }
        }
        
        /* Sample history coherence */
        if (g->output_length > 0) {
            uint32_t last_output = g->output_buffer[g->output_length - 1];
            if (last_output < BYTE_VALUES && g->nodes[last_output].exists) {
                EdgeList *last_out = &g->outgoing[last_output];
                for (uint32_t e = 0; e < last_out->count; e++) {
                    if (last_out->edges[e].to_id == sample_target && last_out->edges[e].active) {
                        sample_history += 1.0f;
                        break;
                    }
                }
            }
        }
        
        samples++;
    }
    
    float avg_input_conn = (samples > 0) ? (sample_input_conn / samples) : 0.01f;
    float avg_context = (samples > 0) ? (sample_context_match / samples) : 0.01f;
    float avg_history = (samples > 0) ? (sample_history / samples) : 0.01f;
    
    /* Compute pattern prediction average */
    float total_pattern_pred = 0.0f;
    uint32_t pattern_count = 0;
    for (uint32_t p = 0; p < g->pattern_count; p++) {
        Pattern *pat = &g->patterns[p];
        if (pat->activation > pat->threshold && pat->activation > 0.1f) {
            total_pattern_pred += pat->activation * pat->strength;
            pattern_count++;
        }
    }
    float avg_pattern_pred = (pattern_count > 0) ? (total_pattern_pred / pattern_count) : 0.01f;
    
    if (avg_input_conn < 0.01f) avg_input_conn = 0.01f;
    if (avg_context < 0.01f) avg_context = 0.01f;
    if (avg_history < 0.01f) avg_history = 0.01f;
    if (avg_pattern_pred < 0.01f) avg_pattern_pred = 0.01f;
    
    float max_activation = 0.0f;
    uint32_t winner_node = BYTE_VALUES;  /* Use invalid node ID to detect no selection */
    
    /* If no nodes have activation, we can't select anything */
    /* This is a real problem - activation should exist from input injection */
    
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
        
        /* FACTOR 1: Information_Carried (from input to this node) - RELATIVE */
        float input_connection = avg_input_conn;  /* Default: relative to system average */
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
        input_connection = (avg_input_conn > 0.0f) ? (input_connection / avg_input_conn) : input_connection;
        
        float context_match = avg_context;  /* Default: relative to system average */
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
        context_match = (avg_context > 0.0f) ? (context_match / avg_context) : context_match;
        
        float history_coherence = avg_history;  /* Default: relative to system average */
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
        history_coherence = (avg_history > 0.0f) ? (history_coherence / avg_history) : history_coherence;
        
        /* INPUT-DRIVEN SELECTION: Base score on activation from INPUT, not output history */
        /* history_coherence creates loops (g→o→g→o), so weight it much lower */
        float input_weight = 0.7f;  /* Strong emphasis on input */
        float history_weight = 0.1f;  /* Weak emphasis on history (prevent loops) */
        float context_weight = 0.2f;  /* Moderate emphasis on patterns */
        
        float information = (input_connection * input_weight) + 
                           (context_match * context_weight) + 
                           (history_coherence * history_weight);
        
        /* FACTOR 2: Learning_Strength (how well-learned is this node?) */
        float node_activation = g->nodes[i].activation;
        float usage = logf(1.0f + g->nodes[i].receive_count) / 10.0f;  /* Log scale, normalized */
        /* CRITICAL FIX: If node has activation, learning should not be zero */
        /* Use activation directly if it exists, even if usage is low (early training) */
        /* No arbitrary multiplier - use activation and usage directly (already relative measures) */
        float learning = (node_activation > 0.01f) ? 
            (node_activation * (1.0f + usage)) : 
            usage;  /* Fallback: use usage directly (no arbitrary 0.1f multiplier) */
        
        /* FACTOR 3: Coherence (does this node fit contextually?) */
        /* Reduce weight of history_coherence to prevent self-reinforcing loops */
        float pattern_alignment = context_match;  /* Same as context_match */
        float sequential_flow = history_coherence * 0.2f;  /* MUCH lower weight */
        float context_fit = context_match;  /* Same as context_match */
        float coherence = pattern_alignment + sequential_flow + context_fit;
        
        /* FACTOR 4: Predictive_Power (how well does this node predict correct output?) - RELATIVE */
        float pattern_prediction = avg_pattern_pred;  /* Default: relative to system average */
        for (uint32_t p = 0; p < g->pattern_count; p++) {
            Pattern *pat = &g->patterns[p];
            if (pat->activation > pat->threshold && pat->activation > 0.1f) {
                for (uint32_t pred = 0; pred < pat->prediction_count; pred++) {
                    if (pat->predicted_nodes[pred] == i) {
                        float raw_pred = pat->activation * pat->strength;
                        pattern_prediction = (avg_pattern_pred > 0.0f) ? (raw_pred / avg_pattern_pred) : raw_pred;
                        break;
                    }
                }
            }
        }
        
        /* Historical accuracy: use average from system state (no arbitrary default) */
        float historical_accuracy = (g->state.avg_pattern_utility > 0.0f) ? 
            g->state.avg_pattern_utility : 0.0f;  /* Use system-wide pattern utility */
        float context_prediction = context_match;  /* Already relative */
        /* No arbitrary offset - historical_accuracy is already in meaningful range */
        float predictive = pattern_prediction * historical_accuracy * context_prediction;
        
        /* COMBINE: Node Quality = Activation × Relative Factors */
        /* Everything is RELATIVE to system state - no arbitrary constants */
        /* Intelligence emerges from how this node compares to the system average */
        
        /* RELATIVE INFORMATION: How well-connected is this node compared to average? */
        /* Compute average connectivity in system (for comparison) */
        float avg_connectivity = 0.0f;
        uint32_t connected_nodes = 0;
        for (int j = 0; j < BYTE_VALUES; j++) {
            if (!g->nodes[j].exists) continue;
            EdgeList *out = &g->outgoing[j];
            if (out->count > 0) {
                avg_connectivity += (float)out->count;
                connected_nodes++;
            }
        }
        if (connected_nodes > 0) {
            avg_connectivity /= connected_nodes;
        }
        
        /* This node's connectivity relative to average */
        EdgeList *node_out = &g->outgoing[i];
        float node_connectivity = (float)node_out->count;
        float relative_connectivity = (avg_connectivity > 0.0f) ? 
            (node_connectivity / avg_connectivity) : 1.0f;
        
        /* Information is relative to connectivity - well-connected nodes carry more information */
        float relative_info = information * relative_connectivity;
        /* CLAMP: Prevent explosion from division by tiny averages */
        relative_info = fmax(0.1f, fmin(relative_info, 10.0f));  /* Keep in [0.1, 10] range */
        
        /* RELATIVE LEARNING: How well-learned is this node compared to average? */
        /* Average usage in system */
        float avg_usage = 0.0f;
        uint32_t used_nodes = 0;
        for (int j = 0; j < BYTE_VALUES; j++) {
            if (!g->nodes[j].exists || g->nodes[j].receive_count == 0) continue;
            avg_usage += logf(1.0f + g->nodes[j].receive_count) / 10.0f;
            used_nodes++;
        }
        if (used_nodes > 0) {
            avg_usage /= used_nodes;
        }
        
        /* This node's usage relative to average */
        float relative_usage = (avg_usage > 0.0f) ? (usage / avg_usage) : 1.0f;
        /* CLAMP: Prevent explosion */
        relative_usage = fmax(0.1f, fmin(relative_usage, 10.0f));  /* Keep in [0.1, 10] range */
        
        /* RELATIVE COHERENCE: How coherent is this node compared to system? */
        /* Use pattern confidence as baseline - nodes predicted by patterns are more coherent */
        float relative_coherence = (g->state.pattern_confidence > 0.0f) ?
            (coherence / g->state.pattern_confidence) : 1.0f;
        /* CLAMP: Prevent explosion */
        relative_coherence = fmax(0.1f, fmin(relative_coherence, 10.0f));  /* Keep in [0.1, 10] range */
        
        /* RELATIVE PREDICTIVE: How well-predicted is this node compared to average? */
        /* Use pattern utility as baseline for what "good prediction" means */
        float relative_predictive = (g->state.avg_pattern_utility > 0.0f) ?
            (predictive / g->state.avg_pattern_utility) : 1.0f;
        /* CLAMP: Prevent explosion */
        relative_predictive = fmax(0.1f, fmin(relative_predictive, 10.0f));  /* Keep in [0.1, 10] range */
        
        /* COMBINE: Score = Activation × Relative Factors */
        /* All factors are ratios - above 1.0 = better than average, below 1.0 = worse */
        /* No arbitrary constants - everything is relative to system state */
        float score = node_activation * relative_info * relative_usage * relative_coherence * relative_predictive;
        
        /* Ensure score is non-negative (activation is always >= 0) */
        if (score < 0.0f) score = 0.0f;
        
        // #region agent log
        if (i == 'c' || i == 'a' || i == 't' || i == 's') {
            FILE *f = fopen("f:\\Melvin_Research\\Melvin_o7\\melvin_o7\\.cursor\\debug.log", "a");
            if (f) {
                fprintf(f, "{\"location\":\"melvin.c:3805\",\"message\":\"node score\",\"data\":{\"node\":\"%c\",\"activation\":%.3f,\"score\":%.6f,\"rel_info\":%.3f,\"rel_usage\":%.3f,\"rel_coh\":%.3f,\"rel_pred\":%.3f},\"timestamp\":%lld,\"sessionId\":\"debug-session\",\"runId\":\"debug3\",\"hypothesisId\":\"H\"}\n",
                        (char)i, node_activation, score, relative_info, relative_usage, relative_coherence, relative_predictive,
                        (long long)time(NULL) * 1000);
                fclose(f);
            }
        }
        // #endregion
        
        /* Skip nodes with negligible activation (relative to system) */
        /* Threshold is relative to system's average activation */
        float min_activation = g->state.avg_activation * 0.1f;  /* 10% of average */
        if (node_activation < min_activation && score < min_activation) continue;
        
        /* SELF-TUNING: Apply loop breaking based on loop pressure */
        /* High loop pressure = stuck in loops → break them aggressively */
        if (g->state.loop_pressure > 0.3f && g->output_length > 2) {
            float loop_penalty = g->state.loop_breaking_strength;
            if (loop_penalty < 0.1f) loop_penalty = g->state.loop_pressure * 10.0f;  /* Fallback calculation */
            
            if (i == g->output_buffer[g->output_length - 1] ||
                i == g->output_buffer[g->output_length - 2] ||
                i == g->output_buffer[g->output_length - 3]) {
                score *= (1.0f - loop_penalty);  /* Strongly suppress looping nodes */
                if (score < 0.01f) score = 0.01f;  /* Don't completely kill, just strongly suppress */
            }
        }
        
        /* SELF-TUNING: Apply diversity pressure (increase output diversity when stuck) */
        if (g->state.diversity_pressure > 0.3f) {
            /* Boost nodes that haven't been output recently */
            bool recently_output = false;
            uint32_t lookback = (g->output_length > 10) ? 10 : g->output_length;
            for (uint32_t j = 0; j < lookback; j++) {
                if (g->output_buffer[g->output_length - 1 - j] == i) {
                    recently_output = true;
                    break;
                }
            }
            if (!recently_output) {
                score *= (1.0f + g->state.diversity_pressure * 2.0f);  /* Boost novel nodes */
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
    /* If no node passed threshold, it means no nodes have sufficient activation */
    /* This indicates a problem with activation propagation, not selection */
    if (winner_node >= BYTE_VALUES) {
        /* DIAGNOSTIC: Find why no node was selected */
        /* Check if any nodes have activation at all */
        float max_act = 0.0f;
        uint32_t best_node = BYTE_VALUES;
        for (int i = 0; i < BYTE_VALUES; i++) {
            if (!g->nodes[i].exists) continue;
            if (g->nodes[i].activation > max_act) {
                max_act = g->nodes[i].activation;
                best_node = i;
            }
        }
        
        /* If we found a node with any activation, return it */
        /* The threshold might be too high, but we need to output something */
        if (best_node < BYTE_VALUES && max_act > 0.0f) {
            return best_node;
        }
        
        /* No nodes have activation - this is a real problem */
        /* Should not happen if input injection and propagation work correctly */
        return BYTE_VALUES;  /* Return invalid to signal problem */
    }
    return winner_node;
    #endif /* End of disabled complex selection code */
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
    
    /* IMPROVED: Detect loops more aggressively and create escape pressure */
    bool is_looping = false;
    
    /* Check for short loops (2-3 character repetition) */
    if (g->output_length >= 4) {
        /* Check if last 2 chars repeat */
        if (g->output_buffer[g->output_length - 1] == g->output_buffer[g->output_length - 3] &&
            g->output_buffer[g->output_length - 2] == g->output_buffer[g->output_length - 4]) {
            is_looping = true;
        }
    }
    
    /* Check for longer loops (3+ character repetition) */
    if (!is_looping && g->output_length >= 6) {
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
    
    /* Check for single character repetition (strongest loop signal) */
    if (!is_looping && g->output_length >= 3) {
        uint32_t last_char = g->output_buffer[g->output_length - 1];
        uint32_t repeat_count = 1;
        for (int i = g->output_length - 2; i >= 0 && i >= (int)g->output_length - 5; i--) {
            if (g->output_buffer[i] == last_char) {
                repeat_count++;
            } else {
                break;
            }
        }
        if (repeat_count >= 3) {
            is_looping = true;  /* Same character repeated 3+ times */
        }
    }
    
    if (is_looping) {
        g->state.loop_pressure = fmin(g->state.loop_pressure + 0.2f, 1.0f);  /* Increase pressure */
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
    // #region agent log
    FILE *f = fopen("f:\\Melvin_Research\\Melvin_o7\\melvin_o7\\.cursor\\debug.log", "a");
    if (f) {
        char output_str[32] = {0};
        char target_str[32] = {0};
        for (uint32_t i = 0; i < g->output_length && i < 31; i++) {
            output_str[i] = (char)g->output_buffer[i];
        }
        for (uint32_t i = 0; i < target_length && i < 31; i++) {
            target_str[i] = (char)target[i];
        }
        fprintf(f, "{\"location\":\"melvin.c:3908\",\"message\":\"apply_feedback called\",\"data\":{\"output\":\"%s\",\"target\":\"%s\",\"output_len\":%u,\"target_len\":%u},\"timestamp\":%lld,\"sessionId\":\"debug-session\",\"runId\":\"debug2\",\"hypothesisId\":\"F\"}\n",
                output_str, target_str, g->output_length, target_length, (long long)time(NULL) * 1000);
        fclose(f);
    }
    // #endregion
    
    /* Compare output to target */
    uint32_t correct = 0;
    uint32_t min_len = (g->output_length < target_length) ? g->output_length : target_length;
    
    /* RICH ERROR SIGNAL: Compute positional error and attribute to components */
    for (uint32_t i = 0; i < min_len; i++) {
        uint32_t predicted = g->output_buffer[i];
        uint32_t expected = target[i];
        
        if (predicted == expected) {
            correct++;
            
            // #region agent log
            FILE *f = fopen("f:\\Melvin_Research\\Melvin_o7\\melvin_o7\\.cursor\\debug.log", "a");
            if (f) {
                fprintf(f, "{\"location\":\"melvin.c:3928\",\"message\":\"correct prediction\",\"data\":{\"pos\":%u,\"predicted\":%u,\"expected\":%u,\"char\":\"%c\"},\"timestamp\":%lld,\"sessionId\":\"debug-session\",\"runId\":\"debug2\",\"hypothesisId\":\"F\"}\n",
                        i, predicted, expected, (char)predicted, (long long)time(NULL) * 1000);
                fclose(f);
            }
            // #endregion
            
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
                                
                                /* SELF-REGULATING: Successful patterns increase rule confidence */
                                /* Patterns that succeed get more rule authority */
                                pat->rule_confidence = fmin(1.0f, pat->rule_confidence + error_share * 0.1f);
                                pat->rule_successes++;
                                
                                /* SELF-REGULATING: Strengthen rule strengths when rules succeed */
                                /* Rules that lead to success get stronger */
                                for (uint32_t r = 0; r < pat->rule_count; r++) {
                                    pat->rule_strengths[r] = fmin(1.0f, pat->rule_strengths[r] + error_share * 0.05f);
                                }
                                
                                break;
                            }
                        }
                    }
                }
            }
            
            /* Strengthen edges that contributed correctly AND increment success_count */
            // #region agent log
            FILE *flog = fopen("f:\\Melvin_Research\\Melvin_o7\\melvin_o7\\.cursor\\debug.log", "a");
            if (flog) {
                fprintf(flog, "{\"location\":\"melvin.c:3970\",\"message\":\"edge contrib loop\",\"data\":{\"predicted\":%u,\"expected\":%u,\"edge_count\":%u},\"timestamp\":%lld,\"sessionId\":\"debug-session\",\"runId\":\"debug3\",\"hypothesisId\":\"G\"}\n",
                        predicted, expected, contrib->edge_count, (long long)time(NULL) * 1000);
                fclose(flog);
            }
            // #endregion
            
            for (uint32_t ec = 0; ec < contrib->edge_count; ec++) {
                uint32_t from = contrib->edges[ec].from_node;
                create_or_strengthen_edge(g, from, predicted);
                
                /* CRITICAL: Increment success_count for correct predictions */
                EdgeList *from_out = &g->outgoing[from];
                for (uint32_t e = 0; e < from_out->count; e++) {
                    if (from_out->edges[e].to_id == predicted && from_out->edges[e].active) {
                        from_out->edges[e].success_count++;
                        // #region agent log
                        FILE *f2 = fopen("f:\\Melvin_Research\\Melvin_o7\\melvin_o7\\.cursor\\debug.log", "a");
                        if (f2) {
                            fprintf(f2, "{\"location\":\"melvin.c:3986\",\"message\":\"edge success incremented\",\"data\":{\"from\":%u,\"to\":%u,\"success\":%llu,\"use\":%llu,\"weight\":%.3f},\"timestamp\":%lld,\"sessionId\":\"debug-session\",\"runId\":\"debug3\",\"hypothesisId\":\"G\"}\n",
                                    from, predicted, (unsigned long long)from_out->edges[e].success_count, 
                                    (unsigned long long)from_out->edges[e].use_count, from_out->edges[e].weight,
                                    (long long)time(NULL) * 1000);
                            fclose(f2);
                        }
                        // #endregion
                        break;
                    }
                }
            }
        } else {
            /* INCORRECT prediction - compute error share and weaken contributors */
            /* SELF-TUNING: Automatically weaken patterns/edges that lead to errors */
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
                    
                    /* SELF-TUNING: Weaken prediction weights that led to wrong prediction */
                    for (uint32_t pred = 0; pred < pat->prediction_count; pred++) {
                        if (pat->predicted_nodes[pred] == predicted) {
                            pat->prediction_weights[pred] -= 
                                g->state.learning_rate * error_share * 0.3f;
                            if (pat->prediction_weights[pred] < 0.0f) {
                                pat->prediction_weights[pred] = 0.0f;
                            }
                            
                                /* SELF-TUNING: Reduce pattern importance when it leads to errors */
                                /* Failed patterns become less important automatically */
                                pat->dynamic_importance *= (1.0f - error_share * 0.1f);
                                if (pat->dynamic_importance < 0.1f) pat->dynamic_importance = 0.1f;
                                
                                /* SELF-TUNING: Reduce accumulated meaning when pattern fails */
                                /* Failed patterns lose meaning (they don't carry correct information) */
                                pat->accumulated_meaning *= (1.0f - error_share * 0.2f);
                                
                                /* SELF-REGULATING: Reduce rule confidence when pattern fails */
                                /* Failed patterns lose rule authority (their rules are unreliable) */
                                pat->rule_confidence *= (1.0f - error_share * 0.15f);
                                if (pat->rule_confidence < 0.1f) pat->rule_confidence = 0.1f;
                                
                                /* SELF-REGULATING: Weaken rule strengths when rules fail */
                                /* Rules that lead to errors get weaker */
                                for (uint32_t r = 0; r < pat->rule_count; r++) {
                                    pat->rule_strengths[r] *= (1.0f - error_share * 0.1f);
                                    if (pat->rule_strengths[r] < 0.1f) pat->rule_strengths[r] = 0.1f;
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
    
    /* CRITICAL: Clear input buffer at start of each episode */
    /* Otherwise input accumulates: "cat" + "dog" = "catdog" */
    g->input_length = 0;
    
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
    
    /* GENERALIZATION: Connect new words to similar patterns */
    /* When a new word is seen, find similar patterns based on byte values and context */
    /* This allows "quokka" to connect to similar patterns like "cat", "bat" */
    if (g->input_length >= 2) {
        connect_to_similar_patterns(g, g->input_buffer, g->input_length);
    }
    
    /* CRITICAL: Compute system state BEFORE propagation */
    /* Wave prop needs current system state (avg_activation, etc.) */
    compute_system_state(g);
    
    /* INPUT AS SPARK: Just initiate wave propagation, don't dominate it */
    /* Input is a spark that tells wave prop where to go */
    /* Meaning comes from patterns (especially highest level patterns) */
    for (uint32_t i = 0; i < input_len && i < g->input_length; i++) {
        uint32_t node_id = g->input_buffer[i];
        if (node_id < BYTE_VALUES && g->nodes[node_id].exists) {
            /* Spark activation - just enough to start the wave */
            g->nodes[node_id].activation = 0.2f;  /* Spark, not dominant */
            /* Wave propagation will follow close connections and find meaning from patterns */
        }
    }
    
    /* Propagate activation for several steps */
    /* OPTIMIZATION: For chat (no target), use fewer steps for speed */
    /* For training (with target), use more steps for better learning */
    uint32_t num_steps;
    if (target == NULL || target_len == 0) {
        /* Chat mode: Fast response, fewer steps */
        num_steps = input_len * 2;  /* Faster for chat */
        if (num_steps < 10) num_steps = 10;  /* Minimum for chat */
        if (num_steps > 50) num_steps = 50;  /* Cap for speed */
    } else {
        /* Training mode: More steps for better learning */
        num_steps = input_len * 3;
        if (num_steps < 20) num_steps = 20;
        if (num_steps > 200) num_steps = 200;
    }
    
    /* Compute system state less frequently for chat mode */
    uint32_t state_update_interval = (target == NULL || target_len == 0) ? 5 : 1;
    
    for (uint32_t step = 0; step < num_steps; step++) {
        /* Compute system state periodically (less frequent for chat) */
        if (step % state_update_interval == 0) {
            compute_system_state(g);
        }
        
        /* Propagate */
        propagate_activation(g);
        
        /* Try to output */
        uint32_t output_node = select_output_node(g);
        
        // #region agent log
        FILE *f = fopen("f:\\Melvin_Research\\Melvin_o7\\melvin_o7\\.cursor\\debug.log", "a");
        if (f) {
            fprintf(f, "{\"location\":\"melvin.c:4233\",\"message\":\"node selected\",\"data\":{\"selected\":%u,\"activations\":{\"c\":%.3f,\"a\":%.3f,\"t\":%.3f,\"s\":%.3f}},\"timestamp\":%lld,\"sessionId\":\"debug-session\",\"hypothesisId\":\"D,E\"}\n",
                    output_node, 
                    g->nodes['c'].exists ? g->nodes['c'].activation : 0.0f,
                    g->nodes['a'].exists ? g->nodes['a'].activation : 0.0f,
                    g->nodes['t'].exists ? g->nodes['t'].activation : 0.0f,
                    g->nodes['s'].exists ? g->nodes['s'].activation : 0.0f,
                    (long long)time(NULL) * 1000);
            fclose(f);
        }
        // #endregion
        
        /* Check if valid node selected */
        if (output_node < BYTE_VALUES && g->nodes[output_node].exists) {
            emit_output(g, output_node);
        }
        /* If output_node >= BYTE_VALUES, no valid node was selected */
        /* This means no nodes have sufficient activation - propagation problem */
        
        /* Stop if output length matches expected range */
        /* TRAINING MODE: Stop when we reach target length */
        if (target != NULL && target_len > 0) {
            if (g->output_length >= target_len) {
                break;  /* Reached target length, stop to get accurate feedback */
            }
        } else {
            /* CHAT MODE: Stop after reasonable output */
            /* Range is relative to input length */
            float expected_ratio = 1.0f + 0.2f * g->state.error_rate;
            uint32_t max_output = (uint32_t)(input_len * expected_ratio + 5);
            
            if (g->output_length >= max_output) {
                break;
            }
            
            /* For chat mode, stop early if we have some output */
            if (g->output_length >= input_len) {
                break;  /* Got response, stop early */
            }
        }
    }
    
    /* ========================================================================
     * SELF-SUPERVISED LEARNING: Data is the answer key
     * Patterns check each other, learn from data structure, hierarchical validation
     * Early corrections help bootstrap, but intelligence comes from data itself
     * ======================================================================== */
    
    if (target != NULL && target_len > 0) {
        /* SUPERVISED LEARNING: Learn from explicit targets (early bootstrap) */
        learn_pattern_predictions(g, target, target_len);
        
        /* Create direct input→target edges */
        for (uint32_t i = 0; i < g->input_length && i < target_len; i++) {
            create_or_strengthen_edge(g, g->input_buffer[i], target[i]);
        }
        
        /* Apply feedback */
        apply_feedback(g, target, target_len);
    }
    
    /* SELF-SUPERVISED LEARNING: Always learn from data structure itself */
    /* Data structure provides feedback - sequences, co-occurrence, patterns */
    
    /* 1. Learn from input sequence structure (data is answer key) */
    if (g->input_length > 1) {
        /* Learn sequential patterns from input */
        for (uint32_t i = 0; i < g->input_length - 1; i++) {
            /* Sequence structure: input[i] → input[i+1] is valid */
            /* This is self-supervised - the sequence itself validates */
            create_or_strengthen_edge(g, g->input_buffer[i], g->input_buffer[i+1]);
        }
    }
    
    /* 2. Learn from output sequence structure (what we generated) */
    if (g->output_length > 1) {
        /* Output sequence structure validates itself */
        for (uint32_t i = 0; i < g->output_length - 1; i++) {
            /* If output[i] → output[i+1] happened, it's valid structure */
            create_or_strengthen_edge(g, g->output_buffer[i], g->output_buffer[i+1]);
        }
    }
    
    /* 3. Hierarchical validation: Higher patterns check lower patterns */
    /* Patterns at deeper levels (more abstract) validate patterns at shallower levels */
    for (uint32_t p1 = 0; p1 < g->pattern_count; p1++) {
        Pattern *pat1 = &g->patterns[p1];
        if (pat1->chain_depth == 0) continue;  /* Skip root patterns */
        
        /* Find parent pattern */
        if (pat1->parent_pattern_id < g->pattern_count) {
            Pattern *parent = &g->patterns[pat1->parent_pattern_id];
            
            /* Parent validates child: if parent matches, child should match */
            /* Check if parent's predictions include child's nodes */
            bool parent_validates_child = false;
            for (uint32_t pred = 0; pred < parent->prediction_count; pred++) {
                for (uint32_t i = 0; i < pat1->length; i++) {
                    if (parent->predicted_nodes[pred] == pat1->node_ids[i]) {
                        parent_validates_child = true;
                        break;
                    }
                }
                if (parent_validates_child) break;
            }
            
            if (parent_validates_child) {
                /* Parent validates child - strengthen child */
                pat1->strength = fmin(1.0f, pat1->strength + 0.01f * g->state.learning_rate);
                pat1->prediction_successes++;  /* Hierarchical validation = success */
            }
        }
    }
    
    /* 4. Pattern co-occurrence validation (patterns check each other) */
    /* If patterns co-occur frequently, they validate each other */
    for (uint32_t p1 = 0; p1 < g->pattern_count; p1++) {
        Pattern *pat1 = &g->patterns[p1];
        if (pat1->activation < pat1->threshold) continue;
        
        for (uint32_t p2 = p1 + 1; p2 < g->pattern_count; p2++) {
            Pattern *pat2 = &g->patterns[p2];
            if (pat2->activation < pat2->threshold) continue;
            
            /* Check if patterns co-occur in input/output */
            bool co_occur = false;
            
            /* Check input */
            if (g->input_length >= pat1->length + pat2->length) {
                for (uint32_t pos = 0; pos <= g->input_length - pat1->length - pat2->length; pos++) {
                    if (pattern_matches(g, p1, g->input_buffer, g->input_length, pos)) {
                        if (pattern_matches(g, p2, g->input_buffer, g->input_length, pos + pat1->length)) {
                            co_occur = true;
                            break;
                        }
                    }
                }
            }
            
            /* Check output */
            if (!co_occur && g->output_length >= pat1->length + pat2->length) {
                for (uint32_t pos = 0; pos <= g->output_length - pat1->length - pat2->length; pos++) {
                    if (pattern_matches(g, p1, g->output_buffer, g->output_length, pos)) {
                        if (pattern_matches(g, p2, g->output_buffer, g->output_length, pos + pat1->length)) {
                            co_occur = true;
                            break;
                        }
                    }
                }
            }
            
            if (co_occur) {
                /* Patterns co-occur - they validate each other */
                learn_pattern_association(g, p1, p2);
                /* Strengthen both patterns (mutual validation) */
                pat1->strength = fmin(1.0f, pat1->strength + 0.005f * g->state.learning_rate);
                pat2->strength = fmin(1.0f, pat2->strength + 0.005f * g->state.learning_rate);
            }
        }
    }
    
    /* 5. Self-consistency checking: Patterns validate their own predictions */
    /* If pattern predicts X and X appears in data, pattern validates itself */
    for (uint32_t p = 0; p < g->pattern_count; p++) {
        Pattern *pat = &g->patterns[p];
        if (pat->activation < pat->threshold) continue;
        
        /* Check if pattern's predictions appear in input/output */
        for (uint32_t pred = 0; pred < pat->prediction_count; pred++) {
            uint32_t predicted_node = pat->predicted_nodes[pred];
            bool prediction_validated = false;
            
            /* Check if predicted node appears in input */
            for (uint32_t i = 0; i < g->input_length; i++) {
                if (g->input_buffer[i] == predicted_node) {
                    prediction_validated = true;
                    break;
                }
            }
            
            /* Check if predicted node appears in output */
            if (!prediction_validated) {
                for (uint32_t i = 0; i < g->output_length; i++) {
                    if (g->output_buffer[i] == predicted_node) {
                        prediction_validated = true;
                        break;
                    }
                }
            }
            
            if (prediction_validated) {
                /* Pattern's prediction validated by data - strengthen */
                pat->prediction_successes++;
                pat->prediction_weights[pred] = fmin(1.0f, pat->prediction_weights[pred] + 0.01f * g->state.learning_rate);
            } else {
                /* Prediction not validated - weaken slightly */
                pat->prediction_weights[pred] = fmax(0.1f, pat->prediction_weights[pred] - 0.001f * g->state.learning_rate);
            }
            
            pat->prediction_attempts++;
        }
    }
    
    /* 6. Pattern detection from data structure (always on, not just supervised) */
    /* Detect patterns in input/output sequences - data structure provides patterns */
    if (g->input_length > 1 || g->output_length > 1) {
        detect_patterns(g);  /* Learn patterns from data structure */
    }
    
    /* 7. Learn propagation and selection parameters from data */
    /* Patterns learn HOW to propagate and HOW to select from what works */
    learn_propagation_selection_parameters(g, target, target_len);
}

/* ============================================================================
 * LEARN PROPAGATION & SELECTION PARAMETERS FROM DATA
 * 
 * Patterns learn HOW to propagate and HOW to select by tracking what works
 * Like transistors - complexity comes from learned activation patterns
 * ============================================================================ */

void learn_propagation_selection_parameters(MelvinGraph *g, const uint8_t *target, uint32_t target_len) {
    /* For each active pattern, learn if its propagation/selection led to success */
    for (uint32_t p = 0; p < g->pattern_count; p++) {
        Pattern *pat = &g->patterns[p];
        if (pat->activation < pat->threshold) continue;
        
        /* Check if pattern's predictions matched what actually happened */
        bool pattern_contributed_to_success = false;
        
        if (target != NULL && target_len > 0) {
            /* Supervised: Check if pattern predicted target */
            for (uint32_t pred = 0; pred < pat->prediction_count; pred++) {
                uint32_t predicted_node = pat->predicted_nodes[pred];
                for (uint32_t i = 0; i < target_len; i++) {
                    if (target[i] == predicted_node) {
                        pattern_contributed_to_success = true;
                        break;
                    }
                }
                if (pattern_contributed_to_success) break;
            }
        } else {
            /* Self-supervised: Check if pattern's predictions appeared in output */
            for (uint32_t pred = 0; pred < pat->prediction_count; pred++) {
                uint32_t predicted_node = pat->predicted_nodes[pred];
                for (uint32_t i = 0; i < g->output_length; i++) {
                    if (g->output_buffer[i] == predicted_node) {
                        pattern_contributed_to_success = true;
                        break;
                    }
                }
                if (pattern_contributed_to_success) break;
            }
        }
        
        /* Update propagation parameters based on success */
        pat->propagation_attempts++;
        if (pattern_contributed_to_success) {
            pat->propagation_successes++;
            
            /* Success: Increase transfer rate (transfer more activation) */
            pat->propagation_transfer_rate = fmin(1.0f, 
                pat->propagation_transfer_rate + 0.01f * g->state.learning_rate);
            
            /* Success: Adjust decay rate toward optimal (keep more activation) */
            pat->propagation_decay_rate = fmin(0.99f, 
                pat->propagation_decay_rate + 0.005f * g->state.learning_rate);
            
            /* Success: Lower threshold (propagate more easily) */
            pat->propagation_threshold = fmax(0.01f, 
                pat->propagation_threshold - 0.001f * g->state.learning_rate);
            
            /* Success: Increase boost factor (patterns that work get stronger) */
            pat->propagation_boost_factor = fmin(2.0f, 
                pat->propagation_boost_factor + 0.01f * g->state.learning_rate);
        } else {
            /* Failure: Decrease transfer rate (transfer less) */
            pat->propagation_transfer_rate = fmax(0.1f, 
                pat->propagation_transfer_rate - 0.005f * g->state.learning_rate);
            
            /* Failure: Increase decay rate (lose activation faster) */
            pat->propagation_decay_rate = fmax(0.5f, 
                pat->propagation_decay_rate - 0.005f * g->state.learning_rate);
            
            /* Failure: Raise threshold (propagate less easily) */
            pat->propagation_threshold = fmin(0.5f, 
                pat->propagation_threshold + 0.001f * g->state.learning_rate);
        }
        
        /* Update selection parameters based on success */
        pat->selection_attempts++;
        if (pattern_contributed_to_success) {
            pat->selection_successes++;
            
            /* Success: Patterns that work increase their selection influence */
            float success_rate = (float)pat->selection_successes / (float)pat->selection_attempts;
            
            /* If pattern is successful, increase factors that matter */
            if (success_rate > 0.6f) {
                /* High success: Increase pattern factor (patterns matter more) */
                pat->selection_pattern_factor = fmin(0.5f, 
                    pat->selection_pattern_factor + 0.01f * g->state.learning_rate);
                
                /* High success: Increase context factor (context matters) */
                pat->selection_context_factor = fmin(0.4f, 
                    pat->selection_context_factor + 0.01f * g->state.learning_rate);
            } else {
                /* Moderate success: Increase weight factor (edges matter) */
                pat->selection_weight_factor = fmin(0.6f, 
                    pat->selection_weight_factor + 0.01f * g->state.learning_rate);
                
                /* Moderate success: Increase activation factor */
                pat->selection_activation_factor = fmin(0.5f, 
                    pat->selection_activation_factor + 0.01f * g->state.learning_rate);
            }
        } else {
            /* Failure: Decrease pattern factor (patterns don't help) */
            pat->selection_pattern_factor = fmax(0.05f, 
                pat->selection_pattern_factor - 0.005f * g->state.learning_rate);
            
            /* Failure: Increase weight factor (fall back to edges) */
            pat->selection_weight_factor = fmin(0.7f, 
                pat->selection_weight_factor + 0.005f * g->state.learning_rate);
        }
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
                                        
                                        /* PHASE 3: Learn activation rule */
                                        /* If pattern A predicts pattern B successfully, learn rule */
                                        float success_rate = (pat1->prediction_attempts > 0) ?
                                            ((float)pat1->prediction_successes / (float)pat1->prediction_attempts) : 0.5f;
                                        float boost_amount = pat1->pattern_prediction_weights[pat1->pattern_prediction_count - 1];
                                        learn_activation_rule(g, p1, p2, boost_amount, success_rate);
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
    
    /* CRITICAL FIX: Learn input→output mappings */
    /* When patterns match INPUT, learn to predict TARGET nodes */
    /* This enables "hello" → "world" learning */
    for (uint32_t p = 0; p < g->pattern_count; p++) {
        Pattern *pat = &g->patterns[p];
        
        /* Check if pattern matches INPUT (not just target) */
        if (g->input_length >= pat->length) {
            for (uint32_t input_pos = 0; input_pos <= g->input_length - pat->length; input_pos++) {
                if (pattern_matches(g, p, g->input_buffer, g->input_length, input_pos)) {
                    /* Pattern matches input! Learn to predict TARGET nodes */
                    if (target_len > 0) {
                        /* Learn to predict first node of target */
                        uint32_t target_node = target[0];
                        
                        /* Find or add prediction */
                        bool found = false;
                        for (uint32_t pred = 0; pred < pat->prediction_count; pred++) {
                            if (pat->predicted_nodes[pred] == target_node) {
                                /* Strengthen existing prediction */
                                pat->prediction_weights[pred] += 0.3f * g->state.learning_rate;
                                if (pat->prediction_weights[pred] > 1.0f) {
                                    pat->prediction_weights[pred] = 1.0f;
                                }
                                found = true;
                                break;
                            }
                        }
                        
                        if (!found) {
                            /* Add new prediction: pattern → target node */
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
                            
                            pat->predicted_nodes[pat->prediction_count] = target_node;
                            pat->prediction_weights[pat->prediction_count] = 1.0f;  /* Full strength when learned from target */
                            pat->prediction_count++;
                        }
                    }
                    break;  /* Found match, move to next pattern */
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
            /* Initialize pattern prediction fields (must be initialized to prevent undefined behavior) */
            pat->predicted_patterns = NULL;
            pat->pattern_prediction_weights = NULL;
            pat->pattern_prediction_count = 0;
            
            /* Initialize all enhancement fields */
            initialize_pattern_enhancements(pat);
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
    
    /* Read tests from test_input.txt */
    FILE *test_file = fopen("test_input.txt", "r");
    if (!test_file) {
        fprintf(stderr, "Warning: Could not open test_input.txt, using default tests\n");
        /* Fallback to default tests */
        uint8_t training_inputs[][10] = {
            {'c', 'a', 't'},
            {'d', 'o', 'g'},
            {'c', 'a', 't'},
            {'d', 'o', 'g'}
        };
        uint32_t training_lengths[] = {3, 3, 3, 3};
        for (int episode = 0; episode < 30; episode++) {
            int idx = episode % 4;
            uint8_t *input = training_inputs[idx];
            uint32_t len = training_lengths[idx];
            run_episode(g, input, len, input, len);
            printf("Ep %2d | Input:  ", episode + 1);
            for (uint32_t i = 0; i < len; i++) printf("%c", input[i]);
            printf(" → Output: ");
            for (uint32_t i = 0; i < g->output_length; i++) {
                printf("%c", (uint8_t)g->output_buffer[i]);
            }
            printf(" | Error: %.3f | LR: %.3f\n", 
                   g->state.error_rate, g->state.learning_rate);
        }
    } else {
        /* Read from test_input.txt */
        char line[256];
        int test_num = 0;
        int simple_tests = 0, complex_tests = 0;
        int simple_correct = 0, complex_correct = 0;
        
        printf("Reading tests from test_input.txt...\n\n");
        printf("FORMAT: Test# | Input -> Output | Expected | Correct | Error | Samples\n");
        printf("----------------------------------------------------------------------\n");
        
        while (fgets(line, sizeof(line), test_file)) {
            if (line[0] == '#' || line[0] == '\n') continue;
            
            char *arrow = strstr(line, "->");
            if (!arrow) continue;
            
            *arrow = '\0';
            char *input_text = line;
            char *expected_text = arrow + 2;
            
            /* Trim whitespace */
            while (*input_text == ' ' || *input_text == '\t') input_text++;
            while (*expected_text == ' ' || *expected_text == '\t') expected_text++;
            char *nl = strchr(expected_text, '\n');
            if (nl) *nl = '\0';
            nl = strchr(input_text, '\n');
            if (nl) *nl = '\0';
            
            if (strlen(input_text) == 0 || strlen(expected_text) == 0) continue;
            
            test_num++;
            int is_complex = (strlen(input_text) > 5 || strlen(expected_text) > 5);
            if (is_complex) complex_tests++; else simple_tests++;
            
            uint8_t *input = (uint8_t*)input_text;
            uint8_t *expected = (uint8_t*)expected_text;
            uint32_t input_len = strlen(input_text);
            uint32_t expected_len = strlen(expected_text);
            
            /* Set ports */
            melvin_set_input_port(g, 0);  /* Port 0 = text */
            melvin_set_output_port(g, 0);
            
            /* Run episode */
            run_episode(g, input, input_len, expected, expected_len);
            
            /* Get output */
            uint32_t *output;
            uint32_t output_len;
            melvin_get_output(g, &output, &output_len);
            
            /* Check correctness */
            int correct = 1;
            if (output_len != expected_len) {
                correct = 0;
            } else {
                for (uint32_t i = 0; i < output_len; i++) {
                    if (output[i] != expected[i]) {
                        correct = 0;
                        break;
                    }
                }
            }
            
            if (correct) {
                if (is_complex) complex_correct++;
                else simple_correct++;
            }
            
            /* Print result */
            printf("Test %2d | Input: %-20s -> Output: ", test_num, input_text);
            for (uint32_t i = 0; i < output_len && i < 30; i++) {
                printf("%c", (char)output[i]);
            }
            printf(" | Expected: %-20s | %s | Error: %.3f | Samples: %d\n",
                   expected_text, correct ? "✓" : "✗",
                   melvin_get_error_rate(g), test_num);
            
            /* Show progress every 5 tests */
            if (test_num % 5 == 0) {
                uint32_t total_edges = 0;
                for (int i = 0; i < BYTE_VALUES; i++) {
                    total_edges += g->outgoing[i].count;
                }
                printf("  [Patterns: %u, Edges: %u, Wave steps: %llu, Simple: %d/%d, Complex: %d/%d]\n",
                       g->pattern_count, total_edges, (unsigned long long)g->state.step,
                       simple_correct, simple_tests, complex_correct, complex_tests);
            }
        }
        
        fclose(test_file);
        
        /* Summary */
        printf("\n=== SUMMARY ===\n");
        printf("Simple tests: %d/%d correct (%.1f%%) - Samples needed: %d\n",
               simple_correct, simple_tests,
               simple_tests > 0 ? (100.0f * simple_correct / simple_tests) : 0.0f,
               simple_tests > 0 ? (simple_correct == simple_tests ? test_num : test_num) : 0);
        printf("Complex tests: %d/%d correct (%.1f%%) - Samples needed: %d\n",
               complex_correct, complex_tests,
               complex_tests > 0 ? (100.0f * complex_correct / complex_tests) : 0.0f,
               complex_tests > 0 ? test_num : 0);
        printf("Total tests run: %d\n", test_num);
        printf("Final error rate: %.3f\n", melvin_get_error_rate(g));
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
    uint32_t active_edges = 0;
    uint32_t successful_edges = 0;
    float avg_edge_success = 0.0f;
    
    for (int i = 0; i < BYTE_VALUES; i++) {
        total_edges += g->outgoing[i].count;
        for (uint32_t j = 0; j < g->outgoing[i].count; j++) {
            Edge *e = &g->outgoing[i].edges[j];
            if (e->active && e->use_count > 0) {
                active_edges++;
                if (e->success_count > 0) {
                    successful_edges++;
                    avg_edge_success += (float)e->success_count / (float)e->use_count;
                }
            }
        }
    }
    
    if (successful_edges > 0) {
        avg_edge_success /= successful_edges;
    }
    
    printf("Total Edges: %u\n", total_edges);
    printf("Active Edges: %u (%.1f%% utilization)\n", active_edges, 
           total_edges > 0 ? (100.0f * active_edges / total_edges) : 0.0f);
    printf("Successful Edges: %u (%.1f%% effective)\n", successful_edges,
           active_edges > 0 ? (100.0f * successful_edges / active_edges) : 0.0f);
    printf("Avg Edge Success Rate: %.3f\n", avg_edge_success);
    
    /* Show top performing patterns */
    printf("\n=== TOP PATTERNS ===\n");
    uint32_t patterns_shown = 0;
    for (uint32_t p = 0; p < g->pattern_count && patterns_shown < 5; p++) {
        Pattern *pat = &g->patterns[p];
        if (pat->length > 0 && pat->length <= 10) {
            printf("Pattern %u: [", p);
            for (uint32_t i = 0; i < pat->length; i++) {
                if (pat->node_ids[i] < 128) {
                    printf("%c", (char)pat->node_ids[i]);
                } else {
                    printf("?");
                }
            }
            float success_rate = pat->prediction_attempts > 0 ? 
                (float)pat->prediction_successes / pat->prediction_attempts : 0.0f;
            printf("] Strength: %.3f, Success: %.1f%% (%llu/%llu)\n",
                   pat->strength, success_rate * 100.0f,
                   (unsigned long long)pat->prediction_successes,
                   (unsigned long long)pat->prediction_attempts);
            patterns_shown++;
        }
    }
    
    /* Show edge directionality proof */
    printf("\n=== EDGE DIRECTIONALITY (Unidirectional Proof) ===\n");
    printf("Checking for bidirectional edges...\n");
    uint32_t bidirectional_count = 0;
    uint32_t unidirectional_count = 0;
    
    for (int i = 0; i < BYTE_VALUES; i++) {
        for (uint32_t j = 0; j < g->outgoing[i].count; j++) {
            uint32_t to = g->outgoing[i].edges[j].to_id;
            if (to < BYTE_VALUES) {
                /* Check if reverse edge exists */
                bool reverse_exists = false;
                for (uint32_t k = 0; k < g->outgoing[to].count; k++) {
                    if (g->outgoing[to].edges[k].to_id == i) {
                        reverse_exists = true;
                        break;
                    }
                }
                if (reverse_exists) {
                    bidirectional_count++;
                } else {
                    unidirectional_count++;
                }
            }
        }
    }
    
    printf("Unidirectional edges: %u\n", unidirectional_count);
    printf("Bidirectional pairs: %u\n", bidirectional_count / 2);
    printf("Unidirectionality: %.1f%%\n", 
           (unidirectional_count + bidirectional_count) > 0 ?
           (100.0f * unidirectional_count / (unidirectional_count + bidirectional_count)) : 0.0f);
    
    /* DEBUG: Show actual edges to understand the "gog" loop */
    printf("\n=== EDGE ANALYSIS (Why \"gog\"?) ===\n");
    printf("Edges involving letters in training:\n");
    const char* check_chars = "catdog";
    for (int i = 0; check_chars[i] != '\0'; i++) {
        char ch = check_chars[i];
        uint8_t node = (uint8_t)ch;
        if (g->outgoing[node].count > 0) {
            printf("'%c' -> ", ch);
            for (uint32_t j = 0; j < g->outgoing[node].count; j++) {
                uint32_t to = g->outgoing[node].edges[j].to_id;
                if (to < 128) {
                    printf("'%c'(w:%.2f,u:%llu) ", (char)to, 
                           g->outgoing[node].edges[j].weight,
                           (unsigned long long)g->outgoing[node].edges[j].use_count);
                }
            }
            printf("\n");
        }
    }
    
    printf("\nNode activations after final episode:\n");
    for (int i = 0; check_chars[i] != '\0'; i++) {
        char ch = check_chars[i];
        uint8_t node = (uint8_t)ch;
        printf("'%c': act=%.3f, fires=%llu, receives=%llu\n", 
               ch, g->nodes[node].activation,
               (unsigned long long)g->nodes[node].fire_count,
               (unsigned long long)g->nodes[node].receive_count);
    }
    
    /* ========================================================================
     * PATTERN HIERARCHIES & WAVE PROPAGATION VISUALIZATION
     * ======================================================================== */
    printf("\n=== PATTERN HIERARCHIES (What Makes This Different) ===\n");
    printf("Pattern hierarchies show how patterns connect to build meaning:\n\n");
    
    uint32_t hierarchies_shown = 0;
    for (uint32_t p = 0; p < g->pattern_count && hierarchies_shown < 10; p++) {
        Pattern *pat = &g->patterns[p];
        if (pat->length == 0 || pat->length > 10) continue;
        
        /* Show pattern with hierarchy info */
        printf("Pattern %u [depth:%u, meaning:%.3f]: \"", 
               p, pat->chain_depth, pat->accumulated_meaning);
        for (uint32_t i = 0; i < pat->length; i++) {
            if (pat->node_ids[i] < 128) {
                printf("%c", (char)pat->node_ids[i]);
            } else {
                printf("_");
            }
        }
        printf("\"");
        
        /* Show parent if exists */
        if (pat->parent_pattern_id != INVALID_PATTERN_ID && pat->parent_pattern_id < g->pattern_count) {
            printf(" (child of pattern %u)", pat->parent_pattern_id);
        } else {
            printf(" (root)");
        }
        
        /* Show children (patterns that have this as parent) */
        uint32_t child_count = 0;
        for (uint32_t q = 0; q < g->pattern_count; q++) {
            if (g->patterns[q].parent_pattern_id == p) {
                child_count++;
            }
        }
        if (child_count > 0) {
            printf(" -> %u children", child_count);
        }
        
        /* Show pattern-to-pattern predictions */
        if (pat->pattern_prediction_count > 0) {
            printf(" -> predicts patterns: ");
            for (uint32_t pp = 0; pp < pat->pattern_prediction_count && pp < 3; pp++) {
                printf("%u(%.2f) ", pat->predicted_patterns[pp], 
                       pat->pattern_prediction_weights[pp]);
            }
        }
        
        printf("\n");
        hierarchies_shown++;
    }
    
    printf("\n=== WAVE PROPAGATION (Multi-Step vs Single Pass) ===\n");
    printf("Standard Neural Net: Input → Layer1 → Layer2 → Output (1 pass)\n");
    printf("Melvin O7: Input → Step1 → Step2 → ... → StepN → Output (multi-step wave)\n");
    printf("\nWave propagation features:\n");
    printf("  - PATH-AWARE: Only follows learned edges (not all connections)\n");
    printf("  - PATTERN-GUIDED: Active patterns boost predicted nodes\n");
    printf("  - CONTEXT-AWARE: Considers input, history, and pattern support\n");
    printf("  - MEANING-BOOSTED: Pattern hierarchies influence path quality\n");
    printf("\nTotal propagation steps in last episode: %llu\n", 
           (unsigned long long)g->state.step);
    
    melvin_destroy(g);
    return 0;
}
#endif /* MELVIN_STANDALONE */


