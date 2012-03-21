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
int getPuckHistory(PuckPosition *hist, unsigned int length);
bool isHomeGoal();
bool isAwayGoal();
bool hasPuckPosition();

float getCameraFrequency();
float getPuckSpeed();

#define IMAGESIZE cvSize(744 / 2, 480 / 2)

#endif //__PUCK_H__