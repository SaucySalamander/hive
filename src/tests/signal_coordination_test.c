/*
 * signal_coordination_test.c — Phase 2.5 Worker Coordination Signals
 *
 * Compile with:
 *   cc -DHIVE_BUILD_TESTS -Isrc std=c23 -Wall -Wextra -Werror \
 *       src/tests/signal_coordination_test.c \
 *       src/core/dynamics/dynamics.c \
 *       src/core/queen/queen.c \
 *       src/core/agent/agent.c \
 *       -o build/signal_test
 */

#ifdef HIVE_BUILD_TESTS

#include "core/dynamics/dynamics.h"
#include "core/queen/queen.h"
#include "core/scheduler/hive_config.h"
#include "common/logging/logger.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ================================================================
 * Mock implementations for test isolation
 * ================================================================ */

struct hive_agent {
    uint32_t dummy;
};

const struct hive_agent *hive_agent_descriptor_for_role(hive_agent_role_t role)
{
    (void)role;
    static struct hive_agent agent = {.dummy = 1};
    return &agent;
}

struct hive_agent *hive_agent_clone_descriptor(const struct hive_agent *desc)
{
    if (desc == NULL) return NULL;
    struct hive_agent *clone = malloc(sizeof(*clone));
    if (clone) *clone = *desc;
    return clone;
}

void hive_agent_free(struct hive_agent *agent)
{
    if (agent != NULL) free(agent);
}

void hive_logger_logf(hive_logger_t *logger,
                      hive_log_level_t level,
                      const char *component,
                      const char *event,
                      const char *format,
                      ...)
{
    (void)logger;
    (void)level;
    (void)component;
    (void)event;
    (void)format;
    /* Stub: no-op logging for tests */
}

/* ================================================================
 * Test 1: Waggle Dance Signal Emission
 *
 * Verify that waggle signals are emitted with correct strength
 * and decay naturally when demand clears.
 * ================================================================ */

static void test_waggle_emission(void)
{
    printf("  [1] Waggle dance signal emission ... ");
    fflush(stdout);

    hive_dynamics_t d;
    hive_dynamics_init(&d, 32);

    /* No demand: waggle should be silent */
    d.demand_buffer_depth = 0;
    hive_queen_emit_waggle(&d);
    assert(d.stats.waggle_strength == 0);

    /* High demand: waggle should be strong */
    d.demand_buffer_depth = 50;
    hive_queen_emit_waggle(&d);
    assert(d.stats.waggle_strength == 50);
    assert(d.stats.waggle_duration == 8);

    /* Very high demand: waggle capped at 100 */
    d.demand_buffer_depth = 200;
    hive_queen_emit_waggle(&d);
    assert(d.stats.waggle_strength == 100);

    hive_dynamics_deinit(&d);
    printf("PASS\n");
}

/* ================================================================
 * Test 2: Waggle Signal Decay
 *
 * Verify that waggle strength decays naturally when demand clears.
 * ================================================================ */

static void test_waggle_decay(void)
{
    printf("  [2] Waggle signal decay ... ");
    fflush(stdout);

    hive_dynamics_t d;
    hive_dynamics_init(&d, 32);

    /* Establish strong waggle */
    d.demand_buffer_depth = 50;
    hive_queen_emit_waggle(&d);
    uint32_t initial_strength = d.stats.waggle_strength;
    assert(initial_strength == 50);

    /* Clear demand and tick: waggle should decay */
    d.demand_buffer_depth = 0;
    uint32_t prev = initial_strength;
    for (int i = 0; i < 15; i++) {  /* Need 10 ticks to decay 50→0 (5 per tick) */
        hive_queen_emit_waggle(&d);
        assert(d.stats.waggle_strength <= prev);
        prev = d.stats.waggle_strength;
    }
    /* After 10+ ticks with no demand, should be zero */
    assert(d.stats.waggle_strength == 0);

    hive_dynamics_deinit(&d);
    printf("PASS\n");
}

/* ================================================================
 * Test 3: Alarm Signal Raising
 *
 * Verify that worker alarms increment consecutive_alarms counter.
 * ================================================================ */

static void test_alarm_raising(void)
{
    printf("  [3] Alarm signal raising ... ");
    fflush(stdout);

    hive_dynamics_t d;
    hive_logger_t log;
    memset(&log, 0, sizeof(log));

    hive_dynamics_init(&d, 32);

    /* Spawn a worker */
    hive_agent_traits_t traits = {
        .temperature_pct = 50,
        .lineage_hash = 1,
        .specialization = 0,
        .resource_cap_pct = 80,
    };
    size_t worker_idx = hive_queen_spawn(&d, HIVE_ROLE_CLEANER,
                                          hive_lifecycle_builtin_worker(),
                                          traits);
    assert(worker_idx != SIZE_MAX);

    /* Send alarm packet */
    hive_groom_packet_t pkt = {
        .agent_idx = worker_idx,
        .task_ticks = 10,
        .success = 0,
        .alarm_flag = 1,  /* Error detected */
    };

    hive_agent_cell_t *worker_before = &d.agents[worker_idx];
    uint8_t before_alarms = worker_before->consecutive_alarms;

    hive_queen_receive_groom(&d, &pkt, &log);

    hive_agent_cell_t *worker_after = &d.agents[worker_idx];
    assert(worker_after->consecutive_alarms == before_alarms + 1);

    hive_dynamics_deinit(&d);
    printf("PASS\n");
}

