#ifndef HIVE_CORE_INFERENCE_ADAPTERS_COPILOTCLI_H
#define HIVE_CORE_INFERENCE_ADAPTERS_COPILOTCLI_H

#include "core/inference/inference.h"

#ifdef __cplusplus
extern "C" {
#endif

extern const inf_adapter_vtable_t copilotcli_adapter_vtable;

int copilotcli_adapter_register(void);

#ifdef __cplusplus
}
#endif

#endif
