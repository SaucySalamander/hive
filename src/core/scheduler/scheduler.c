/*
 * OPTION 3 IMPLEMENTATION — WORKER-CELL MAPPING
 * scheduler.c — per-worker pipeline scheduler implementation.
 *
 * Design decisions (v1):
 *   - Single-threaded execution: the scheduler loop runs all dynamics and
 *     dispatch on the calling thread.  pthread_rwlock_t is present in the
 *     struct but wrlock is taken before every mutation so that read-only
 *     accessors (TUI, metrics threads) can be added safely in future.
 *   - Per-worker contexts are activated lazily when a cell is first seen with
 *     a bound_agent, and freed eagerly in scan_and_retire() when a cell's
 *     role transitions to EMPTY.
 *   - The Queen cell is the canonical output arbiter: only its completed
 *     cycle is promoted into runtime->session (global project state).
 *   - Each dispatch builds a context-specific "prior_output" string from the
 *     worker's local hive_agent_context_t so that workers operate
 *     independently without mutating the shared session.
 */

#include "core/scheduler/scheduler.h"

#include "core/queen/queen.h"
#include "core/agent/agent.h"
#include "core/agent/orchestrator.h"
#include "core/agent/planner.h"
#include "core/agent/coder.h"
#include "core/agent/tester.h"
#include "core/agent/verifier.h"
#include "core/agent/editor.h"
#include "core/evaluator/evaluator.h"
#include "core/runtime.h"
#include "core/trace/trace.h"
#include "common/logging/logger.h"
#include "common/strings.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>

/* ================================================================
 * Internal string helper
 * ================================================================ */

static const char *safe_str(const char *s)
{
    return s != NULL ? s : "";
}

/* ================================================================
 * hive_agent_stage_to_string
 * ================================================================ */

const char *hive_agent_stage_to_string(hive_agent_stage_t stage)
{
    switch (stage) {
    case HIVE_AGENT_STAGE_ORCHESTRATE: return "orchestrate";
    case HIVE_AGENT_STAGE_PLAN:        return "plan";
    case HIVE_AGENT_STAGE_CODE:        return "code";
    case HIVE_AGENT_STAGE_TEST:        return "test";
    case HIVE_AGENT_STAGE_VERIFY:      return "verify";
    case HIVE_AGENT_STAGE_EDIT:        return "edit";
    case HIVE_AGENT_STAGE_EVALUATE:    return "evaluate";
    case HIVE_AGENT_STAGE_DONE:        return "done";
    default:                           return "unknown";
    }
}

/* ================================================================
 * hive_agent_descriptor_for_role is implemented in agent.c
 * (declared in both agent.h and scheduler.h for convenience).
 * ================================================================ */

/* ================================================================
 * Lock helpers (compiled to no-ops under HIVE_SINGLE_THREADED)
 * ================================================================ */

#if HIVE_SINGLE_THREADED
static void sched_rdlock(hive_scheduler_t *s) { (void)s; }
static void sched_wrlock(hive_scheduler_t *s) { (void)s; }
static void sched_unlock(hive_scheduler_t *s) { (void)s; }
#else
static void sched_rdlock(hive_scheduler_t *s)
{
    if (s->lock_initialized) pthread_rwlock_rdlock(&s->dynamics_lock);
}
static void sched_wrlock(hive_scheduler_t *s)
{
    if (s->lock_initialized) pthread_rwlock_wrlock(&s->dynamics_lock);
}
static void sched_unlock(hive_scheduler_t *s)
{
    if (s->lock_initialized) pthread_rwlock_unlock(&s->dynamics_lock);
}
#endif

/* ================================================================
 * Context lifecycle helpers
 * ================================================================ */

/* Free all heap strings in @ctx and zero it.  Sets in_use = false. */
static void context_clear(hive_agent_context_t *ctx)
{
    if (ctx == NULL) return;
    free(ctx->orchestrator_brief);
    free(ctx->plan);
    free(ctx->code);
    free(ctx->tests);
    free(ctx->verification);
    free(ctx->edit);
    free(ctx->final_output);
    free(ctx->last_critique);
    memset(ctx, 0, sizeof(*ctx));
    /* in_use = false is covered by the memset above */
}

/*
 * Reset a context to start a fresh cycle.  Frees per-cycle strings but
 * preserves cycle_count.  Called after HIVE_AGENT_STAGE_DONE before
 * re-entering ORCHESTRATE.
 */
static void context_reset_cycle(hive_agent_context_t *ctx)
{
    if (ctx == NULL) return;
    free(ctx->orchestrator_brief); ctx->orchestrator_brief = NULL;
    free(ctx->plan);               ctx->plan               = NULL;
    free(ctx->code);               ctx->code               = NULL;
    free(ctx->tests);              ctx->tests              = NULL;
    free(ctx->verification);       ctx->verification       = NULL;
    free(ctx->edit);               ctx->edit               = NULL;
    free(ctx->final_output);       ctx->final_output       = NULL;
    free(ctx->last_critique);      ctx->last_critique      = NULL;
    memset(&ctx->last_score, 0, sizeof(ctx->last_score));
    memset(ctx->score_history, 0, sizeof(ctx->score_history));
    ctx->score_history_count = 0U;
    ctx->current_stage = HIVE_AGENT_STAGE_ORCHESTRATE;
    ctx->iteration     = 0U;
    ctx->cycle_count++;         /* cycle_count and in_use are preserved */
}

