#ifndef HIVE_CORE_DYNAMICS_H
#define HIVE_CORE_DYNAMICS_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ----------------------------------------------------------------
 * Agent roles — mapped from bee biology
 * ---------------------------------------------------------------- */
typedef enum hive_agent_role {
    HIVE_ROLE_EMPTY = 0,
    HIVE_ROLE_QUEEN,
    HIVE_ROLE_CLEANER,
    HIVE_ROLE_NURSE,
    HIVE_ROLE_BUILDER,
    HIVE_ROLE_GUARD,
    HIVE_ROLE_FORAGER,
    HIVE_ROLE_DRONE,
    HIVE_ROLE_COUNT
} hive_agent_role_t;

/* ----------------------------------------------------------------
 * Signal types — chemical / physical / democratic
 * ---------------------------------------------------------------- */
typedef enum hive_signal_type {
    HIVE_SIGNAL_PHEROMONE = 0,
    HIVE_SIGNAL_WAGGLE,
    HIVE_SIGNAL_ALARM,
    HIVE_SIGNAL_PROPOSAL,
    HIVE_SIGNAL_TYPE_COUNT
} hive_signal_type_t;

/* ----------------------------------------------------------------
 * Lifecycle templates — age-driven role progressions
 * ---------------------------------------------------------------- */

/** Maximum stages per template and slots in the registry. */
#define HIVE_LIFECYCLE_MAX_STAGES    8
#define HIVE_LIFECYCLE_MAX_TEMPLATES 16

/**
 * One stage of a lifecycle template.
 * start_tick: the agent's age_ticks value at which this stage begins.
 * role:       role the agent occupies during this stage.
 * firmness:   100 = forced transition; 0-99 = per-tick chance (%).
 */
typedef struct hive_lifecycle_stage {
    uint32_t          start_tick;
    hive_agent_role_t role;
    uint8_t           firmness;   /* 0–100 */
} hive_lifecycle_stage_t;

/**
 * A named, ordered sequence of lifecycle stages.
 * Stages must be sorted by start_tick ascending and start at tick 0.
 */
typedef struct hive_lifecycle_template {
    const char             *name;
    const char             *description;
    hive_lifecycle_stage_t  stages[HIVE_LIFECYCLE_MAX_STAGES];
    size_t                  stage_count;
} hive_lifecycle_template_t;

/** Template identifier; 0 == HIVE_LIFECYCLE_NONE (no template). */
typedef uint8_t hive_lifecycle_id_t;
#define HIVE_LIFECYCLE_NONE 0

/* ----------------------------------------------------------------
 * Per-agent trait flags — set at spawn time by the Queen
 * ---------------------------------------------------------------- */
typedef struct hive_agent_traits {
    uint8_t  temperature_pct;  /* 0–100; scaled inference temperature         */
    uint32_t lineage_hash;     /* parent lineage identifier (inherits Queen's) */
    uint8_t  specialization;   /* bitmask: which task types this worker handles */
    uint8_t  resource_cap_pct; /* 0–100; % of token budget available           */
} hive_agent_traits_t;

/* ----------------------------------------------------------------
 * Grooming metadata packet — sent from worker to Queen each task
 * ---------------------------------------------------------------- */
typedef struct hive_groom_packet {
    size_t   agent_idx;    /* index into dynamics.agents[]                 */
    uint32_t task_ticks;   /* ticks spent on last task                     */
    uint8_t  success;      /* 0 = failed, 100 = perfect                    */
    uint8_t  alarm_flag;   /* 1 if worker encountered an error condition   */
} hive_groom_packet_t;

/* ----------------------------------------------------------------
 * Per-agent cell state
 * ---------------------------------------------------------------- */
typedef struct hive_agent_cell {
    hive_agent_role_t   role;
    uint32_t            age_ticks;
    uint32_t            perf_score;     /* 0–100 */
    uint32_t            signal_count;   /* recent signals emitted */
    hive_lifecycle_id_t lifecycle_id;   /* 0 = no template (legacy transitions) */
    hive_agent_traits_t traits;         /* per-agent specialisation flags       */
    uint8_t             vitality_seen;  /* last vitality checksum sniffed       */
    bool                conditioned_ok; /* allowed to execute this tick?        */
} hive_agent_cell_t;

/* ----------------------------------------------------------------
 * Aggregate hive statistics
 * ---------------------------------------------------------------- */
typedef struct hive_dynamics_stats {
    unsigned role_counts[HIVE_ROLE_COUNT];
    unsigned total_agents;
    unsigned total_signals;
    unsigned active_pheromones;
    unsigned active_waggles;
    unsigned active_alarms;
    unsigned pending_proposals;
    unsigned backlog_depth;
    unsigned queen_reviews;
    unsigned worker_spawns;
    unsigned quorum_votes;
    unsigned quorum_threshold;
    unsigned requeue_events;     /* count of successful re-queening events       */
} hive_dynamics_stats_t;

