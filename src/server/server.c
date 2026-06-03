#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <poll.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "../../include/server/globals.h"
#include "../../include/server/network.h"
#include "../../include/server/socker.h"
#include "../../include/server/whisper.h"

char names[__FD_SETSIZE][20];
bool debug = true;

int main(int argc, char *argv[]) {

	// ---- Server setup ----
	int chat_listener, socker_listener;

	int fd_size = 10;
	int fd_count = 0;
	struct pollfd *pfds = malloc(sizeof *pfds * fd_size);

	if (argc != 2) {
		fprintf(stderr, "Usage: pollserver <port>");
		exit(1);
	}

	const char *port = argv[1];

	// ---- Chat server setup ----
	chat_listener = get_chat_listener_socket(port);

	if (chat_listener == -1) {
		fprintf(stderr, "error getting listening socket\n");
		exit(1);
	}

	pfds[0].fd = chat_listener;
	pfds[0].events = POLLIN;

	fd_count = 1;

	puts("pollserver: waiting for connections...");

	// ---- Socker server setup ----

	struct Socker socker;
	init_socker(&socker);

	socker_listener = get_socker_listener_socket(&socker, port);
	if (socker_listener == -1) {
		fprintf(stderr, "error getting socker socker\n");
		exit(1);
	}

	puts("socker: server successfully started\n");

	pfds[1].fd = socker_listener;
	pfds[1].events = POLLIN;

	fd_count = 2;

	// Main loop
	while (true) {
		int poll_count = poll(pfds, fd_count, -1);

		if (poll_count == -1) {
			perror("poll");
			exit(1);
		}

		process_connections(chat_listener, socker_listener, &fd_count, &fd_size, &pfds, &socker);
	}

	return 0;
}