#include "../../include/server/whisper.h"

// Get file descriptor to send whispered message to
int get_fd_from_whispered_name(char *name_buf) {
	for (int i = 0; i < __FD_SETSIZE; i++) {
		if (strcmp(names[i], name_buf) == 0) {
			return i;
		}
	}
	return -1;
}

// Write whispered message
void write_whispered_message(char *buf, char *whisper_msg_with_name, int size, int name_len, int *sender_fd) {
	const char *msg_start = buf + name_len + 10;
	size_t msg_len = strlen(msg_start);

	char whisper_msg[256];
	if (msg_len >= sizeof(whisper_msg)) {
		msg_len = sizeof(whisper_msg) - 1;
	}

	memcpy(whisper_msg, msg_start, msg_len);
	whisper_msg[msg_len] = '\0';

	snprintf(whisper_msg_with_name, size, "(%s) %s", names[*sender_fd], whisper_msg);
}