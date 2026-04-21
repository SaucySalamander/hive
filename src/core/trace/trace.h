/*
 * trace.h — Append-only ring buffer of agent thought processes and side
 * effects.
 *
 * Every inference call (THOUGHT), tool execution (TOOL_CALL), and scheduler
 * mutation (SIDE_EFFECT) is recorded here with a timestamp, the originating
 * agent index/name, the stage, and truncated excerpts of the prompt, output,
 * and critique.
 *
 * The ring buffer is a fixed-size in-place array — no heap allocation per
 * entry — so it never fails or blocks.  Old entries are overwritten once the
 * buffer wraps.
 *
 * Thread safety: all writes are serialised by an embedded mutex.  Readers
 * (e.g. GTK UI thread) should take a snapshot with hive_trace_snapshot().
 */

#ifndef HIVE_CORE_TRACE_H
#define HIVE_CORE_TRACE_H

#include "core/scheduler/scheduler.h"  /* hive_agent_stage_t */
#include "tools/registry.h"            /* hive_tool_kind_t  */

#if !HIVE_SINGLE_THREADED
#  include <pthread.h>
#endif

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ----------------------------------------------------------------
 * Sizing constants
 * ---------------------------------------------------------------- */

#ifndef HIVE_TRACE_RING_LEN
#  define HIVE_TRACE_RING_LEN 2048U
#endif

/** Maximum character length for each text field in a trace entry. */
#define HIVE_TRACE_TEXT_LEN  256U
/** Maximum length for the agent name stored per entry. */
#define HIVE_TRACE_NAME_LEN  48U

/* ----------------------------------------------------------------
 * Entry kind
 * ---------------------------------------------------------------- */

typedef enum hive_trace_kind {
    HIVE_TRACE_THOUGHT     = 0,  /**< Inference call: prompt → response → critique  */
    HIVE_TRACE_TOOL_CALL   = 1,  /**< Tool invocation: kind + path/cmd → result      */
    HIVE_TRACE_SIDE_EFFECT = 2,  /**< Scheduler mutation: spawn, session write, score */
    HIVE_TRACE_KIND_COUNT
} hive_trace_kind_t;

/* ----------------------------------------------------------------
 * Trace entry (fixed-size, no heap pointers)
 * ---------------------------------------------------------------- */

typedef struct hive_trace_entry {
    hive_trace_kind_t   kind;
    uint64_t            timestamp_ns;           /**< CLOCK_MONOTONIC nanoseconds      */
    int                 agent_index;            /**< cell slot; -1 = system           */
    hive_agent_stage_t  stage;                  /**< pipeline stage at time of entry  */
    char                agent_name[HIVE_TRACE_NAME_LEN];
    /** For THOUGHT: first N chars of the composed prompt.
     *  For TOOL_CALL: tool kind name + " " + path/command.
     *  For SIDE_EFFECT: short description of the mutation.  */
    char                summary[HIVE_TRACE_TEXT_LEN];
    /** For THOUGHT: first N chars of the inference response.
     *  For TOOL_CALL: first N chars of the tool output.
     *  For SIDE_EFFECT: new value or result.                */
    char                output[HIVE_TRACE_TEXT_LEN];
    /** For THOUGHT: first N chars of the reflexion critique.
     *  For TOOL_CALL: exit code as decimal string.
     *  For SIDE_EFFECT: empty.                              */
    char                critique[HIVE_TRACE_TEXT_LEN];
    int                 score_overall;          /**< -1 = not applicable              */
    hive_tool_kind_t    tool_kind;              /**< only valid for HIVE_TRACE_TOOL_CALL */
} hive_trace_entry_t;

/* ----------------------------------------------------------------
 * Ring buffer
 * ---------------------------------------------------------------- */

typedef struct hive_trace_ring {
    hive_trace_entry_t  entries[HIVE_TRACE_RING_LEN];
    unsigned            head;     /**< next write index (wraps at HIVE_TRACE_RING_LEN) */
    unsigned            count;    /**< total entries ever written (may exceed RING_LEN) */
#if !HIVE_SINGLE_THREADED
    pthread_mutex_t     mutex;
#endif
} hive_trace_ring_t;

