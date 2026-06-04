#include "../../include/server/socker.h"

// Initialise socker
void init_socker(struct Socker *socker) {

	socker->player_count = 0;
	socker->player_size = 10;

	socker->player_udp_addrs = malloc(sizeof(struct sockaddr_storage) * socker->player_size);
	socker->player_tcp_fds = malloc(sizeof(int) * socker->player_size);
	socker->player_positions = malloc(sizeof(int[2]) * socker->player_size);

	if (socker->player_tcp_fds == NULL || socker->player_udp_addrs == NULL || socker->player_positions == NULL) {
		perror("malloc");
		exit(1);
	}

	socker->field_size[0] = 80;
	socker->field_size[1] = 24;

	socker->ball_position[0] = socker->field_size[0] / 2;
	socker->ball_position[1] = socker->field_size[1] / 2;
}

// Add player to socker
int add_player_to_socker(struct Socker *socker, struct sockaddr_storage *addr, int tcp_fd) {
	if (socker->player_count == socker->player_size) {
		socker->player_size *= 2;

		socker->player_udp_addrs = realloc(socker->player_udp_addrs, sizeof(struct sockaddr_storage) * socker->player_size);
		socker->player_tcp_fds = realloc(socker->player_tcp_fds, sizeof(int) * socker->player_size);
		socker->player_positions = realloc(socker->player_positions, sizeof(int[2]) * socker->player_size);

		if (socker->player_tcp_fds == NULL || socker->player_udp_addrs == NULL || socker->player_positions == NULL) {
			perror("realloc");
			exit(1);
		}
	}

	int idx = socker->player_count;

	socker->player_udp_addrs[idx] = *addr;
	socker->player_tcp_fds[idx] = tcp_fd;
	socker->player_positions[idx][0] = socker->field_size[0] / 2;
	socker->player_positions[idx][1] = socker->field_size[1] / 2;

	socker->player_count++;

	send_count_data(socker);

	return socker->player_count - 1;
}

// Delete player from socker
int delete_player(struct Socker *socker, int index) {
	int last = socker->player_count - 1;

	socker->player_udp_addrs[index] = socker->player_udp_addrs[last];
	socker->player_tcp_fds[index] = socker->player_tcp_fds[last];
	socker->player_positions[index][0] = socker->player_positions[last][0];
	socker->player_positions[index][1] = socker->player_positions[last][1];

	socker->player_count--;

	send_count_data(socker);

	return last;
}

void send_all_player_data(struct Socker *socker, int id) {
	char position_buf[1024];

	int offset = 0;

	offset += snprintf(
	    position_buf + offset,
	    sizeof(position_buf) - offset,
	    "/data ball %d %d\n",
	    socker->ball_position[0],
	    socker->ball_position[1]);

	offset += snprintf(
	    position_buf + offset,
	    sizeof(position_buf) - offset,
	    "/data count %d \n",
	    socker->player_count);

	for (int j = 0; j < socker->player_count; j++) {
		offset += snprintf(
		    position_buf + offset,
		    sizeof(position_buf) - offset,
		    "/data player %d %d %d\n",
		    j,
		    socker->player_positions[j][0],
		    socker->player_positions[j][1]);
	}

	int dest_fd = socker->player_tcp_fds[id];

	int position_len = offset;

	if (sendall(dest_fd, position_buf, &position_len) == -1) {
		perror("send");
	}
}

void send_player_data(struct Socker *socker, int id) {
	char position_buf[40];

	snprintf(position_buf,
	         sizeof(position_buf),
	         "/data player %d %d %d\n",
	         id,
	         socker->player_positions[id][0],
	         socker->player_positions[id][1]);

	int position_len = strlen(position_buf);

	for (int i = 0; i < socker->player_count; i++) {
		struct sockaddr_storage dest_fd = socker->player_udp_addrs[i];
		socklen_t addr_len = sizeof(dest_fd);

		if (dest_fd.ss_family != AF_INET) {
			continue;
		}

		if (sendto(socker->server, position_buf, position_len, 0, (struct sockaddr *)&dest_fd, addr_len) == -1) {
			perror("sendto");
		}
	}
}

void send_ball_data(struct Socker *socker) {
	char ball_buf[40];

	snprintf(ball_buf,
	         sizeof(ball_buf),
	         "/data ball %d %d\n",
	         socker->ball_position[0], socker->ball_position[1]);

	int ball_len = strlen(ball_buf);

	for (int i = 0; i < socker->player_count; i++) {
		struct sockaddr_storage dest_fd = socker->player_udp_addrs[i];
		socklen_t addr_len = sizeof(dest_fd);

		if (dest_fd.ss_family != AF_INET) {
			continue;
		}

		if (sendto(socker->server, ball_buf, ball_len, 0, (struct sockaddr *)&dest_fd, addr_len) == -1) {
			perror("sendto");
		}
	}
}

void send_count_data(struct Socker *socker) {
	char count_buf[40];

	snprintf(count_buf,
	         sizeof(count_buf),
	         "/data count %d\n",
	         socker->player_count);

	int count_len = strlen(count_buf);

	for (int i = 0; i < socker->player_count; i++) {
		int dest_fd = socker->player_tcp_fds[i];

		if (sendall(dest_fd, count_buf, &count_len) == -1) {
			perror("send");
		}
	}
}

void reset_ball_position(struct Socker *socker) {
	socker->ball_position[0] = socker->field_size[0] / 2;
	socker->ball_position[1] = socker->field_size[1] / 2;
}

void update_positions(struct Socker *socker, char *buf, struct sockaddr_storage *client_addr) {
	int id;
	int dx;
	int dy;

	if (sscanf(buf + 6, "%d %d %d", &id, &dx, &dy) != 3) {
		fprintf(stderr, "malformed data received\n");
		return;
	}

	if (id < 0 || id >= socker->player_count) {
		return;
	}

	socker->player_udp_addrs[id] = *client_addr;

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
		send_ball_data(socker);
	}

	send_player_data(socker, id);
}