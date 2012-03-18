#ifndef __PUCK_H__
#define __PUCK_H__

#include "ObjectTracker.h"
#include "CamCapture.h"

#include "cv.h"

struct PuckPosition {
	int x;
	int y;
};

bool initializeTracking();
void startTrackingPuck();
void stopTrackingPuck();

ObjectTracker *getPuckTracker();
CamCapture *getCamCapture();
PuckPosition getPuckPosition();

#define IMAGESIZE cvSize(744 / 2, 480 / 2)

#endif //__PUCK_H__