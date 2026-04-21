/*
 * OPTION 3 IMPLEMENTATION — WORKER-CELL MAPPING
 * scheduler.h — per-worker pipeline scheduler.
 *
 * The scheduler owns the dynamics simulation and dispatches pipeline stages to
 * individual worker cells based on their pheromone-gated eligibility.  Each
 * active hive_agent_cell_t in hive_dynamics_t.agents[] is permanently bound
 * (1:1) to one hive_agent_t descriptor.  The scheduler maintains a parallel
 * hive_agent_context_t array indexed by cell slot to hold each worker's
 * private pipeline state.
 *
 * Invariants:
 *   - contexts[i] is meaningful only when dynamics->agents[i].role != EMPTY
 *     and dynamics->agents[i].bound_agent != NULL.
 *   - Only the Queen cell (dynamics->queen_idx) may write the canonical output
 *     back into runtime->session when its pipeline cycle completes.
 *   - The scheduler never dispatches to a cell where conditioned_ok == false.
 *   - hive_agent_cell_t.bound_agent is allocated by hive_queen_spawn() and
 *     freed by hive_scheduler_deinit() or retire_cell() (on role → EMPTY).
 */

#ifndef HIVE_CORE_SCHEDULER_H
#define HIVE_CORE_SCHEDULER_H

#include "core/dynamics/dynamics.h"
#include "core/agent/agent.h"
#include "core/types.h"
#include "core/scheduler/hive_config.h"

#if !HIVE_SINGLE_THREADED
#  include <pthread.h>
#endif

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declaration — full type in core/runtime.h. */
typedef struct hive_runtime hive_runtime_t;

/* ----------------------------------------------------------------
 * Per-worker pipeline stage enum
 * ---------------------------------------------------------------- */

/**
 * The current stage of one worker cell's independent hierarchical pipeline.
 * Mirrors hive_state_t from state_machine.h but is private to each cell.
 */
typedef enum hive_agent_stage {
    HIVE_AGENT_STAGE_ORCHESTRATE = 0,
    HIVE_AGENT_STAGE_PLAN,
    HIVE_AGENT_STAGE_CODE,
    HIVE_AGENT_STAGE_TEST,
    HIVE_AGENT_STAGE_VERIFY,
    HIVE_AGENT_STAGE_EDIT,
    HIVE_AGENT_STAGE_EVALUATE,
    HIVE_AGENT_STAGE_DONE,   /* cycle finished; will be reset to ORCHESTRATE */
    HIVE_AGENT_STAGE_COUNT
} hive_agent_stage_t;

/* ----------------------------------------------------------------
 * Per-worker mutable context (private pipeline state)
 * ---------------------------------------------------------------- */

/**
 * All mutable output accumulated by one worker cell as it advances through the
 * ORCHESTRATE→EVALUATE pipeline cycle.  Owned exclusively by the scheduler
 * (hive_scheduler_t.contexts[]).  Freed when the cell is retired (role →
 * EMPTY or DRONE) via retire_cell().
 */
typedef struct hive_agent_context {
    hive_agent_stage_t  current_stage;
    unsigned            iteration;          /* refinement pass within one cycle   */
    unsigned            cycle_count;        /* total completed cycles this cell ran */
    char               *orchestrator_brief;
    char               *plan;
    char               *code;
    char               *tests;
    char               *verification;
    char               *edit;
    char               *final_output;
    char               *last_critique;
    hive_score_t        last_score;
    bool                in_use;             /* true while this slot is active     */

    /* Per-iteration score history: ring buffer of the last 4 evaluation scores. */
    hive_score_t        score_history[4];
    unsigned            score_history_count; /* total evals stored (wraps at 4)   */
} hive_agent_context_t;

/* ----------------------------------------------------------------
 * Scheduler runtime options
 * ---------------------------------------------------------------- */

typedef struct hive_scheduler_options {
    unsigned max_concurrent_workers;  /* 0 → HIVE_SCHEDULER_MAX_CONCURRENT_WORKERS */
    unsigned backoff_ms_suppressed;   /* 0 → HIVE_SCHEDULER_BACKOFF_MS_SUPPRESSED   */
    unsigned min_perf_score;          /* 0 → HIVE_SCHEDULER_MIN_PERF_SCORE          */
    unsigned score_threshold;         /* overall score floor to accept a cycle; 0→72 */
    unsigned iteration_limit;         /* max refinement iterations per cycle; 0→3    */
    uint8_t  binding_mode;            /* HIVE_BINDING_MODE_1_TO_1 (default)          */
} hive_scheduler_options_t;

/* ----------------------------------------------------------------
 * Scheduler state
 * ---------------------------------------------------------------- */

