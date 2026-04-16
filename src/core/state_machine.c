#include "core/state_machine.h"

#include "common/strings.h"
#include "core/coder.h"
#include "core/editor.h"
#include "core/evaluator.h"
#include "core/orchestrator.h"
#include "core/planner.h"
#include "core/runtime.h"
#include "core/tester.h"
#include "core/verifier.h"
#include "logging/logger.h"

#include <stdlib.h>
#include <string.h>

static const char *safe_text(const char *text)
{
    return text != NULL ? text : "";
}

static const char *charness_state_to_string(charness_state_t state)
{
    switch (state) {
    case CHARNESS_STATE_ORCHESTRATE:
        return "orchestrate";
    case CHARNESS_STATE_PLAN:
        return "plan";
    case CHARNESS_STATE_CODE:
        return "code";
    case CHARNESS_STATE_TEST:
        return "test";
    case CHARNESS_STATE_VERIFY:
        return "verify";
    case CHARNESS_STATE_EDIT:
        return "edit";
    case CHARNESS_STATE_EVALUATE:
        return "evaluate";
    case CHARNESS_STATE_DONE:
        return "done";
    case CHARNESS_STATE_ERROR:
        return "error";
    default:
        return "unknown";
    }
}

static void replace_owned_text(char **slot, char *replacement)
{
    if (slot == NULL) {
        free(replacement);
        return;
    }

    free(*slot);
    *slot = replacement;
}

static char *build_context_for_planner(const charness_runtime_t *runtime)
{
    return charness_string_format(
        "Supervisor brief:\n%s\n\nUser prompt:\n%s\n\nLast critique:\n%s\n",
        safe_text(runtime->session.orchestrator_brief),
        safe_text(runtime->session.user_prompt),
        safe_text(runtime->session.last_critique));
}

static char *build_context_for_coder(const charness_runtime_t *runtime)
{
    return charness_string_format(
        "Brief:\n%s\n\nPlan:\n%s\n\nLast critique:\n%s\n",
        safe_text(runtime->session.orchestrator_brief),
        safe_text(runtime->session.plan),
        safe_text(runtime->session.last_critique));
}

static char *build_context_for_tester(const charness_runtime_t *runtime)
{
    return charness_string_format(
        "Plan:\n%s\n\nCode draft:\n%s\n\nLast critique:\n%s\n",
        safe_text(runtime->session.plan),
        safe_text(runtime->session.code),
        safe_text(runtime->session.last_critique));
}

static char *build_context_for_verifier(const charness_runtime_t *runtime)
{
    return charness_string_format(
        "Plan:\n%s\n\nCode draft:\n%s\n\nTests:\n%s\n\nLast critique:\n%s\n",
        safe_text(runtime->session.plan),
        safe_text(runtime->session.code),
        safe_text(runtime->session.tests),
        safe_text(runtime->session.last_critique));
}

static char *build_context_for_editor(const charness_runtime_t *runtime)
{
    return charness_string_format(
        "Plan:\n%s\n\nCode draft:\n%s\n\nTests:\n%s\n\nVerification:\n%s\n\nLast critique:\n%s\n",
        safe_text(runtime->session.plan),
        safe_text(runtime->session.code),
        safe_text(runtime->session.tests),
        safe_text(runtime->session.verification),
        safe_text(runtime->session.last_critique));
}

static char *build_context_for_final_orchestrator(const charness_runtime_t *runtime)
{
    return charness_string_format(
        "Iteration %u summary:\n\nPlan:\n%s\n\nCode:\n%s\n\nTests:\n%s\n\nVerification:\n%s\n\nEditor:\n%s\n\nScore:\ncorrectness=%u security=%u style=%u test_coverage=%u overall=%u\n",
        runtime->session.iteration,
        safe_text(runtime->session.plan),
        safe_text(runtime->session.code),
        safe_text(runtime->session.tests),
        safe_text(runtime->session.verification),
        safe_text(runtime->session.edit),
        runtime->session.last_score.correctness,
        runtime->session.last_score.security,
        runtime->session.last_score.style,
        runtime->session.last_score.test_coverage,
        runtime->session.last_score.overall);
}

