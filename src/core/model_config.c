#include "core/model_config.h"

#include "common/strings.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char *parse_string_value(const char *line)
{
    const char *start = strchr(line, '"');
    if (start == NULL) return NULL;
    
    start++;
    const char *end = strchr(start, '"');
    if (end == NULL) return NULL;
    
    size_t len = end - start;
    char *result = malloc(len + 1);
    if (result == NULL) return NULL;
    
    memcpy(result, start, len);
    result[len] = '\0';
    return result;
}

static int parse_array_strings(const char *line, const char **out_array, size_t max_count)
{
    const char *start = strchr(line, '[');
    if (start == NULL) return 0;
    
    const char *end = strchr(start, ']');
    if (end == NULL) return 0;
    
    size_t count = 0;
    const char *pos = start + 1;
    
    while (pos < end && count < max_count) {
        while (pos < end && (isspace((unsigned char)*pos) || *pos == ',')) {
            pos++;
        }
        
        if (*pos == '"') {
            const char *quote_end = strchr(pos + 1, '"');
            if (quote_end == NULL || quote_end > end) break;
            
            size_t len = quote_end - pos - 1;
            char *str = malloc(len + 1);
            if (str == NULL) break;
            
            memcpy(str, pos + 1, len);
            str[len] = '\0';
            out_array[count++] = str;
            pos = quote_end + 1;
        } else {
            break;
        }
    }
    
    return (int)count;
}

hive_status_t hive_model_config_load(hive_model_config_t *config, const char *path)
{
    if (config == NULL || path == NULL) {
        return HIVE_STATUS_INVALID_ARGUMENT;
    }

    memset(config, 0, sizeof(*config));
    config->default_model = "gpt-4o-mini";

    FILE *file = fopen(path, "rb");
    if (file == NULL) {
        /* Config file is optional; use defaults */
        config->models[0] = hive_string_dup("gpt-4o-mini");
        config->models[1] = hive_string_dup("gpt-4-turbo");
        config->models[2] = hive_string_dup("gpt-4o");
        config->model_count = 3;
        return HIVE_STATUS_OK;
    }

    char buffer[512];
    int in_models_section = 0;

    while (fgets(buffer, sizeof(buffer), file) != NULL) {
        /* Remove newline */
        size_t len = strlen(buffer);
        if (len > 0 && buffer[len - 1] == '\n') {
            buffer[len - 1] = '\0';
        }

        /* Check for [models] section */
        if (strncmp(buffer, "[models]", 8) == 0) {
            in_models_section = 1;
            continue;
        }

        if (in_models_section && buffer[0] == '[') {
            in_models_section = 0;
            continue;
        }

        if (!in_models_section) continue;

        /* Parse lines in models section */
        if (strncmp(buffer, "available", 9) == 0) {
            const char *array[HIVE_MAX_MODELS];
            int count = parse_array_strings(buffer, array, HIVE_MAX_MODELS);
            for (int i = 0; i < count && config->model_count < HIVE_MAX_MODELS; i++) {
                config->models[config->model_count++] = array[i];
            }
        } else if (strncmp(buffer, "default_model", 13) == 0) {
            char *model = parse_string_value(buffer);
            if (model != NULL) {
                config->default_model = model;
            }
        } else if (strncmp(buffer, "role_", 5) == 0) {
            const char *eq = strchr(buffer, '=');
            if (eq != NULL) {
                /* Extract role name */
                size_t role_len = eq - buffer - 5;
                if (role_len > 0 && config->role_model_count < HIVE_MAX_ROLE_MODELS) {
                    char role_name[64];
                    strncpy(role_name, buffer + 5, role_len);
                    role_name[role_len] = '\0';

                    char *model = parse_string_value(buffer);
                    if (model != NULL) {
                        config->role_models[config->role_model_count].role_name = hive_string_dup(role_name);
                        config->role_models[config->role_model_count].model_name = model;
                        config->role_model_count++;
                    }
                }
            }
        }
    }

    fclose(file);

    /* Ensure at least default models are available */
    if (config->model_count == 0) {
        config->models[0] = hive_string_dup("gpt-4o-mini");
        config->models[1] = hive_string_dup("gpt-4-turbo");
        config->models[2] = hive_string_dup("gpt-4o");
        config->model_count = 3;
    }

    return HIVE_STATUS_OK;
}

void hive_model_config_free(hive_model_config_t *config)
{
    if (config == NULL) {
        return;
    }

    for (size_t i = 0; i < config->model_count; i++) {
        free((void *)config->models[i]);
        config->models[i] = NULL;
    }

    for (size_t i = 0; i < config->role_model_count; i++) {
        free((void *)config->role_models[i].role_name);
        free((void *)config->role_models[i].model_name);
        config->role_models[i].role_name = NULL;
        config->role_models[i].model_name = NULL;
    }

    free((void *)config->default_model);
    config->default_model = NULL;
    memset(config, 0, sizeof(*config));
}

const char *hive_model_config_get_model_for_kind(const hive_model_config_t *config,
                                                   hive_agent_kind_t kind)
{
    if (config == NULL) {
        return "gpt-4o-mini";
    }

    const char *kind_str = hive_agent_kind_to_string(kind);
    if (kind_str == NULL) {
        return config->default_model;
    }

    for (size_t i = 0; i < config->role_model_count; i++) {
        if (strcmp(config->role_models[i].role_name, kind_str) == 0) {
            return config->role_models[i].model_name;
        }
    }

    return config->default_model;
}