/* ----------------------------------------------------------------
 * Colony-level vitality constants
 * ---------------------------------------------------------------- */
/** Minimum vitality checksum for workers to be conditioned to execute. */
#define HIVE_VITALITY_MIN          20U
/** Spawn a new worker when demand > active_workers × this ratio.       */
#define HIVE_SPAWN_DEMAND_RATIO    3U
/** Queen vitality below this triggers re-queening.                     */
#define HIVE_REQUEUE_THRESHOLD     30U

/* ----------------------------------------------------------------
 * Hive dynamics state
 * ---------------------------------------------------------------- */
#define HIVE_DYNAMICS_MAX_AGENTS 256

typedef struct hive_dynamics {
    hive_agent_cell_t agents[HIVE_DYNAMICS_MAX_AGENTS];
    size_t            agent_count;
    hive_dynamics_stats_t stats;

    /* Queen / vitality pheromone fields */
    uint32_t vitality_checksum;    /* Queen Substance: rolling success-rate hash  */
    uint32_t demand_buffer_depth;  /* pending task queue depth fed from outside   */
    size_t   queen_idx;            /* index of the current Queen in agents[]      */
    bool     queen_alive;          /* false triggers re-queening on next tick     */
    uint32_t lineage_generation;   /* incremented on every re-queening event      */

    /* Runtime-tunable colony thresholds (initialised from the #define defaults) */
    uint32_t cfg_vitality_min;        /* conditioned-ok floor (default 20)          */
    uint32_t cfg_spawn_demand_ratio;  /* demand/worker spawn ratio (default 3)      */
    uint32_t cfg_requeue_threshold;   /* Queen perf below this re-queens (default 30)*/
} hive_dynamics_t;

/* ----------------------------------------------------------------
 * Lifecycle registry API
 * ---------------------------------------------------------------- */

/** Initialise the registry and register built-in templates.
 *  Called automatically by hive_dynamics_init(). */
void hive_lifecycle_init_registry(void);

/** Register a template; returns its 1-based id or HIVE_LIFECYCLE_NONE on failure. */
hive_lifecycle_id_t hive_lifecycle_register(const hive_lifecycle_template_t *tmpl);

/** Look up a registered template by id. Returns NULL for id == 0 or out-of-range. */
const hive_lifecycle_template_t *hive_lifecycle_get(hive_lifecycle_id_t id);

/** Number of registered templates. */
size_t hive_lifecycle_count(void);

/** IDs of the built-in templates (valid after hive_lifecycle_init_registry()). */
hive_lifecycle_id_t hive_lifecycle_builtin_worker(void);
hive_lifecycle_id_t hive_lifecycle_builtin_queen(void);
hive_lifecycle_id_t hive_lifecycle_builtin_drone(void);

/**
 * Return the expected role for an agent at @age_ticks under @tmpl.
 * Pure function. Returns HIVE_ROLE_EMPTY if tmpl is NULL or empty.
 */
hive_agent_role_t hive_role_for_age(uint32_t age_ticks,
                                    const hive_lifecycle_template_t *tmpl);

/* ----------------------------------------------------------------
 * API
 * ---------------------------------------------------------------- */

/** Initialize dynamics with a simulated hive population. */
void hive_dynamics_init(hive_dynamics_t *d, size_t agent_count);

/** Advance one simulation tick (age, role transitions, signal decay). */
void hive_dynamics_tick(hive_dynamics_t *d);

/** Recompute aggregate stats from the agent array. */
void hive_dynamics_recompute_stats(hive_dynamics_t *d);

/** Role → short display name. */
const char *hive_role_to_string(hive_agent_role_t role);

/** Role → single-char badge (Q, C, N, B, G, F, D). */
char hive_role_to_badge(hive_agent_role_t role);

/** Signal type → display name. */
const char *hive_signal_type_to_string(hive_signal_type_t type);

/* ----------------------------------------------------------------
 * Vitality / Queen accessor helpers
 * ---------------------------------------------------------------- */

/** True when the colony's vitality signal is strong enough for workers to run. */
static inline bool hive_vitality_ok(const hive_dynamics_t *d)
{
    if (d == NULL) return false;
    uint32_t min = d->cfg_vitality_min ? d->cfg_vitality_min : HIVE_VITALITY_MIN;
    return d->queen_alive && d->vitality_checksum >= min;
}

#ifdef __cplusplus
}
#endif

#endif /* HIVE_CORE_DYNAMICS_H */
