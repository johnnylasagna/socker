#include "../../include/server/network.h"

// Convert IP address into printable format
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

// Get listener socket for server
int get_listener_socket(const char *port) {
	int listener;
	int yes = 1;
	int rv;

	struct addrinfo hints, *ai, *p;

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	if ((rv = getaddrinfo(NULL, port, &hints, &ai)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return 1;
	}

	for (p = ai; p != NULL; p = p->ai_next) {
		if ((listener = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) <
		    0) {
			perror("listener: socket");
			continue;
		}

		setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));

		if (bind(listener, p->ai_addr, p->ai_addrlen) == -1) {
			close(listener);
			perror("listener: bind");
			continue;
		}
		break;
	}

	if (p == NULL) {
		fprintf(stderr, "server: failed to bind\n");
		return -1;
	}

	freeaddrinfo(ai);

	if (listen(listener, BACKLOG) == -1) {
		perror("listen");
		return -1;
	}

	return listener;
}

// Send to all other clients
void send_to_all_clients(int listener, int *fd_count, struct pollfd *pfds, int *sender_fd, char *buf, size_t size) {
	for (int j = 0; j < *fd_count; j++) {
		int dest_fd = pfds[j].fd;

		if (dest_fd != listener && dest_fd != *sender_fd) {
			int buf_len = strlen(buf);
			if (sendall(dest_fd, buf, &buf_len) == -1) {
				perror("send");
			}
		}
	}
}

// Add connected client to list
void add_to_pfds(struct pollfd **pfds, int newfd, int *fd_count, int *fd_size) {
	if (*fd_count == *fd_size) {
		*fd_size *= 2;
		*pfds = realloc(*pfds, sizeof(**pfds) * (*fd_size));
	}

	(*pfds)[*fd_count].fd = newfd;
	(*pfds)[*fd_count].events = POLLIN;
	(*pfds)[*fd_count].revents = 0;

	(*fd_count)++;
}

// Delete disconnected client from list
void del_from_pfds(struct pollfd pfds[], int i, int *fd_count) {
	pfds[i] = pfds[*fd_count - 1];
	(*fd_count)--;
}

// Handle new connection
void handle_new_connection(int listener, int *fd_count, int *fd_size, struct pollfd **pfds) {
	struct sockaddr_storage remoteaddr;
	socklen_t addrlen;
	int newfd;
	char remoteIP[INET6_ADDRSTRLEN];

	addrlen = sizeof remoteaddr;
	newfd = accept(listener, (struct sockaddr *)&remoteaddr, &addrlen);

	if (newfd == -1) {
		perror("accept");
	} else {
		add_to_pfds(pfds, newfd, fd_count, fd_size);

		printf("pollserver: new connection from %s on socket %d\n",
		       inet_ntop2(&remoteaddr, remoteIP, sizeof remoteIP), newfd);
	}
}

