#include "core/queen/queen.h"

#include <string.h>

/* ================================================================
 * Internal helpers
 * ================================================================ */

/** Return the number of active (non-empty) workers (non-Queen) in the colony. */
static unsigned count_active_workers(const hive_dynamics_t *d)
{
    unsigned count = 0U;
    for (size_t i = 0; i < d->agent_count; ++i) {
        const hive_agent_cell_t *a = &d->agents[i];
        if (a->role != HIVE_ROLE_EMPTY && a->role != HIVE_ROLE_QUEEN)
            count++;
    }
    return count;
}

/** Return the index of the non-Queen agent with the highest perf_score,
 *  or SIZE_MAX if there are no eligible workers. */
static size_t best_worker_idx(const hive_dynamics_t *d)
{
    size_t   best_i     = SIZE_MAX;
    uint32_t best_score = 0U;

    for (size_t i = 0; i < d->agent_count; ++i) {
        const hive_agent_cell_t *a = &d->agents[i];
        if (a->role == HIVE_ROLE_EMPTY || a->role == HIVE_ROLE_QUEEN)
            continue;
        if (a->perf_score > best_score) {
            best_score = a->perf_score;
            best_i     = i;
        }
    }
    return best_i;
}

/* ================================================================
 * hive_queen_emit_pheromone
 * ================================================================ */

void hive_queen_emit_pheromone(hive_dynamics_t *d)
{
    if (d == NULL || !d->queen_alive) return;

    /* Compute the rolling mean perf_score across all active agents. */
    uint64_t total = 0U;
    unsigned count = 0U;

    for (size_t i = 0; i < d->agent_count; ++i) {
        const hive_agent_cell_t *a = &d->agents[i];
        if (a->role != HIVE_ROLE_EMPTY) {
            total += a->perf_score;
            count++;
        }
    }

    uint32_t mean = (count > 0U) ? (uint32_t)(total / count) : 0U;

    /* Blend 80 % old checksum + 20 % new mean for smooth decay/recovery. */
    d->vitality_checksum = (d->vitality_checksum * 80U + mean * 20U) / 100U;

    /* Hard floor / ceiling */
    if (d->vitality_checksum > 100U) d->vitality_checksum = 100U;

    /* Propagate into each worker's vitality_seen and gate conditioned_ok. */
    for (size_t i = 0; i < d->agent_count; ++i) {
        hive_agent_cell_t *a = &d->agents[i];
        if (a->role == HIVE_ROLE_EMPTY) continue;
        a->vitality_seen  = (uint8_t)(d->vitality_checksum > 255U
                                       ? 255U : d->vitality_checksum);
        a->conditioned_ok = hive_vitality_ok(d);
    }
}

/* ================================================================
 * hive_queen_spawn
 * ================================================================ */

size_t hive_queen_spawn(hive_dynamics_t     *d,
                        hive_agent_role_t    initial_role,
                        hive_lifecycle_id_t  lifecycle_id,
                        hive_agent_traits_t  traits)
{
    if (d == NULL) return SIZE_MAX;

    /* Sub-spawning suppression: only allow when the Queen is alive. */
    if (!d->queen_alive) return SIZE_MAX;

    if (d->agent_count >= HIVE_DYNAMICS_MAX_AGENTS) return SIZE_MAX;

    size_t idx = d->agent_count;
    hive_agent_cell_t *cell = &d->agents[idx];
    memset(cell, 0, sizeof(*cell));

    cell->role          = initial_role;
    cell->age_ticks     = 0U;
    cell->perf_score    = 50U;  /* start at mid-range */
    cell->signal_count  = 1U;
    cell->lifecycle_id  = lifecycle_id;
    cell->traits        = traits;
    cell->vitality_seen = (uint8_t)(d->vitality_checksum > 255U
                                     ? 255U : d->vitality_checksum);
    cell->conditioned_ok = hive_vitality_ok(d);

    d->agent_count++;
    d->stats.worker_spawns++;
    d->stats.total_signals  += 2U;
    d->stats.active_pheromones++;
    d->stats.active_waggles++;
    d->stats.quorum_votes = d->stats.worker_spawns;
    d->stats.quorum_threshold = d->stats.backlog_depth > 0U
        ? d->stats.backlog_depth
        : 1U;

    return idx;
}

/* ================================================================
 * hive_queen_receive_groom
 * ================================================================ */