/* ================================================================
 * Cell retirement
 * ================================================================ */

/*
 * retire_cell — free the bound_agent and context for cell @idx.
 *
 * Called when a cell's role transitions to EMPTY or DRONE (the cell can no
 * longer execute pipeline stages).  The bound_agent pointer on the cell is
 * cleared here so hive_dynamics_deinit() doesn't double-free it.
 */
static void retire_cell(hive_scheduler_t *sched,
                        size_t            idx,
                        hive_logger_t    *log)
{
    hive_agent_context_t *ctx  = &sched->contexts[idx];
    hive_agent_cell_t    *cell = &sched->dynamics->agents[idx];

    if (!ctx->in_use) return;

    if (log != NULL) {
        hive_logger_logf(log, HIVE_LOG_DEBUG, "scheduler", "retire_cell",
                         "retiring cell[%zu] (stage=%s cycles=%u)",
                         idx,
                         hive_agent_stage_to_string(ctx->current_stage),
                         ctx->cycle_count);
    }

    context_clear(ctx);   /* sets in_use = false */

    /* Free the heap-cloned descriptor owned by this cell. */
    if (cell->bound_agent != NULL) {
        hive_agent_free(cell->bound_agent);
        cell->bound_agent = NULL;
    }
}

/*
 * scan_and_retire — walk the agent array and retire any cell whose role
 * transitioned to EMPTY or DRONE since the last tick.
 */
static void scan_and_retire(hive_scheduler_t *sched, hive_logger_t *log)
{
    sched_wrlock(sched);
    for (size_t i = 0; i < sched->dynamics->agent_count; ++i) {
        hive_agent_role_t cur  = sched->dynamics->agents[i].role;
        hive_agent_role_t last = sched->last_seen_role[i];

        if ((last != HIVE_ROLE_EMPTY && last != HIVE_ROLE_DRONE) &&
            (cur  == HIVE_ROLE_EMPTY || cur  == HIVE_ROLE_DRONE)) {
            retire_cell(sched, i, log);
        }

        sched->last_seen_role[i] = cur;
    }
    sched_unlock(sched);
}

/* ================================================================
 * Eligible worker selection
 * ================================================================ */

typedef struct eligible_entry {
    size_t   idx;
    uint32_t perf_score;
    uint32_t age_ticks;
} eligible_entry_t;

/* Comparator: highest perf_score first; tie-break by oldest age first. */
static int cmp_eligible(const void *a, const void *b)
{
    const eligible_entry_t *ea = (const eligible_entry_t *)a;
    const eligible_entry_t *eb = (const eligible_entry_t *)b;
    if (eb->perf_score != ea->perf_score)
        return (int)eb->perf_score - (int)ea->perf_score;
    return (int)eb->age_ticks - (int)ea->age_ticks;
}

/*
 * build_eligible_list — fill @out with cells that may be dispatched to.
 *
 * A cell is eligible when ALL of the following hold:
 *   - role != EMPTY and role != DRONE
 *   - conditioned_ok == true  (pheromone gate from Queen)
 *   - bound_agent    != NULL  (has a bound pipeline agent)
 *   - perf_score     >= min_perf
 *   - contexts[i].in_use == true
 *
 * @return Count of entries written into @out.
 */
static size_t build_eligible_list(const hive_scheduler_t *sched,
                                  unsigned                min_perf,
                                  eligible_entry_t       *out,
                                  size_t                  out_cap)
{
    size_t count = 0U;
    for (size_t i = 0;
         i < sched->dynamics->agent_count && count < out_cap;
         ++i)
    {
        const hive_agent_cell_t    *cell = &sched->dynamics->agents[i];
        const hive_agent_context_t *ctx  = &sched->contexts[i];

        if (cell->role == HIVE_ROLE_EMPTY || cell->role == HIVE_ROLE_DRONE) continue;
        if (!cell->conditioned_ok)   continue;
        if (cell->bound_agent == NULL) continue;
        if (cell->perf_score < min_perf) continue;
        if (!ctx->in_use)            continue;

        out[count].idx        = i;
        out[count].perf_score = cell->perf_score;
        out[count].age_ticks  = cell->age_ticks;
        count++;
    }
    qsort(out, count, sizeof(*out), cmp_eligible);
    return count;
}

/* ================================================================
 * Observability metrics update
 * ================================================================ */