/* ================================================================
 * Test 4: Worker Quarantine
 *
 * Verify that workers are quarantined after too many alarms.
 * ================================================================ */

static void test_worker_quarantine(void)
{
    printf("  [4] Worker quarantine on alarm threshold ... ");
    fflush(stdout);

    hive_dynamics_t d;
    hive_logger_t log;
    memset(&log, 0, sizeof(log));

    hive_dynamics_init(&d, 32);

    /* Spawn a worker */
    hive_agent_traits_t traits = {
        .temperature_pct = 50,
        .lineage_hash = 1,
        .specialization = 0,
        .resource_cap_pct = 80,
    };
    size_t worker_idx = hive_queen_spawn(&d, HIVE_ROLE_CLEANER,
                                          hive_lifecycle_builtin_worker(),
                                          traits);
    assert(worker_idx != SIZE_MAX);

    hive_agent_cell_t *worker = &d.agents[worker_idx];
    assert(worker->conditioned_ok == true);

    /* Send alarm packets until quarantine threshold */
    hive_groom_packet_t pkt = {
        .agent_idx = worker_idx,
        .task_ticks = 10,
        .success = 0,
        .alarm_flag = 1,
    };

    for (unsigned i = 0; i < HIVE_QUARANTINE_ALARM_THRESHOLD; i++) {
        hive_queen_receive_groom(&d, &pkt, &log);
    }

    /* Worker should be quarantined */
    assert(worker->conditioned_ok == false);
    assert(worker->perf_score == 0);

    hive_dynamics_deinit(&d);
    printf("PASS\n");
}

/* ================================================================
 * Test 5: Alarm Recovery
 *
 * Verify that alarm counters reset on success.
 * ================================================================ */

static void test_alarm_recovery(void)
{
    printf("  [5] Alarm recovery on success ... ");
    fflush(stdout);

    hive_dynamics_t d;
    hive_logger_t log;
    memset(&log, 0, sizeof(log));

    hive_dynamics_init(&d, 32);

    /* Spawn a worker and send some alarms */
    hive_agent_traits_t traits = {
        .temperature_pct = 50,
        .lineage_hash = 1,
        .specialization = 0,
        .resource_cap_pct = 80,
    };
    size_t worker_idx = hive_queen_spawn(&d, HIVE_ROLE_CLEANER,
                                          hive_lifecycle_builtin_worker(),
                                          traits);
    assert(worker_idx != SIZE_MAX);

    hive_groom_packet_t alarm_pkt = {
        .agent_idx = worker_idx,
        .task_ticks = 10,
        .success = 0,
        .alarm_flag = 1,
    };

    hive_agent_cell_t *worker = &d.agents[worker_idx];

    /* Send two alarms */
    hive_queen_receive_groom(&d, &alarm_pkt, &log);
    hive_queen_receive_groom(&d, &alarm_pkt, &log);
    assert(worker->consecutive_alarms == 2);

    /* Send success packet: should reset alarm counter */
    hive_groom_packet_t success_pkt = {
        .agent_idx = worker_idx,
        .task_ticks = 10,
        .success = 100,
        .alarm_flag = 0,
    };
    hive_queen_receive_groom(&d, &success_pkt, &log);

    /* Consecutive alarms should reset; perf_score should improve */
    assert(worker->consecutive_alarms == 0);
    assert(worker->perf_score > 0);  /* EMA blend of old perf + success */

    hive_dynamics_deinit(&d);
    printf("PASS\n");
}

/* ================================================================
 * Test 6: End-to-End Signal Coordination
 *
 * Comprehensive test: spawn workers, inject demand, verify waggle,
 * simulate task collisions, verify quarantine, verify recovery.
 * ================================================================ */

