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

	// Listener setup
	int listener;

	int fd_size = 10;
	int fd_count = 0;
	struct pollfd *pfds = malloc(sizeof *pfds * fd_size);

	if (argc != 2) {
		fprintf(stderr, "Usage: pollserver <port>");
		exit(1);
	}

	const char *port = argv[1];

	listener = get_listener_socket(port);

	if (listener == -1) {
		fprintf(stderr, "error getting listening socket\n");
		exit(1);
	}

	pfds[0].fd = listener;
	pfds[0].events = POLLIN;

	fd_count = 1;

	puts("pollserver: waiting for connections...");

	// Socker setup

	struct Socker socker;
	init_socker(&socker);

	// Main loop
	for (;;) {
		int poll_count = poll(pfds, fd_count, -1);

		if (poll_count == -1) {
			perror("poll");
			exit(1);
		}

		process_connections(listener, &fd_count, &fd_size, &pfds, &socker);
	}

	return 0;
}