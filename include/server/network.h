#ifndef SERVER_NETWORK_H
#define SERVER_NETWORK_H

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

#include "globals.h"
#include "whisper.h"

struct Socker;

// Convert IP address into printable format
const char *inet_ntop2(void *addr, char *buf, size_t size);

void *get_in_addr(struct sockaddr *sa);

// Get listener socket for chat server
int get_chat_listener_socket(const char *port);

// Get listener socket for socker server
int get_socker_listener_socket(struct Socker *socker, const char *port);

// Send to all other clients
void send_to_all_clients(int listener, int socker_listener, int *fd_count, struct pollfd *pfds, int *sender_fd, char *buf, size_t size);

// Add connected client to list
void add_to_pfds(struct pollfd **pfds, int newfd, int *fd_count, int *fd_size);

// Delete disconnected client from list
void del_from_pfds(struct pollfd pfds[], int i, int *fd_count);

// Handle new connection
void handle_new_connection(int listener, int *fd_count, int *fd_size, struct pollfd **pfds);

// Handle client messages
void handle_client_data(int listener, int socker_listener, int *fd_count, struct pollfd *pfds, int *pfd_i, struct Socker *socker);

// Handle socker data
void handle_socker_data(int socker_listener, struct Socker *socker);

// Handle disconnects
void remove_socker_client(struct Socker *socker, int fd);

// Process connections
void process_connections(int listener, int socker_listener, int *fd_count, int *fd_size, struct pollfd **pfds, struct Socker *socker);

// Fully send tcp stream to client
int sendall(int s, char *buf, int *len);

#endif // SERVER_NETWORK_H
