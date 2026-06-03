#include "../../include/client/network.h"

// Getting address regardless of ipv6 or ipv4
void *get_in_addr(struct sockaddr *sa) {
	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in *)sa)->sin_addr);
	} else {
		return &(((struct sockaddr_in6 *)sa)->sin6_addr);
	}
}

// Getting server socket from ip and port
int get_chat_server_socket(const char *server_name, const char *port) {
	int server = -1;
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

	fcntl(server, F_SETFL, O_NONBLOCK);

	return server;
}

int get_socker_server_socket(const char *server_name, const char *port) {
	int server;
	struct addrinfo hints, *ai, *p;

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_DGRAM;

	int rv;
	if ((rv = getaddrinfo(server_name, port, &hints, &ai)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return -1;
	}

	for (p = ai; p != NULL; p = p->ai_next) {

		server = socket(
		    p->ai_family,
		    p->ai_socktype,
		    p->ai_protocol);

		if (server == -1) {
			perror("client: udp socket");
			continue;
		}

		if (connect(server,
		            p->ai_addr,
		            p->ai_addrlen) == -1) {

			close(server);
			perror("client: udp connect");
			continue;
		}

		break;
	}

	freeaddrinfo(ai);

	if (p == NULL) {
		fprintf(stderr, "client: failed to connect\n");
		return -1;
	}

	fcntl(server, F_SETFL, O_NONBLOCK);

	return server;
}

void send_command(int chat_server, const char *input) {
	char msg[280];
	snprintf(msg, sizeof(msg), "%s\n", input);
	int msg_len = strlen(msg);
	if (sendall(chat_server, msg, &msg_len) == -1) {
		fprintf(stderr, "failed sending to server\n");
		perror("sendall");
	}
}

void send_message(int chat_server, const char *name, const char *input) {
	char msg[280];
	snprintf(msg, sizeof(msg), "%s:%s\n", name, input);
	int msg_len = strlen(msg);
	if (sendall(chat_server, msg, &msg_len) == -1) {
		fprintf(stderr, "failed sending to server\n");
		perror("sendall");
	}
}

int sendall(int chat_server, char *buf, int *len) {
	int total = 0;
	int bytesleft = *len;
	int n = 0;

	while (total < *len) {
		n = send(chat_server, buf + total, bytesleft, 0);
		if (n == -1) {
			break;
		}
		total += n;
		bytesleft -= n;
	}

	*len = total;
	if (n == -1) {
		return -1;
	} else {
		return 0;
	}
}

void gamble(int *gamble_amount) {
	if (rand() % 2 == 0) {
		*gamble_amount *= 2;
	} else {
		*gamble_amount = 1;
	}
}