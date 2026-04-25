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

const char *inet_ntop2(void *addr, char *buf, size_t size) {
	struct sockaddr_storage *sas = addr;
	struct sockaddr_in *sa4;
	struct sockaddr_in6 *sa6;
	void *src;

	switch (sas->ss_family) {
	case AF_INET:
		sa4 = addr;
		src = &(sa4->sin_addr);
		break;
	case AF_INET6:
		sa6 = addr;
		src = &(sa6->sin6_addr);
		break;
	default:
		return NULL;
	}

	return inet_ntop(sas->ss_family, src, buf, size);
}

void *get_in_addr(struct sockaddr *sa) {
	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in *)sa)->sin_addr);
	} else {
		return &(((struct sockaddr_in6 *)sa)->sin6_addr);
	}
}

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
		return 1;
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

int main(int argc, char *argv[]) {

	int server;

	int fd_size = 2;
	int fd_count = 0;
	struct pollfd *pfds = malloc(sizeof *pfds * fd_size);

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

	pfds[0].fd = STDIN_FILENO;
	pfds[0].events = POLLIN;

	pfds[1].fd = server;
	pfds[1].events = POLLIN;

	fd_count = 2;

	char name[20];
	printf("Enter name to connect to chatroom with: ");
	fgets(name, sizeof name, stdin);

	char messages[100][256];
	int count = 0;

	name[strcspn(name, "\n")] = ':';

	char you[] = "You:";
	size_t you_length = 4;

	for (;;) {
		if (poll(pfds, fd_count, -1) == -1) {
			perror("poll");
			exit(1);
		}

		if (pfds[0].revents & POLLIN) {
			char buf[256];

			if (!fgets(buf, sizeof buf, stdin))
				break;

			strcpy(messages[count], you);
			strcpy(messages[count] + you_length, buf);
			count++;
			// printf("\033[2J\033[H");
			system("clear");
			for (int i = 0; i < count; i++) {
				printf("%s", messages[i]);
			}

			if (send(server, name, strlen(name), 0) == -1)
				perror("send");

			if (send(server, buf, strlen(buf), 0) == -1)
				perror("send");
		}

		if (pfds[1].revents & POLLIN) {
			char buf[256];

			int n = recv(server, buf, sizeof(buf) - 1, 0);

			if (n == 0) {
				printf("Server closed connection\n");
				break;
			} else if (n < 0) {
				perror("recv");
				break;
			}

			buf[n] = '\0';

			strcpy(messages[count], buf);
			count++;

			system("clear");

			for (int i = 0; i < count; i++) {
				printf("%s", messages[i]);
			}
		}
	}

	close(server);

	return 0;
}