void hive_queen_receive_groom(hive_dynamics_t           *d,
                              const hive_groom_packet_t *pkt)
{
    if (d == NULL || pkt == NULL) return;
    if (pkt->agent_idx >= d->agent_count) return;

    hive_agent_cell_t *worker = &d->agents[pkt->agent_idx];

    /* Update per-worker performance from grooming feedback. */
    uint32_t prev = worker->perf_score;
    uint32_t next = (prev * 7U + pkt->success * 3U) / 10U;   /* EMA 70/30 */
    if (next > 100U) next = 100U;
    worker->perf_score = next;

    /* Alarm: raise the colony alarm counter when a worker signals trouble. */
    if (pkt->alarm_flag) {
        d->stats.active_alarms =
            d->stats.active_alarms < 255U
                ? d->stats.active_alarms + 1U
                : 255U;
    }

    /* Each grooming packet also emits a small pheromone burst. */
    d->stats.active_pheromones++;
    d->stats.total_signals++;
}

/* ================================================================
 * hive_queen_regulate_population
 * ================================================================ */

void hive_queen_regulate_population(hive_dynamics_t *d)
{
    if (d == NULL || !d->queen_alive) return;
    if (d->demand_buffer_depth == 0U) return;

    unsigned workers = count_active_workers(d);

    /* Spawn one worker per tick until ratio is satisfied or hive is full. */
    while (d->demand_buffer_depth > workers * HIVE_SPAWN_DEMAND_RATIO
           && d->agent_count < HIVE_DYNAMICS_MAX_AGENTS)
    {
        /* Default traits: balanced worker, inherits Queen's lineage. */
        hive_agent_traits_t traits = {
            .temperature_pct  = 50U,
            .lineage_hash     = d->lineage_generation,
            .specialization   = 0U,
            .resource_cap_pct = 80U,
        };
        size_t new_idx = hive_queen_spawn(d, HIVE_ROLE_CLEANER,
                                          hive_lifecycle_builtin_worker(),
                                          traits);
        if (new_idx == SIZE_MAX) break;   /* capacity reached */
        workers++;
    }
}

/* ================================================================
 * hive_queen_requeue_if_needed
 * ================================================================ */

bool hive_queen_requeue_if_needed(hive_dynamics_t *d)
{
    if (d == NULL) return false;

    /* Check current Queen's vitality. */
    if (d->queen_idx >= d->agent_count) {
        /* Queen slot is invalid — mark dead to force promotion. */
        d->queen_alive = false;
    }

    const hive_agent_cell_t *queen = &d->agents[d->queen_idx];
    if (d->queen_alive
        && queen->role == HIVE_ROLE_QUEEN
        && queen->perf_score >= HIVE_REQUEUE_THRESHOLD)
    {
        return false;   /* Queen is healthy; no action needed. */
    }

    /* Find the best worker to promote. */
    size_t candidate = best_worker_idx(d);
    if (candidate == SIZE_MAX) {
        /* No eligible workers; Queen stays in place even if weak. */
        d->queen_alive = true;
        return false;
    }

    /* Promote candidate to Queen. */
    hive_agent_cell_t *new_queen = &d->agents[candidate];
    uint32_t old_lineage = new_queen->traits.lineage_hash;

    new_queen->role         = HIVE_ROLE_QUEEN;
    new_queen->lifecycle_id = hive_lifecycle_builtin_queen();
    new_queen->age_ticks    = 0U;   /* reset age on coronation */

    /* Bump lineage generation and propagate. */
    d->lineage_generation++;
    new_queen->traits.lineage_hash = d->lineage_generation;
    (void)old_lineage;   /* retained for future audit use */

    /* Retire the old Queen cell if slot is valid. */
    if (d->queen_idx < d->agent_count
        && d->agents[d->queen_idx].role == HIVE_ROLE_QUEEN
        && d->queen_idx != candidate)
    {
        d->agents[d->queen_idx].role = HIVE_ROLE_EMPTY;
    }

    d->queen_idx   = candidate;
    d->queen_alive = true;

    /* Re-queening resets vitality partially — the colony needs to stabilise. */
    d->vitality_checksum = 50U;

    /* Emit a pheromone burst to notify workers. */
    d->stats.active_pheromones += 5U;
    d->stats.total_signals     += 5U;

    return true;
}