// Handle client messages
void handle_client_data(int listener, int *fd_count, struct pollfd *pfds, int *pfd_i, struct Socker *socker) {
	char buf[256];

	int n = recv(pfds[*pfd_i].fd, buf, sizeof(buf) - 1, 0);
	int sender_fd = pfds[*pfd_i].fd;

	if (n <= 0) {
		// Connection closed
		if (n == 0) {
			printf("pollserver: socket %d hung up\n", sender_fd);
		} else {
			perror("recv");
		}

		close(pfds[*pfd_i].fd);

		del_from_pfds(pfds, *pfd_i, fd_count);

		(*pfd_i)--;
	} else {
		buf[n] = '\0';
		printf("pollserver: recv from fd %d: %.*s", sender_fd, n, buf);

		if (buf[0] == '/') {
			if (strncmp(buf, "/quit", 5) == 0) {
				char quit_buf[40];
				snprintf(quit_buf, sizeof(quit_buf), "%s left\n", names[sender_fd]);

				memset(names[*pfd_i], 0, sizeof(names[sender_fd]));
				close(sender_fd);
				del_from_pfds(pfds, *pfd_i, fd_count);

				send_to_all_clients(listener, fd_count, pfds, &sender_fd, quit_buf, strlen(quit_buf));

			} else if (strncmp(buf, "/name", 5) == 0) {
				size_t len = strcspn(buf + 6, "\n");

				if (len >= sizeof(names[sender_fd]))
					len = sizeof(names[sender_fd]) - 1;

				memcpy(names[sender_fd], buf + 6, len);
				names[sender_fd][len] = '\0';
				printf("Name saved at fd %d: %s\n", sender_fd, names[sender_fd]);

				char name_buf[40];
				snprintf(name_buf, sizeof(name_buf), "%s just joined us\n", names[sender_fd]);

				send_to_all_clients(listener, fd_count, pfds, &sender_fd, name_buf, strlen(name_buf));

			} else if (strncmp(buf, "/whisper ", 9) == 0) {
				char whisper_msg_with_name[280];
				size_t whisper_msg_with_name_size = sizeof(whisper_msg_with_name);

				char name_buf[20];
				size_t name_len = strcspn(buf + 9, " ");

				if (name_len >= sizeof(name_buf)) {
					name_len = sizeof(name_buf) - 1;
				}

				memcpy(name_buf, buf + 9, name_len);
				name_buf[name_len] = '\0';

				write_whispered_message(buf, whisper_msg_with_name, whisper_msg_with_name_size, name_len, &sender_fd);

				int whispered_fd = get_fd_from_whispered_name(name_buf);
				if (whispered_fd != -1) {
					int whisper_len = strlen(whisper_msg_with_name);
					if (sendall(whispered_fd, whisper_msg_with_name, &whisper_len) == -1) {
						perror("send");
					}
				}

			} else if (strncmp(buf, "/save", 5) == 0) {
				char save_buf[40];
				snprintf(save_buf, sizeof(save_buf), "%s saved the chat locally\n", names[sender_fd]);
				send_to_all_clients(listener, fd_count, pfds, &sender_fd, save_buf, strlen(save_buf));

			} else if (strncmp(buf, "/socker", 7) == 0) {
				int id = add_player_to_socker(socker, sender_fd);
				char id_buf[22];
				snprintf(id_buf, sizeof(id_buf), "/data id %d\n", id);
				int id_len = strlen(id_buf);
				sendall(sender_fd, id_buf, &id_len);

				char socker_buf[40];
				snprintf(socker_buf, sizeof(socker_buf), "%s joined socker\n", names[sender_fd]);
				send_to_all_clients(listener, fd_count, pfds, &sender_fd, socker_buf, strlen(socker_buf));

				send_player_data(socker);

			} else if (strncmp(buf, "/data", 5) == 0) {
				update_positions(socker, buf);
				send_player_data(socker);

			} else if (strncmp(buf, "/leave", 6) == 0) {
				int id;

				if (sscanf(buf + strlen("/leave"), "%d", &id) != 1) {
					fprintf(stderr, "malformed leave received\n");
					exit(1);
				}

				int old_id = delete_player(socker, id);

				char new_id_buf[20];
				snprintf(new_id_buf, sizeof(new_id_buf), "/data id %d\n", id);

				int new_id_len = strlen(new_id_buf);
				sendall(socker->player_fds[old_id], new_id_buf, &new_id_len);
				send_player_data(socker);
			}
		} else {
			send_to_all_clients(listener, fd_count, pfds, &sender_fd, buf, strlen(buf));
		}
	}
}

// Process connections
void process_connections(int listener, int *fd_count, int *fd_size, struct pollfd **pfds, struct Socker *socker) {
	for (int i = 0; i < *fd_count; i++) {
		if ((*pfds)[i].revents & (POLLIN | POLLHUP)) {
			if ((*pfds)[i].fd == listener) {
				handle_new_connection(listener, fd_count, fd_size, pfds);
			} else {
				handle_client_data(listener, fd_count, *pfds, &i, socker);
			}
		}
	}
}

int sendall(int s, char *buf, int *len) {
	int total = 0;
	int bytesleft = *len;
	int n = 0;

	while (total < *len) {
		n = send(s, buf + total, bytesleft, 0);
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