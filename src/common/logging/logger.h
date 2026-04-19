#ifndef HIVE_LOGGING_LOGGER_H
#define HIVE_LOGGING_LOGGER_H

#include "common/strings.h"
#include "core/types.h"

#include <pthread.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum hive_log_level {
    HIVE_LOG_DEBUG = 0,
    HIVE_LOG_INFO,
    HIVE_LOG_WARN,
    HIVE_LOG_ERROR
} hive_log_level_t;

typedef struct hive_logger {
    pthread_mutex_t mutex;
    char program_name[64];
    hive_log_level_t level;
    bool initialized;
    bool use_syslog;
} hive_logger_t;

hive_status_t hive_logger_init(hive_logger_t *logger,
                                       const char *program_name,
                                       bool enable_syslog,
                                       hive_log_level_t level);

void hive_logger_deinit(hive_logger_t *logger);

void hive_logger_set_level(hive_logger_t *logger, hive_log_level_t level);

void hive_logger_log(hive_logger_t *logger,
                         hive_log_level_t level,
                         const char *component,
                         const char *event,
                         const char *message);

void hive_logger_logf(hive_logger_t *logger,
                          hive_log_level_t level,
                          const char *component,
                          const char *event,
                          const char *format,
                          ...) HIVE_PRINTF_LIKE(5, 6);

const char *hive_log_level_to_string(hive_log_level_t level);

#ifdef __cplusplus
}
#endif

#endif
