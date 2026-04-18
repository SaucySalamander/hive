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
 * Per-agent cell state
 * ---------------------------------------------------------------- */
typedef struct hive_agent_cell {
    hive_agent_role_t role;
    uint32_t age_ticks;
    uint32_t perf_score;     /* 0–100 */
    uint32_t signal_count;   /* recent signals emitted */
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
} hive_dynamics_stats_t;

/* ----------------------------------------------------------------
 * Hive dynamics state
 * ---------------------------------------------------------------- */
#define HIVE_DYNAMICS_MAX_AGENTS 256

typedef struct hive_dynamics {
    hive_agent_cell_t agents[HIVE_DYNAMICS_MAX_AGENTS];
    size_t agent_count;
    hive_dynamics_stats_t stats;
} hive_dynamics_t;

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

#ifdef __cplusplus
}
#endif

#endif /* HIVE_CORE_DYNAMICS_H */
