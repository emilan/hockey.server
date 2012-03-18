#include "Team.h"

#include <iostream>

// Class methods
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

// Instances and helper functions
Team homeTeam;
Team awayTeam;

Team *getTeamById(int id) {
	if (id == 0)
		return &homeTeam;
	else if (id == 1)
		return &awayTeam;
	return 0;
}

Team *getHomeTeam() {
	return &homeTeam;
}

Team *getAwayTeam() {
	return &awayTeam;
}

bool initPlayerPositions() {
	for (int i = 0; i < 2; i++) {
		Team *pTeam = getTeamById(i);
		for (int j = 0; j < 6; j++) {
			char fileName[20];
			sprintf(fileName, "player%d%d.txt", i, j);
			if (!pTeam->getPlayer(j)->readLocations(fileName))
				return false;
		}
	}
	return true;
}
