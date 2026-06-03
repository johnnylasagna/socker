#include "../../include/client/terminal.h"

// To handle user pressing ctrl+c to end program
void handle_sigint(int sig) {
	endwin();
	printf("\nDisconnected from chatroom.\n");
	(void)sig;
	exit(0);
}

// To handle window resizing
volatile sig_atomic_t terminal_resized = 0;

void handle_sigwinch(int sig) {
	terminal_resized = 1;
	(void)sig;
}

// Initialise ncurses
void init_ncurses() {
	signal(SIGINT, handle_sigint);
	signal(SIGWINCH, handle_sigwinch);
	initscr();
	cbreak();
	noecho();
	keypad(stdscr, TRUE);
	nodelay(stdscr, TRUE);
	mousemask(0, NULL);
	curs_set(0);
}

// Initialise colors
void init_colors() {
	start_color();

	if (!has_colors()) {
		endwin();
		printf("No color support\n");
		exit(1);
	}

	init_pair(1, COLOR_RED, COLOR_BLACK);
	init_pair(2, COLOR_BLUE, COLOR_BLACK);

	bkgd(COLOR_PAIR(1));
}

// Initialise messages window
WINDOW *init_messages_window(int y, int x) {
	WINDOW *messages_win = newwin(y - 3, x, 0, 0);
	return messages_win;
}

// Initialise input window
WINDOW *init_input_window(int y, int x) {
	WINDOW *input_win = newwin(3, x, y - 3, 0);

	keypad(input_win, TRUE);
	nodelay(input_win, TRUE);

	return input_win;
}

// Initialise text window
WINDOW *init_messages_text_window(WINDOW *messages_win, int y, int x) {
	WINDOW *messages_text_win = derwin(messages_win, y - 5, x - 2, 1, 1);
	return messages_text_win;
}

// Initialise help window
WINDOW *init_help_window(int y, int x) {
	WINDOW *help_win = newwin(HEIGHT, WIDTH, (y - HEIGHT) / 2, (x - WIDTH) / 2);
	keypad(help_win, TRUE);
	nodelay(help_win, TRUE);

	werase(help_win);
	wrefresh(help_win);

	return help_win;
}

// Initialise socker window
WINDOW *init_socker_window(int y, int x) {
	WINDOW *socker_win = newwin(HEIGHT, WIDTH, (y - HEIGHT) / 2, (x - WIDTH) / 2);
	keypad(socker_win, TRUE);
	nodelay(socker_win, TRUE);

	werase(socker_win);

	werase(socker_win);
	wrefresh(socker_win);

	return socker_win;
}

// Refresh messages window
void refresh_messages_window(WINDOW *messages_win, WINDOW *messages_text_win, struct message **messages, int *count, int max_lines, int window_WIDTH) {
	werase(messages_win);
	werase(messages_text_win);

	box(messages_win, 0, 0);

	while (*count > max_lines) {
		struct message *temp = *messages;
		(*messages) = (*messages)->next;
		free(temp);
		(*count)--;
	}

	int row = 0;

	struct message *p = *messages;
	while (p != NULL) {
		if (strncmp(p->content, "You:", strlen("You:")) == 0) {
			wattron(messages_text_win, COLOR_PAIR(1));
		} else {
			wattron(messages_text_win, COLOR_PAIR(2));
		}
		mvwprintw(messages_text_win, row++, 0, "%s", p->content);
		row += strlen(p->content) / window_WIDTH;
		if (strncmp(p->content, "You:", strlen("You:")) == 0) {
			wattroff(messages_text_win, COLOR_PAIR(1));
		} else {
			wattroff(messages_text_win, COLOR_PAIR(2));
		}
		p = p->next;
	}

	wrefresh(messages_win);
	wrefresh(messages_text_win);
}

// Refresh message window
void refresh_input_window(WINDOW *input_win, char *input, int *input_len, int window_WIDTH) {
	werase(input_win);

	box(input_win, 0, 0);

	size_t offset = *input_len / (window_WIDTH - 2);
	size_t remaining_input = *input_len % (window_WIDTH - 2);

	if (remaining_input == 0 && offset != 0) {
		mvwprintw(input_win, 1, 1, "> %s", input + (offset - 1) * (window_WIDTH - 2));
		wmove(input_win, 1, 3 + window_WIDTH - 2);
	} else {
		wmove(input_win, 1, 3 + remaining_input);
		mvwprintw(input_win, 1, 1, "> %s", input + offset * (window_WIDTH - 2));
	}

	wrefresh(input_win);
}

// Refresh socker window
void refresh_socker_window(WINDOW *socker_win, struct Socker *socker) {
	werase(socker_win);

	box(socker_win, 0, 0);

	mvwprintw(socker_win, socker->ball_position[1], socker->ball_position[0], "O");

	for (int i = 0; i < socker->player_count; i++) {
		mvwprintw(socker_win, socker->player_positions[i][1], socker->player_positions[i][0], "X");
	}

	wrefresh(socker_win);
}

// Hide message window
void hide_message_windows(WINDOW *messages_win, WINDOW *messages_text_win, WINDOW *input_win) {
	werase(messages_win);
	werase(messages_text_win);
	werase(input_win);
	wrefresh(messages_win);
	wrefresh(messages_text_win);
	wrefresh(input_win);
}

// Hide socker window
void hide_socker_window(WINDOW *socker_win) {
	werase(socker_win);
	wrefresh(socker_win);
}

// Draw help window once
void draw_help_window(WINDOW *help_win) {
	werase(help_win);

	char help_buf[] = " Messaging window:\n"
	                  " 	/whisper <name> <message>: allows you to send a message privately to\n"
	                  " 		another person in the chat\n"
	                  " 	/name <name>: allows you to change your name\n"
	                  " 	/save: allows you to save recent chat history locally\n"
	                  " 	/quit: quit the program\n"
	                  " Games:\n"
	                  " 	/socker: allows you to join a socker game with your friends\n"
	                  " 	/gamble: allows you to gamble alone\n"
	                  " Help:\n"
	                  " 	/help opens the help section\n"
	                  " \n\n\n"
	                  " PRESS 'q' TO QUIT HELP WINDOW \n"
	                  " \n";

	mvwprintw(help_win, 1, 0, "%s", help_buf);
	box(help_win, 0, 0);

	wrefresh(help_win);
}

// Resize all windows
void resize_windows(WINDOW *messages_win, WINDOW *input_win, WINDOW *messages_text_win, WINDOW *socker_win, WINDOW *help_win, int y, int x) {
	wresize(messages_win, y - 3, x);
	wresize(input_win, 3, x);
	wresize(messages_text_win, y - 5, x - 2);
	mvwin(input_win, y - 3, 0);
	mvwin(socker_win, (y - HEIGHT) / 2, (x - WIDTH) / 2);
	mvwin(help_win, (y - HEIGHT) / 2, (x - WIDTH) / 2);
}