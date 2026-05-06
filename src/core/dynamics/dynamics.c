#include "core/dynamics/dynamics.h"
#include "core/queen/queen.h"
/*
 * OPTION 3 IMPLEMENTATION — WORKER-CELL MAPPING
 * Include agent.h so that hive_dynamics_deinit() can call hive_agent_free()
 * to release bound_agent pointers allocated by hive_queen_spawn().
 */
#include "core/agent/agent.h"

#include <stdlib.h>
#include <string.h>
#include <time.h>

/* ================================================================
 * Lifecycle template registry
 * ================================================================ */

static hive_lifecycle_template_t g_lifecycle_registry[HIVE_LIFECYCLE_MAX_TEMPLATES];
static size_t g_lifecycle_count = 0;

static hive_lifecycle_id_t g_builtin_worker_id = HIVE_LIFECYCLE_NONE;
static hive_lifecycle_id_t g_builtin_queen_id  = HIVE_LIFECYCLE_NONE;
static hive_lifecycle_id_t g_builtin_drone_id  = HIVE_LIFECYCLE_NONE;

hive_lifecycle_id_t hive_lifecycle_register(const hive_lifecycle_template_t *tmpl)
{
    if (tmpl == NULL || g_lifecycle_count >= HIVE_LIFECYCLE_MAX_TEMPLATES)
        return HIVE_LIFECYCLE_NONE;
    g_lifecycle_registry[g_lifecycle_count] = *tmpl;
    g_lifecycle_count++;
    return (hive_lifecycle_id_t)g_lifecycle_count; /* 1-based */
}

const hive_lifecycle_template_t *hive_lifecycle_get(hive_lifecycle_id_t id)
{
    if (id == HIVE_LIFECYCLE_NONE || id > (hive_lifecycle_id_t)g_lifecycle_count)
        return NULL;
    return &g_lifecycle_registry[id - 1];
}

size_t hive_lifecycle_count(void)      { return g_lifecycle_count; }
hive_lifecycle_id_t hive_lifecycle_builtin_worker(void) { return g_builtin_worker_id; }
hive_lifecycle_id_t hive_lifecycle_builtin_queen(void)  { return g_builtin_queen_id; }
hive_lifecycle_id_t hive_lifecycle_builtin_drone(void)  { return g_builtin_drone_id; }

hive_agent_role_t hive_role_for_age(uint32_t age_ticks,
                                    const hive_lifecycle_template_t *tmpl)
{
    if (tmpl == NULL || tmpl->stage_count == 0) return HIVE_ROLE_EMPTY;
    hive_agent_role_t role = tmpl->stages[0].role;
    for (size_t i = 1; i < tmpl->stage_count; ++i) {
        if (tmpl->stages[i].start_tick <= age_ticks)
            role = tmpl->stages[i].role;
        else
            break;
    }
    return role;
}

void hive_lifecycle_init_registry(void)
{
    g_lifecycle_count = 0;

    /* ---- Worker template (temporal polyethism, bee-biology timings) ---- */
    hive_lifecycle_template_t worker = {
        .name        = "Worker",
        .description = "Temporal polyethism: cleaner → nurse → builder → guard → forager. "
                       "Mirrors the age-based role progression of a honey-bee worker.",
        .stage_count = 5,
        .stages = {
            { .start_tick =  0, .role = HIVE_ROLE_CLEANER, .firmness = 100 },
            { .start_tick =  4, .role = HIVE_ROLE_NURSE,   .firmness = 100 },
            { .start_tick = 13, .role = HIVE_ROLE_BUILDER, .firmness =  80 },
            { .start_tick = 19, .role = HIVE_ROLE_GUARD,   .firmness =  60 },
            { .start_tick = 22, .role = HIVE_ROLE_FORAGER, .firmness = 100 },
        }
    };
    g_builtin_worker_id = hive_lifecycle_register(&worker);

    /* ---- Queen template (remains queen; no transitions) ---- */
    hive_lifecycle_template_t queen = {
        .name        = "Queen",
        .description = "Stays in the Queen role for life, maintaining chemical control "
                       "and directing hive activity through pheromone signals.",
        .stage_count = 1,
        .stages = {
            { .start_tick = 0, .role = HIVE_ROLE_QUEEN, .firmness = 100 },
        }
    };
    g_builtin_queen_id = hive_lifecycle_register(&queen);

    /* ---- Drone template (born a drone; short lifecycle) ---- */
    hive_lifecycle_template_t drone = {
        .name        = "Drone",
        .description = "Born as a drone with a single purpose: mating flight. "
                       "Does not progress through worker stages.",
        .stage_count = 1,
        .stages = {
            { .start_tick = 0, .role = HIVE_ROLE_DRONE, .firmness = 100 },
        }
    };
    g_builtin_drone_id = hive_lifecycle_register(&drone);
}


