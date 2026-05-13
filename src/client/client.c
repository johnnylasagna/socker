#include <arpa/inet.h>
#include <errno.h>
#include <ncurses.h>
#include <netdb.h>
#include <netinet/in.h>
#include <poll.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#include "../../include/client/buffers.h"
#include "../../include/client/globals.h"
#include "../../include/client/messages.h"
#include "../../include/client/network.h"
#include "../../include/client/socker.h"
#include "../../include/client/terminal.h"

int main(int argc, char *argv[]) {

	// Server Setup
	int server;

	if (argc != 3) {
		fprintf(stderr, "Usage: pollclient <server> <port>\n");
		exit(1);
	}

	const char *server_name = argv[1];
	const char *port = argv[2];

	server = get_server_socket(server_name, port);

	if (server == -1) {
		fprintf(stderr, "error getting listening socket\n");
		exit(1);
	}

	struct pollfd pfd;
	pfd.fd = server;
	pfd.events = POLLIN;

	// Name setup
	char name[20];
	set_name(server, name, sizeof(name));

	// Messages setup
	struct message *messages = NULL;
	struct message *messages_tail = NULL;
	int count = 0;

	// Input setup
	char input[256] = {0};
	int input_len = 0;

	// ncurses setup
	signal(SIGINT, handle_sigint);
	signal(SIGWINCH, handle_sigwinch);
	initscr();
	cbreak();
	noecho();
	keypad(stdscr, TRUE);
	nodelay(stdscr, TRUE);
	curs_set(0);

	int y, x;
	getmaxyx(stdscr, y, x);

	// Color setup
	start_color();

	if (!has_colors()) {
		endwin();
		printf("No color support\n");
		return 1;
	}

	init_pair(1, COLOR_RED, COLOR_BLACK);
	init_pair(2, COLOR_BLUE, COLOR_BLACK);

	// Socker setup
	int id = 0;
	int height = 24;
	int width = 80;
	WINDOW *socker_win = newwin(height, width, (y - height) / 2, (x - width) / 2);
	keypad(socker_win, TRUE);
	nodelay(socker_win, TRUE);
	werase(socker_win);

	bool socker = false;

	werase(socker_win);
	wrefresh(socker_win);

	// Messages setup
	WINDOW *messages_win = newwin(y - 3, x, 0, 0);
	WINDOW *input_win = newwin(3, x, y - 3, 0);
	WINDOW *messages_text_win = derwin(messages_win, y - 5, x - 2, 1, 1);
	int max_lines = y - 5;

	bool show_messages = true;

	keypad(input_win, TRUE);
	nodelay(input_win, TRUE);

	// Help setup
	WINDOW *help_win = newwin(y, x, 0, 0);
	keypad(help_win, TRUE);
	nodelay(help_win, TRUE);

	bool help = false;

	werase(help_win);
	wrefresh(help_win);

	// Initial screen clear
	werase(messages_win);
	box(messages_win, 0, 0);
	wrefresh(messages_win);

	werase(input_win);
	box(input_win, 0, 0);
	mvwprintw(input_win, 1, 1, "> ");
	wrefresh(input_win);

	// Gamble setup
	int gamble_amount = 1;
	srand(time(NULL));

	// Main loop
	for (;;) {
		if (poll(&pfd, 1, 50) == -1) {
			if (errno == EINTR) continue;
			endwin();
			perror("poll");
			exit(1);
		}

		if (terminal_resized) {
			terminal_resized = 0;
			endwin();
			refresh();
			clear();

			getmaxyx(stdscr, y, x);
			max_lines = y - 5;

			wresize(messages_win, y - 3, x);
			wresize(input_win, 3, x);
			wresize(messages_text_win, y - 5, x - 2);
			mvwin(input_win, y - 3, 0);
			mvwin(socker_win, (y - height) / 2, (x - width) / 2);
			wresize(help_win, y, x);

			if (show_messages) {
				refresh_input_window(input_win, input, &input_len, x - 2);
				refresh_messages_window(messages_win, messages_text_win, &messages, &count, max_lines, x - 2);
			} else if (socker) {
				werase(socker_win);
				box(socker_win, 0, 0);
				wrefresh(socker_win);
			} else if (help) {
				draw_help_window(help_win);
			}
		}

		if (pfd.revents & POLLIN) {
			char buf[1024];

			int n = recv(server, buf, sizeof(buf) - 1, 0);

			if (n == 0) {
				endwin();
				close(server);
				printf("Server closed connection\n");
				break;
			} else if (n < 0) {
				endwin();
				close(server);
				perror("recv");
				break;
			}

			buf[n] = '\0';

			if (buf[0] != '/') {
				add_message(&messages, &messages_tail, buf, &count);
			} else {
				if (strncmp(buf, "/data", 5) == 0) {
					refresh_socker_window(socker_win, buf, &id);
				}
			}

			if (show_messages) {
				refresh_messages_window(messages_win, messages_text_win, &messages, &count, max_lines, x - 2);
			}
		}

		int ch;
		if (show_messages) {
			ch = wgetch(input_win);
		} else if (socker) {
			ch = wgetch(socker_win);
		} else if (help) {
			ch = wgetch(help_win);
		}

		if (ch == KEY_RESIZE) {
			continue;
		}

		if (show_messages) {
			if (ch != ERR) {
				if (ch == '\n') {
					if (input_len > 0) {

						if (input[0] == '/') {
							char msg[280];
							snprintf(msg, sizeof(msg), "%s\n", input);
							send(server, msg, strlen(msg), 0);

							if (strncmp(input, "/name", 5) == 0) {
								sscanf(input + strlen("/name "), "%s", name);

							} else if (strncmp(input, "/whisper ", 9) == 0) {
								char whispered_msg[300];
								generate_whispered_message(whispered_msg, sizeof(whispered_msg), input);
								add_message(&messages, &messages_tail, whispered_msg, &count);

							} else if (strncmp(input, "/save", 5) == 0) {
								save_chat_contents(messages);

							} else if (strncmp(input, "/socker", 7) == 0) {
								socker = true;
								show_messages = false;

								hide_message_windows(messages_win, messages_text_win, input_win);

							} else if (strncmp(input, "/help", 5) == 0) {
								help = true;
								show_messages = false;

								hide_message_windows(messages_win, messages_text_win, input_win);

								draw_help_window(help_win);

							} else if (strncmp(input, "/gamble", 7) == 0) {
								if (rand() % 2 == 0) {
									gamble_amount *= 2;
								} else {
									gamble_amount = 1;
								}

								char gamble_msg[30];
								snprintf(gamble_msg, sizeof(gamble_msg), "You now have %d coins\n", gamble_amount);
								add_message(&messages, &messages_tail, gamble_msg, &count);
							}

						} else {
							char msg[280];
							snprintf(msg, sizeof(msg), "%s:%s\n", name, input);
							send(server, msg, strlen(msg), 0);

							char messages_msg[280];
							snprintf(messages_msg, sizeof(messages_msg), "You:%s\n", input);

							add_message(&messages, &messages_tail, messages_msg, &count);
						}

						input_len = 0;
						input[0] = '\0';
					}
				} else if (ch == KEY_BACKSPACE || ch == 127 || ch == 8) {
					if (input_len > 0) {
						input_len--;

						input[input_len] = '\0';
					}
				} else {
					if (input_len < 255) {
						input[input_len++] = ch;
						input[input_len] = '\0';
					}
				}

				if (show_messages) {
					refresh_input_window(input_win, input, &input_len, x - 2);
					refresh_messages_window(messages_win, messages_text_win, &messages, &count, max_lines, x - 2);
				}
			}
		} else if (socker) {
			if (ch != ERR) {
				if (ch == 'q') {
					socker = false;
					show_messages = true;
					werase(socker_win);
					wrefresh(socker_win);
					refresh_input_window(input_win, input, &input_len, x - 2);
					refresh_messages_window(messages_win, messages_text_win, &messages, &count, max_lines, x - 2);

					char leave_buf[15];
					snprintf(leave_buf, sizeof(leave_buf), "/leave %d\n", id);
					send(server, leave_buf, strlen(leave_buf), 0);

				} else {
					handle_socker_input(ch, server, id);
				}
			}
		} else if (help) {
			if (ch != ERR) {
				if (ch == 'q') {
					show_messages = true;
					help = false;
					refresh_input_window(input_win, input, &input_len, x - 2);
					refresh_messages_window(messages_win, messages_text_win, &messages, &count, max_lines, x - 2);
				}
			}
		}

		// if (ch == KEY_RESIZE) {
		// 	endwin();

		// 	getmaxyx(stdscr, y, x);
		// 	max_lines = y - 5;

		// 	clear();
		// 	refresh();

		// 	wresize(messages_win, y - 3, x);
		// 	wresize(input_win, 3, x);
		// 	wresize(messages_text_win, y - 5, x - 2);
		// 	mvwin(input_win, y - 3, 0);
		// 	mvwin(socker_win, (y - height) / 2, (x - width) / 2);
		// 	wresize(help_win, y, x);

		// 	if (show_messages) {
		// 		refresh_input_window(input_win, input, &input_len, x - 2);
		// 		refresh_messages_window(messages_win, messages_text_win, &messages, &count, max_lines, x - 2);
		// 	} else if (socker) {
		// 		werase(socker_win);
		// 		box(socker_win, 0, 0);
		// 		wrefresh(socker_win);
		// 	} else if (help) {
		// 		draw_help_window(help_win);
		// 	}
		// }
	}
	endwin();
	close(server);

	return 0;
}