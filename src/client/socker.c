#include "../../include/client/socker.h"

void handle_socker_input(char ch, int server, int id) {
	if (ch == 'w') {
		char data_buf[15];
		snprintf(data_buf, sizeof(data_buf), "/data %d 0 2\n", id);
		send(server, data_buf, strlen(data_buf), 0);

	} else if (ch == 'a') {
		char data_buf[15];
		snprintf(data_buf, sizeof(data_buf), "/data %d 2 0\n", id);
		send(server, data_buf, strlen(data_buf), 0);

	} else if (ch == 's') {
		char data_buf[15];
		snprintf(data_buf, sizeof(data_buf), "/data %d 0 1\n", id);
		send(server, data_buf, strlen(data_buf), 0);

	} else if (ch == 'd') {
		char data_buf[15];
		snprintf(data_buf, sizeof(data_buf), "/data %d 1 0\n", id);
		send(server, data_buf, strlen(data_buf), 0);
	}
}