#ifndef CLIENT_SOCKER_H
#define CLIENT_SOCKER_H
#include <ncurses.h>
#include <string.h>

#include "network.h"

struct Socker {
	int field_size[2];
	int ball_position[2];
	int player_count;
	int player_size;
	int *player_fds;
	int (*player_positions)[2];
};

void init_socker(struct Socker *socker, int id);

void resize_socker(struct Socker *socker, int count);

void handle_socker_data(char *buf, int *id, struct Socker *socker);

void handle_socker_input(char ch, int server, int id);

#endif // CLIENT_SOCKER_H