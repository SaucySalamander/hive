#ifndef HIVE_CORE_QUEEN_H
#define HIVE_CORE_QUEEN_H

/*
 * Queen module — lifecycle anchor and exclusive spawning factory.
 *
 * The Queen is the sole agent that may add new workers to the colony.
 * She maintains the vitality pheromone (Queen Substance) which workers
 * sniff each tick to decide whether to execute.  If her vitality falls
 * below HIVE_REQUEUE_THRESHOLD the highest-performing worker is promoted
 * through the re-queening (Royal Jelly) protocol.
 */

#include "core/dynamics/dynamics.h"
#include "common/logging/logger.h"

#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ----------------------------------------------------------------
 * Vitality pheromone management
 * ---------------------------------------------------------------- */

/**
 * Emit a Queen Substance tick.
 *
 * Recalculates d->vitality_checksum from the rolling average of worker
 * perf_score values.  Should be called once per simulation tick by the
 * Queen before workers check conditioned_ok.
 *
 * @param d  Colony dynamics state.
 */
void hive_queen_emit_pheromone(hive_dynamics_t *d);

/* ----------------------------------------------------------------
 * Exclusive spawning factory
 * ---------------------------------------------------------------- */

/**
 * Spawn a new agent in the colony on behalf of the Queen.
 *
 * This is the ONLY sanctioned path for adding agents to d->agents[].
 * Calling code outside this function must not write to d->agents[] or
 * increment d->agent_count directly.
 *
 * Returns the new agent's index (0-based), or SIZE_MAX on failure
 * (colony at capacity, or Queen is dead and re-queening is pending).
 *
 * @param d            Colony dynamics state.
 * @param initial_role Starting role for the new agent.
 * @param lifecycle_id Lifecycle template (use HIVE_LIFECYCLE_NONE for none).
 * @param traits       Per-agent specialisation flags set at birth.
 */
size_t hive_queen_spawn(hive_dynamics_t        *d,
                        hive_agent_role_t       initial_role,
                        hive_lifecycle_id_t     lifecycle_id,
                        hive_agent_traits_t     traits);

/* ----------------------------------------------------------------
 * Grooming feedback channel
 * ---------------------------------------------------------------- */

/**
 * Receive a grooming metadata packet from a worker.
 *
 * Each worker should call this after completing a task.  The packet
 * contributes to the Queen's rolling vitality calculation and may
 * raise an alarm signal visible in the colony stats.
 *
 * @param d   Colony dynamics state.
 * @param pkt Grooming packet produced by the worker.
 */
void hive_queen_receive_groom(hive_dynamics_t           *d,
                              const hive_groom_packet_t *pkt,
                              hive_logger_t             *log);

/* ----------------------------------------------------------------
 * Demand-driven population regulation
 * ---------------------------------------------------------------- */

/**
 * Regulate colony population based on the current demand buffer.
 *
 * Spawns workers while:
 *   d->demand_buffer_depth > active_workers × HIVE_SPAWN_DEMAND_RATIO
 * and the colony is below HIVE_DYNAMICS_MAX_AGENTS.
 *
 * Automatically called from hive_dynamics_tick().
 *
 * @param d  Colony dynamics state.
 */
void hive_queen_regulate_population(hive_dynamics_t *d);

/* ----------------------------------------------------------------
 * Re-queening (Royal Jelly) protocol
 * ---------------------------------------------------------------- */

/**
 * Evaluate whether re-queening is needed and execute if so.
 *
 * If the current Queen's perf_score < HIVE_REQUEUE_THRESHOLD, the
 * worker with the highest perf_score is promoted to Queen:
 *   - Its role is set to HIVE_ROLE_QUEEN and lifecycle to the built-in
 *     queen template.
 *   - Its lineage_hash inherits from the old Queen with the generation
 *     counter incremented.
 *   - d->queen_idx is updated and d->lineage_generation is bumped.
 *
 * Returns true if a re-queening event occurred.
 *
 * @param d  Colony dynamics state.
 */
bool hive_queen_requeue_if_needed(hive_dynamics_t *d);

#ifdef __cplusplus
}
#endif

#endif /* HIVE_CORE_QUEEN_H */
