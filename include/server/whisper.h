#ifndef SERVER_WHISPER_H
#define SERVER_WHISPER_H

#include <stdio.h>
#include <string.h>
#include <sys/socket.h>

#include "globals.h"

// Get file descriptor to send whispered message to
int get_fd_from_whispered_name(char *name_buf);

// Write whispered message
void write_whispered_message(char *buf, char *whisper_msg_with_name, int size, int name_len, int *sender_fd);

#endif // SERVER_WHISPER_H
