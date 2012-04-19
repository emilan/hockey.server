#include "Mask.h"

#include <highgui.h>

Mask::Mask(char *filename) {
	pImg = cvLoadImage(filename, CV_LOAD_IMAGE_GRAYSCALE);
	if (!pImg)
		throw "Mask file not found";
}

bool Mask::contains(int x, int y) {
	CvSize sz = cvGetSize(pImg);

	// Transform to area mask pixel coordinate
	y += sz.height / 2;
	x += sz.width / 2;

	if (x > sz.width - 1 || y > sz.height - 1 || x < 0 || y < 0)
		return false;

	CvScalar s = cvGet2D(pImg, y, x);
	return s.val[0] != 0;
}