#ifndef CLIENT_SOCKER_H
#define CLIENT_SOCKER_H
#include <ncurses.h>
#include <string.h>

#include "network.h"

// Socker struct
struct Socker {
	int field_size[2];
	int ball_position[2];
	int player_count;
	int player_size;
	int (*player_positions)[2];
};

// Initialise socker struct
void init_socker(struct Socker *socker, int id);

// Initialise socker struct
void resize_socker(struct Socker *socker, int count);

// Handle socker data received from server
void handle_socker_data(char *buf, int *id, struct Socker *socker);

// Handle input for socker
void handle_socker_input(char ch, int server, int id);

#endif // CLIENT_SOCKER_H