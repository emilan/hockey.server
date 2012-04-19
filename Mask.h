#ifndef __MASK_H__
#define __MASK_H__

#include <cv.h>

class Mask {
private:
	IplImage *pImg;
public:
	Mask(char *filename);
	bool contains(int x, int y);
};

#endif //__MASK_H__