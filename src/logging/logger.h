#ifndef HIVE_LOGGING_LOGGER_H
#define HIVE_LOGGING_LOGGER_H

#include "common/strings.h"
#include "core/types.h"

#include <pthread.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Log levels emitted by the structured logger.
 */
typedef enum hive_log_level {
    HIVE_LOG_DEBUG = 0,
    HIVE_LOG_INFO,
    HIVE_LOG_WARN,
    HIVE_LOG_ERROR
} hive_log_level_t;

/**
 * Structured logger state.
 */
typedef struct hive_logger {
    pthread_mutex_t mutex;
    char program_name[64];
    hive_log_level_t level;
    bool initialized;
    bool use_syslog;
} hive_logger_t;

/**
 * Initialize a structured logger.
 *
 * @param logger Logger state to initialize.
 * @param program_name Process name used in log records.
 * @param enable_syslog true to duplicate records to syslog when available.
 * @param level Minimum log level to emit.
 * @return HIVE_STATUS_OK on success.
 */
hive_status_t hive_logger_init(hive_logger_t *logger,
                                       const char *program_name,
                                       bool enable_syslog,
                                       hive_log_level_t level);

/**
 * Destroy a structured logger.
 *
 * @param logger Logger state to destroy.
 */
void hive_logger_deinit(hive_logger_t *logger);

/**
 * Change the minimum log level.
 *
 * @param logger Logger state.
 * @param level New minimum level.
 */
void hive_logger_set_level(hive_logger_t *logger, hive_log_level_t level);

/**
 * Emit one structured log record.
 *
 * @param logger Logger state.
 * @param level Record severity.
 * @param component Logical subsystem name.
 * @param event Short event identifier.
 * @param message Human-readable message.
 */
void hive_logger_log(hive_logger_t *logger,
                         hive_log_level_t level,
                         const char *component,
                         const char *event,
                         const char *message);

/**
 * Emit one structured log record using printf-style formatting.
 *
 * @param logger Logger state.
 * @param level Record severity.
 * @param component Logical subsystem name.
 * @param event Short event identifier.
 * @param format printf-style message format.
 * @param ... Formatting arguments.
 */
void hive_logger_logf(hive_logger_t *logger,
                          hive_log_level_t level,
                          const char *component,
                          const char *event,
                          const char *format,
                          ...) HIVE_PRINTF_LIKE(5, 6);

/**
 * Convert a log level to a stable string.
 *
 * @param level Log level.
 * @return Human-readable level name.
 */
const char *hive_log_level_to_string(hive_log_level_t level);

#ifdef __cplusplus
}
#endif

#endif
