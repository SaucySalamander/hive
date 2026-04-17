#ifndef HIVE_GTK_GTK_H
#define HIVE_GTK_GTK_H

#include "core/types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct hive_runtime hive_runtime_t;

hive_status_t hive_gtk_run(hive_runtime_t *runtime);

#ifdef __cplusplus
}
#endif

#endif
