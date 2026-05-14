#ifndef CLIENT_BUFFERS_H
#define CLIENT_BUFFERS_H

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <time.h>

#include "messages.h"
#include "network.h"

// Write whispered message to a buffer
void generate_whispered_message(char *whispered_msg, size_t size, char *input);

// Send message to server to store name on initial join
int send_join_message(int server, char *name);

// Set name locally and on server
void set_name(int server, char *name, size_t size);

// Save recent chat contents
void save_chat_contents(struct message *messages);

#endif // CLIENT_BUFFERS_H
