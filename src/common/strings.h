#ifndef HIVE_COMMON_STRINGS_H
#define HIVE_COMMON_STRINGS_H

#include <stdarg.h>
#include <stddef.h>

#if defined(__GNUC__) || defined(__clang__)
#define HIVE_PRINTF_LIKE(format_index, first_arg_index) __attribute__((format(printf, format_index, first_arg_index)))
#else
#define HIVE_PRINTF_LIKE(format_index, first_arg_index)
#endif

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Duplicate a UTF-8 string with heap ownership.
 *
 * @param text Source string, or NULL.
 * @return A newly allocated copy, or NULL on allocation failure.
 */
char *hive_string_dup(const char *text);

/**
 * Duplicate at most `length` bytes from a UTF-8 string with heap ownership.
 *
 * @param text Source string, or NULL.
 * @param length Maximum bytes to copy.
 * @return A newly allocated, NUL-terminated copy, or NULL on allocation failure.
 */
char *hive_string_ndup(const char *text, size_t length);

/**
 * Format a string into a heap allocation using a va_list.
 *
 * @param format printf-style format string.
 * @param args Formatting arguments.
 * @return A newly allocated formatted string, or NULL on allocation failure.
 */
char *hive_string_vformat(const char *format, va_list args);

/**
 * Format a string into a heap allocation.
 *
 * @param format printf-style format string.
 * @param ... Formatting arguments.
 * @return A newly allocated formatted string, or NULL on allocation failure.
 */
char *hive_string_format(const char *format, ...) HIVE_PRINTF_LIKE(1, 2);

#ifdef __cplusplus
}
#endif

#endif
