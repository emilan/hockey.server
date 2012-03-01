#ifndef __PLAYER_H__
#define __PLAYER_H__

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
	unsigned char trans;
	unsigned char rot;
	PlayerLocation locations[256];
public:
	void update(unsigned char trans, unsigned char rot);
	unsigned char getTrans();
	PlayerLocation getCurrentLocation();
	PlayerLocation getLocation(int trans);
	bool readLocations(char *fileName);
};

#endif //__PLAYER_H__