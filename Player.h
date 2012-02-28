#ifndef __PLAYER_H__
#define __PLAYER_H__

class Player {
private:
	unsigned char trans;
	unsigned char rot;
public:
	void update(unsigned char trans, unsigned char rot);
	unsigned char getTrans();
};

#endif //__PLAYER_H__