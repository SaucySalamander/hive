#include "core/state_machine.h"

#include "common/strings.h"
#include "core/agent/coder.h"
#include "core/agent/editor.h"
#include "core/evaluator/evaluator.h"
#include "core/agent/orchestrator.h"
#include "core/agent/planner.h"
#include "core/runtime.h"
#include "core/agent/tester.h"
#include "core/agent/verifier.h"
#include "common/logging/logger.h"

#include <stdlib.h>
#include <string.h>

static const char *safe_text(const char *text)
{
    return text != NULL ? text : "";
}

static const char *hive_state_to_string(hive_state_t state)
{
    switch (state) {
    case HIVE_STATE_ORCHESTRATE:
        return "orchestrate";
    case HIVE_STATE_PLAN:
        return "plan";
    case HIVE_STATE_CODE:
        return "code";
    case HIVE_STATE_TEST:
        return "test";
    case HIVE_STATE_VERIFY:
        return "verify";
    case HIVE_STATE_EDIT:
        return "edit";
    case HIVE_STATE_EVALUATE:
        return "evaluate";
    case HIVE_STATE_DONE:
        return "done";
    case HIVE_STATE_ERROR:
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

static char *build_context_for_planner(const hive_runtime_t *runtime)
{
    return hive_string_format(
        "Supervisor brief:\n%s\n\nUser prompt:\n%s\n\nLast critique:\n%s\n",
        safe_text(runtime->session.orchestrator_brief),
        safe_text(runtime->session.user_prompt),
        safe_text(runtime->session.last_critique));
}

static char *build_context_for_coder(const hive_runtime_t *runtime)
{
    return hive_string_format(
        "Brief:\n%s\n\nPlan:\n%s\n\nLast critique:\n%s\n",
        safe_text(runtime->session.orchestrator_brief),
        safe_text(runtime->session.plan),
        safe_text(runtime->session.last_critique));
}

static char *build_context_for_tester(const hive_runtime_t *runtime)
{
    return hive_string_format(
        "Plan:\n%s\n\nCode draft:\n%s\n\nLast critique:\n%s\n",
        safe_text(runtime->session.plan),
        safe_text(runtime->session.code),
        safe_text(runtime->session.last_critique));
}

static char *build_context_for_verifier(const hive_runtime_t *runtime)
{
    return hive_string_format(
        "Plan:\n%s\n\nCode draft:\n%s\n\nTests:\n%s\n\nLast critique:\n%s\n",
        safe_text(runtime->session.plan),
        safe_text(runtime->session.code),
        safe_text(runtime->session.tests),
        safe_text(runtime->session.last_critique));
}

static char *build_context_for_editor(const hive_runtime_t *runtime)
{
    return hive_string_format(
        "Plan:\n%s\n\nCode draft:\n%s\n\nTests:\n%s\n\nVerification:\n%s\n\nLast critique:\n%s\n",
        safe_text(runtime->session.plan),
        safe_text(runtime->session.code),
        safe_text(runtime->session.tests),
        safe_text(runtime->session.verification),
        safe_text(runtime->session.last_critique));
}

static char *build_context_for_final_orchestrator(const hive_runtime_t *runtime)
{
    return hive_string_format(
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

static hive_status_t handle_orchestrate(hive_state_machine_t *machine, hive_runtime_t *runtime)
{
    (void)machine;

    char *critique = NULL;
    char *brief = NULL;
    hive_status_t status = hive_orchestrator_run(runtime, runtime->session.user_prompt, &critique, &brief);
    if (status != HIVE_STATUS_OK) {
        free(critique);
        free(brief);
        return status;
    }

    replace_owned_text(&runtime->session.orchestrator_brief, brief);
    replace_owned_text(&runtime->session.last_critique, critique);
    runtime->session.iteration = 0U;

    if (runtime->logger.initialized) {
        hive_logger_log(&runtime->logger, HIVE_LOG_INFO, "state_machine", "orchestrate", "orchestrator brief ready");
    }

    machine->state = HIVE_STATE_PLAN;
    return HIVE_STATUS_OK;
}

static hive_status_t handle_plan(hive_state_machine_t *machine, hive_runtime_t *runtime)
{
    (void)machine;

    char *context = build_context_for_planner(runtime);
    if (context == NULL) {
        return HIVE_STATUS_OUT_OF_MEMORY;
    }

    char *critique = NULL;
    char *plan = NULL;
    hive_status_t status = hive_planner_run(runtime, context, &critique, &plan);
    free(context);
    if (status != HIVE_STATUS_OK) {
        free(critique);
        free(plan);
        return status;
    }

    replace_owned_text(&runtime->session.plan, plan);
    replace_owned_text(&runtime->session.last_critique, critique);

    if (runtime->logger.initialized) {
        hive_logger_log(&runtime->logger, HIVE_LOG_INFO, "state_machine", "plan", "planner output ready");
    }

    machine->state = HIVE_STATE_CODE;
    return HIVE_STATUS_OK;
}

static hive_status_t handle_code(hive_state_machine_t *machine, hive_runtime_t *runtime)
{
    (void)machine;

    char *context = build_context_for_coder(runtime);
    if (context == NULL) {
        return HIVE_STATUS_OUT_OF_MEMORY;
    }

    char *critique = NULL;
    char *code = NULL;
    hive_status_t status = hive_coder_run(runtime, context, &critique, &code);
    free(context);
    if (status != HIVE_STATUS_OK) {
        free(critique);
        free(code);
        return status;
    }

    replace_owned_text(&runtime->session.code, code);
    replace_owned_text(&runtime->session.last_critique, critique);

    if (runtime->logger.initialized) {
        hive_logger_log(&runtime->logger, HIVE_LOG_INFO, "state_machine", "code", "coder output ready");
    }

    machine->state = HIVE_STATE_TEST;
    return HIVE_STATUS_OK;
}

static hive_status_t handle_test(hive_state_machine_t *machine, hive_runtime_t *runtime)
{
    (void)machine;

    char *context = build_context_for_tester(runtime);
    if (context == NULL) {
        return HIVE_STATUS_OUT_OF_MEMORY;
    }

    char *critique = NULL;
    char *tests = NULL;
    hive_status_t status = hive_tester_run(runtime, context, &critique, &tests);
    free(context);
    if (status != HIVE_STATUS_OK) {
        free(critique);
        free(tests);
        return status;
    }

    replace_owned_text(&runtime->session.tests, tests);
    replace_owned_text(&runtime->session.last_critique, critique);

    if (runtime->logger.initialized) {
        hive_logger_log(&runtime->logger, HIVE_LOG_INFO, "state_machine", "test", "tester output ready");
    }

    machine->state = HIVE_STATE_VERIFY;
    return HIVE_STATUS_OK;
}

static hive_status_t handle_verify(hive_state_machine_t *machine, hive_runtime_t *runtime)
{
    (void)machine;

    char *context = build_context_for_verifier(runtime);
    if (context == NULL) {
        return HIVE_STATUS_OUT_OF_MEMORY;
    }

    char *critique = NULL;
    char *verification = NULL;
    hive_status_t status = hive_verifier_run(runtime, context, &critique, &verification);
    free(context);
    if (status != HIVE_STATUS_OK) {
        free(critique);
        free(verification);
        return status;
    }

    replace_owned_text(&runtime->session.verification, verification);
    replace_owned_text(&runtime->session.last_critique, critique);

    if (runtime->logger.initialized) {
        hive_logger_log(&runtime->logger, HIVE_LOG_INFO, "state_machine", "verify", "verifier output ready");
    }

    machine->state = HIVE_STATE_EDIT;
    return HIVE_STATUS_OK;
}

static hive_status_t handle_edit(hive_state_machine_t *machine, hive_runtime_t *runtime)
{
    (void)machine;

    char *context = build_context_for_editor(runtime);
    if (context == NULL) {
        return HIVE_STATUS_OUT_OF_MEMORY;
    }

    char *critique = NULL;
    char *edit = NULL;
    hive_status_t status = hive_editor_run(runtime, context, &critique, &edit);
    free(context);
    if (status != HIVE_STATUS_OK) {
        free(critique);
        free(edit);
        return status;
    }

    replace_owned_text(&runtime->session.edit, edit);
    replace_owned_text(&runtime->session.last_critique, critique);

    if (runtime->logger.initialized) {
        hive_logger_log(&runtime->logger, HIVE_LOG_INFO, "state_machine", "edit", "editor output ready");
    }

    machine->state = HIVE_STATE_EVALUATE;
    return HIVE_STATUS_OK;
}

static unsigned average_score(const hive_score_t *scores, size_t count)
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

static hive_status_t handle_evaluate(hive_state_machine_t *machine, hive_runtime_t *runtime)
{
    hive_score_t scores[5];
    if (hive_evaluator_score("Planner", runtime->session.plan, &scores[0]) != HIVE_STATUS_OK ||
        hive_evaluator_score("Coder", runtime->session.code, &scores[1]) != HIVE_STATUS_OK ||
        hive_evaluator_score("Tester", runtime->session.tests, &scores[2]) != HIVE_STATUS_OK ||
        hive_evaluator_score("Verifier", runtime->session.verification, &scores[3]) != HIVE_STATUS_OK ||
        hive_evaluator_score("Editor", runtime->session.edit, &scores[4]) != HIVE_STATUS_OK) {
        return HIVE_STATUS_ERROR;
    }

    runtime->session.last_score.correctness = (scores[0].correctness + scores[1].correctness + scores[2].correctness + scores[3].correctness + scores[4].correctness) / 5U;
    runtime->session.last_score.security = (scores[0].security + scores[1].security + scores[2].security + scores[3].security + scores[4].security) / 5U;
    runtime->session.last_score.style = (scores[0].style + scores[1].style + scores[2].style + scores[3].style + scores[4].style) / 5U;
    runtime->session.last_score.test_coverage = (scores[0].test_coverage + scores[1].test_coverage + scores[2].test_coverage + scores[3].test_coverage + scores[4].test_coverage) / 5U;
    runtime->session.last_score.overall = average_score(scores, 5U);

    if (runtime->logger.initialized) {
        hive_logger_logf(&runtime->logger,
                             HIVE_LOG_INFO,
                             "state_machine",
                             "evaluate",
                             "scores: correctness=%u security=%u style=%u test_coverage=%u overall=%u",
                             runtime->session.last_score.correctness,
                             runtime->session.last_score.security,
                             runtime->session.last_score.style,
                             runtime->session.last_score.test_coverage,
                             runtime->session.last_score.overall);
    }

    const bool acceptable = hive_evaluator_is_acceptable(&runtime->session.last_score, machine->score_threshold);
    const bool exhausted = (runtime->session.iteration + 1U) >= machine->iteration_limit;

    free(runtime->session.last_critique);
    runtime->session.last_critique = hive_string_format(
        "Iteration %u score summary: correctness=%u security=%u style=%u test_coverage=%u overall=%u",
        runtime->session.iteration + 1U,
        runtime->session.last_score.correctness,
        runtime->session.last_score.security,
        runtime->session.last_score.style,
        runtime->session.last_score.test_coverage,
        runtime->session.last_score.overall);

    if (runtime->session.last_critique == NULL) {
        return HIVE_STATUS_OUT_OF_MEMORY;
    }

    if (acceptable || exhausted) {
        char *context = build_context_for_final_orchestrator(runtime);
        if (context == NULL) {
            return HIVE_STATUS_OUT_OF_MEMORY;
        }

        char *critique = NULL;
        char *final_output = NULL;
        hive_status_t status = hive_orchestrator_run(runtime, context, &critique, &final_output);
        free(context);
        if (status != HIVE_STATUS_OK) {
            free(critique);
            free(final_output);
            return status;
        }

        replace_owned_text(&runtime->session.last_critique, critique);
        replace_owned_text(&runtime->session.final_output, final_output);
        machine->state = HIVE_STATE_DONE;
        return HIVE_STATUS_OK;
    }

    runtime->session.iteration += 1U;
    if (runtime->logger.initialized) {
        hive_logger_log(&runtime->logger, HIVE_LOG_INFO, "state_machine", "refine", "score below threshold, starting another loop");
    }

    machine->state = HIVE_STATE_PLAN;
    return HIVE_STATUS_OK;
}

static hive_status_t handle_done(hive_state_machine_t *machine, hive_runtime_t *runtime)
{
    (void)machine;
    (void)runtime;
    return HIVE_STATUS_OK;
}

static hive_status_t handle_error(hive_state_machine_t *machine, hive_runtime_t *runtime)
{
    (void)machine;
    (void)runtime;
    return HIVE_STATUS_ERROR;
}

hive_status_t hive_state_machine_init(hive_state_machine_t *machine,
                                              unsigned iteration_limit)
{
    if (machine == NULL) {
        return HIVE_STATUS_INVALID_ARGUMENT;
    }

    memset(machine, 0, sizeof(*machine));
    machine->state = HIVE_STATE_ORCHESTRATE;
    machine->iteration_limit = iteration_limit == 0U ? 1U : iteration_limit;
    machine->score_threshold = 72U;
    machine->handlers[HIVE_STATE_ORCHESTRATE] = handle_orchestrate;
    machine->handlers[HIVE_STATE_PLAN] = handle_plan;
    machine->handlers[HIVE_STATE_CODE] = handle_code;
    machine->handlers[HIVE_STATE_TEST] = handle_test;
    machine->handlers[HIVE_STATE_VERIFY] = handle_verify;
    machine->handlers[HIVE_STATE_EDIT] = handle_edit;
    machine->handlers[HIVE_STATE_EVALUATE] = handle_evaluate;
    machine->handlers[HIVE_STATE_DONE] = handle_done;
    machine->handlers[HIVE_STATE_ERROR] = handle_error;
    return HIVE_STATUS_OK;
}

void hive_state_machine_reset(hive_state_machine_t *machine)
{
    if (machine == NULL) {
        return;
    }

    machine->state = HIVE_STATE_ORCHESTRATE;
}

hive_status_t hive_state_machine_run(hive_state_machine_t *machine,
                                             hive_runtime_t *runtime)
{
    if (machine == NULL || runtime == NULL) {
        return HIVE_STATUS_INVALID_ARGUMENT;
    }

    while (machine->state != HIVE_STATE_DONE && machine->state != HIVE_STATE_ERROR) {
        const hive_state_handler_t handler = machine->handlers[machine->state];
        if (handler == NULL) {
            machine->state = HIVE_STATE_ERROR;
            break;
        }

        const hive_status_t status = handler(machine, runtime);
        if (status != HIVE_STATUS_OK) {
            if (runtime->logger.initialized) {
                hive_logger_logf(&runtime->logger,
                                     HIVE_LOG_ERROR,
                                     "state_machine",
                                     "handler_failed",
                                     "state %s failed with %s",
                                     hive_state_to_string(machine->state),
                                     hive_status_to_string(status));
            }

            free(runtime->session.last_error);
            runtime->session.last_error = hive_string_format("state %s failed with %s",
                                                                 hive_state_to_string(machine->state),
                                                                 hive_status_to_string(status));
            machine->state = HIVE_STATE_ERROR;
            return status;
        }
    }

    return machine->state == HIVE_STATE_DONE ? HIVE_STATUS_OK : HIVE_STATUS_ERROR;
}
