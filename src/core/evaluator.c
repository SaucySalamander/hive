#include "core/evaluator.h"

#include <stddef.h>
#include <string.h>

static const char *safe_text(const char *text)
{
    return text != NULL ? text : "";
}

static unsigned clamp_score(unsigned value)
{
    return value > 100U ? 100U : value;
}

static unsigned count_lines(const char *text)
{
    unsigned lines = 1U;
    for (const char *cursor = safe_text(text); *cursor != '\0'; ++cursor) {
        if (*cursor == '\n') {
            ++lines;
        }
    }
    return lines;
}

static bool contains_any(const char *text, const char *const needles[])
{
    const char *haystack = safe_text(text);
    for (size_t index = 0U; needles[index] != NULL; ++index) {
        if (strstr(haystack, needles[index]) != NULL) {
            return true;
        }
    }
    return false;
}

hive_status_t hive_evaluator_score(const char *agent_name,
                                           const char *output,
                                           hive_score_t *score_out)
{
    if (score_out == NULL) {
        return HIVE_STATUS_INVALID_ARGUMENT;
    }

    memset(score_out, 0, sizeof(*score_out));

    const char *safe_agent = safe_text(agent_name);
    const char *safe_output = safe_text(output);
    const size_t length = strlen(safe_output);

    if (length > 0U) {
        score_out->correctness = 50U;
        if (length > 120U) {
            score_out->correctness += 20U;
        }
        if (strstr(safe_output, safe_agent) != NULL) {
            score_out->correctness += 10U;
        }
        if (strstr(safe_output, "plan") != NULL || strstr(safe_output, "test") != NULL || strstr(safe_output, "verify") != NULL) {
            score_out->correctness += 10U;
        }
    }

    score_out->security = 95U;
    const char *security_penalties[] = {
        "system(",
        "popen(",
        "strcpy",
        "gets(",
        "rm -rf",
        "TODO",
        NULL,
    };
    if (contains_any(safe_output, security_penalties)) {
        score_out->security = 55U;
    }

    score_out->style = 55U;
    if (count_lines(safe_output) > 1U) {
        score_out->style += 15U;
    }
    if (length > 80U) {
        score_out->style += 10U;
    }
    if (strstr(safe_output, "#") != NULL || strstr(safe_output, "-") != NULL) {
        score_out->style += 10U;
    }

    if (strstr(safe_agent, "Tester") != NULL || strstr(safe_agent, "Verifier") != NULL) {
        score_out->test_coverage = 70U;
        if (strstr(safe_output, "test") != NULL || strstr(safe_output, "assert") != NULL) {
            score_out->test_coverage += 20U;
        }
    } else {
        score_out->test_coverage = length > 0U ? 40U : 0U;
    }

    score_out->correctness = clamp_score(score_out->correctness);
    score_out->security = clamp_score(score_out->security);
    score_out->style = clamp_score(score_out->style);
    score_out->test_coverage = clamp_score(score_out->test_coverage);
    score_out->overall = clamp_score((score_out->correctness * 40U +
                                      score_out->security * 25U +
                                      score_out->style * 20U +
                                      score_out->test_coverage * 15U) / 100U);

    return HIVE_STATUS_OK;
}

bool hive_evaluator_is_acceptable(const hive_score_t *score, unsigned threshold)
{
    if (score == NULL) {
        return false;
    }

    return score->overall >= threshold && score->security >= 60U && score->correctness >= 50U;
}
