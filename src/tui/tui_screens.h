#ifndef TUI_SCREEN_H
#define TUI_SCREEN_H

#include "../cli.h"
#include <ncurses.h>

// Layout constants for split-panel design
#define LEFT_PANEL_WIDTH 45
#define SEPARATOR_COL (LEFT_PANEL_WIDTH)
#define RIGHT_PANEL_START (LEFT_PANEL_WIDTH + 2)
#define RIGHT_PANEL_WIDTH (COLS - RIGHT_PANEL_START - 2)

// Screen identifiers
typedef enum {
    MAIN_SCREEN = 0,
    INPUT_SCREEN,
    OPTIONS_SCREEN,
    PROCESSING_SCREEN,
    RESULTS_SCREEN,
    CONFIG_SCREEN,
    BAD_CHARS_SCREEN,
    ABOUT_SCREEN,
    BATCH_SCREEN,
    ML_METRICS_SCREEN,
    OUTPUT_FORMAT_SCREEN,
    ADVANCED_OPTIONS_SCREEN,
    EXIT_SCREEN
} screen_id_t;

// Function declarations for each screen
int show_main_screen(byvalver_config_t *config);
void draw_main_menu(byvalver_config_t *config, int current_selection);
int show_input_screen(byvalver_config_t *config, int *current_selection);
int show_options_screen(byvalver_config_t *config, int *current_selection);
int show_processing_screen(byvalver_config_t *config);
int show_results_screen(byvalver_config_t *config, int *current_selection);
int show_config_screen(byvalver_config_t *config, int *current_selection);
int show_bad_chars_screen(byvalver_config_t *config, int *current_selection);
int show_about_screen();
int show_batch_screen(byvalver_config_t *config, int *current_selection);
int show_ml_metrics_screen(byvalver_config_t *config, int *current_selection);
int show_output_format_screen(byvalver_config_t *config, int *current_selection);
int show_advanced_options_screen(byvalver_config_t *config, int *current_selection);

// Panel drawing functions
void clear_right_panel();
void draw_vertical_separator();
void draw_right_panel_header(const char *title);

// Utility functions
void clear_screen();
void draw_header(const char *title);
void draw_footer();
void draw_menu_item(int row, int col, const char *text, int selected);

#endif