/* ================================================================
 * String helpers
 * ================================================================ */

const char *hive_role_to_string(hive_agent_role_t role)
{
    switch (role) {
    case HIVE_ROLE_EMPTY:   return "Empty";
    case HIVE_ROLE_QUEEN:   return "Queen";
    case HIVE_ROLE_CLEANER: return "Cleaner";
    case HIVE_ROLE_NURSE:   return "Nurse";
    case HIVE_ROLE_BUILDER: return "Builder";
    case HIVE_ROLE_GUARD:   return "Guard";
    case HIVE_ROLE_FORAGER: return "Forager";
    case HIVE_ROLE_DRONE:   return "Drone";
    default:                return "Unknown";
    }
}

char hive_role_to_badge(hive_agent_role_t role)
{
    switch (role) {
    case HIVE_ROLE_QUEEN:   return 'Q';
    case HIVE_ROLE_CLEANER: return 'C';
    case HIVE_ROLE_NURSE:   return 'N';
    case HIVE_ROLE_BUILDER: return 'B';
    case HIVE_ROLE_GUARD:   return 'G';
    case HIVE_ROLE_FORAGER: return 'F';
    case HIVE_ROLE_DRONE:   return 'D';
    default:                return ' ';
    }
}

const char *hive_signal_type_to_string(hive_signal_type_t type)
{
    switch (type) {
    case HIVE_SIGNAL_PHEROMONE: return "Pheromone";
    case HIVE_SIGNAL_WAGGLE:   return "Waggle Dance";
    case HIVE_SIGNAL_ALARM:    return "Alarm";
    case HIVE_SIGNAL_PROPOSAL: return "Proposal";
    default:                   return "Unknown";
    }
}

/* ================================================================
 * Initialization — start with the queen only
 * ================================================================ */

void hive_dynamics_init(hive_dynamics_t *d, size_t agent_count)
{
    if (d == NULL) return;

    /* Initialise the lifecycle registry (idempotent on repeated calls) */
    if (g_lifecycle_count == 0)
        hive_lifecycle_init_registry();

    memset(d, 0, sizeof(*d));

    if (agent_count == 0) {
        d->agent_count = 0;
        return;
    }

    if (agent_count > HIVE_DYNAMICS_MAX_AGENTS)
        agent_count = HIVE_DYNAMICS_MAX_AGENTS;

    d->agent_count = 1U;

    d->agents[0].role         = HIVE_ROLE_QUEEN;
    d->agents[0].age_ticks    = 0U;
    d->agents[0].perf_score   = 100U;
    d->agents[0].signal_count = 0U;
    d->agents[0].lifecycle_id = g_builtin_queen_id;
    /* Queen starts with identity traits and full vitality. */
    d->agents[0].traits = (hive_agent_traits_t){
        .temperature_pct  = 50U,
        .lineage_hash     = 1U,
        .specialization   = 0xFFU,  /* Queen can direct all task types */
        .resource_cap_pct = 100U,
    };
    d->agents[0].vitality_seen  = 100U;
    d->agents[0].conditioned_ok = true;

    /* Initialise colony vitality state. */
    d->vitality_checksum   = 100U;
    d->demand_buffer_depth = 0U;
    d->queen_idx           = 0U;
    d->queen_alive         = true;
    d->lineage_generation  = 1U;

    /* Runtime-tunable thresholds — seeds from compile-time defaults. */
    d->cfg_vitality_min        = HIVE_VITALITY_MIN;
    d->cfg_spawn_demand_ratio  = HIVE_SPAWN_DEMAND_RATIO;
    d->cfg_requeue_threshold   = HIVE_REQUEUE_THRESHOLD;

    /* OPTION 3: zero run-level stats so the scheduler gets a clean slate. */
    d->stats.requeen_events_this_run = 0U;
    d->stats.active_bound_workers    = 0U;
    d->stats.suppressed_workers      = 0U;
    d->stats.average_pheromone_latency_ns = 0U;

    hive_dynamics_recompute_stats(d);
}