static void update_metrics(hive_scheduler_t *sched)
{
    unsigned bound      = 0U;
    unsigned suppressed = 0U;

    for (size_t i = 0; i < sched->dynamics->agent_count; ++i) {
        const hive_agent_cell_t *cell = &sched->dynamics->agents[i];
        if (cell->role == HIVE_ROLE_EMPTY || cell->role == HIVE_ROLE_DRONE) continue;
        if (cell->bound_agent  != NULL) bound++;
        if (!cell->conditioned_ok)      suppressed++;
    }

    sched->active_bound_workers = bound;
    sched->suppressed_workers   = suppressed;

    /* Push metrics into dynamics->stats so TUI / OpenTelemetry stub can read. */
    sched->dynamics->stats.active_bound_workers      = bound;
    sched->dynamics->stats.suppressed_workers        = suppressed;
    sched->dynamics->stats.average_pheromone_latency_ns =
        sched->average_pheromone_latency_ns;
    sched->dynamics->stats.requeen_events_this_run   =
        sched->requeen_events_this_run;
}

/* ================================================================
 * Back-off helper
 * ================================================================ */

static void scheduler_backoff(unsigned ms)
{
    if (ms == 0U) return;
    struct timespec ts;
    ts.tv_sec  = (time_t)(ms / 1000U);
    ts.tv_nsec = (long)((ms % 1000U) * 1000000L);
    nanosleep(&ts, NULL);
}

/* ================================================================
 * Per-stage dispatch helpers
 *
 * Each function builds a context-specific "prior_output" string from the
 * worker's local hive_agent_context_t, passes it to the appropriate agent
 * run function, and advances ctx->current_stage on success.
 *
 * The shared runtime->session fields (workspace_root, user_prompt,
 * project_rules) are read-only from the worker's perspective; only the Queen
 * writes back to session when its cycle completes (promote_queen_output).
 * ================================================================ */

static void replace_str(char **slot, char *replacement)
{
    if (slot == NULL) { free(replacement); return; }
    free(*slot);
    *slot = replacement;
}

static hive_status_t dispatch_orchestrate(hive_agent_context_t *ctx,
                                          hive_runtime_t       *rt)
{
    /*
     * The orchestrator runs against the global user_prompt and produces a
     * supervisor brief for this worker's private context.
     */
    char *critique = NULL, *brief = NULL;
    hive_status_t s = hive_orchestrator_run(rt,
                                            rt->session.user_prompt,
                                            &critique, &brief);
    if (s != HIVE_STATUS_OK) { free(critique); free(brief); return s; }

    replace_str(&ctx->orchestrator_brief, brief);
    replace_str(&ctx->last_critique,      critique);
    ctx->iteration     = 0U;
    ctx->current_stage = HIVE_AGENT_STAGE_PLAN;
    return HIVE_STATUS_OK;
}

static hive_status_t dispatch_plan(hive_agent_context_t *ctx,
                                   hive_runtime_t       *rt)
{
    /* Build prior_output from worker-local context so this worker's brief and
     * critique drive the planner, not the global session state. */
    char *prior = hive_string_format(
        "Supervisor brief:\n%s\n\nUser prompt:\n%s\n\nLast critique:\n%s\n",
        safe_str(ctx->orchestrator_brief),
        safe_str(rt->session.user_prompt),
        safe_str(ctx->last_critique));
    if (prior == NULL) return HIVE_STATUS_OUT_OF_MEMORY;

    char *critique = NULL, *plan = NULL;
    hive_status_t s = hive_planner_run(rt, prior, &critique, &plan);
    free(prior);
    if (s != HIVE_STATUS_OK) { free(critique); free(plan); return s; }

    replace_str(&ctx->plan,          plan);
    replace_str(&ctx->last_critique, critique);
    ctx->current_stage = HIVE_AGENT_STAGE_CODE;
    return HIVE_STATUS_OK;
}

static hive_status_t dispatch_code(hive_agent_context_t *ctx,
                                   hive_runtime_t       *rt)
{
    char *prior = hive_string_format(
        "Brief:\n%s\n\nPlan:\n%s\n\nLast critique:\n%s\n",
        safe_str(ctx->orchestrator_brief),
        safe_str(ctx->plan),
        safe_str(ctx->last_critique));
    if (prior == NULL) return HIVE_STATUS_OUT_OF_MEMORY;

    char *critique = NULL, *code = NULL;
    hive_status_t s = hive_coder_run(rt, prior, &critique, &code);
    free(prior);
    if (s != HIVE_STATUS_OK) { free(critique); free(code); return s; }

    replace_str(&ctx->code,          code);
    replace_str(&ctx->last_critique, critique);
    ctx->current_stage = HIVE_AGENT_STAGE_TEST;
    return HIVE_STATUS_OK;
}

static hive_status_t dispatch_test(hive_agent_context_t *ctx,
                                   hive_runtime_t       *rt)
{
    char *prior = hive_string_format(
        "Plan:\n%s\n\nCode draft:\n%s\n\nLast critique:\n%s\n",
        safe_str(ctx->plan),
        safe_str(ctx->code),
        safe_str(ctx->last_critique));
    if (prior == NULL) return HIVE_STATUS_OUT_OF_MEMORY;

    char *critique = NULL, *tests = NULL;
    hive_status_t s = hive_tester_run(rt, prior, &critique, &tests);
    free(prior);
    if (s != HIVE_STATUS_OK) { free(critique); free(tests); return s; }

    replace_str(&ctx->tests,         tests);
    replace_str(&ctx->last_critique, critique);
    ctx->current_stage = HIVE_AGENT_STAGE_VERIFY;
    return HIVE_STATUS_OK;
}

