/*
 * trace.c — Append-only ring buffer of agent thought processes and side
 * effects.
 */

#include "core/trace/trace.h"
#include "core/scheduler/hive_config.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

/* ----------------------------------------------------------------
 * Internal helpers
 * ---------------------------------------------------------------- */

/** Monotonic timestamp in nanoseconds. */
static uint64_t now_ns(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * UINT64_C(1000000000) + (uint64_t)ts.tv_nsec;
}

/**
 * Copy at most (dst_len - 1) characters from @p src into @p dst, always
 * NUL-terminating.  Equivalent to strlcpy but available everywhere.
 */
static void safe_copy(char *dst, const char *src, size_t dst_len)
{
    if (dst_len == 0) return;
    if (src == NULL) {
        dst[0] = '\0';
        return;
    }
    snprintf(dst, dst_len, "%s", src);
}

#if HIVE_SINGLE_THREADED
static void ring_lock(hive_trace_ring_t *r)   { (void)r; }
static void ring_unlock(hive_trace_ring_t *r) { (void)r; }
#else
static void ring_lock(hive_trace_ring_t *r)   { pthread_mutex_lock(&r->mutex);   }
static void ring_unlock(hive_trace_ring_t *r) { pthread_mutex_unlock(&r->mutex); }
#endif

/* Append one entry under the lock (caller must NOT hold the lock). */
static void append(hive_trace_ring_t *ring, const hive_trace_entry_t *e)
{
    if (ring == NULL) return;
    ring_lock(ring);
    ring->entries[ring->head % HIVE_TRACE_RING_LEN] = *e;
    ring->head++;
    if (ring->count < HIVE_TRACE_RING_LEN) ring->count++;
    ring_unlock(ring);
}

/* ----------------------------------------------------------------
 * Public API
 * ---------------------------------------------------------------- */

void hive_trace_init(hive_trace_ring_t *ring)
{
    if (ring == NULL) return;
    memset(ring, 0, sizeof(*ring));
#if !HIVE_SINGLE_THREADED
    pthread_mutex_init(&ring->mutex, NULL);
#endif
}

void hive_trace_deinit(hive_trace_ring_t *ring)
{
    if (ring == NULL) return;
#if !HIVE_SINGLE_THREADED
    pthread_mutex_destroy(&ring->mutex);
#endif
}

void hive_trace_log_thought(hive_trace_ring_t *ring,
                            int agent_index,
                            const char *agent_name,
                            hive_agent_stage_t stage,
                            const char *prompt,
                            const char *response,
                            const char *critique)
{
    if (ring == NULL) return;

    hive_trace_entry_t e;
    memset(&e, 0, sizeof(e));
    e.kind          = HIVE_TRACE_THOUGHT;
    e.timestamp_ns  = now_ns();
    e.agent_index   = agent_index;
    e.stage         = stage;
    e.score_overall = -1;
    e.tool_kind     = HIVE_TOOL_KIND_COUNT; /* not applicable */

    safe_copy(e.agent_name, agent_name, sizeof(e.agent_name));
    safe_copy(e.summary,    prompt,     sizeof(e.summary));
    safe_copy(e.output,     response,   sizeof(e.output));
    safe_copy(e.critique,   critique,   sizeof(e.critique));

    append(ring, &e);
}

void hive_trace_log_tool(hive_trace_ring_t *ring,
                         int agent_index,
                         const char *agent_name,
                         hive_agent_stage_t stage,
                         hive_tool_kind_t tool_kind,
                         const char *path_or_cmd,
                         const char *result_text,
                         int exit_code)
{
    if (ring == NULL) return;

    hive_trace_entry_t e;
    memset(&e, 0, sizeof(e));
    e.kind          = HIVE_TRACE_TOOL_CALL;
    e.timestamp_ns  = now_ns();
    e.agent_index   = agent_index;
    e.stage         = stage;
    e.score_overall = -1;
    e.tool_kind     = tool_kind;

    safe_copy(e.agent_name, agent_name, sizeof(e.agent_name));

    /* summary = "<kind_name> <path_or_cmd>" */
    snprintf(e.summary, sizeof(e.summary), "%s %s",
             hive_tool_kind_to_string(tool_kind),
             path_or_cmd != NULL ? path_or_cmd : "");

    safe_copy(e.output, result_text, sizeof(e.output));

    /* critique field carries the exit code */
    snprintf(e.critique, sizeof(e.critique), "exit=%d", exit_code);

    append(ring, &e);
}

void hive_trace_log_side_effect(hive_trace_ring_t *ring,
                                int agent_index,
                                const char *agent_name,
                                hive_agent_stage_t stage,
                                const char *description,
                                const char *new_value)
{
    if (ring == NULL) return;

    hive_trace_entry_t e;
    memset(&e, 0, sizeof(e));
    e.kind          = HIVE_TRACE_SIDE_EFFECT;
    e.timestamp_ns  = now_ns();
    e.agent_index   = agent_index;
    e.stage         = stage;
    e.score_overall = -1;
    e.tool_kind     = HIVE_TOOL_KIND_COUNT; /* not applicable */

    safe_copy(e.agent_name,  agent_name,   sizeof(e.agent_name));
    safe_copy(e.summary,     description,  sizeof(e.summary));
    safe_copy(e.output,      new_value,    sizeof(e.output));

    append(ring, &e);
}

