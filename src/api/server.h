#ifndef CHARNESS_API_SERVER_H
#define CHARNESS_API_SERVER_H

#include "core/types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct charness_runtime charness_runtime_t;

/**
 * Run the optional HTTP/JSON API server, or return unavailable when disabled.
 *
 * @param runtime Runtime state.
 * @return CHARNESS_STATUS_OK on success.
 */
charness_status_t charness_api_server_run(charness_runtime_t *runtime);

#ifdef __cplusplus
}
#endif

#endif