static hive_status_t dispatch_verify(hive_agent_context_t *ctx,
                                     hive_runtime_t       *rt)
{
    char *prior = hive_string_format(
        "Plan:\n%s\n\nCode draft:\n%s\n\nTests:\n%s\n\nLast critique:\n%s\n",
        safe_str(ctx->plan),
        safe_str(ctx->code),
        safe_str(ctx->tests),
        safe_str(ctx->last_critique));
    if (prior == NULL) return HIVE_STATUS_OUT_OF_MEMORY;

    char *critique = NULL, *verif = NULL;
    hive_status_t s = hive_verifier_run(rt, prior, &critique, &verif);
    free(prior);
    if (s != HIVE_STATUS_OK) { free(critique); free(verif); return s; }

    replace_str(&ctx->verification,  verif);
    replace_str(&ctx->last_critique, critique);
    ctx->current_stage = HIVE_AGENT_STAGE_EDIT;
    return HIVE_STATUS_OK;
}

static hive_status_t dispatch_edit(hive_agent_context_t *ctx,
                                   hive_runtime_t       *rt)
{
    char *prior = hive_string_format(
        "Plan:\n%s\n\nCode draft:\n%s\n\nTests:\n%s\n\nVerification:\n%s\n\n"
        "Last critique:\n%s\n",
        safe_str(ctx->plan),
        safe_str(ctx->code),
        safe_str(ctx->tests),
        safe_str(ctx->verification),
        safe_str(ctx->last_critique));
    if (prior == NULL) return HIVE_STATUS_OUT_OF_MEMORY;

    char *critique = NULL, *edit = NULL;
    hive_status_t s = hive_editor_run(rt, prior, &critique, &edit);
    free(prior);
    if (s != HIVE_STATUS_OK) { free(critique); free(edit); return s; }

    replace_str(&ctx->edit,          edit);
    replace_str(&ctx->last_critique, critique);
    ctx->current_stage = HIVE_AGENT_STAGE_EVALUATE;
    return HIVE_STATUS_OK;
}

static hive_status_t dispatch_evaluate(hive_agent_context_t   *ctx,
                                       hive_runtime_t         *rt,
                                       const hive_scheduler_t *sched)
{
    hive_score_t scores[5] = {{0}};

    if (hive_evaluator_score("Planner",  ctx->plan,         &scores[0]) != HIVE_STATUS_OK ||
        hive_evaluator_score("Coder",    ctx->code,         &scores[1]) != HIVE_STATUS_OK ||
        hive_evaluator_score("Tester",   ctx->tests,        &scores[2]) != HIVE_STATUS_OK ||
        hive_evaluator_score("Verifier", ctx->verification, &scores[3]) != HIVE_STATUS_OK ||
        hive_evaluator_score("Editor",   ctx->edit,         &scores[4]) != HIVE_STATUS_OK) {
        return HIVE_STATUS_ERROR;
    }

    ctx->last_score.correctness   = (scores[0].correctness   + scores[1].correctness   +
                                     scores[2].correctness   + scores[3].correctness   +
                                     scores[4].correctness)   / 5U;
    ctx->last_score.security      = (scores[0].security      + scores[1].security      +
                                     scores[2].security      + scores[3].security      +
                                     scores[4].security)      / 5U;
    ctx->last_score.style         = (scores[0].style         + scores[1].style         +
                                     scores[2].style         + scores[3].style         +
                                     scores[4].style)         / 5U;
    ctx->last_score.test_coverage = (scores[0].test_coverage + scores[1].test_coverage +
                                     scores[2].test_coverage + scores[3].test_coverage +
                                     scores[4].test_coverage) / 5U;
    ctx->last_score.overall       = (ctx->last_score.correctness   * 40U +
                                     ctx->last_score.security      * 25U +
                                     ctx->last_score.style         * 20U +
                                     ctx->last_score.test_coverage * 15U) / 100U;
    if (ctx->last_score.overall > 100U) ctx->last_score.overall = 100U;

    /* Push score into per-iteration ring buffer (last 4 evaluations). */
    ctx->score_history[ctx->score_history_count % 4U] = ctx->last_score;
    ctx->score_history_count++;

    /* Build a critique that includes a delta vs the previous iteration. */
    free(ctx->last_critique);
    if (ctx->score_history_count >= 2U) {
        unsigned prev_idx = (ctx->score_history_count - 2U) % 4U;
        int delta = (int)ctx->last_score.overall
                    - (int)ctx->score_history[prev_idx].overall;
        ctx->last_critique = hive_string_format(
            "Iteration %u score: correctness=%u security=%u style=%u "
            "test_coverage=%u overall=%u (delta=%+d vs iter %u)",
            ctx->iteration + 1U,
            ctx->last_score.correctness,
            ctx->last_score.security,
            ctx->last_score.style,
            ctx->last_score.test_coverage,
            ctx->last_score.overall,
            delta,
            ctx->iteration);
    } else {
        ctx->last_critique = hive_string_format(
            "Iteration %u score: correctness=%u security=%u style=%u "
            "test_coverage=%u overall=%u",
            ctx->iteration + 1U,
            ctx->last_score.correctness,
            ctx->last_score.security,
            ctx->last_score.style,
            ctx->last_score.test_coverage,
            ctx->last_score.overall);
    }
    if (ctx->last_critique == NULL) return HIVE_STATUS_OUT_OF_MEMORY;

    bool acceptable = hive_evaluator_is_acceptable(&ctx->last_score,
                                                    sched->opts.score_threshold);
    bool exhausted   = (ctx->iteration + 1U) >= sched->opts.iteration_limit;

    if (acceptable || exhausted) {
        /* Final pass: Orchestrator summarises the full cycle. */
        char *summary = hive_string_format(
            "Iteration %u summary:\n\nPlan:\n%s\n\nCode:\n%s\n\nTests:\n%s\n\n"
            "Verification:\n%s\n\nEditor:\n%s\n\n"
            "Score: correctness=%u security=%u style=%u "
            "test_coverage=%u overall=%u\n",
            ctx->iteration + 1U,
            safe_str(ctx->plan), safe_str(ctx->code), safe_str(ctx->tests),
            safe_str(ctx->verification), safe_str(ctx->edit),
            ctx->last_score.correctness, ctx->last_score.security,
            ctx->last_score.style, ctx->last_score.test_coverage,
            ctx->last_score.overall);
        if (summary == NULL) return HIVE_STATUS_OUT_OF_MEMORY;

        char *critique = NULL, *final_out = NULL;
        hive_status_t s = hive_orchestrator_run(rt, summary,
                                                &critique, &final_out);
        free(summary);
        if (s != HIVE_STATUS_OK) { free(critique); free(final_out); return s; }

        replace_str(&ctx->last_critique, critique);
        replace_str(&ctx->final_output,  final_out);
        ctx->current_stage = HIVE_AGENT_STAGE_DONE;
    } else {
        ctx->iteration++;
        ctx->current_stage = HIVE_AGENT_STAGE_PLAN;
    }

    return HIVE_STATUS_OK;
}

