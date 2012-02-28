#include "Player.h"

void Player::update(unsigned char trans, unsigned char rot) {
	this->trans = trans;
	this->rot = rot;
}

unsigned char Player::getTrans() {
	return this->trans;
}