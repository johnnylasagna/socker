#include "../../include/server/network.h"
#include "../../include/server/socker.h"

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

// Getting address regardless of ipv6 or ipv4
void *get_in_addr(struct sockaddr *sa) {
	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in *)sa)->sin_addr);
	} else {
		return &(((struct sockaddr_in6 *)sa)->sin6_addr);
	}
}

// Get listener socket for chat server
int get_chat_listener_socket(const char *port) {
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

// Get listener socket for socker server
int get_socker_listener_socket(struct Socker *socker, const char *port) {
	int sockfd;
	struct addrinfo hints, *servinfo, *p;
	int rv;

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_flags = AI_PASSIVE;

	if ((rv = getaddrinfo(NULL, port, &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return 1;
	}

	for (p = servinfo; p != NULL; p = p->ai_next) {
		if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) ==
		    -1) {
			perror("listener: socket");
			continue;
		}

		if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
			close(sockfd);
			perror("listener: bind");
			continue;
		}
		break;
	}

	if (p == NULL) {
		fprintf(stderr, "Listener: failed to bind socket\n");
		exit(1);
	}

	freeaddrinfo(servinfo);

	socker->server = sockfd;

	return sockfd;
}

// Send to all other clients
void send_to_all_clients(int listener, int socker_listener, int *fd_count, struct pollfd *pfds, int *sender_fd, char *buf, size_t size) {
	for (int j = 0; j < *fd_count; j++) {
		int dest_fd = pfds[j].fd;

		if (dest_fd != listener && dest_fd != socker_listener && dest_fd != *sender_fd) {
			int buf_len = strlen(buf);
			if (sendall(dest_fd, buf, &buf_len) == -1) {
				perror("send");
			}
		}
	}

	(void)size;
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
void handle_client_data(int listener, int socker_listener, int *fd_count, struct pollfd *pfds, int *pfd_i, struct Socker *socker) {
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
			if (strncmp(buf, "/quit", strlen("/quit")) == 0) {
				memset(names[sender_fd], 0, sizeof(names[sender_fd]));
				close(sender_fd);
				del_from_pfds(pfds, *pfd_i, fd_count);

				char quit_buf[40];
				snprintf(quit_buf, sizeof(quit_buf), "%s left\n", names[sender_fd]);
				send_to_all_clients(listener, socker_listener, fd_count, pfds, &sender_fd, quit_buf, strlen(quit_buf));

			} else if (strncmp(buf, "/name ", strlen("/name ")) == 0) {
				size_t len = strcspn(buf + strlen("/name "), "\n");

				if (len >= sizeof(names[sender_fd]))
					len = sizeof(names[sender_fd]) - 1;

				memcpy(names[sender_fd], buf + strlen("/name "), len);
				names[sender_fd][len] = '\0';
				printf("Name saved at fd %d: %s\n", sender_fd, names[sender_fd]);

				char name_buf[40];
				snprintf(name_buf, sizeof(name_buf), "%s just joined us\n", names[sender_fd]);
				send_to_all_clients(listener, socker_listener, fd_count, pfds, &sender_fd, name_buf, strlen(name_buf));

			} else if (strncmp(buf, "/whisper ", strlen("/whisper ")) == 0) {
				char whisper_msg_with_name[280];
				size_t whisper_msg_with_name_size = sizeof(whisper_msg_with_name);

				char name_buf[20];
				size_t name_len = strcspn(buf + strlen("/whisper "), " ");

				if (name_len >= sizeof(name_buf)) {
					name_len = sizeof(name_buf) - 1;
				}

				memcpy(name_buf, buf + strlen("/whisper "), name_len);
				name_buf[name_len] = '\0';
				write_whispered_message(buf, whisper_msg_with_name, whisper_msg_with_name_size, name_len, &sender_fd);

				int whispered_fd = get_fd_from_whispered_name(name_buf);

				if (whispered_fd != -1) {
					int whisper_len = strlen(whisper_msg_with_name);
					if (sendall(whispered_fd, whisper_msg_with_name, &whisper_len) == -1) {
						perror("send");
					}
				}

			} else if (strncmp(buf, "/save", strlen("/save")) == 0) {
				char save_buf[40];
				snprintf(save_buf, sizeof(save_buf), "%s saved the chat locally\n", names[sender_fd]);
				send_to_all_clients(listener, socker_listener, fd_count, pfds, &sender_fd, save_buf, strlen(save_buf));

			} else if (strncmp(buf, "/socker", strlen("/socker")) == 0) {
				char socker_buf[40];
				snprintf(socker_buf, sizeof(socker_buf), "%s joined socker\n", names[sender_fd]);
				send_to_all_clients(listener, socker_listener, fd_count, pfds, &sender_fd, socker_buf, strlen(socker_buf));

				struct sockaddr_storage udp_addr;

				int id = add_player_to_socker(socker, &udp_addr, sender_fd);
				char id_buf[22];
				snprintf(id_buf, sizeof(id_buf), "/data id %d\n", id);

				int id_len = strlen(id_buf);

				if (sendall(sender_fd, id_buf, &id_len) == -1) {
					perror("send");
				}

				send_all_player_data(socker, id);

			} else if (strncmp(buf, "/leave ", strlen("/leave ")) == 0) {
				int id;

				if (sscanf(buf + strlen("/leave "), "%d", &id) != 1) {
					fprintf(stderr, "malformed leave received\n");
					return;
				}

				int old_id = delete_player(socker, id);

				char new_id_buf[20];
				snprintf(new_id_buf, sizeof(new_id_buf), "/data id %d\n", id);
				int new_id_len = strlen(new_id_buf);

				if (sendall(socker->player_tcp_fds[old_id], new_id_buf, &new_id_len) == -1) {
					perror("send");
				}

				for (int i = 0; i < socker->player_count; i++) {
					send_all_player_data(socker, i);
				}
			}
		} else {
			send_to_all_clients(listener, socker_listener, fd_count, pfds, &sender_fd, buf, strlen(buf));
		}
	}
}