/*
 * dispatch_stage — advance one pipeline step for @ctx.
 *
 * Returns HIVE_STATUS_OK even when current_stage is DONE; the caller handles
 * promotion and cycle reset.
 */
static hive_status_t dispatch_stage(hive_agent_context_t   *ctx,
                                    hive_runtime_t         *rt,
                                    const hive_scheduler_t *sched)
{
    switch (ctx->current_stage) {
    case HIVE_AGENT_STAGE_ORCHESTRATE: return dispatch_orchestrate(ctx, rt);
    case HIVE_AGENT_STAGE_PLAN:        return dispatch_plan(ctx, rt);
    case HIVE_AGENT_STAGE_CODE:        return dispatch_code(ctx, rt);
    case HIVE_AGENT_STAGE_TEST:        return dispatch_test(ctx, rt);
    case HIVE_AGENT_STAGE_VERIFY:      return dispatch_verify(ctx, rt);
    case HIVE_AGENT_STAGE_EDIT:        return dispatch_edit(ctx, rt);
    case HIVE_AGENT_STAGE_EVALUATE:    return dispatch_evaluate(ctx, rt, sched);
    case HIVE_AGENT_STAGE_DONE:        return HIVE_STATUS_OK;
    default:                           return HIVE_STATUS_INVALID_ARGUMENT;
    }
}

/* ================================================================
 * Grooming packet construction
 * ================================================================ */

static void build_groom_packet(const hive_agent_cell_t    *cell,
                               size_t                      cell_idx,
                               hive_agent_stage_t          stage_before,
                               hive_status_t               stage_status,
                               const hive_agent_context_t *ctx,
                               uint64_t                    elapsed_ns,
                               hive_groom_packet_t        *pkt_out)
{
    (void)stage_before;
    memset(pkt_out, 0, sizeof(*pkt_out));

    pkt_out->agent_idx        = cell_idx;
    pkt_out->task_ticks       = 1U;
    pkt_out->success          = (stage_status == HIVE_STATUS_OK) ? 100U : 0U;
    pkt_out->alarm_flag       = (stage_status != HIVE_STATUS_OK) ? 1U  : 0U;
    pkt_out->stage_latency_ns = elapsed_ns;
    pkt_out->confidence_score = (uint8_t)(ctx->last_score.overall <= 100U
                                           ? ctx->last_score.overall : 100U);

    /* bit 0 = low perf, bit 1 = stage error */
    pkt_out->anomaly_flags  = 0U;
    if (cell->perf_score < 30U)         pkt_out->anomaly_flags |= 0x01U;
    if (stage_status != HIVE_STATUS_OK) pkt_out->anomaly_flags |= 0x02U;
}

