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

	// ---- Server Setup ----
	int chat_server, socker_server;

	if (argc != 3) {
		fprintf(stderr, "Usage: pollclient <server> <port>\n");
		exit(1);
	}

	const char *server_name = argv[1];
	const char *port = argv[2];

	// ---- Chat Server ----
	chat_server = get_chat_server_socket(server_name, port);

	if (chat_server == -1) {
		fprintf(stderr, "error getting listening socket\n");
		exit(1);
	}

	// ---- Socker server ----
	socker_server = get_socker_server_socket(server_name, port);

	if (socker_server == -1) {
		fprintf(stderr, "error getting udp socket\n");
		exit(1);
	}

	// ---- Poll Setup ----
	struct pollfd pfds[2];
	pfds[0].fd = chat_server;
	pfds[0].events = POLLIN;
	pfds[1].fd = socker_server;
	pfds[1].events = POLLIN;

	// ---- Name setup ----
	char name[20];
	set_name(chat_server, name, sizeof(name));

	// ---- Messages setup ----
	struct message *messages = NULL;
	struct message *messages_tail = NULL;
	int count = 0;

	// ---- Input setup ----
	char input[256] = {0};
	int input_len = 0;

	// ---- ncurses setup ----
	init_ncurses();

	int y, x;
	getmaxyx(stdscr, y, x);

	int max_lines = y - 5;

	// ---- Color setup ----
	init_colors();

	// ---- Socker setup ----
	struct Socker socker_data;
	init_socker(&socker_data, 1);
	int id = -1;

	WINDOW *socker_win = init_socker_window(y, x);

	bool socker = false;

	// ---- Messages setup ----
	WINDOW *messages_win = init_messages_window(y, x);
	WINDOW *input_win = init_input_window(y, x);
	WINDOW *messages_text_win = init_messages_text_window(messages_win, y, x);

	bool show_messages = true;

	// ---- Help setup ----
	WINDOW *help_win = init_help_window(y, x);

	bool help = false;

	// ---- Gamble setup ----
	int gamble_amount = 1;
	srand(time(NULL));

	// ---- Initial screen clear ----
	werase(messages_win);
	box(messages_win, 0, 0);
	wrefresh(messages_win);

	werase(input_win);
	box(input_win, 0, 0);
	mvwprintw(input_win, 1, 1, "> ");
	wrefresh(input_win);

	// ---- Main loop ----
	while (true) {
		// ---- Polling ----
		if (poll(pfds, 2, 50) == -1) {
			if (errno == EINTR) continue;
			endwin();
			perror("poll");
			exit(1);
		}

		// ---- Window resize ----
		if (terminal_resized) {
			terminal_resized = 0;
			endwin();
			refresh();
			clear();

			getmaxyx(stdscr, y, x);
			max_lines = y - 5;

			resize_windows(messages_win, input_win, messages_text_win, socker_win, help_win, y, x);

			if (show_messages) {
				refresh_input_window(input_win, input, &input_len, x - 2);
				refresh_messages_window(messages_win, messages_text_win, &messages, &count, max_lines, x - 2);
			} else if (socker) {
				refresh_socker_window(socker_win, &socker_data);
			} else if (help) {
				draw_help_window(help_win);
			}
		}

		// ---- TCP Messages ----
		if (pfds[0].revents & POLLIN) {
			static char tcp_buf[4096] = {0};
			static int tcp_buf_len = 0;
			char buf[1024];

			while (true) {
				int n = recv(chat_server, buf, sizeof(buf) - 1, 0);

				if (n > 0) {
					buf[n] = '\0';
					if (tcp_buf_len + n < (int)sizeof(tcp_buf)) {
						strncat(tcp_buf, buf, n);
						tcp_buf_len += n;
					}
				} else if (n == 0) {
					endwin();
					close(chat_server);
					printf("Server closed connection\n");
					exit(0);
				} else {
					if (errno == EAGAIN || errno == EWOULDBLOCK) {
						break;
					} else {
						endwin();
						close(chat_server);
						perror("recv");
						exit(1);
					}
				}
			}

			char *newline;
			bool socker_updated = false;

			while ((newline = strchr(tcp_buf, '\n')) != NULL) {
				int msg_len = newline - tcp_buf;
				char msg[1024];

				strncpy(msg, tcp_buf, msg_len);
				msg[msg_len] = '\0';

				if (msg[0] != '/') {
					add_message(&messages, &messages_tail, msg, &count);

				} else if (strncmp(msg, "/data", strlen("/data")) == 0) {
					handle_socker_data(msg, &id, &socker_data);
					socker_updated = true;
				}

				int remaining = tcp_buf_len - msg_len - 1;
				memmove(tcp_buf, newline + 1, remaining);
				tcp_buf_len = remaining;
				tcp_buf[tcp_buf_len] = '\0';
			}

			if (socker && socker_updated) {
				refresh_socker_window(socker_win, &socker_data);
			}

			if (show_messages) {
				refresh_messages_window(messages_win, messages_text_win, &messages, &count, max_lines, x - 2);
			}
		}

		// ---- UDP Messages ----
		if (pfds[1].revents & POLLIN) {
			char buf[1024];
			int n = recv(socker_server, buf, sizeof(buf) - 1, 0);

			if (n > 0) {
				buf[n] = '\0';
				if (strncmp(buf, "/data", strlen("/data")) == 0) {
					handle_socker_data(buf, &id, &socker_data);
					if (socker) {
						refresh_socker_window(socker_win, &socker_data);
					}
				}
			}
		}

		// ---- Handle input ----
		int ch = ERR;
		if (show_messages) {
			ch = wgetch(input_win);
		} else if (socker) {
			ch = wgetch(socker_win);
		} else if (help) {
			ch = wgetch(help_win);
		}

		// ---- Ignore
		if (ch == KEY_RESIZE || ch == KEY_UP || ch == KEY_DOWN || ch == KEY_NPAGE || ch == KEY_PPAGE) {
			continue;
		}

		if (show_messages) {
			if (ch != ERR) {
				if (ch == '\n') {
					if (input_len > 0) {
						if (input[0] == '/') {
							send_command(chat_server, input);

							if (strncmp(input, "/name", strlen("/name")) == 0) {
								sscanf(input + strlen("/name "), "%s", name);

							} else if (strncmp(input, "/whisper", strlen("/whisper")) == 0) {
								char whispered_msg[300];
								generate_whispered_message(whispered_msg, sizeof(whispered_msg), input);
								add_message(&messages, &messages_tail, whispered_msg, &count);

							} else if (strncmp(input, "/save", strlen("/save")) == 0) {
								save_chat_contents(messages);

							} else if (strncmp(input, "/socker", strlen("/socker")) == 0) {
								socker = true;
								show_messages = false;

								hide_message_windows(messages_win, messages_text_win, input_win);

							} else if (strncmp(input, "/help", strlen("/help")) == 0) {
								help = true;
								show_messages = false;

								hide_message_windows(messages_win, messages_text_win, input_win);
								draw_help_window(help_win);

							} else if (strncmp(input, "/gamble", strlen("/gamble")) == 0) {
								gamble(&gamble_amount);

								char gamble_msg[32];
								snprintf(gamble_msg, sizeof(gamble_msg), "You now have %d coins\n", gamble_amount);
								add_message(&messages, &messages_tail, gamble_msg, &count);
							}

						} else {
							send_message(chat_server, name, input);

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

					hide_socker_window(socker_win);
					refresh_input_window(input_win, input, &input_len, x - 2);
					refresh_messages_window(messages_win, messages_text_win, &messages, &count, max_lines, x - 2);

					char leave_buf[15];
					snprintf(leave_buf, sizeof(leave_buf), "/leave %d\n", id);
					int leave_len = strlen(leave_buf);
					if (sendall(chat_server, leave_buf, &leave_len) == -1) {
						fprintf(stderr, "failed sending to server\n");
						perror("sendall");
					}

				} else {
					handle_socker_input(ch, socker_server, id);
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

		if (ch == KEY_RESIZE) {
			endwin();
			clear();
			refresh();

			getmaxyx(stdscr, y, x);
			max_lines = y - 5;

			resize_windows(messages_win, input_win, messages_text_win, socker_win, help_win, y, x);

			if (show_messages) {
				refresh_input_window(input_win, input, &input_len, x - 2);
				refresh_messages_window(messages_win, messages_text_win, &messages, &count, max_lines, x - 2);
			} else if (socker) {
				refresh_socker_window(socker_win, &socker_data);
			} else if (help) {
				draw_help_window(help_win);
			}
		}
	}

	endwin();
	close(chat_server);
	close(socker_server);

	return 0;
}