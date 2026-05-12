#include "../../include/client/messages.h"

// Linked list that gets messages
struct message {
	char content[512];
	struct message *next;
};

// Add message to message linked list
void add_message(struct message **messages, struct message **messages_tail, char *msg, int *count) {
	struct message *new_msg = malloc(sizeof(struct message));
	if (new_msg == NULL) {
		perror("Failed to allocate memory for new message");
		return;
	}

	snprintf(new_msg->content, sizeof(new_msg->content), "%s", msg);
	new_msg->next = NULL;

	if (*messages == NULL) {
		*messages = new_msg;
		*messages_tail = new_msg;
	} else {
		(*messages_tail)->next = new_msg;
		*messages_tail = new_msg;
	}
	(*count)++;
}