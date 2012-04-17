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
	FILE *pFile = fopen(fileName, "r");
	if (pFile == NULL)
		return false;

	float x, y;

	while (fscanf(pFile, "%f %f", &x, &y) != EOF) {
		PlayerLocation loc = { x, y };
		points.push_back(loc);
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

	for (int i = 0; i < 256; i++) {
		double p = i / 256.0;
		int q = 0;
		while (lengths[q] <= p * totalLength) {
			++q;
			if (q == points.size())
				break;
		}
		--q;
		double f = (p * totalLength - lengths[q]) / (lengths[q + 1] - lengths[q]);
		this->locations[i] = points[q].add(relative[q].multiply(f));
	}

	delete[] relative;
	delete[] lengths;

	return true;
}

bool Player::loadArea(char *filename) {
	this->pImgArea = cvLoadImage(filename, CV_LOAD_IMAGE_GRAYSCALE);
	return pImgArea != NULL;
}

bool Player::canAccessCoordinate(int x, int y) {
#define WIDTH	836
#define HEIGHT	460

	CvSize sz = cvGetSize(pImgArea);
	if (x > sz.width - 1 || y > sz.height - 1 || x < 0 || y < 0)
		return false;

	// Transform to area mask pixel coordinate
	y += HEIGHT / 2;
	x += WIDTH / 2;

	CvScalar s = cvGet2D(pImgArea, y, x);
	return s.val[0] != 0;
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