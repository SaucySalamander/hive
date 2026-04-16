#include "logging/logger.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#if HIVE_HAVE_SYSLOG
#include <syslog.h>
#endif

static const char *safe_text(const char *text)
{
    return text != NULL ? text : "";
}

static int level_to_syslog_priority(hive_log_level_t level)
{
    switch (level) {
    case HIVE_LOG_DEBUG:
        return 7;
    case HIVE_LOG_INFO:
        return 6;
    case HIVE_LOG_WARN:
        return 4;
    case HIVE_LOG_ERROR:
    default:
        return 3;
    }
}

const char *hive_log_level_to_string(hive_log_level_t level)
{
    switch (level) {
    case HIVE_LOG_DEBUG:
        return "debug";
    case HIVE_LOG_INFO:
        return "info";
    case HIVE_LOG_WARN:
        return "warn";
    case HIVE_LOG_ERROR:
        return "error";
    default:
        return "unknown";
    }
}

static void json_escape(FILE *stream, const char *text)
{
    const unsigned char *cursor = (const unsigned char *)safe_text(text);
    for (; *cursor != '\0'; ++cursor) {
        switch (*cursor) {
        case '\\':
            fputs("\\\\", stream);
            break;
        case '"':
            fputs("\\\"", stream);
            break;
        case '\b':
            fputs("\\b", stream);
            break;
        case '\f':
            fputs("\\f", stream);
            break;
        case '\n':
            fputs("\\n", stream);
            break;
        case '\r':
            fputs("\\r", stream);
            break;
        case '\t':
            fputs("\\t", stream);
            break;
        default:
            if (*cursor < 0x20U) {
                fprintf(stream, "\\u%04x", (unsigned int)*cursor);
            } else {
                fputc(*cursor, stream);
            }
            break;
        }
    }
}

static void log_record(hive_logger_t *logger,
                       hive_log_level_t level,
                       const char *component,
                       const char *event,
                       const char *message)
{
    if (logger == NULL || !logger->initialized) {
        return;
    }

    if (level < logger->level) {
        return;
    }

    time_t now = time(NULL);
    struct tm tm_utc;
    (void)gmtime_r(&now, &tm_utc);

    char timestamp[32];
    if (strftime(timestamp, sizeof(timestamp), "%Y-%m-%dT%H:%M:%SZ", &tm_utc) == 0U) {
        (void)snprintf(timestamp, sizeof(timestamp), "1970-01-01T00:00:00Z");
    }

    pthread_mutex_lock(&logger->mutex);

    fputc('{', stderr);
    fputs("\"ts\":\"", stderr);
    json_escape(stderr, timestamp);
    fputs("\",\"level\":\"", stderr);
    json_escape(stderr, hive_log_level_to_string(level));
    fputs("\",\"component\":\"", stderr);
    json_escape(stderr, safe_text(component));
    fputs("\",\"event\":\"", stderr);
    json_escape(stderr, safe_text(event));
    fputs("\",\"message\":\"", stderr);
    json_escape(stderr, safe_text(message));
    fputs("\"}\n", stderr);
    fflush(stderr);

#if HIVE_HAVE_SYSLOG
    if (logger->use_syslog) {
        syslog(level_to_syslog_priority(level), "%s:%s: %s", safe_text(component), safe_text(event), safe_text(message));
    }
#endif

    pthread_mutex_unlock(&logger->mutex);
}

hive_status_t hive_logger_init(hive_logger_t *logger,
                                       const char *program_name,
                                       bool enable_syslog,
                                       hive_log_level_t level)
{
    if (logger == NULL) {
        return HIVE_STATUS_INVALID_ARGUMENT;
    }

    memset(logger, 0, sizeof(*logger));

    const int mutex_status = pthread_mutex_init(&logger->mutex, NULL);
    if (mutex_status != 0) {
        return HIVE_STATUS_ERROR;
    }

    (void)snprintf(logger->program_name, sizeof(logger->program_name), "%s", safe_text(program_name));
    logger->level = level;
    logger->initialized = true;

#if HIVE_HAVE_SYSLOG
    logger->use_syslog = enable_syslog && true;
    if (logger->use_syslog) {
        openlog(logger->program_name[0] != '\0' ? logger->program_name : "hive", LOG_PID | LOG_NDELAY, LOG_USER);
    }
#else
    logger->use_syslog = false;
    (void)enable_syslog;
#endif

    return HIVE_STATUS_OK;
}

void hive_logger_deinit(hive_logger_t *logger)
{
    if (logger == NULL || !logger->initialized) {
        return;
    }

#if HIVE_HAVE_SYSLOG
    if (logger->use_syslog) {
        closelog();
    }
#endif

    (void)pthread_mutex_destroy(&logger->mutex);
    memset(logger, 0, sizeof(*logger));
}

void hive_logger_set_level(hive_logger_t *logger, hive_log_level_t level)
{
    if (logger == NULL || !logger->initialized) {
        return;
    }

    logger->level = level;
}

void hive_logger_log(hive_logger_t *logger,
                         hive_log_level_t level,
                         const char *component,
                         const char *event,
                         const char *message)
{
    log_record(logger, level, component, event, message);
}

void hive_logger_logf(hive_logger_t *logger,
                          hive_log_level_t level,
                          const char *component,
                          const char *event,
                          const char *format,
                          ...)
{
    if (logger == NULL) {
        return;
    }

    va_list args;
    va_start(args, format);
    char *message = hive_string_vformat(format, args);
    va_end(args);

    if (message == NULL) {
        log_record(logger, HIVE_LOG_ERROR, component, "log_format_failed", "failed to format log message");
        return;
    }

    log_record(logger, level, component, event, message);
    free(message);
}
