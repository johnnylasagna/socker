#include "../../include/server/socker.h"

// Initialise socker
void init_socker(struct Socker *socker) {

	socker->player_count = 0;
	socker->player_size = 10;

	socker->player_fds = malloc(sizeof(int) * socker->player_size);

	socker->player_positions = malloc(sizeof(int[2]) * socker->player_size);

	if (socker->player_fds == NULL || socker->player_positions == NULL) {
		perror("malloc");
		exit(1);
	}

	socker->field_size[0] = 80;
	socker->field_size[1] = 24;

	socker->ball_position[0] = socker->field_size[0] / 2;
	socker->ball_position[1] = socker->field_size[1] / 2;
}

// Add player to socker
int add_player_to_socker(struct Socker *socker, int fd) {
	if (socker->player_count == socker->player_size) {
		socker->player_size *= 2;
		socker->player_fds = realloc(socker->player_fds, sizeof(int) * socker->player_size);
		socker->player_positions = realloc(socker->player_positions, sizeof(int[2]) * socker->player_size);
		if (socker->player_fds == NULL || socker->player_positions == NULL) {
			perror("realloc");
			exit(1);
		}
	}

	int idx = socker->player_count;

	socker->player_fds[idx] = fd;
	socker->player_positions[idx][0] = socker->field_size[0] / 2;
	socker->player_positions[idx][1] = socker->field_size[1] / 2;

	socker->player_count++;

	return socker->player_count - 1;
}

// Delete player from socker
int delete_player(struct Socker *socker, int index) {
	int last = socker->player_count - 1;

	socker->player_fds[index] = socker->player_fds[last];
	socker->player_positions[index][0] = socker->player_positions[last][0];
	socker->player_positions[index][1] = socker->player_positions[last][1];

	socker->player_count--;

	return last;
}

void send_player_data(struct Socker *socker) {
	char position_buf[1024];

	int offset = 0;

	offset += snprintf(
	    position_buf + offset,
	    sizeof(position_buf) - offset,
	    "/data ball %d %d\n",
	    socker->ball_position[0],
	    socker->ball_position[1]);

	for (int j = 0; j < socker->player_count;
	     j++) {
		offset += snprintf(
		    position_buf + offset,
		    sizeof(position_buf) - offset,
		    "/data player %d %d\n",
		    socker->player_positions[j][0],
		    socker->player_positions[j][1]);
	}

	for (int i = 0; i < socker->player_count; i++) {
		int dest_fd = socker->player_fds[i];

		if (send(dest_fd, position_buf, offset, 0) == -1) {
			perror("send");
		}
	}
}

void reset_ball_position(struct Socker *socker) {
	socker->ball_position[0] = socker->field_size[0] / 2;
	socker->ball_position[1] = socker->field_size[1] / 2;
}

void update_positions(struct Socker *socker, char *buf) {
	int id;
	int dx;
	int dy;

	if (sscanf(buf + 6, "%d %d %d", &id, &dx, &dy) != 3) {
		fprintf(stderr, "malformed data received\n");
		exit(1);
	}
	if (dx == 1) {
		if (socker->player_positions[id][0] != socker->field_size[0] - 1)
			socker->player_positions[id][0] += 1;
	} else if (dx == 2) {
		if (socker->player_positions[id][0] != 0)
			socker->player_positions[id][0] -= 1;
	}

	if (dy == 1) {
		if (socker->player_positions[id][1] != socker->field_size[1] - 1)
			socker->player_positions[id][1] += 1;
	} else if (dy == 2) {
		if (socker->player_positions[id][1] != 0)
			socker->player_positions[id][1] -= 1;
	}

	if (socker->player_positions[id][0] == socker->ball_position[0] &&
	    socker->player_positions[id][1] == socker->ball_position[1]) {
		if (dx == 1) {
			socker->ball_position[0] += 2;
			if (socker->ball_position[0] > socker->field_size[0] - 1)
				reset_ball_position(socker);

		} else if (dx == 2) {
			socker->ball_position[0] -= 2;
			if (socker->ball_position[0] < 0)
				reset_ball_position(socker);
			if (socker->ball_position[0] > socker->field_size[0] - 1)
				reset_ball_position(socker);
		}

		if (dy == 1) {
			socker->ball_position[1] += 2;
			if (socker->ball_position[1] > socker->field_size[1] - 1)
				reset_ball_position(socker);

		} else if (dy == 2) {
			socker->ball_position[1] -= 2;
			if (socker->ball_position[1] < 0)
				reset_ball_position(socker);
		}
	}
}