/* ================================================================
 * Queen promotion: copy worker context → global session
 * ================================================================ */

/*
 * promote_queen_output — called when the Queen cell reaches DONE.
 *
 * Copies the accumulated context strings into runtime->session so that the
 * GTK / TUI surfaces, the API server, and subsequent hive_runtime_run()
 * callers all see the canonical project output.
 *
 * The context strings are duplicated (not moved) so the context remains
 * valid until context_reset_cycle() cleans it up for the next pass.
 */
static void promote_queen_output(const hive_agent_context_t *ctx,
                                 hive_runtime_t             *rt)
{
    if (ctx == NULL || rt == NULL) return;

#define COPY_FIELD(dst, src)   \
    do {                       \
        free(dst);             \
        (dst) = hive_string_dup(safe_str(src)); \
    } while (0)

    COPY_FIELD(rt->session.final_output,        ctx->final_output);
    COPY_FIELD(rt->session.orchestrator_brief,  ctx->orchestrator_brief);
    COPY_FIELD(rt->session.plan,                ctx->plan);
    COPY_FIELD(rt->session.code,                ctx->code);
    COPY_FIELD(rt->session.tests,               ctx->tests);
    COPY_FIELD(rt->session.verification,        ctx->verification);
    COPY_FIELD(rt->session.edit,                ctx->edit);
    COPY_FIELD(rt->session.last_critique,       ctx->last_critique);

#undef COPY_FIELD

    rt->session.last_score = ctx->last_score;
    rt->session.iteration  = ctx->iteration;
}

/* ================================================================
 * hive_scheduler_init
 * ================================================================ */

hive_status_t hive_scheduler_init(hive_scheduler_t              *sched,
                                  hive_dynamics_t               *dynamics,
                                  const hive_scheduler_options_t *opts)
{
    if (sched == NULL || dynamics == NULL) return HIVE_STATUS_INVALID_ARGUMENT;

    memset(sched, 0, sizeof(*sched));
    sched->dynamics = dynamics;

    /* Apply caller options with compile-time fallbacks. */
    if (opts != NULL) sched->opts = *opts;
    if (sched->opts.max_concurrent_workers == 0U)
        sched->opts.max_concurrent_workers = HIVE_SCHEDULER_MAX_CONCURRENT_WORKERS;
    if (sched->opts.backoff_ms_suppressed == 0U)
        sched->opts.backoff_ms_suppressed  = HIVE_SCHEDULER_BACKOFF_MS_SUPPRESSED;
    if (sched->opts.min_perf_score == 0U)
        sched->opts.min_perf_score         = HIVE_SCHEDULER_MIN_PERF_SCORE;
    if (sched->opts.score_threshold == 0U)
        sched->opts.score_threshold        = 72U;
    if (sched->opts.iteration_limit == 0U)
        sched->opts.iteration_limit        = 3U;

    /* Seed last_seen_role array from the current dynamics state. */
    for (size_t i = 0; i < HIVE_DYNAMICS_MAX_AGENTS; ++i)
        sched->last_seen_role[i] = HIVE_ROLE_EMPTY;
    for (size_t i = 0; i < dynamics->agent_count; ++i)
        sched->last_seen_role[i] = dynamics->agents[i].role;

    /*
     * Bind agents to every active cell that does not yet have a bound_agent.
     * At cold start (called after hive_dynamics_init()) this covers the Queen
     * cell at slot 0 which is created by hive_dynamics_init() before the
     * scheduler exists.  hive_queen_spawn() handles cells created after init.
     */
    for (size_t i = 0; i < dynamics->agent_count; ++i) {
        hive_agent_cell_t *cell = &dynamics->agents[i];
        if (cell->role == HIVE_ROLE_EMPTY || cell->role == HIVE_ROLE_DRONE)
            continue;

        if (cell->bound_agent == NULL) {
            const hive_agent_t *desc = hive_agent_descriptor_for_role(cell->role);
            if (desc != NULL) {
                cell->bound_agent        = hive_agent_clone_descriptor(desc);
                cell->binding_generation = 1U;
            }
        }

        if (cell->bound_agent != NULL) {
            sched->contexts[i].in_use        = true;
            sched->contexts[i].current_stage = HIVE_AGENT_STAGE_ORCHESTRATE;
        }
    }

#if !HIVE_SINGLE_THREADED
    if (pthread_rwlock_init(&sched->dynamics_lock, NULL) != 0)
        return HIVE_STATUS_ERROR;
    sched->lock_initialized = true;
#endif

    return HIVE_STATUS_OK;
}

/* ================================================================
 * hive_scheduler_deinit
 * ================================================================ */

