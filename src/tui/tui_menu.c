#include "tui_menu.h"
#include "tui_screens.h"
#include "tui_config_builder.h"
#include <string.h>
#include <stdlib.h>

// Function to run the interactive TUI menu
int run_tui_menu(byvalver_config_t *config) {
    // Initialize the TUI environment
    if (init_tui() != 0) {
        return -1;
    }

    // With the new split-panel design, show_main_screen handles everything
    // It manages the left panel (menu) and right panel (content) internally
    int result = show_main_screen(config);

    cleanup_tui();
    return (result == EXIT_SCREEN) ? 0 : -1;
}

// Function to initialize ncurses and set up the TUI environment
int init_tui() {
    // Initialize ncurses
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    start_color();
    init_pair(1, COLOR_WHITE, COLOR_BLUE);    // Main header
    init_pair(2, COLOR_BLACK, COLOR_WHITE);   // Menu items
    init_pair(3, COLOR_WHITE, COLOR_RED);     // Error messages
    init_pair(4, COLOR_GREEN, COLOR_BLACK);   // Success messages
    init_pair(5, COLOR_YELLOW, COLOR_BLACK);  // Warnings
    
    // Clear the screen
    clear();
    refresh();
    
    return 0;
}

// Function to clean up ncurses and restore terminal settings
void cleanup_tui() {
    endwin();
}