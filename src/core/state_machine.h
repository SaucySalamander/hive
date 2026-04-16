#ifndef CHARNESS_CORE_STATE_MACHINE_H
#define CHARNESS_CORE_STATE_MACHINE_H

#include "core/types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct charness_runtime charness_runtime_t;
typedef struct charness_state_machine charness_state_machine_t;

/**
 * State machine phases for the hierarchical agent loop.
 */
typedef enum charness_state {
    CHARNESS_STATE_ORCHESTRATE = 0,
    CHARNESS_STATE_PLAN,
    CHARNESS_STATE_CODE,
    CHARNESS_STATE_TEST,
    CHARNESS_STATE_VERIFY,
    CHARNESS_STATE_EDIT,
    CHARNESS_STATE_EVALUATE,
    CHARNESS_STATE_DONE,
    CHARNESS_STATE_ERROR,
    CHARNESS_STATE_COUNT
} charness_state_t;

/**
 * Function pointer type for a state handler.
 */
typedef charness_status_t (*charness_state_handler_t)(charness_state_machine_t *machine,
                                                      charness_runtime_t *runtime);

/**
 * Pure C state machine with handler table.
 */
struct charness_state_machine {
    charness_state_t state;
    unsigned iteration_limit;
    unsigned score_threshold;
    charness_state_handler_t handlers[CHARNESS_STATE_COUNT];
};

/**
 * Initialize a state machine.
 *
 * @param machine State machine state.
 * @param iteration_limit Maximum refinement loops.
 * @return CHARNESS_STATUS_OK on success.
 */
charness_status_t charness_state_machine_init(charness_state_machine_t *machine,
                                              unsigned iteration_limit);

/**
 * Reset a state machine back to the orchestrator phase.
 *
 * @param machine State machine state.
 */
void charness_state_machine_reset(charness_state_machine_t *machine);

/**
 * Run the state machine to completion.
 *
 * @param machine State machine state.
 * @param runtime Runtime state.
 * @return CHARNESS_STATUS_OK on success.
 */
charness_status_t charness_state_machine_run(charness_state_machine_t *machine,
                                             charness_runtime_t *runtime);

#ifdef __cplusplus
}
#endif

#endif
