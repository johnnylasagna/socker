#ifndef CLIENT_TERMINAL_H
#define CLIENT_TERMINAL_H

#include <ncurses.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>

#include "globals.h"
#include "messages.h"

// To handle user pressing ctrl+c or ctrl+v to end program
void handle_sigint(int sig);

void handle_sigwinch(int sig);

// Refresh messages window
void refresh_messages_window(WINDOW *messages_win, WINDOW *messages_text_win, struct message **messages, int *count, int max_lines, int window_width);

// Refresh message window
void refresh_input_window(WINDOW *input_win, char *input, int *input_len, int window_width);

// Refresh socker window
void refresh_socker_window(WINDOW *socker_win, char *buf, int *id);

// Hide message windows
void hide_message_windows(WINDOW *messages_win, WINDOW *messages_text_win, WINDOW *input_win);

// Draw help window once
void draw_help_window(WINDOW *help_win);

#endif // CLIENT_TERMINAL_H
