#include "Player.h"

#include <stdio.h>
#include <vector>
#include <cv.h>
#include <highgui.h>

using namespace std;

void Player::update(unsigned char trans, unsigned char rot) {
	this->trans = trans;
	this->rot = rot;
}

unsigned char Player::getTrans() {
	return this->trans;
}

PlayerLocation Player::getLocation(int trans) {
	return this->locations[trans];
}

PlayerLocation Player::getCurrentLocation() {
	return getLocation(this->trans);
}

unsigned char Player::getCurrentRotation() {
	return this->rot;
}

bool Player::readLocations(char *fileName) {
	vector<PlayerLocation> points;
	vector<int> positions;
	FILE *pFile = fopen(fileName, "r");
	if (pFile == NULL)
		return false;

	char buf[100];
	while (fgets(buf, 100, pFile) != NULL) {
		float x, y;
		int i;
		int scanRes = sscanf(buf, "%f %f %d", &x, &y, &i);
		if (scanRes >= 2) {
			PlayerLocation loc = { x, y };
			points.push_back(loc);
		}

		if (scanRes == 3)
			positions.push_back(i);
		else positions.push_back(-1);
	}
	fclose(pFile);

	if (points.size() < 2)
		return false;

	PlayerLocation *relative = new PlayerLocation[points.size()];
	double *lengths = new double[points.size()];
	
	lengths[0] = 0;
	for (int i = 1; i < points.size(); i++) {
		relative[i - 1] = points[i].subtract(points[i - 1]);
		lengths[i] = lengths[i - 1] + relative[i - 1].length();
	}
	double totalLength = lengths[points.size() - 1];

	positions[0] = 0;
	positions[positions.size() - 1] = 255;
	for (int i = 1; i < points.size() - 1; i++) {
		if (positions[i] == -1)
			positions[i] = lengths[i] / totalLength * 255;
	}

	int position = 0;
	for (int i = 0; i < positions.size() - 1; i++) {
		for (; position < positions[i + 1]; position++) {
			double f = (1.0 * position - positions[i]) / (positions[i + 1] - positions[i]);
			this->locations[position] = points[i].add(relative[i].multiply(f));
		}
	}
	this->locations[255] = points[points.size() - 1];

	delete[] relative;
	delete[] lengths;

	return true;
}

bool Player::loadArea(char *filename) {
	try {
		pAreaMask = new Mask(filename);	// TODO: delete
		return true;
	} catch (...) {
		return false;
	}
}

bool Player::canAccessCoordinate(int x, int y) {
	return pAreaMask->contains(x, y);
}

PlayerLocation PlayerLocation::subtract(PlayerLocation loc) {
	PlayerLocation res = { this->x - loc.x, this->y - loc.y };
	return res;
}

PlayerLocation PlayerLocation::add(PlayerLocation loc) {
	PlayerLocation res = { this->x + loc.x, this->y + loc.y };
	return res;
}

PlayerLocation PlayerLocation::multiply(double m) {
	PlayerLocation res = { this->x * m, this->y * m };
	return res;
}

double PlayerLocation::length() {
	return sqrt((double)(x*x + y*y));
}