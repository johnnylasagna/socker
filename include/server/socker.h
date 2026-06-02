#ifndef SERVER_SOCKER_H
#define SERVER_SOCKER_H

#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>

#include "network.h"

// Socker struct
struct Socker {
	int field_size[2];
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

void send_all_player_data(struct Socker *socker, int id);

void send_player_data(struct Socker *socker, int id);

void send_count_data(struct Socker *socker);

void reset_ball_position(struct Socker *socker);

void update_positions(struct Socker *socker, char *buf);

#endif // SERVER_SOCKER_H
