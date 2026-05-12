#include "../../include/client/terminal.h"

// To handle user pressing ctrl+c or ctrl+v to end program
void handle_sigint(int sig) {
	endwin();
	printf("\nDisconnected from chatroom.\n");
	exit(0);
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
		mvwprintw(messages_text_win, row++, 0, "%s", p->content);
		row += strlen(p->content) / window_width;
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
void refresh_socker_window(WINDOW *socker_win) {
	werase(socker_win);
	box(socker_win, 0, 0);

	wrefresh(socker_win);
}

// Draw help window once
void draw_help_window(WINDOW *help_win) {
	werase(help_win);

	char help_buf[] = " /socker starts a socker game with you friends\n"
	                  " /whisper <name> <message> allows you to send a message privately to another person in the chat\n"
	                  " /name <name> allows you to change your name\n"
	                  " /save allows you to save recent chat history locally\n"
	                  " /help opens the help section\n";

	mvwprintw(help_win, 0, 0, help_buf);
	box(help_win, 0, 0);

	wrefresh(help_win);
}