/* ================================================================
 * Tick — simulate one step of the hive
 * ================================================================ */

void hive_dynamics_tick(hive_dynamics_t *d)
{
    if (d == NULL) return;

    /* 1. Queen emits pheromone — must happen before workers check conditioned_ok. */
    hive_queen_emit_pheromone(d);

    /* 2. Queen emits waggle dance signal when work arrives. */
    hive_queen_emit_waggle(d);

    /* 3. Re-queening check — promote best worker if Queen is weak. */
    hive_queen_requeue_if_needed(d);

    unsigned seed = (unsigned)time(NULL) ^ (unsigned)d->stats.total_signals;

    for (size_t i = 0; i < d->agent_count; ++i) {
        hive_agent_cell_t *a = &d->agents[i];
        if (a->role == HIVE_ROLE_EMPTY) continue;

        a->age_ticks++;

        /* Conditioned execution gate: workers stand by if vitality signal fails. */
        if (!a->conditioned_ok && a->role != HIVE_ROLE_QUEEN) {
            /* Worker is in standby — emit an alarm and skip task behaviour. */
            d->stats.active_alarms =
                d->stats.active_alarms < 255U
                    ? d->stats.active_alarms + 1U
                    : 255U;
            continue;
        }

        /* Foragers accumulate signals faster */
        if (a->role == HIVE_ROLE_FORAGER) {
            seed = seed * 1103515245U + 12345U;
            if (((seed >> 16) % 100) < 30)
                a->signal_count++;
        }

        /* Guards respond to alarms */
        if (a->role == HIVE_ROLE_GUARD) {
            seed = seed * 1103515245U + 12345U;
            if (((seed >> 16) % 100) < 10)
                a->signal_count++;
        }

        if (a->lifecycle_id != HIVE_LIFECYCLE_NONE) {
            /* ---- Template-driven transitions ---- */
            const hive_lifecycle_template_t *tmpl = hive_lifecycle_get(a->lifecycle_id);
            if (tmpl != NULL && tmpl->stage_count > 0) {
                /* Find the active stage (last stage whose start_tick <= age_ticks) */
                const hive_lifecycle_stage_t *active = &tmpl->stages[0];
                for (size_t s = 1; s < tmpl->stage_count; ++s) {
                    if (tmpl->stages[s].start_tick <= a->age_ticks)
                        active = &tmpl->stages[s];
                    else
                        break;
                }
                if (active->role != HIVE_ROLE_EMPTY && active->role != a->role) {
                    if (active->firmness >= 100) {
                        a->role = active->role;
                    } else {
                        seed = seed * 1103515245U + 12345U;
                        /* perf_score can nudge the chance by up to +10 pp */
                        uint32_t chance = active->firmness + (a->perf_score / 10);
                        if (chance > 100) chance = 100;
                        if (((seed >> 16) % 100) < chance)
                            a->role = active->role;
                    }
                }
            }
        } else {
            /* ---- Legacy stochastic transitions (no template) ---- */

            /* Age-based transitions: cleaners → nurses after ~20 ticks */
            if (a->role == HIVE_ROLE_CLEANER && a->age_ticks > 20) {
                seed = seed * 1103515245U + 12345U;
                if (((seed >> 16) % 100) < 15)
                    a->role = HIVE_ROLE_NURSE;
            }

            /* Nurses → builders after ~50 ticks */
            if (a->role == HIVE_ROLE_NURSE && a->age_ticks > 50) {
                seed = seed * 1103515245U + 12345U;
                if (((seed >> 16) % 100) < 10)
                    a->role = HIVE_ROLE_BUILDER;
            }
        }

        /* Performance drift (all conditioned agents) */
        seed = seed * 1103515245U + 12345U;
        int delta = ((int)((seed >> 16) % 7)) - 3;
        int new_score = (int)a->perf_score + delta;
        if (new_score < 0) new_score = 0;
        if (new_score > 100) new_score = 100;
        a->perf_score = (uint32_t)new_score;
    }

    /* 4. Demand-driven spawning — spawn workers if backlog pressure is high. */
    hive_queen_regulate_population(d);

    /* Global signal activity */
    d->stats.total_signals++;
    seed = seed * 1103515245U + 12345U;
    /* Pheromones are now managed by queen, but keep minimum noise floor. */
    if (d->stats.active_pheromones < 5U) {
        d->stats.active_pheromones = 5U + (seed >> 16) % 10U;
    }
    /* Waggle signals are now emitted by queen; no need for random values here */
    seed = seed * 1103515245U + 12345U;
    /* Natural decay: alarms reduce each tick + waggle duration countdown */
    if (d->stats.active_alarms > 0U)
        d->stats.active_alarms = (d->stats.active_alarms > 1U)
            ? d->stats.active_alarms - 1U : 0U;
    /* Waggle duration decays naturally when no new demand */
    if (d->demand_buffer_depth == 0U && d->stats.waggle_duration > 0U)
        d->stats.waggle_duration--;
    seed = seed * 1103515245U + 12345U;
    d->stats.pending_proposals = (seed >> 16) % 4;
    seed = seed * 1103515245U + 12345U;
    d->stats.quorum_votes = (seed >> 16) % 15;
    d->stats.quorum_threshold = 10;

    hive_dynamics_recompute_stats(d);
}

