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
#include "../../include/client/messages.h"
#include "../../include/client/network.h"
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
	initscr();
	cbreak();
	noecho();
	keypad(stdscr, TRUE);
	nodelay(stdscr, TRUE);
	curs_set(0);

	int y, x;
	getmaxyx(stdscr, y, x);

	// Socker setup
	bool socker = false;
	int id = 0;
	WINDOW *socker_win = newwin(y, x, 0, 0);
	keypad(socker_win, TRUE);
	nodelay(socker_win, TRUE);
	werase(socker_win);

	werase(socker_win);
	wrefresh(socker_win);

	WINDOW *messages_win = newwin(y - 3, x, 0, 0);
	WINDOW *input_win = newwin(3, x, y - 3, 0);
	WINDOW *messages_text_win = derwin(messages_win, y - 5, x - 2, 1, 1);
	int max_lines = y - 5;

	keypad(input_win, TRUE);
	nodelay(input_win, TRUE);

	werase(messages_win);
	box(messages_win, 0, 0);
	wrefresh(messages_win);

	werase(input_win);
	box(input_win, 0, 0);
	mvwprintw(input_win, 1, 1, "> ");
	wrefresh(input_win);

	// Main loop
	for (;;) {
		if (poll(&pfd, 1, 50) == -1) {
			endwin();
			perror("poll");
			exit(1);
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
					werase(socker_win);

					box(socker_win, 0, 0);

					char *line = strtok(buf, "\n");

					while (line != NULL) {
						int pos_x, pos_y;

						if (strncmp(line, "/data id", 8) == 0) {
							if (sscanf(line + 10, "%d", &id) != 1) {
								fprintf(stderr, "id malformed\n");
							}

						} else if (strncmp(line, "/data ball", 10) == 0) {
							if (sscanf(line + 13, "%d %d", &pos_x, &pos_y) == 2) {
								mvwprintw(socker_win, pos_y, pos_x, "O");
							}

						} else if (strncmp(line, "player", 6) == 0) {
							if (sscanf(line + 8, "%d %d", &pos_x, &pos_y) == 2) {
								mvwprintw(socker_win, pos_y, pos_x, "X");
							}
						}

						line = strtok(NULL, "\n");
					}

					wrefresh(socker_win);
				}
			}

			if (!socker) {
				refresh_messages_window(messages_win, messages_text_win, &messages, &count, max_lines, x - 2);
			}
		}

		int ch;

		if (!socker) {
			ch = wgetch(input_win);
		} else {
			ch = wgetch(socker_win);
		}

		if (!socker) {
			if (ch != ERR) {
				if (ch == '\n') {
					if (input_len > 0) {

						if (input[0] == '/') {
							char msg[280];
							snprintf(msg, sizeof(msg), "%s\n", input);
							send(server, msg, strlen(msg), 0);

							if (strncmp(input, "/whisper ", 9) == 0) {
								char whispered_msg[300];
								generate_whispered_message(whispered_msg, sizeof(whispered_msg), input);
								add_message(&messages, &messages_tail, whispered_msg, &count);

							} else if (strncmp(input, "/save", 5) == 0) {
								save_chat_contents(messages);

							} else if (strncmp(input, "/socker", 7) == 0) {
								socker = true;
								werase(messages_win);
								werase(messages_text_win);
								werase(input_win);
								wrefresh(messages_win);
								wrefresh(messages_text_win);
								wrefresh(input_win);
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

				if (!socker) {
					refresh_input_window(input_win, input, &input_len, x - 2);
					refresh_messages_window(messages_win, messages_text_win, &messages, &count, max_lines, x - 2);
				}
			}
		} else if (socker) {
			if (ch != ERR) {
				if (ch == 'q') {
					socker = false;
					werase(socker_win);
					wrefresh(socker_win);
					refresh_input_window(input_win, input, &input_len, x - 2);
					refresh_messages_window(messages_win, messages_text_win, &messages, &count, max_lines, x - 2);

				} else if (ch == 'w') {
					char data_buf[15];
					snprintf(data_buf, sizeof(data_buf), "/data %d 2 0\n", id);
					send(server, data_buf, strlen(data_buf), 0);

				} else if (ch == 'a') {
					char data_buf[15];
					snprintf(data_buf, sizeof(data_buf), "/data %d 0 2\n", id);
					send(server, data_buf, strlen(data_buf), 0);

				} else if (ch == 's') {
					char data_buf[15];
					snprintf(data_buf, sizeof(data_buf), "/data %d 1 0\n", id);
					send(server, data_buf, strlen(data_buf), 0);

				} else if (ch == 'd') {
					char data_buf[15];
					snprintf(data_buf, sizeof(data_buf), "/data %d 0 1\n", id);
					send(server, data_buf, strlen(data_buf), 0);
				}
			}
		}
	}
	endwin();
	close(server);

	return 0;
}