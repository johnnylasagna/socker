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

struct message {
	char content[256];
	struct message *next;
};

int main(int argc, char *argv[]) {

	int server;

	if (argc != 3) {
		fprintf(stderr, "Usage: pollclient <server> <port>");
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

	char name[20];
	printf("Enter name to connect to chatroom with: ");
	fgets(name, sizeof name, stdin);

	struct message *messages = NULL;
	struct message *messages_tail = NULL;
	int count = 0;

	char input[256] = {0};
	int input_len = 0;

	int pos = strcspn(name, "\n");
	name[pos] = ':';
	name[pos + 1] = '\0';

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

	WINDOW *messages_win = newwin(y - 3, x, 0, 0);
	WINDOW *message_win = newwin(3, x, y - 3, 0);

	keypad(message_win, TRUE);
	nodelay(message_win, TRUE);

	werase(messages_win);
	box(messages_win, 0, 0);
	wrefresh(messages_win);

	werase(message_win);
	box(message_win, 0, 0);
	mvwprintw(message_win, 1, 1, "> ");
	wrefresh(message_win);

	for (;;) {
		if (poll(&pfd, 1, 50) == -1) {
			endwin();
			perror("poll");
			exit(1);
		}

		if (pfd.revents & POLLIN) {
			char buf[256];

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

			struct message *new_msg = malloc(sizeof(struct message));
			strncpy(new_msg->content, buf, 255);
			new_msg->content[255] = '\0';
			new_msg->next = NULL;

			if (messages == NULL) {
				messages = new_msg;
				messages_tail = new_msg;
			} else {
				messages_tail->next = new_msg;
				messages_tail = new_msg;
			}
			count++;

			werase(messages_win);

			box(messages_win, 0, 0);

			int max_lines = y - 5;

			if (count > max_lines) {
				struct message *temp = messages;
				messages = messages->next;
				free(temp);
				count--;
			}

			int row = 1;

			struct message *p = messages;
			while (p != NULL) {
				mvwprintw(messages_win, row++, 1, "%s", p->content);
				p = p->next;
			}

			wrefresh(messages_win);
		}

		int ch = wgetch(message_win);

		if (ch != ERR) {
			if (ch == '\n') {
				if (input_len > 0) {
					if (strcmp(input, "/quit") == 0) {
						break;
					}

					char msg[512];

					snprintf(msg, sizeof(msg), "%s%s\n", name, input);

					send(server, msg, strlen(msg), 0);

					struct message *new_msg = malloc(sizeof(struct message));
					snprintf(new_msg->content, sizeof(new_msg->content), "You:%s\n", input);
					new_msg->next = NULL;

					// Append to list
					if (messages == NULL) {
						messages = new_msg;
						messages_tail = new_msg;
					} else {
						messages_tail->next = new_msg;
						messages_tail = new_msg;
					}
					count++;

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

			werase(message_win);

			box(message_win, 0, 0);

			mvwprintw(message_win, 1, 1, "> %s", input);

			wmove(message_win, 1, 3 + input_len);

			wrefresh(message_win);

			werase(messages_win);

			box(messages_win, 0, 0);

			int max_lines = y - 5;

			if (count > max_lines) {
				struct message *temp = messages;
				messages = messages->next;
				free(temp);
				count--;
			}

			int row = 1;

			struct message *p = messages;
			while (p != NULL) {
				mvwprintw(messages_win, row++, 1, "%s", p->content);
				p = p->next;
			}

			wrefresh(messages_win);
		}
	}

	endwin();
	close(server);

	return 0;
}
