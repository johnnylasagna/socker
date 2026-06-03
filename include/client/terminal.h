#ifndef CLIENT_TERMINAL_H
#define CLIENT_TERMINAL_H

#include <ncurses.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>

#include "globals.h"
#include "messages.h"
#include "socker.h"

#define HEIGHT 24
#define WIDTH 80

// To handle user pressing ctrl+c or ctrl+v to end program
void handle_sigint(int sig);

// To handle window resizing
void handle_sigwinch(int sig);

// Initialise ncurses
void init_ncurses();

// Initialise colors
void init_colors();

// Initialise messages window
WINDOW *init_messages_window(int y, int x);

// Initialise input window
WINDOW *init_input_window(int y, int x);

// Initialise text window
WINDOW *init_messages_text_window(WINDOW *messages_win, int y, int x);

// Initialise help window
WINDOW *init_help_window(int y, int x);

// Initialise socker window
WINDOW *init_socker_window(int y, int x);

// Refresh messages window
void refresh_messages_window(WINDOW *messages_win, WINDOW *messages_text_win, struct message **messages, int *count, int max_lines, int window_width);

// Refresh message window
void refresh_input_window(WINDOW *input_win, char *input, int *input_len, int window_width);

// Refresh socker window
void refresh_socker_window(WINDOW *socker_win, struct Socker *socker);

// Hide message windows
void hide_message_windows(WINDOW *messages_win, WINDOW *messages_text_win, WINDOW *input_win);

// Hide socker window
void hide_socker_window(WINDOW *socker_win);

// Draw help window once
void draw_help_window(WINDOW *help_win);

// Resize all windows
void resize_windows(WINDOW *messages_win, WINDOW *input_win, WINDOW *messages_text_win, WINDOW *socker_win, WINDOW *help_win, int y, int x);

#endif // CLIENT_TERMINAL_H
