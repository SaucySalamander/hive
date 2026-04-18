#include "core/dynamics.h"

#include <stdlib.h>
#include <string.h>
#include <time.h>

/* ----------------------------------------------------------------
 * String helpers
 * ---------------------------------------------------------------- */

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

/* ----------------------------------------------------------------
 * Initialization — start with the queen only
 * ---------------------------------------------------------------- */

void hive_dynamics_init(hive_dynamics_t *d, size_t agent_count)
{
    if (d == NULL) return;

    memset(d, 0, sizeof(*d));

    if (agent_count == 0) {
        d->agent_count = 0;
        return;
    }

    if (agent_count > HIVE_DYNAMICS_MAX_AGENTS)
        agent_count = HIVE_DYNAMICS_MAX_AGENTS;

    d->agent_count = 1U;

    d->agents[0].role = HIVE_ROLE_QUEEN;
    d->agents[0].age_ticks = 0U;
    d->agents[0].perf_score = 100U;
    d->agents[0].signal_count = 0U;

    hive_dynamics_recompute_stats(d);
}

/* ----------------------------------------------------------------
 * Tick — simulate one step of the hive
 * ---------------------------------------------------------------- */

void hive_dynamics_tick(hive_dynamics_t *d)
{
    if (d == NULL) return;

    unsigned seed = (unsigned)time(NULL) ^ (unsigned)d->stats.total_signals;

    for (size_t i = 0; i < d->agent_count; ++i) {
        hive_agent_cell_t *a = &d->agents[i];
        if (a->role == HIVE_ROLE_EMPTY) continue;

        a->age_ticks++;

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

        /* Performance drift */
        seed = seed * 1103515245U + 12345U;
        int delta = ((int)((seed >> 16) % 7)) - 3;
        int new_score = (int)a->perf_score + delta;
        if (new_score < 0) new_score = 0;
        if (new_score > 100) new_score = 100;
        a->perf_score = (uint32_t)new_score;
    }

    /* Global signal activity */
    d->stats.total_signals++;
    seed = seed * 1103515245U + 12345U;
    d->stats.active_pheromones = 5 + (seed >> 16) % 20;
    seed = seed * 1103515245U + 12345U;
    d->stats.active_waggles = (seed >> 16) % 8;
    seed = seed * 1103515245U + 12345U;
    d->stats.active_alarms = (seed >> 16) % 3;
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
}
