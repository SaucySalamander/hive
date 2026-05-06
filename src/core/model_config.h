#ifndef HIVE_CORE_MODEL_CONFIG_H
#define HIVE_CORE_MODEL_CONFIG_H

#include "core/types.h"
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Maximum models and role model mappings */
#define HIVE_MAX_MODELS 16
#define HIVE_MAX_ROLE_MODELS HIVE_AGENT_KIND_COUNT

/**
 * Per-role model selection.
 */
typedef struct hive_role_model_mapping {
    const char *role_name;
    const char *model_name;
} hive_role_model_mapping_t;

/**
 * Model configuration state.
 * Loaded at runtime from hive-model.toml.
 */
typedef struct hive_model_config {
    /* Available models */
    const char *models[HIVE_MAX_MODELS];
    size_t model_count;

    /* Role-based model defaults */
    hive_role_model_mapping_t role_models[HIVE_MAX_ROLE_MODELS];
    size_t role_model_count;

    /* Global default model */
    const char *default_model;
} hive_model_config_t;

/**
 * Load model configuration from a TOML file.
 * Allocates strings that must be freed by hive_model_config_free().
 *
 * @param config Configuration structure to populate.
 * @param path Path to hive-model.toml file.
 * @return HIVE_STATUS_OK on success.
 */
hive_status_t hive_model_config_load(hive_model_config_t *config, const char *path);

/**
 * Free all allocated memory in model config.
 *
 * @param config Configuration structure to free.
 */
void hive_model_config_free(hive_model_config_t *config);

/**
 * Get the default model for a given agent kind.
 *
 * @param config Model configuration.
 * @param kind Agent kind (HIVE_AGENT_CODER, HIVE_AGENT_TESTER, etc).
 * @return Model name, or config->default_model if no mapping exists.
 */
const char *hive_model_config_get_model_for_kind(const hive_model_config_t *config,
                                                   hive_agent_kind_t kind);

#ifdef __cplusplus
}
#endif

#endif