void hive_scheduler_deinit(hive_scheduler_t *sched)
{
    if (sched == NULL) return;

    /*
     * Free per-worker contexts.  Also free any bound_agent pointers that are
     * still set on cells (e.g. if the scheduler is destroyed before the
     * colony finishes).  hive_dynamics_deinit() will skip already-NULL slots.
     */
    for (size_t i = 0; i < HIVE_DYNAMICS_MAX_AGENTS; ++i) {
        if (sched->contexts[i].in_use)
            context_clear(&sched->contexts[i]);

        if (sched->dynamics != NULL && i < sched->dynamics->agent_count) {
            hive_agent_cell_t *cell = &sched->dynamics->agents[i];
            if (cell->bound_agent != NULL) {
                hive_agent_free(cell->bound_agent);
                cell->bound_agent = NULL;
            }
        }
    }

#if !HIVE_SINGLE_THREADED
    if (sched->lock_initialized) {
        pthread_rwlock_destroy(&sched->dynamics_lock);
        sched->lock_initialized = false;
    }
#endif

    memset(sched, 0, sizeof(*sched));
}

/* ================================================================
 * hive_scheduler_run — main scheduler loop
 * ================================================================ */

hive_status_t hive_scheduler_run(hive_scheduler_t *sched,
                                 hive_runtime_t   *runtime)
{
    if (sched == NULL || runtime == NULL || sched->dynamics == NULL)
        return HIVE_STATUS_INVALID_ARGUMENT;

    hive_logger_t    *log      = runtime->logger.initialized ? &runtime->logger : NULL;
    eligible_entry_t  eligible[HIVE_DYNAMICS_MAX_AGENTS];
    bool              queen_done = false;

    while (!queen_done) {

        /* ---- 1. Advance the colony simulation ---- */
        size_t prev_queen_idx = sched->dynamics->queen_idx;

        sched_wrlock(sched);
        hive_dynamics_tick(sched->dynamics);
        sched_unlock(sched);

        /* Detect re-queening events by comparing queen_idx before and after. */
        if (sched->dynamics->queen_idx != prev_queen_idx) {
            sched->requeen_events_this_run++;
            if (log != NULL) {
                hive_logger_logf(log, HIVE_LOG_INFO, "scheduler", "requeue",
                                 "re-queening: slot %zu → %zu (generation %u)",
                                 prev_queen_idx,
                                 sched->dynamics->queen_idx,
                                 sched->dynamics->lineage_generation);
            }
        }

        /* ---- 2. Retire cells that transitioned to EMPTY since last tick ---- */
        scan_and_retire(sched, log);

        /* ---- 3. Activate contexts for newly-spawned or promoted cells ---- */
        sched_rdlock(sched);
        for (size_t i = 0; i < sched->dynamics->agent_count; ++i) {
            hive_agent_cell_t *cell = &sched->dynamics->agents[i];
            if (cell->role == HIVE_ROLE_EMPTY || cell->role == HIVE_ROLE_DRONE)
                continue;
            if (cell->bound_agent != NULL && !sched->contexts[i].in_use) {
                sched->contexts[i].in_use        = true;
                sched->contexts[i].current_stage = HIVE_AGENT_STAGE_ORCHESTRATE;
            }
        }
        sched_unlock(sched);

        /* ---- 4. Update observability metrics ---- */
        update_metrics(sched);

        /* ---- 5. Build eligible worker list ---- */
        sched_rdlock(sched);
        size_t n_eligible = build_eligible_list(sched,
                                                sched->opts.min_perf_score,
                                                eligible,
                                                HIVE_DYNAMICS_MAX_AGENTS);
        sched_unlock(sched);

        /* ---- 6. Handle Queenless / fully-suppressed colony ---- */
        if (n_eligible == 0U) {
            if (log != NULL) {
                hive_logger_log(log, HIVE_LOG_WARN, "scheduler", "suppressed",
                                "Queenless / suppressed colony \u2014 no eligible workers");
            }
            /* Hard abort: colony is truly dead with no agents at all. */
            if (!sched->dynamics->queen_alive &&
                sched->dynamics->agent_count == 0U) {
                if (log != NULL) {
                    hive_logger_log(log, HIVE_LOG_ERROR, "scheduler",
                                    "dead_colony",
                                    "colony has zero agents; aborting scheduler");
                }
                return HIVE_STATUS_ERROR;
            }
            scheduler_backoff(sched->opts.backoff_ms_suppressed);
            continue;
        }

        /* ---- 7. Dispatch one stage per eligible cell ---- */
        size_t dispatched = 0U;
        size_t max_w      = sched->opts.max_concurrent_workers;

        for (size_t ei = 0; ei < n_eligible && dispatched < max_w; ++ei) {
            size_t             idx  = eligible[ei].idx;
            hive_agent_cell_t *cell = &sched->dynamics->agents[idx];
            hive_agent_context_t *ctx = &sched->contexts[idx];

            /* Re-verify conditioned_ok (may have changed during this tick). */
            sched_rdlock(sched);
            bool cond_ok = cell->conditioned_ok;
            sched_unlock(sched);
            if (!cond_ok) continue;

            /* Measure wall-clock latency for the grooming packet. */
            struct timespec t0 = {0}, t1 = {0};
            clock_gettime(CLOCK_MONOTONIC, &t0);

            hive_agent_stage_t stage_before = ctx->current_stage;

            /* Set the dispatch context so hive_agent_generate() can tag
             * trace entries with the correct cell index and stage. */
            runtime->trace_agent_index = (int)idx;
            runtime->trace_stage       = ctx->current_stage;

            hive_status_t      stage_status = dispatch_stage(ctx, runtime, sched);

            clock_gettime(CLOCK_MONOTONIC, &t1);
            uint64_t elapsed_ns = (uint64_t)(t1.tv_sec  - t0.tv_sec)  * 1000000000ULL
                                + (uint64_t)(t1.tv_nsec - t0.tv_nsec);

            /* Item #15: treat a stage that ran too long as a failure. */
            if (stage_status == HIVE_STATUS_OK && elapsed_ns > HIVE_STAGE_TIMEOUT_NS) {
                stage_status = HIVE_STATUS_IO_ERROR;
                if (log != NULL) {
                    hive_logger_logf(log, HIVE_LOG_WARN, "scheduler", "stage_timeout",
                                     "cell[%zu] stage %s exceeded timeout (%.1fs)",
                                     idx,
                                     hive_agent_stage_to_string(stage_before),
                                     (double)elapsed_ns / 1.0e9);
                }
            }

            if (log != NULL) {
                hive_logger_logf(log, HIVE_LOG_DEBUG, "scheduler", "dispatch",
                                 "cell[%zu] role=%s %s\u2192%s status=%s %lluns",
                                 idx,
                                 hive_role_to_string(cell->role),
                                 hive_agent_stage_to_string(stage_before),
                                 hive_agent_stage_to_string(ctx->current_stage),
                                 hive_status_to_string(stage_status),
                                 (unsigned long long)elapsed_ns);
            }

            /* Build and deliver grooming packet to the Queen. */
            hive_groom_packet_t pkt;
            build_groom_packet(cell, idx, stage_before, stage_status,
                               ctx, elapsed_ns, &pkt);
            sched_wrlock(sched);
            hive_queen_receive_groom(sched->dynamics, &pkt,
                                     runtime->logger.initialized ? &runtime->logger : NULL);
            sched_unlock(sched);

            /* Handle cycle completion. */
            if (ctx->current_stage == HIVE_AGENT_STAGE_DONE) {
                bool is_queen = (idx == sched->dynamics->queen_idx);

                if (is_queen) {
                    /*
                     * Queen completed a full cycle.  Promote its outputs into
                     * the canonical runtime->session and signal outer loop to
                     * exit.
                     */
                    promote_queen_output(ctx, runtime);

                    /* Trace: record the session-mutation side effect. */
                    {
                        char score_buf[32];
                        snprintf(score_buf, sizeof(score_buf),
                                 "overall=%u", ctx->last_score.overall);
                        hive_trace_log_side_effect(
                            &runtime->tracer, (int)idx, "queen",
                            HIVE_AGENT_STAGE_DONE,
                            "queen promoted outputs to session",
                            score_buf);
                    }

                    if (log != NULL) {
                        hive_logger_logf(log, HIVE_LOG_INFO, "scheduler",
                                         "queen_cycle_done",
                                         "Queen cell[%zu] completed cycle %u "
                                         "(overall score=%u); promoting output",
                                         idx, ctx->cycle_count,
                                         ctx->last_score.overall);
                    }
                    queen_done = true;
                } else {
                    /* Trace: record worker cycle completion. */
                    {
                        char score_buf[32];
                        snprintf(score_buf, sizeof(score_buf),
                                 "overall=%u cycle=%u", ctx->last_score.overall,
                                 ctx->cycle_count);
                        hive_trace_log_side_effect(
                            &runtime->tracer, (int)idx,
                            hive_role_to_string(cell->role),
                            HIVE_AGENT_STAGE_DONE,
                            "worker completed cycle",
                            score_buf);
                    }

                    if (log != NULL) {
                        hive_logger_logf(log, HIVE_LOG_INFO, "scheduler",
                                         "worker_cycle_done",
                                         "worker cell[%zu] completed cycle %u "
                                         "(overall score=%u)",
                                         idx, ctx->cycle_count,
                                         ctx->last_score.overall);
                    }
                }

                /* Reset context for the next cycle. */
                context_reset_cycle(ctx);
            }

            if (stage_status != HIVE_STATUS_OK && log != NULL) {
                hive_logger_logf(log, HIVE_LOG_WARN, "scheduler", "stage_error",
                                 "cell[%zu] stage %s failed: %s",
                                 idx,
                                 hive_agent_stage_to_string(stage_before),
                                 hive_status_to_string(stage_status));
            }

            dispatched++;
        } /* for each eligible cell */

        if (log != NULL) {
            hive_logger_logf(log, HIVE_LOG_DEBUG, "scheduler", "tick_summary",
                             "dispatched=%zu eligible=%zu bound=%u "
                             "suppressed=%u vitality=%u queen_alive=%d",
                             dispatched, n_eligible,
                             sched->active_bound_workers,
                             sched->suppressed_workers,
                             sched->dynamics->vitality_checksum,
                             (int)sched->dynamics->queen_alive);
        }

    } /* while !queen_done */

    return HIVE_STATUS_OK;
}
