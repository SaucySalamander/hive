#ifndef CHARNESS_LOGGING_LOGGER_H
#define CHARNESS_LOGGING_LOGGER_H

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
typedef enum charness_log_level {
    CHARNESS_LOG_DEBUG = 0,
    CHARNESS_LOG_INFO,
    CHARNESS_LOG_WARN,
    CHARNESS_LOG_ERROR
} charness_log_level_t;

/**
 * Structured logger state.
 */
typedef struct charness_logger {
    pthread_mutex_t mutex;
    char program_name[64];
    charness_log_level_t level;
    bool initialized;
    bool use_syslog;
} charness_logger_t;

/**
 * Initialize a structured logger.
 *
 * @param logger Logger state to initialize.
 * @param program_name Process name used in log records.
 * @param enable_syslog true to duplicate records to syslog when available.
 * @param level Minimum log level to emit.
 * @return CHARNESS_STATUS_OK on success.
 */
charness_status_t charness_logger_init(charness_logger_t *logger,
                                       const char *program_name,
                                       bool enable_syslog,
                                       charness_log_level_t level);

/**
 * Destroy a structured logger.
 *
 * @param logger Logger state to destroy.
 */
void charness_logger_deinit(charness_logger_t *logger);

/**
 * Change the minimum log level.
 *
 * @param logger Logger state.
 * @param level New minimum level.
 */
void charness_logger_set_level(charness_logger_t *logger, charness_log_level_t level);

/**
 * Emit one structured log record.
 *
 * @param logger Logger state.
 * @param level Record severity.
 * @param component Logical subsystem name.
 * @param event Short event identifier.
 * @param message Human-readable message.
 */
void charness_logger_log(charness_logger_t *logger,
                         charness_log_level_t level,
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
void charness_logger_logf(charness_logger_t *logger,
                          charness_log_level_t level,
                          const char *component,
                          const char *event,
                          const char *format,
                          ...) CHARNESS_PRINTF_LIKE(5, 6);

/**
 * Convert a log level to a stable string.
 *
 * @param level Log level.
 * @return Human-readable level name.
 */
const char *charness_log_level_to_string(charness_log_level_t level);

#ifdef __cplusplus
}
#endif

#endif
