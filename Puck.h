#ifndef __PUCK_H__
#define __PUCK_H__

#include "ObjectTracker.h"
#include "CamCapture.h"

#include "cv.h"

namespace puck {
	struct Position {
		int x;
		int y;
	};

	bool initializeTracking(void(*homeGoalMade)(void), void(*awayGoalMade)(void));
	void startTracking();
	void stopTracking();

	ObjectTracker *getPuckTracker();
	ObjectTracker *getHomeGoalTracker();
	ObjectTracker *getAwayGoalTracker();
	CamCapture *getCamCapture();

	Position getPosition();
	int getHistory(Position *hist, unsigned int length);
	bool isHomeGoal();
	bool isAwayGoal();
	bool hasPosition();
	void trackGoals(IplImage *pFrame);

	float getCameraFrequency();
	float getSpeed();

	#define IMAGESIZE cvSize(744 / 2, 480 / 2)
}

#endif //__PUCK_H__