#include "common/strings.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char *hive_string_dup(const char *text)
{
    if (text == NULL) {
        return NULL;
    }

    const size_t length = strlen(text);
    char *copy = malloc(length + 1U);
    if (copy == NULL) {
        return NULL;
    }

    memcpy(copy, text, length + 1U);
    return copy;
}

char *hive_string_ndup(const char *text, size_t length)
{
    if (text == NULL) {
        return NULL;
    }

    char *copy = malloc(length + 1U);
    if (copy == NULL) {
        return NULL;
    }

    memcpy(copy, text, length);
    copy[length] = '\0';
    return copy;
}

char *hive_string_vformat(const char *format, va_list args)
{
    if (format == NULL) {
        return NULL;
    }

    va_list first_pass;
    va_copy(first_pass, args);
    const int required = vsnprintf(NULL, 0U, format, first_pass);
    va_end(first_pass);

    if (required < 0) {
        return NULL;
    }

    char *formatted = malloc((size_t)required + 1U);
    if (formatted == NULL) {
        return NULL;
    }

    va_list second_pass;
    va_copy(second_pass, args);
    const int written = vsnprintf(formatted, (size_t)required + 1U, format, second_pass);
    va_end(second_pass);

    if (written < 0) {
        free(formatted);
        return NULL;
    }

    return formatted;
}

char *hive_string_format(const char *format, ...)
{
    va_list args;
    va_start(args, format);
    char *formatted = hive_string_vformat(format, args);
    va_end(args);
    return formatted;
}
