#include "../../include/server/socker.h"

// Socker struct
struct Socker {
	int ball_position[2];

	int player_count;
	int player_size;

	int *player_fds;
	int (*player_positions)[2];
};

// Initialise socker
void init_socker(struct Socker *socker) {

	socker->player_count = 0;
	socker->player_size = 10;

	socker->player_fds = malloc(sizeof(int) * socker->player_size);

	socker->player_positions = malloc(sizeof(int[2]) * socker->player_size);

	if (socker->player_fds == NULL ||
	    socker->player_positions == NULL) {
		perror("malloc");
		exit(1);
	}
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
	socker->player_positions[idx][0] = 10;
	socker->player_positions[idx][1] = 10;

	socker->player_count++;

	return socker->player_count - 1;
}

// Delete player from socker
void delete_player(struct Socker *socker, int index) {
	int last = socker->player_count - 1;

	socker->player_fds[index] = socker->player_fds[last];
	socker->player_positions[index][0] = socker->player_positions[last][0];
	socker->player_positions[index][1] = socker->player_positions[last][1];

	socker->player_count--;
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
		    "player %d %d\n",
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