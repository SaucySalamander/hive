#ifndef HIVE_CORE_EVALUATOR_H
#define HIVE_CORE_EVALUATOR_H

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
 * @return HIVE_STATUS_OK on success.
 */
hive_status_t hive_evaluator_score(const char *agent_name,
                                           const char *output,
                                           hive_score_t *score_out);

/**
 * Decide whether a score is good enough to stop another refinement loop.
 *
 * @param score Score bundle.
 * @param threshold Minimum overall score.
 * @return true when the score is acceptable.
 */
bool hive_evaluator_is_acceptable(const hive_score_t *score, unsigned threshold);

#ifdef __cplusplus
}
#endif

#endif