unsigned hive_trace_snapshot(const hive_trace_ring_t *ring,
                             hive_trace_entry_t *out,
                             unsigned max_out)
{
    if (ring == NULL || out == NULL || max_out == 0) return 0U;

    /* Cast away const only to acquire the mutex; we do not modify entries. */
    hive_trace_ring_t *mring = (hive_trace_ring_t *)(uintptr_t)ring;

    ring_lock(mring);

    unsigned stored = ring->count < HIVE_TRACE_RING_LEN
                      ? ring->count
                      : HIVE_TRACE_RING_LEN;
    unsigned to_copy = stored < max_out ? stored : max_out;

    /*
     * head points to the slot that will be written NEXT, so:
     *   newest entry lives at (head - 1) mod RING_LEN
     *   oldest entry lives at (head - stored) mod RING_LEN
     *
     * We copy newest-first so index 0 in @out is the most recent entry.
     */
    for (unsigned i = 0; i < to_copy; i++) {
        unsigned idx = (ring->head - 1U - i) % HIVE_TRACE_RING_LEN;
        out[i] = ring->entries[idx];
    }

    ring_unlock(mring);
    return to_copy;
}

const char *hive_trace_kind_to_string(hive_trace_kind_t kind)
{
    switch (kind) {
    case HIVE_TRACE_THOUGHT:     return "thought";
    case HIVE_TRACE_TOOL_CALL:   return "tool";
    case HIVE_TRACE_SIDE_EFFECT: return "effect";
    default:                     return "unknown";
    }
}

/* ----------------------------------------------------------------
 * hive_trace_build_cot_context
 * ---------------------------------------------------------------- */

#define COT_MAX_ENTRIES  16U
#define COT_LINE_CAP     192U   /* max chars contributed per entry */

char *hive_trace_build_cot_context(const hive_trace_ring_t *ring,
                                   int agent_index,
                                   unsigned max_entries)
{
    if (ring == NULL) return NULL;

    if (max_entries == 0U)           max_entries = COT_MAX_ENTRIES;
    if (max_entries > COT_MAX_ENTRIES) max_entries = COT_MAX_ENTRIES;

    /* Take a local snapshot to avoid holding the lock during formatting. */
    hive_trace_entry_t snap[COT_MAX_ENTRIES];
    unsigned avail = hive_trace_snapshot(ring, snap, COT_MAX_ENTRIES);
    if (avail == 0U) return NULL;

    /*
     * Filter and score entries:
     *   - When agent_index != -1, skip entries from other agents.
     *   - SIDE_EFFECT "completed cycle" entries carry the score in output[].
     *     We use them as anchors: remember the last seen score so we can tag
     *     preceding THOUGHT/TOOL lines with it.
     *
     * Snapshot is newest-first; we process oldest-first for coherent context
     * by reversing the relevant slice.
     */

    /* Collect indices of matching entries (oldest-first). */
    unsigned indices[COT_MAX_ENTRIES];
    unsigned n = 0U;
    for (unsigned i = avail; i > 0U && n < max_entries; i--) {
        const hive_trace_entry_t *e = &snap[i - 1U];
        if (agent_index != -1 && e->agent_index != -1 && e->agent_index != agent_index)
            continue;
        indices[n++] = i - 1U;
    }
    if (n == 0U) return NULL;

    /*
     * Format each collected entry into a line then concatenate.
     * Pre-allocate generously: n × COT_LINE_CAP + header.
     */
    size_t buf_cap = (size_t)n * COT_LINE_CAP + 128U;
    char  *buf     = malloc(buf_cap);
    if (buf == NULL) return NULL;

    size_t pos = 0U;

    /* Header */
    int written = snprintf(buf + pos, buf_cap - pos,
                           "=== Chain-of-thought context (last %u steps) ===\n", n);
    if (written > 0) pos += (size_t)written;

    for (unsigned i = 0U; i < n && pos < buf_cap - 1U; i++) {
        const hive_trace_entry_t *e = &snap[indices[i]];
        char line[COT_LINE_CAP];
        int  lw = 0;

        switch (e->kind) {
        case HIVE_TRACE_THOUGHT:
            /* [thought] <agent>/<stage>: "<summary excerpt>" → "<output excerpt>" */
            lw = snprintf(line, sizeof(line),
                          "[thought] %s/%s: \"%.60s\" \xe2\x86\x92 \"%.60s\"\n",
                          e->agent_name[0] != '\0' ? e->agent_name : "?",
                          hive_agent_stage_to_string(e->stage),
                          e->summary,
                          e->output);
            break;

        case HIVE_TRACE_TOOL_CALL:
            /* [tool] <summary> → <output excerpt> (<critique=exit>) */
            lw = snprintf(line, sizeof(line),
                          "[tool] %.80s \xe2\x86\x92 \"%.50s\" (%s)\n",
                          e->summary,
                          e->output,
                          e->critique);
            break;

        case HIVE_TRACE_SIDE_EFFECT:
            /* [effect] <agent>: <description> (<new_value>) */
            lw = snprintf(line, sizeof(line),
                          "[effect] %s: %s (%s)\n",
                          e->agent_name[0] != '\0' ? e->agent_name : "system",
                          e->summary,
                          e->output[0] != '\0' ? e->output : "-");
            break;

        default:
            lw = snprintf(line, sizeof(line), "[?] (unknown entry kind)\n");
            break;
        }

        if (lw > 0 && pos + (size_t)lw < buf_cap) {
            memcpy(buf + pos, line, (size_t)lw);
            pos += (size_t)lw;
        }
    }

    buf[pos] = '\0';
    return buf;
}