static charness_status_t handle_orchestrate(charness_state_machine_t *machine, charness_runtime_t *runtime)
{
    (void)machine;

    char *critique = NULL;
    char *brief = NULL;
    charness_status_t status = charness_orchestrator_run(runtime, runtime->session.user_prompt, &critique, &brief);
    if (status != CHARNESS_STATUS_OK) {
        free(critique);
        free(brief);
        return status;
    }

    replace_owned_text(&runtime->session.orchestrator_brief, brief);
    replace_owned_text(&runtime->session.last_critique, critique);
    runtime->session.iteration = 0U;

    if (runtime->logger.initialized) {
        charness_logger_log(&runtime->logger, CHARNESS_LOG_INFO, "state_machine", "orchestrate", "orchestrator brief ready");
    }

    machine->state = CHARNESS_STATE_PLAN;
    return CHARNESS_STATUS_OK;
}

static charness_status_t handle_plan(charness_state_machine_t *machine, charness_runtime_t *runtime)
{
    (void)machine;

    char *context = build_context_for_planner(runtime);
    if (context == NULL) {
        return CHARNESS_STATUS_OUT_OF_MEMORY;
    }

    char *critique = NULL;
    char *plan = NULL;
    charness_status_t status = charness_planner_run(runtime, context, &critique, &plan);
    free(context);
    if (status != CHARNESS_STATUS_OK) {
        free(critique);
        free(plan);
        return status;
    }

    replace_owned_text(&runtime->session.plan, plan);
    replace_owned_text(&runtime->session.last_critique, critique);

    if (runtime->logger.initialized) {
        charness_logger_log(&runtime->logger, CHARNESS_LOG_INFO, "state_machine", "plan", "planner output ready");
    }

    machine->state = CHARNESS_STATE_CODE;
    return CHARNESS_STATUS_OK;
}

static charness_status_t handle_code(charness_state_machine_t *machine, charness_runtime_t *runtime)
{
    (void)machine;

    char *context = build_context_for_coder(runtime);
    if (context == NULL) {
        return CHARNESS_STATUS_OUT_OF_MEMORY;
    }

    char *critique = NULL;
    char *code = NULL;
    charness_status_t status = charness_coder_run(runtime, context, &critique, &code);
    free(context);
    if (status != CHARNESS_STATUS_OK) {
        free(critique);
        free(code);
        return status;
    }

    replace_owned_text(&runtime->session.code, code);
    replace_owned_text(&runtime->session.last_critique, critique);

    if (runtime->logger.initialized) {
        charness_logger_log(&runtime->logger, CHARNESS_LOG_INFO, "state_machine", "code", "coder output ready");
    }

    machine->state = CHARNESS_STATE_TEST;
    return CHARNESS_STATUS_OK;
}

static charness_status_t handle_test(charness_state_machine_t *machine, charness_runtime_t *runtime)
{
    (void)machine;

    char *context = build_context_for_tester(runtime);
    if (context == NULL) {
        return CHARNESS_STATUS_OUT_OF_MEMORY;
    }

    char *critique = NULL;
    char *tests = NULL;
    charness_status_t status = charness_tester_run(runtime, context, &critique, &tests);
    free(context);
    if (status != CHARNESS_STATUS_OK) {
        free(critique);
        free(tests);
        return status;
    }

    replace_owned_text(&runtime->session.tests, tests);
    replace_owned_text(&runtime->session.last_critique, critique);

    if (runtime->logger.initialized) {
        charness_logger_log(&runtime->logger, CHARNESS_LOG_INFO, "state_machine", "test", "tester output ready");
    }

    machine->state = CHARNESS_STATE_VERIFY;
    return CHARNESS_STATUS_OK;
}

static charness_status_t handle_verify(charness_state_machine_t *machine, charness_runtime_t *runtime)
{
    (void)machine;

    char *context = build_context_for_verifier(runtime);
    if (context == NULL) {
        return CHARNESS_STATUS_OUT_OF_MEMORY;
    }

    char *critique = NULL;
    char *verification = NULL;
    charness_status_t status = charness_verifier_run(runtime, context, &critique, &verification);
    free(context);
    if (status != CHARNESS_STATUS_OK) {
        free(critique);
        free(verification);
        return status;
    }

    replace_owned_text(&runtime->session.verification, verification);
    replace_owned_text(&runtime->session.last_critique, critique);

    if (runtime->logger.initialized) {
        charness_logger_log(&runtime->logger, CHARNESS_LOG_INFO, "state_machine", "verify", "verifier output ready");
    }

    machine->state = CHARNESS_STATE_EDIT;
    return CHARNESS_STATUS_OK;
}

