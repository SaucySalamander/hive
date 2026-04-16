#ifndef HIVE_CORE_STATE_MACHINE_H
#define HIVE_CORE_STATE_MACHINE_H

#include "core/types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct hive_runtime hive_runtime_t;
typedef struct hive_state_machine hive_state_machine_t;

/**
 * State machine phases for the hierarchical agent loop.
 */
typedef enum hive_state {
    HIVE_STATE_ORCHESTRATE = 0,
    HIVE_STATE_PLAN,
    HIVE_STATE_CODE,
    HIVE_STATE_TEST,
    HIVE_STATE_VERIFY,
    HIVE_STATE_EDIT,
    HIVE_STATE_EVALUATE,
    HIVE_STATE_DONE,
    HIVE_STATE_ERROR,
    HIVE_STATE_COUNT
} hive_state_t;

/**
 * Function pointer type for a state handler.
 */
typedef hive_status_t (*hive_state_handler_t)(hive_state_machine_t *machine,
                                                      hive_runtime_t *runtime);

/**
 * Pure C state machine with handler table.
 */
struct hive_state_machine {
    hive_state_t state;
    unsigned iteration_limit;
    unsigned score_threshold;
    hive_state_handler_t handlers[HIVE_STATE_COUNT];
};

/**
 * Initialize a state machine.
 *
 * @param machine State machine state.
 * @param iteration_limit Maximum refinement loops.
 * @return HIVE_STATUS_OK on success.
 */
hive_status_t hive_state_machine_init(hive_state_machine_t *machine,
                                              unsigned iteration_limit);

/**
 * Reset a state machine back to the orchestrator phase.
 *
 * @param machine State machine state.
 */
void hive_state_machine_reset(hive_state_machine_t *machine);

/**
 * Run the state machine to completion.
 *
 * @param machine State machine state.
 * @param runtime Runtime state.
 * @return HIVE_STATUS_OK on success.
 */
hive_status_t hive_state_machine_run(hive_state_machine_t *machine,
                                             hive_runtime_t *runtime);

#ifdef __cplusplus
}
#endif

#endif
