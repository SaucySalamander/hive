#ifndef HIVE_API_SERVER_H
#define HIVE_API_SERVER_H

#include "core/types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct hive_runtime hive_runtime_t;

/**
 * Run the optional HTTP/JSON API server, or return unavailable when disabled.
 *
 * @param runtime Runtime state.
 * @return HIVE_STATUS_OK on success.
 */
hive_status_t hive_api_server_run(hive_runtime_t *runtime);

#ifdef __cplusplus
}
#endif

#endif
