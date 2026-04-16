#ifndef HIVE_RAYGUI_RAYGUI_H
#define HIVE_RAYGUI_RAYGUI_H

#include "core/types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct hive_runtime hive_runtime_t;

/**
 * Run the RayGUI (VSCode-styled) graphical frontend, or return unavailable
 * when optional GUI support is not enabled.
 *
 * @param runtime Runtime state.
 * @return HIVE_STATUS_OK on success.
 */
hive_status_t hive_raygui_run(hive_runtime_t *runtime);

#ifdef __cplusplus
}
#endif

#endif
