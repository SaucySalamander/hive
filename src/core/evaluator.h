#ifndef CHARNESS_CORE_EVALUATOR_H
#define CHARNESS_CORE_EVALUATOR_H

#include "core/types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Score a stage output against the project heuristics.
 *
 * @param agent_name Name of the stage or agent being scored.
 * @param output Output text to score.
 * @param score_out Filled with the resulting score bundle.
 * @return CHARNESS_STATUS_OK on success.
 */
charness_status_t charness_evaluator_score(const char *agent_name,
                                           const char *output,
                                           charness_score_t *score_out);

/**
 * Decide whether a score is good enough to stop another refinement loop.
 *
 * @param score Score bundle.
 * @param threshold Minimum overall score.
 * @return true when the score is acceptable.
 */
bool charness_evaluator_is_acceptable(const charness_score_t *score, unsigned threshold);

#ifdef __cplusplus
}
#endif

#endif
