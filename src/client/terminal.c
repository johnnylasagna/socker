#include "../../include/client/terminal.h"

// To handle user pressing ctrl+c to end program
void handle_sigint(int sig) {
	endwin();
	printf("\nDisconnected from chatroom.\n");
	exit(0);
}

// To handle window resizing
volatile sig_atomic_t terminal_resized = 0;

void handle_sigwinch(int sig) {
	terminal_resized = 1;
}

// Refresh messages window
void refresh_messages_window(WINDOW *messages_win, WINDOW *messages_text_win, struct message **messages, int *count, int max_lines, int window_width) {
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
		if (strncmp(p->content, "You:", 4) == 0) {
			wattron(messages_text_win, COLOR_PAIR(1));
		} else {
			wattron(messages_text_win, COLOR_PAIR(2));
		}
		mvwprintw(messages_text_win, row++, 0, "%s", p->content);
		row += strlen(p->content) / window_width;
		if (strncmp(p->content, "You:", 4) == 0) {
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
void refresh_input_window(WINDOW *input_win, char *input, int *input_len, int window_width) {
	werase(input_win);

	box(input_win, 0, 0);

	size_t offset = *input_len / (window_width - 2);
	size_t remaining_input = *input_len % (window_width - 2);

	if (remaining_input == 0 && offset != 0) {
		mvwprintw(input_win, 1, 1, "> %s", input + (offset - 1) * (window_width - 2));
		wmove(input_win, 1, 3 + window_width - 2);
	} else {
		wmove(input_win, 1, 3 + remaining_input);
		mvwprintw(input_win, 1, 1, "> %s", input + offset * (window_width - 2));
	}

	wrefresh(input_win);
}

// Refresh socker window
void refresh_socker_window(WINDOW *socker_win, char *buf, int *id) {
	werase(socker_win);

	box(socker_win, 0, 0);

	char *line = strtok(buf, "\n");

	while (line != NULL) {
		int pos_x, pos_y;

		if (strncmp(line, "/data id", strlen("/data id")) == 0) {
			if (sscanf(line + strlen("/data id"), "%d", id) != 1) {
				fprintf(stderr, "id malformed\n");
			}

		} else if (strncmp(line, "/data ball", strlen("/data ball")) == 0) {
			if (sscanf(line + strlen("/data ball"), "%d %d", &pos_x, &pos_y) == 2) {
				mvwprintw(socker_win, pos_y, pos_x, "O");
			}

		} else if (strncmp(line, "player", strlen("player")) == 0) {
			if (sscanf(line + strlen("player"), "%d %d", &pos_x, &pos_y) == 2) {
				mvwprintw(socker_win, pos_y, pos_x, "X");
			}
		}

		line = strtok(NULL, "\n");
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

// Draw help window once
void draw_help_window(WINDOW *help_win) {
	werase(help_win);

	char help_buf[] = " Messaging window:\n"
	                  " 	/whisper <name> <message>: allows you to send a message privately to another person in the chat\n"
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

	mvwprintw(help_win, 1, 0, help_buf);
	box(help_win, 0, 0);

	wrefresh(help_win);
}