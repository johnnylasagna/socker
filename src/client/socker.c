#include "../../include/client/socker.h"
#include "../../include/server/network.h"

void handle_socker_input(char ch, int server, int id) {
	if (ch == 'w') {
		char data_buf[15];
		snprintf(data_buf, sizeof(data_buf), "/data %d 0 2\n", id);

		int data_len = strlen(data_buf);

		if (sendall(server, data_buf, &data_len) == -1) {
			fprintf(stderr, "failed sending to server\n");
			perror("sendall");
		}

	} else if (ch == 'a') {
		char data_buf[15];
		snprintf(data_buf, sizeof(data_buf), "/data %d 2 0\n", id);

		int data_len = strlen(data_buf);
		if (sendall(server, data_buf, &data_len) == -1) {
			fprintf(stderr, "failed sending to server\n");
			perror("sendall");
		}

	} else if (ch == 's') {
		char data_buf[15];
		snprintf(data_buf, sizeof(data_buf), "/data %d 0 1\n", id);

		int data_len = strlen(data_buf);
		if (sendall(server, data_buf, &data_len) == -1) {
			fprintf(stderr, "failed sending to server\n");
			perror("sendall");
		}

	} else if (ch == 'd') {
		char data_buf[15];
		snprintf(data_buf, sizeof(data_buf), "/data %d 1 0\n", id);

		int data_len = strlen(data_buf);
		if (sendall(server, data_buf, &data_len) == -1) {
			fprintf(stderr, "failed sending to server\n");
			perror("sendall");
		}
	}
}