/* ----------------------------------------------------------------
 * Public API
 * ---------------------------------------------------------------- */

/**
 * Initialise a trace ring.  Must be called once before any log calls.
 */
void hive_trace_init(hive_trace_ring_t *ring);

/**
 * Destroy a trace ring (releases mutex).
 */
void hive_trace_deinit(hive_trace_ring_t *ring);

/**
 * Record an inference THOUGHT entry.
 *
 * @param ring         Trace ring.
 * @param agent_index  Cell slot index; pass -1 for the system.
 * @param agent_name   Human-readable agent name.
 * @param stage        Current pipeline stage.
 * @param prompt       Full prompt text (will be truncated).
 * @param response     Raw inference response (will be truncated).
 * @param critique     Reflexion critique (will be truncated; may be NULL).
 */
void hive_trace_log_thought(hive_trace_ring_t *ring,
                            int agent_index,
                            const char *agent_name,
                            hive_agent_stage_t stage,
                            const char *prompt,
                            const char *response,
                            const char *critique);

/**
 * Record a TOOL_CALL entry.
 *
 * @param ring         Trace ring.
 * @param agent_index  Cell slot index; -1 for unknown.
 * @param agent_name   Human-readable agent name.
 * @param stage        Current pipeline stage.
 * @param tool_kind    Which tool was invoked.
 * @param path_or_cmd  Path, pattern, or command string.
 * @param result_text  Tool output (will be truncated; may be NULL).
 * @param exit_code    Tool exit code.
 */
void hive_trace_log_tool(hive_trace_ring_t *ring,
                         int agent_index,
                         const char *agent_name,
                         hive_agent_stage_t stage,
                         hive_tool_kind_t tool_kind,
                         const char *path_or_cmd,
                         const char *result_text,
                         int exit_code);

/**
 * Record a SIDE_EFFECT entry.
 *
 * @param ring         Trace ring.
 * @param agent_index  Cell slot index; -1 for scheduler/system.
 * @param agent_name   Human-readable agent name.
 * @param stage        Current pipeline stage.
 * @param description  Short human-readable description of the mutation.
 * @param new_value    New value or result string (may be NULL).
 */
void hive_trace_log_side_effect(hive_trace_ring_t *ring,
                                int agent_index,
                                const char *agent_name,
                                hive_agent_stage_t stage,
                                const char *description,
                                const char *new_value);

/**
 * Copy up to @p max_out entries into the caller's buffer, newest first.
 *
 * @param ring     Trace ring.
 * @param out      Caller-allocated array of at least @p max_out entries.
 * @param max_out  Maximum entries to copy.
 * @return Number of entries actually copied.
 */
unsigned hive_trace_snapshot(const hive_trace_ring_t *ring,
                             hive_trace_entry_t *out,
                             unsigned max_out);

/**
 * Convert a trace kind to a stable string ("thought", "tool", "effect").
 */
const char *hive_trace_kind_to_string(hive_trace_kind_t kind);

/**
 * Build a concise Chain-of-Thought context string from recent trace entries
 * for inclusion in an agent prompt.
 *
 * The function walks the ring buffer newest-first and collects up to
 * @p max_entries entries, favouring high-scoring cycles.  The returned string
 * is formatted as a compact human-readable block that fits inside a prompt
 * without inflating token count excessively.
 *
 * Ownership: the caller must free() the returned pointer.  Returns NULL on
 * allocation failure or if the ring contains no relevant entries.
 *
 * @param ring         Trace ring.
 * @param agent_index  Filter to entries from this cell slot.  Pass -1 to
 *                     include all agents (useful for Queen / cross-agent CoT).
 * @param max_entries  Maximum number of entries to format (capped at 16).
 * @return Heap-allocated NUL-terminated string, or NULL.
 */
char *hive_trace_build_cot_context(const hive_trace_ring_t *ring,
                                   int agent_index,
                                   unsigned max_entries);

#ifdef __cplusplus
}
#endif

#endif /* HIVE_CORE_TRACE_H */