static void test_e2e_signal_coordination(void)
{
    printf("  [6] End-to-end signal coordination ... ");
    fflush(stdout);

    hive_dynamics_t d;
    hive_logger_t log;
    memset(&log, 0, sizeof(log));

    hive_dynamics_init(&d, 64);

    /* Spawn 4 workers */
    hive_agent_traits_t traits = {
        .temperature_pct = 50,
        .lineage_hash = 1,
        .specialization = 0,
        .resource_cap_pct = 80,
    };
    size_t workers[4];
    for (int i = 0; i < 4; i++) {
        workers[i] = hive_queen_spawn(&d, HIVE_ROLE_CLEANER,
                                       hive_lifecycle_builtin_worker(),
                                       traits);
        assert(workers[i] != SIZE_MAX);
    }

    /* Phase 1: Inject high demand — waggle should activate */
    d.demand_buffer_depth = 75;
    hive_queen_emit_waggle(&d);
    assert(d.stats.waggle_strength == 75);
    assert(d.stats.waggle_duration == 8);

    /* Phase 2: Simulate task execution cycle with mixed results */
    for (int cycle = 0; cycle < 3; cycle++) {
        d.demand_buffer_depth = 100 - (cycle * 25);  /* Decreasing demand */

        hive_queen_emit_waggle(&d);

        /* Worker 0 & 1 attempt same task (collision) */
        if (cycle == 0) {
            hive_groom_packet_t pkt_success = {
                .agent_idx = workers[0],
                .task_ticks = 10,
                .success = 100,
                .alarm_flag = 0,
            };
            hive_queen_receive_groom(&d, &pkt_success, &log);

            hive_groom_packet_t pkt_collision = {
                .agent_idx = workers[1],
                .task_ticks = 10,
                .success = 0,
                .alarm_flag = 1,  /* Detected collision */
            };
            hive_queen_receive_groom(&d, &pkt_collision, &log);
        }

        /* Other workers report success */
        hive_groom_packet_t pkt_ok = {
            .agent_idx = workers[2 + (cycle % 2)],
            .task_ticks = 5,
            .success = 80,
            .alarm_flag = 0,
        };
        hive_queen_receive_groom(&d, &pkt_ok, &log);
    }

    /* Phase 3: Verify state after coordination */
    assert(d.agents[workers[1]].consecutive_alarms >= 1);  /* Collision victim */
    assert(d.agents[workers[0]].consecutive_alarms == 0);  /* Success */
    assert(d.agents[workers[1]].perf_score < 100);         /* Degraded */

    /* Phase 4: Send more alarms until quarantine */
    hive_groom_packet_t collision_pkt = {
        .agent_idx = workers[1],
        .task_ticks = 10,
        .success = 0,
        .alarm_flag = 1,
    };
    for (unsigned i = 0; i < HIVE_QUARANTINE_ALARM_THRESHOLD; i++) {
        hive_queen_receive_groom(&d, &collision_pkt, &log);
    }

    /* Worker 1 should be quarantined */
    assert(d.agents[workers[1]].conditioned_ok == false);

    /* Phase 5: Clear demand and verify waggle decay */
    d.demand_buffer_depth = 0;
    uint32_t prev_strength = d.stats.waggle_strength;
    for (int i = 0; i < 15; i++) {  /* Need 10+ ticks to fully decay */
        hive_queen_emit_waggle(&d);
        assert(d.stats.waggle_strength <= prev_strength);
        prev_strength = d.stats.waggle_strength;
    }
    assert(d.stats.waggle_strength == 0);

    hive_dynamics_deinit(&d);
    printf("PASS\n");
}

/* ================================================================
 * Test 7: Dynamics Tick Integration
 *
 * Verify that waggle signals are properly integrated into the
 * per-tick cycle, executed in correct order.
 * ================================================================ */

static void test_dynamics_tick_integration(void)
{
    printf("  [7] Dynamics tick integration ... ");
    fflush(stdout);

    hive_dynamics_t d;
    hive_dynamics_init(&d, 32);

    d.demand_buffer_depth = 50;

    /* Before tick: no waggle */
    assert(d.stats.waggle_strength == 0);

    /* Tick: should emit waggle */
    hive_dynamics_tick(&d);

    /* After tick: waggle should be active */
    assert(d.stats.waggle_strength == 50);
    assert(d.stats.waggle_duration == 8);

    /* Tick again with demand */
    hive_dynamics_tick(&d);
    assert(d.stats.waggle_strength == 50);  /* Refreshed */

    /* Clear demand and tick */
    d.demand_buffer_depth = 0;
    hive_dynamics_tick(&d);
    assert(d.stats.waggle_strength < 50);  /* Decayed */

    hive_dynamics_deinit(&d);
    printf("PASS\n");
}

/* ================================================================
 * Main test runner
 * ================================================================ */

int main(void)
{
    printf("\n=== Phase 2.5 Worker Coordination Signals Tests ===\n\n");

    test_waggle_emission();
    test_waggle_decay();
    test_alarm_raising();
    test_worker_quarantine();
    test_alarm_recovery();
    test_e2e_signal_coordination();
    test_dynamics_tick_integration();

    printf("\n=== All tests passed! ===\n\n");
    return 0;
}

#else

typedef int dummy_tu;

#endif  /* HIVE_BUILD_TESTS */