// Handle socker data
void handle_socker_data(int socker_listener, struct Socker *socker) {
	char buf[256];

	struct sockaddr_storage client_addr;
	socklen_t addrlen = sizeof(client_addr);

	int n = recvfrom(socker_listener, buf, sizeof(buf) - 1, 0, (struct sockaddr *)&client_addr, &addrlen);

	char client_ip[INET_ADDRSTRLEN];
	inet_ntop(client_addr.ss_family,
	          get_in_addr((struct sockaddr *)&client_addr), client_ip, sizeof client_ip);

	if (n <= 0) {
		// Connection closed
		if (n == 0) {
			printf("socker server: socket %s hung up", client_ip);
		} else {
			perror("recvfrom");
		}
	} else {
		buf[n] = '\0';
		printf("socker server: recv from fd %s: %.*s", client_ip, n, buf);

		if (buf[0] == '/') {
			if (strncmp(buf, "/data", strlen("/data")) == 0) {
				update_positions(socker, buf, &client_addr);
			}
		}
	}
}

// Process connections
void process_connections(int chat_listener, int socker_listener, int *fd_count, int *fd_size, struct pollfd **pfds, struct Socker *socker) {
	// Separate pollhup and pollin later
	for (int i = 0; i < *fd_count; i++) {
		if ((*pfds)[i].revents & (POLLIN | POLLHUP)) {
			if ((*pfds)[i].fd == chat_listener) {
				handle_new_connection(chat_listener, fd_count, fd_size, pfds);

			} else if ((*pfds)[i].fd == socker_listener) {
				handle_socker_data(socker_listener, socker);

			} else {
				handle_client_data(chat_listener, socker_listener, fd_count, *pfds, &i, socker);
			}
		}
	}
}

// Fully send tcp stream to client
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