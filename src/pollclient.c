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

// To handle user pressing ctrl+c or ctrl+v to end program
void handle_sigint(int sig) {
	endwin();
	printf("\nDisconnected from chatroom.\n");
	exit(0);
}

// Getting address regardless of ipv6 or ipv4
void *get_in_addr(struct sockaddr *sa) {
	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in *)sa)->sin_addr);
	} else {
		return &(((struct sockaddr_in6 *)sa)->sin6_addr);
	}
}

// Getting server socket from ip and port
int get_server_socket(const char *server_name, const char *port) {
	int server;
	int rv;

	struct addrinfo hints, *ai, *p;
	char s[INET6_ADDRSTRLEN];

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;

	if ((rv = getaddrinfo(server_name, port, &hints, &ai)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return -1;
	}

	for (p = ai; p != NULL; p = p->ai_next) {
		if ((server = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) ==
		    -1) {
			perror("client: socket");
			continue;
		}
		inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr), s, sizeof s);
		printf("client: attempting connection to %s\n", s);
		if (connect(server, p->ai_addr, p->ai_addrlen) == -1) {
			perror("client:connect");
			close(server);
			continue;
		}
		break;
	}

	if (p == NULL) {
		fprintf(stderr, "client: failed to connect\n");
		return -1;
	}

	freeaddrinfo(ai);

	return server;
}

// Linked list that gets messages
struct message {
	char content[512];
	struct message *next;
};

// Add message to message linked list
void add_message(struct message **messages, struct message **messages_tail, char *msg, int *count) {
	struct message *new_msg = malloc(sizeof(struct message));
	if (new_msg == NULL) {
		perror("Failed to allocate memory for new message");
		return;
	}

	snprintf(new_msg->content, sizeof(new_msg->content), "%s", msg);
	new_msg->next = NULL;

	if (*messages == NULL) {
		*messages = new_msg;
		*messages_tail = new_msg;
	} else {
		(*messages_tail)->next = new_msg;
		*messages_tail = new_msg;
	}
	(*count)++;
}

// Write whispered message to a buffer
void generate_whispered_message(char *whispered_msg, size_t size, char *input) {
	char name_buf[20];
	size_t name_len = strcspn(input + 9, " ");

	if (name_len >= sizeof(name_buf)) {
		name_len = sizeof(name_buf) - 1;
	}

	memcpy(name_buf, input + 9, name_len);
	name_buf[name_len] = '\0';

	char whispered_to[] = "whispered to ";

	snprintf(whispered_msg, size, "(%s%s) %s", whispered_to, name_buf, input + 9 + name_len + 1);
}

// Send message to server to store name on initial join
int send_join_message(int server, char *name) {
	char com_buf[27];
	strcpy(com_buf, "/name ");
	strcpy(com_buf + 6, name);
	if (send(server, com_buf, strlen(com_buf), 0) == -1) {
		return -2;
	}

	return 0;
}

// Set name locally and on server
void set_name(int server, char *name, size_t size) {
	printf("Enter name to connect to chatroom with: ");
	fgets(name, size, stdin);

	if (send_join_message(server, name) == -2) {
		fprintf(stderr, "error sending joining message\n");
		exit(1);
	}

	int pos = strcspn(name, "\n");
	name[pos] = '\0';
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

// Save recent chat contents
void save_chat_contents(struct message *messages) {

	time_t now = time(NULL);

	struct tm *t = localtime(&now);

	char filename[20];
	sprintf(filename, "chat_%02d_%02d_%02d.txt", t->tm_hour, t->tm_min, t->tm_sec);

	FILE *fp = fopen(filename, "wb");

	if (fp == NULL) {
		perror("fopen");
		exit(1);
	}

	struct message *p = messages;
	while (p != NULL) {
		fwrite(p->content, sizeof(char), strlen(p->content), fp);
		p = p->next;
	}
	fclose(fp);
}

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