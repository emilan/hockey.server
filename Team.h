#ifndef __TEAM_H__
#define __TEAM_H__

#include "Player.h"

class Team {
private:
	Player players[6];
	int goals;
public:
	int getGoals();
	void update(unsigned char *status);
	Player *getPlayer(int id);
};

#endif //__TEAM_H__