/* ----------------------------------------------------------------
 * Recompute stats
 * ---------------------------------------------------------------- */

void hive_dynamics_recompute_stats(hive_dynamics_t *d)
{
    if (d == NULL) return;

    memset(d->stats.role_counts, 0, sizeof(d->stats.role_counts));
    d->stats.total_agents = 0;

    for (size_t i = 0; i < d->agent_count; ++i) {
        hive_agent_role_t r = d->agents[i].role;
        if (r > HIVE_ROLE_EMPTY && r < HIVE_ROLE_COUNT) {
            d->stats.role_counts[r]++;
            d->stats.total_agents++;
        }
    }

    /* Mirror demand buffer into stats so the UI can surface it. */
    d->stats.backlog_depth = d->demand_buffer_depth;
}

/* ================================================================
 * hive_dynamics_deinit
 * OPTION 3 IMPLEMENTATION — WORKER-CELL MAPPING
 *
 * Frees all bound_agent pointers in agents[] allocated by
 * hive_queen_spawn() via hive_agent_clone_descriptor(), then zeroes the
 * struct.  Must be called before the struct is discarded when Option 3 is
 * active; safe to call on a zeroed or freshly initialised struct.
 * ================================================================ */

void hive_dynamics_deinit(hive_dynamics_t *d)
{
    if (d == NULL) return;

    for (size_t i = 0; i < d->agent_count; ++i) {
        if (d->agents[i].bound_agent != NULL) {
            hive_agent_free(d->agents[i].bound_agent);
            d->agents[i].bound_agent = NULL;
        }
    }

    memset(d, 0, sizeof(*d));
}
