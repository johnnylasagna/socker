#ifndef CLIENT_NETWORK_H
#define CLIENT_NETWORK_H

#include <arpa/inet.h>
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

#ifdef __cplusplus
extern "C" {
#endif

// Getting address regardless of ipv6 or ipv4
void *get_in_addr(struct sockaddr *sa);

// Getting server socket from ip and port
int get_server_socket(const char *server_name, const char *port);

int sendall(int s, char *buf, int *len);

#ifdef __cplusplus
}
#endif

#endif // CLIENT_NETWORK_H