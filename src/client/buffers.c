#include "../../include/client/buffers.h"

// Write whispered message to a buffer
void generate_whispered_message(char *whispered_msg, size_t size, char *input) {
	char name_buf[20];
	size_t name_len = strcspn(input + 9, " ");

	if (name_len >= sizeof(name_buf)) {
		name_len = sizeof(name_buf) - 1;
	}

	memcpy(name_buf, input + 9, name_len);
	name_buf[name_len] = '\0';

	char whispered_to[] = "whispered to ";

	snprintf(whispered_msg, size, "(%s%s) %s", whispered_to, name_buf, input + 9 + name_len + 1);
}

// Send message to server to store name on initial join
int send_join_message(int server, char *name) {
	char com_buf[27];
	strcpy(com_buf, "/name ");
	strcpy(com_buf + 6, name);

	int com_len = strlen(com_buf);

	if (sendall(server, com_buf, &com_len) == -1) {
		return -2;
	}

	return 0;
}

// Set name locally and on server
void set_name(int server, char *name, size_t size) {
	printf("Enter name to connect to chatroom with: ");
	if (fgets(name, size, stdin) == NULL) {
		fprintf(stderr, "error reading input stream\n");
		exit(1);
	}

	if (send_join_message(server, name) == -2) {
		fprintf(stderr, "error sending joining message\n");
		exit(1);
	}

	int pos = strcspn(name, "\n");
	name[pos] = '\0';
}

// Save recent chat contents
void save_chat_contents(struct message *messages) {

	time_t now = time(NULL);

	struct tm *t = localtime(&now);

	char filename[20];
	sprintf(filename, "chat_%02d_%02d_%02d.txt", t->tm_hour, t->tm_min, t->tm_sec);

	FILE *fp = fopen(filename, "wb");

	if (fp == NULL) {
		perror("fopen");
		exit(1);
	}

	struct message *p = messages;
	while (p != NULL) {
		fwrite(p->content, sizeof(char), strlen(p->content), fp);
		p = p->next;
	}
	fclose(fp);
}