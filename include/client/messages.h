#ifndef CLIENT_MESSAGES_H
#define CLIENT_MESSAGES_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Linked list that gets messages
struct message {
	char content[512];
	struct message *next;
};

// Add message to message linked list
void add_message(struct message **messages, struct message **messages_tail, char *msg, int *count);

#endif // CLIENT_MESSAGES_H
