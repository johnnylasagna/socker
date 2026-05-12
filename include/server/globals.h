#ifndef SERVER_GLOBALS_H
#define SERVER_GLOBALS_H

#include <stdbool.h>
#include <sys/socket.h>

#define BACKLOG 10
extern bool debug;
extern char names[__FD_SETSIZE][20];

#endif