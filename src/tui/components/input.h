#ifndef HIVE_TUI_COMPONENTS_INPUT_H
#define HIVE_TUI_COMPONENTS_INPUT_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Simple text input component for ncurses.
 * Provides a text input field with basic editing.
 */
typedef struct hive_tui_input {
    char *buffer;         /* Input buffer */
    size_t buffer_size;   /* Allocated buffer size */
    size_t text_length;   /* Current text length */
    int start_row;        /* Starting row for display */
    int start_col;        /* Starting column for display */
    int field_width;      /* Display width of input field */
} hive_tui_input_t;

/**
 * Initialize an input component.
 * @param input Input structure to initialize
 * @param buffer_size Allocated buffer size for text
 * @param start_row Starting row for display
 * @param start_col Starting column for display
 * @param field_width Display width of input field
 */
void hive_tui_input_init(hive_tui_input_t *input, size_t buffer_size,
                         int start_row, int start_col, int field_width);

/**
 * Draw the input field on screen.
 */
void hive_tui_input_draw(hive_tui_input_t *input);

/**
 * Handle user input for text editing.
 * Returns 1 when user presses Enter, 0 for other keys, -1 for Esc.
 * @param input Input structure
 * @param key Input key from getch()
 * @return 1 on Enter, -1 on Esc, 0 otherwise
 */
int hive_tui_input_handle_key(hive_tui_input_t *input, int key);

/**
 * Get current input text.
 */
const char *hive_tui_input_get_text(const hive_tui_input_t *input);

/**
 * Set input text.
 */
void hive_tui_input_set_text(hive_tui_input_t *input, const char *text);

/**
 * Clear input field.
 */
void hive_tui_input_clear(hive_tui_input_t *input);

/**
 * Free input component.
 */
void hive_tui_input_deinit(hive_tui_input_t *input);

#ifdef __cplusplus
}
#endif

#endif
