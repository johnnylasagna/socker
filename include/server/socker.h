#ifndef SERVER_SOCKER_H
#define SERVER_SOCKER_H

#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>

// Socker struct
struct Socker {
	int ball_position[2];
	int player_count;
	int player_size;
	int *player_fds;
	int (*player_positions)[2];
};

// Initialise socker
void init_socker(struct Socker *socker);

// Add player to socker
int add_player_to_socker(struct Socker *socker, int fd);

// Delete player from socker
int delete_player(struct Socker *socker, int index);

void send_player_data(struct Socker *socker);

#endif // SERVER_SOCKER_H
