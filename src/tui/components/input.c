#include "tui/components/input.h"

#if HIVE_HAVE_NCURSES
#if __has_include(<ncurses.h>)
#include <ncurses.h>
#elif __has_include(<curses.h>)
#include <curses.h>
#endif
#endif

#include <stdlib.h>
#include <string.h>

void hive_tui_input_init(hive_tui_input_t *input, size_t buffer_size,
                         int start_row, int start_col, int field_width)
{
    if (input == NULL) return;
    input->buffer = malloc(buffer_size);
    if (input->buffer == NULL) {
        input->buffer_size = 0;
        return;
    }
    input->buffer[0] = '\0';
    input->buffer_size = buffer_size;
    input->text_length = 0;
    input->start_row = start_row;
    input->start_col = start_col;
    input->field_width = field_width;
}

void hive_tui_input_draw(hive_tui_input_t *input)
{
    if (input == NULL || input->buffer == NULL) return;

#if HIVE_HAVE_NCURSES
    mvprintw(input->start_row, input->start_col, "[%-*s]",
             input->field_width, input->buffer);
    refresh();
#endif
}

int hive_tui_input_handle_key(hive_tui_input_t *input, int key)
{
    if (input == NULL || input->buffer == NULL) return 0;

    if (key == 27) {  /* ESC */
        return -1;
    }
    if (key == '\n' || key == '\r') {
        return 1;
    }
    if (key == KEY_BACKSPACE || key == 127 || key == 8) {  /* Backspace */
        if (input->text_length > 0) {
            input->buffer[--input->text_length] = '\0';
        }
        return 0;
    }
    if (key >= 32 && key < 127 && input->text_length + 1 < input->buffer_size) {
        input->buffer[input->text_length++] = (char)key;
        input->buffer[input->text_length] = '\0';
        return 0;
    }
    return 0;
}

const char *hive_tui_input_get_text(const hive_tui_input_t *input)
{
    if (input == NULL || input->buffer == NULL) return "";
    return input->buffer;
}

void hive_tui_input_set_text(hive_tui_input_t *input, const char *text)
{
    if (input == NULL || input->buffer == NULL) return;
    if (text == NULL) {
        input->buffer[0] = '\0';
        input->text_length = 0;
        return;
    }
    size_t len = strlen(text);
    if (len >= input->buffer_size) len = input->buffer_size - 1;
    memcpy(input->buffer, text, len);
    input->buffer[len] = '\0';
    input->text_length = len;
}

void hive_tui_input_clear(hive_tui_input_t *input)
{
    if (input == NULL || input->buffer == NULL) return;
    input->buffer[0] = '\0';
    input->text_length = 0;
}

void hive_tui_input_deinit(hive_tui_input_t *input)
{
    if (input == NULL) return;
    if (input->buffer != NULL) {
        free(input->buffer);
        input->buffer = NULL;
    }
    input->buffer_size = 0;
    input->text_length = 0;
}
