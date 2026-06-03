#ifndef CLIENT_NETWORK_H
#define CLIENT_NETWORK_H

#include <arpa/inet.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

// Getting address regardless of ipv6 or ipv4
void *get_in_addr(struct sockaddr *sa);

// Getting chat server socket from ip and port
int get_chat_server_socket(const char *server_name, const char *port);

// Getting socker server socket from ip and port
int get_socker_server_socket(const char *server_name, const char *port);

// Send command to server
void send_command(int chat_server, const char *input);

// Send message to server
void send_message(int chat_server, const char *name, const char *input);

// Fully send tcp stream to server
int sendall(int chat_server, char *buf, int *len);

// Gamble your money
void gamble(int *gamble_amount);

#endif // CLIENT_NETWORK_H