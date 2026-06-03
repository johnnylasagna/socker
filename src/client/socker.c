#include "../../include/client/socker.h"
#include "../../include/client/network.h"

void init_socker(struct Socker *socker, int id) {
	socker->player_count = 0;
	socker->player_size = id + 1;

	socker->player_positions = malloc(sizeof(int[2]) * socker->player_size);

	if (socker->player_positions == NULL) {
		perror("malloc");
		exit(1);
	}

	socker->field_size[0] = 80;
	socker->field_size[1] = 24;

	socker->ball_position[0] = socker->field_size[0] / 2;
	socker->ball_position[1] = socker->field_size[1] / 2;
}

void resize_socker(struct Socker *socker, int count) {
	socker->player_size = 2 * count + 1;
	socker->player_positions = realloc(socker->player_positions, sizeof(int[2]) * socker->player_size);
	if (socker->player_positions == NULL) {
		perror("realloc");
		exit(1);
	}
}

void handle_socker_data(char *buf, int *id, struct Socker *socker) {
	char *line = strtok(buf, "\n");

	while (line != NULL) {
		int pos_x, pos_y;

		if (strncmp(line, "/data id", strlen("/data id")) == 0) {
			if (sscanf(line + strlen("/data id"), "%d", id) != 1) {
				fprintf(stderr, "id malformed\n");
			} else {
				init_socker(socker, *id);
			}

		} else if (strncmp(line, "/data count", strlen("/data count")) == 0) {
			int count;
			if (sscanf(line + strlen("/data count"), "%d", &count) == 1) {
				socker->player_count = count;
				resize_socker(socker, count);
			}

		} else if (strncmp(line, "/data ball", strlen("/data ball")) == 0) {
			if (sscanf(line + strlen("/data ball"), "%d %d", &pos_x, &pos_y) == 2) {
				socker->ball_position[0] = pos_x;
				socker->ball_position[1] = pos_y;
			}

		} else if (strncmp(line, "/data player", strlen("/data player")) == 0) {
			int pos_id;

			if (sscanf(line + strlen("/data player"), "%d %d %d", &pos_id, &pos_x, &pos_y) == 3) {
				socker->player_positions[pos_id][0] = pos_x;
				socker->player_positions[pos_id][1] = pos_y;
			}
		}

		line = strtok(NULL, "\n");
	}
}

void handle_socker_input(char ch, int socker_server, int id) {
	if (ch == 'w') {
		char data_buf[15];
		snprintf(data_buf, sizeof(data_buf), "/data %d 0 2\n", id);

		int data_len = strlen(data_buf);

		if (send(socker_server, data_buf, data_len, 0) == -1) {
			fprintf(stderr, "failed sending to server\n");
			perror("sendall");
		}

	} else if (ch == 'a') {
		char data_buf[15];
		snprintf(data_buf, sizeof(data_buf), "/data %d 2 0\n", id);

		int data_len = strlen(data_buf);
		if (send(socker_server, data_buf, data_len, 0) == -1) {
			fprintf(stderr, "failed sending to server\n");
			perror("sendall");
		}

	} else if (ch == 's') {
		char data_buf[15];
		snprintf(data_buf, sizeof(data_buf), "/data %d 0 1\n", id);

		int data_len = strlen(data_buf);
		if (send(socker_server, data_buf, data_len, 0) == -1) {
			fprintf(stderr, "failed sending to server\n");
			perror("sendall");
		}

	} else if (ch == 'd') {
		char data_buf[15];
		snprintf(data_buf, sizeof(data_buf), "/data %d 1 0\n", id);

		int data_len = strlen(data_buf);
		if (send(socker_server, data_buf, data_len, 0) == -1) {
			fprintf(stderr, "failed sending to server\n");
			perror("sendall");
		}
	}
}