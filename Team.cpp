#include "Team.h"

int Team::getGoals() {
	return this->goals;
}

void Team::update(unsigned char *status) {
	for (int i = 0; i < 6; i++) {
		this->players[i].update(status[i * 2], status[i * 2 + 1]);
	}
}

Player *Team::getPlayer(int playerId) {
	// TODO: Check bounds
	return &players[playerId];
}