static charness_status_t handle_edit(charness_state_machine_t *machine, charness_runtime_t *runtime)
{
    (void)machine;

    char *context = build_context_for_editor(runtime);
    if (context == NULL) {
        return CHARNESS_STATUS_OUT_OF_MEMORY;
    }

    char *critique = NULL;
    char *edit = NULL;
    charness_status_t status = charness_editor_run(runtime, context, &critique, &edit);
    free(context);
    if (status != CHARNESS_STATUS_OK) {
        free(critique);
        free(edit);
        return status;
    }

    replace_owned_text(&runtime->session.edit, edit);
    replace_owned_text(&runtime->session.last_critique, critique);

    if (runtime->logger.initialized) {
        charness_logger_log(&runtime->logger, CHARNESS_LOG_INFO, "state_machine", "edit", "editor output ready");
    }

    machine->state = CHARNESS_STATE_EVALUATE;
    return CHARNESS_STATUS_OK;
}

static unsigned average_score(const charness_score_t *scores, size_t count)
{
    if (scores == NULL || count == 0U) {
        return 0U;
    }

    unsigned total = 0U;
    for (size_t index = 0U; index < count; ++index) {
        total += scores[index].overall;
    }

    return total / (unsigned)count;
}

static charness_status_t handle_evaluate(charness_state_machine_t *machine, charness_runtime_t *runtime)
{
    charness_score_t scores[5];
    if (charness_evaluator_score("Planner", runtime->session.plan, &scores[0]) != CHARNESS_STATUS_OK ||
        charness_evaluator_score("Coder", runtime->session.code, &scores[1]) != CHARNESS_STATUS_OK ||
        charness_evaluator_score("Tester", runtime->session.tests, &scores[2]) != CHARNESS_STATUS_OK ||
        charness_evaluator_score("Verifier", runtime->session.verification, &scores[3]) != CHARNESS_STATUS_OK ||
        charness_evaluator_score("Editor", runtime->session.edit, &scores[4]) != CHARNESS_STATUS_OK) {
        return CHARNESS_STATUS_ERROR;
    }

    runtime->session.last_score.correctness = (scores[0].correctness + scores[1].correctness + scores[2].correctness + scores[3].correctness + scores[4].correctness) / 5U;
    runtime->session.last_score.security = (scores[0].security + scores[1].security + scores[2].security + scores[3].security + scores[4].security) / 5U;
    runtime->session.last_score.style = (scores[0].style + scores[1].style + scores[2].style + scores[3].style + scores[4].style) / 5U;
    runtime->session.last_score.test_coverage = (scores[0].test_coverage + scores[1].test_coverage + scores[2].test_coverage + scores[3].test_coverage + scores[4].test_coverage) / 5U;
    runtime->session.last_score.overall = average_score(scores, 5U);

    if (runtime->logger.initialized) {
        charness_logger_logf(&runtime->logger,
                             CHARNESS_LOG_INFO,
                             "state_machine",
                             "evaluate",
                             "scores: correctness=%u security=%u style=%u test_coverage=%u overall=%u",
                             runtime->session.last_score.correctness,
                             runtime->session.last_score.security,
                             runtime->session.last_score.style,
                             runtime->session.last_score.test_coverage,
                             runtime->session.last_score.overall);
    }

    const bool acceptable = charness_evaluator_is_acceptable(&runtime->session.last_score, machine->score_threshold);
    const bool exhausted = (runtime->session.iteration + 1U) >= machine->iteration_limit;

    free(runtime->session.last_critique);
    runtime->session.last_critique = charness_string_format(
        "Iteration %u score summary: correctness=%u security=%u style=%u test_coverage=%u overall=%u",
        runtime->session.iteration + 1U,
        runtime->session.last_score.correctness,
        runtime->session.last_score.security,
        runtime->session.last_score.style,
        runtime->session.last_score.test_coverage,
        runtime->session.last_score.overall);

    if (runtime->session.last_critique == NULL) {
        return CHARNESS_STATUS_OUT_OF_MEMORY;
    }

    if (acceptable || exhausted) {
        char *context = build_context_for_final_orchestrator(runtime);
        if (context == NULL) {
            return CHARNESS_STATUS_OUT_OF_MEMORY;
        }

        char *critique = NULL;
        char *final_output = NULL;
        charness_status_t status = charness_orchestrator_run(runtime, context, &critique, &final_output);
        free(context);
        if (status != CHARNESS_STATUS_OK) {
            free(critique);
            free(final_output);
            return status;
        }

        replace_owned_text(&runtime->session.last_critique, critique);
        replace_owned_text(&runtime->session.final_output, final_output);
        machine->state = CHARNESS_STATE_DONE;
        return CHARNESS_STATUS_OK;
    }

    runtime->session.iteration += 1U;
    if (runtime->logger.initialized) {
        charness_logger_log(&runtime->logger, CHARNESS_LOG_INFO, "state_machine", "refine", "score below threshold, starting another loop");
    }

    machine->state = CHARNESS_STATE_PLAN;
    return CHARNESS_STATUS_OK;
}