typedef struct hive_scheduler {
    hive_scheduler_options_t opts;

    /* Colony dynamics — borrowed pointer; owned by hive_runtime_t. */
    hive_dynamics_t *dynamics;

    /* Per-cell pipeline contexts indexed by cell slot [0..MAX_AGENTS-1]. */
    hive_agent_context_t contexts[HIVE_DYNAMICS_MAX_AGENTS];

    /* Cell roles observed in the previous tick; used to detect retirement. */
    hive_agent_role_t last_seen_role[HIVE_DYNAMICS_MAX_AGENTS];

    /* ---- Observability metrics (pushed to dynamics->stats each iteration) ---- */
    unsigned active_bound_workers;         /* cells with bound_agent != NULL       */
    unsigned suppressed_workers;           /* cells where conditioned_ok == false  */
    uint64_t average_pheromone_latency_ns; /* stub; set to 0 until backend exposes */
    unsigned requeen_events_this_run;      /* re-queening events since init        */

#if !HIVE_SINGLE_THREADED
    /**
     * Reader–writer lock protecting all reads/writes to dynamics->agents[].
     * In v1 the scheduler is single-threaded but the lock is present so that
     * future reader threads (TUI, metrics) can be added safely.
     */
    pthread_rwlock_t dynamics_lock;
    bool             lock_initialized;
#endif
} hive_scheduler_t;

/* ----------------------------------------------------------------
 * Public API
 * ---------------------------------------------------------------- */

/**
 * Initialize the scheduler.  Borrows a pointer to @dynamics (owned by the
 * caller).  Does NOT tick dynamics; call hive_dynamics_init() first.
 *
 * For every active cell in dynamics->agents[] that has no bound_agent, this
 * function allocates and binds an appropriate hive_agent_t descriptor using
 * hive_agent_descriptor_for_role().
 *
 * @param sched    Scheduler state to initialize.
 * @param dynamics Initialized colony dynamics.
 * @param opts     Scheduler options; pass NULL for all defaults.
 * @return HIVE_STATUS_OK on success.
 */
hive_status_t hive_scheduler_init(hive_scheduler_t              *sched,
                                  hive_dynamics_t               *dynamics,
                                  const hive_scheduler_options_t *opts);

/**
 * Destroy all scheduler state.  Frees per-worker contexts and the agent
 * descriptors bound to each cell.  Does NOT destroy dynamics (owned by caller).
 *
 * @param sched Scheduler state to destroy.
 */
void hive_scheduler_deinit(hive_scheduler_t *sched);

/**
 * Run the scheduler until the Queen cell completes a full pipeline cycle or
 * an unrecoverable error occurs.
 *
 * Per-iteration sequence:
 *   1. hive_dynamics_tick() — pheromone, re-queening, spawning, ageing.
 *   2. Retire cells whose role transitioned to EMPTY since the last tick.
 *   3. Activate contexts for newly-spawned cells.
 *   4. Update observability metrics.
 *   5. Build the eligible worker list (conditioned_ok, bound_agent, perf).
 *   6. Dispatch one pipeline stage per eligible cell (highest perf first).
 *   7. For each dispatch: build a grooming packet, call
 *      hive_queen_receive_groom(), handle cycle completion.
 *   8. If no eligible workers: log suppression, back off, continue.
 *
 * When the Queen cell's pipeline reaches HIVE_AGENT_STAGE_DONE, its
 * accumulated context is promoted into runtime->session (global project state)
 * and the function returns HIVE_STATUS_OK.
 *
 * @param sched   Initialized scheduler.
 * @param runtime Full runtime (session, logger, adapter, tools).
 * @return HIVE_STATUS_OK on success.
 */
hive_status_t hive_scheduler_run(hive_scheduler_t *sched,
                                 hive_runtime_t   *runtime);

/**
 * Map a colony role to the appropriate agent descriptor for that role.
 *
 * Role → Agent mapping:
 *   QUEEN    → Orchestrator
 *   CLEANER  → Planner
 *   NURSE    → Planner
 *   BUILDER  → Coder
 *   GUARD    → Verifier
 *   FORAGER  → Tester
 *   DRONE    → NULL (drones do not execute agent stages)
 *   EMPTY    → NULL
 *
 * @param role Colony cell role.
 * @return Pointer to a static agent descriptor, or NULL if no mapping exists.
 */
const hive_agent_t *hive_agent_descriptor_for_role(hive_agent_role_t role);

/**
 * Return a human-readable name for a pipeline stage.
 *
 * @param stage Stage enum value.
 * @return Stable string; never NULL.
 */
const char *hive_agent_stage_to_string(hive_agent_stage_t stage);

#ifdef __cplusplus
}
#endif

#endif /* HIVE_CORE_SCHEDULER_H */
