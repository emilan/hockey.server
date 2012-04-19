#ifndef __PLAYER_H__
#define __PLAYER_H__

#include "Mask.h"

#include <cv.h>

struct PlayerLocation {
	double x;
	double y;
	PlayerLocation subtract(PlayerLocation loc);
	PlayerLocation add(PlayerLocation loc);
	PlayerLocation multiply(double m);
	double length();
};

class Player {
private:
	Mask *pAreaMask;
	unsigned char trans;
	unsigned char rot;
	PlayerLocation locations[256];
public:
	void update(unsigned char trans, unsigned char rot);
	unsigned char getTrans();
	PlayerLocation getCurrentLocation();
	unsigned char getCurrentRotation();
	PlayerLocation getLocation(int trans);
	bool readLocations(char *fileName);
	bool loadArea(char *filename);
	bool canAccessCoordinate(int x, int y);
};

#endif //__PLAYER_H__