static charness_status_t handle_done(charness_state_machine_t *machine, charness_runtime_t *runtime)
{
    (void)machine;
    (void)runtime;
    return CHARNESS_STATUS_OK;
}

static charness_status_t handle_error(charness_state_machine_t *machine, charness_runtime_t *runtime)
{
    (void)machine;
    (void)runtime;
    return CHARNESS_STATUS_ERROR;
}

charness_status_t charness_state_machine_init(charness_state_machine_t *machine,
                                              unsigned iteration_limit)
{
    if (machine == NULL) {
        return CHARNESS_STATUS_INVALID_ARGUMENT;
    }

    memset(machine, 0, sizeof(*machine));
    machine->state = CHARNESS_STATE_ORCHESTRATE;
    machine->iteration_limit = iteration_limit == 0U ? 1U : iteration_limit;
    machine->score_threshold = 72U;
    machine->handlers[CHARNESS_STATE_ORCHESTRATE] = handle_orchestrate;
    machine->handlers[CHARNESS_STATE_PLAN] = handle_plan;
    machine->handlers[CHARNESS_STATE_CODE] = handle_code;
    machine->handlers[CHARNESS_STATE_TEST] = handle_test;
    machine->handlers[CHARNESS_STATE_VERIFY] = handle_verify;
    machine->handlers[CHARNESS_STATE_EDIT] = handle_edit;
    machine->handlers[CHARNESS_STATE_EVALUATE] = handle_evaluate;
    machine->handlers[CHARNESS_STATE_DONE] = handle_done;
    machine->handlers[CHARNESS_STATE_ERROR] = handle_error;
    return CHARNESS_STATUS_OK;
}

void charness_state_machine_reset(charness_state_machine_t *machine)
{
    if (machine == NULL) {
        return;
    }

    machine->state = CHARNESS_STATE_ORCHESTRATE;
}

charness_status_t charness_state_machine_run(charness_state_machine_t *machine,
                                             charness_runtime_t *runtime)
{
    if (machine == NULL || runtime == NULL) {
        return CHARNESS_STATUS_INVALID_ARGUMENT;
    }

    while (machine->state != CHARNESS_STATE_DONE && machine->state != CHARNESS_STATE_ERROR) {
        const charness_state_handler_t handler = machine->handlers[machine->state];
        if (handler == NULL) {
            machine->state = CHARNESS_STATE_ERROR;
            break;
        }

        const charness_status_t status = handler(machine, runtime);
        if (status != CHARNESS_STATUS_OK) {
            if (runtime->logger.initialized) {
                charness_logger_logf(&runtime->logger,
                                     CHARNESS_LOG_ERROR,
                                     "state_machine",
                                     "handler_failed",
                                     "state %s failed with %s",
                                     charness_state_to_string(machine->state),
                                     charness_status_to_string(status));
            }

            free(runtime->session.last_error);
            runtime->session.last_error = charness_string_format("state %s failed with %s",
                                                                 charness_state_to_string(machine->state),
                                                                 charness_status_to_string(status));
            machine->state = CHARNESS_STATE_ERROR;
            return status;
        }
    }

    return machine->state == CHARNESS_STATE_DONE ? CHARNESS_STATUS_OK : CHARNESS_STATUS_ERROR;
}
