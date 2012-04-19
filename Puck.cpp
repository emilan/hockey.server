#include "Puck.h"
#include "ObjectTracker.h"
#include "CamCapture.h"
#include "Limits.h"

#include "cv.h"

#include <process.h>

#include <vector>

using namespace std;

// Class methods
#define DISTANCETOGOAL	315
#define PUCKHISTORY		100

struct Puck {
	vector<PuckPosition> history;
	HANDLE historyMutex;
	bool hasPosition;
	bool homeGoal;
	bool awayGoal;
	bool goalDetected;

	CvPoint2D32f goal1;
	CvPoint2D32f goal2;

	Puck();
	
	void updatePosition(CvPoint2D32f* point);
	void addHistory(PuckPosition pos);
};

void Puck::addHistory(PuckPosition pos) {
	WaitForSingleObject(historyMutex, INFINITE);
	history.push_back(pos);
	if (history.size() > PUCKHISTORY)
		history.erase(history.begin());
	ReleaseMutex(historyMutex);
}

Puck::Puck() {
	historyMutex = CreateMutex(NULL, false, "readMutex");

	PuckPosition pos = {0, 0};
	addHistory(pos);
		
	goal1 = cvPoint2D32f(62, 117);	//skapar punkter med målens ungefärliga position
	goal2 = cvPoint2D32f(324, 116);

	goalDetected = false;
}

void Puck::updatePosition(CvPoint2D32f* ps){
	//gör linjär transformation pixelkordinatrer -> millimeter med hjälp utav målens position
	hasPosition = !(ps->x == 0 && ps->y == 0);
	if (hasPosition) {
		PuckPosition pos;
 		pos.x = ps->x - (goal1.x + goal2.x) / 2;
		pos.y = ps->y - (goal1.y + goal2.y) / 2;

		float v1x = goal2.x - goal1.x;
		float v1y = goal2.y - goal1.y; 
		float v2x = v1y;
		float v2y = -v1x;
		float norm = sqrt(v1x * v1x + v1y * v1y);

		v1x = v1x / norm;
		v1y = v1y / norm;
		v2x = v2x / norm;
		v2y = v2y / norm;
		pos.x = v1x * pos.x + v1y * pos.y;
		pos.y = v2x * pos.x + v2y * pos.y;
		pos.x *= 2 * DISTANCETOGOAL / norm;
		pos.y *= 2 * DISTANCETOGOAL / norm;

		addHistory(pos);
		homeGoal = awayGoal = goalDetected = false;

		limits::update();
	}
	else {	// Puck becomes invisible when in goal. This code checks if the puck's trajectory is towards one of the goals.
		if (history.size() >= 2 && !goalDetected) {
			// TODO: Don't use just two last points (only if it doesn't work)
			// TODO: Either put something on top of goals or allow detection of puck in goal (because sometimes it succeeds)
			PuckPosition last = history[history.size() - 1];
			float awayGoallineX = 284;
			float goallineMinY = -50;
			float goallineMaxY = 50;

			float homeGoallineX = -284;

			PuckPosition secondLast = history[history.size() - 2];
			// Between center of field and finnish goal line and moving towards finnish goal
			if (last.x < awayGoallineX && last.x > 0 && secondLast.x < last.x) {
				// y = (y2 - y1) / (x2 - x1) * (x - x1) + y1
				float y = 1.0f * (last.y - secondLast.y) / (last.x - secondLast.x) * (awayGoallineX - secondLast.x) + secondLast.y;
				if (y > goallineMinY && y < goallineMaxY) {
					// Goal detected. Ask limits if okay goal
					if (limits::isOkayHomeGoal()) {
						homeGoal = true;
						awayGoal = false;
					}
					else homeGoal = awayGoal = false;
					goalDetected = true;
				}
				else homeGoal = awayGoal = false;
			}
			// Between center and swedish goal line and moving towards swedish goal
			else if (last.x > homeGoallineX && last.x < 0 && secondLast.x > last.x) {
				float y = 1.0f * (last.y - secondLast.y) / (last.x - secondLast.x) * (homeGoallineX - secondLast.x) + secondLast.y;
				if (y > goallineMinY && y < goallineMaxY) {
					// Goal detected. Ask limits if okay goal
					if (limits::isOkayAwayGoal()) {
						homeGoal = false;
						awayGoal = true;
					}
					else homeGoal = awayGoal = false;
					goalDetected = true;
				}
				else homeGoal = awayGoal = false;
			}
		}
	}
}

// Instances, etc.
namespace puckns {
	CamCapture *capture;
	ObjectTracker track_puck;
	ObjectTracker track_goal1;
	ObjectTracker track_goal2;

	Puck puck;

	bool trackingInitialized = false;
	volatile bool trackingPuck = false;
	HANDLE cameraThreadHandle;

	float cameraFrequency;
}
using namespace puckns;

unsigned __stdcall cameraThread(void* param) {
	CvPoint2D32f* puckPoint = new CvPoint2D32f();
	IplImage* frame = cvCreateImage(IMAGESIZE, 8, 3);//skapar en buffer för en bild

	while (trackingPuck) {
		static clock_t tOld = 0;
		static unsigned char counter = 0;
		if (++counter == 20) {
			clock_t tNew = clock();
			cameraFrequency = 20.0f / (1.0f * (tNew - tOld) / CLOCKS_PER_SEC);
			tOld = tNew;
			counter = 0;
		}

		//fyller buffren med en ny bild
		capture->myQueryFrame(frame); //40ms
		//hanterar bilden
		track_puck.trackObject(frame, puckPoint);//20ms
		//uppdaterar puckens position med ny data
		puck.updatePosition(puckPoint);
	}
	return NULL;
}

void trackGoals(IplImage *pFrame) {
	track_goal1.trackObject(pFrame, &puck.goal1);	//hittar målens position
	track_goal2.trackObject(pFrame, &puck.goal2);
}

bool initializeTracking() {
	if(!trackingInitialized) {
		capture = new CamCapture();
		limits::init();
		
		if (!capture->initCam()) {
			fprintf(stderr, "Could not initialize capturing...\n");
			return false;
		}
		try {
			/*skapar ett bildbehandlings objekt somletar efter grönt på spelplanen, se Objecttracker.cpp*/
			track_puck = ObjectTracker(cvLoadImage("playField.bmp", 0), "green");
			track_goal1 = ObjectTracker(cvLoadImage("goal1.bmp", 0), "goal1");	//object som letar efter ena målet
			track_goal2 = ObjectTracker(cvLoadImage("goal2.bmp", 0), "goal2");	//object som letar efter andra målet
		} catch(exception e) {
			cout << "no mask or color information" << endl;
			return false;
		}
		trackingInitialized=true;

		//tar en bild och använder den för att skapa en puckrepresentation
		IplImage* frame = cvCreateImage(IMAGESIZE, 8, 3);
		capture->myQueryFrame(frame);
		trackGoals(frame);
		
		startTrackingPuck();
	}
	return true;
}

void startTrackingPuck() {
	if (!trackingPuck) {
		trackingPuck = true;
		_beginthreadex(NULL, 0, cameraThread, NULL, 0, NULL);
	}
}

void stopTrackingPuck() {
	if (trackingPuck)
		trackingPuck = false;
}

PuckPosition getPuckPosition() {
	WaitForSingleObject(puck.historyMutex, INFINITE);
	PuckPosition pos = puck.history.back();
	ReleaseMutex(puck.historyMutex);
	return pos;
}

int getPuckHistory(PuckPosition *hist, unsigned int length) {
	WaitForSingleObject(puck.historyMutex, INFINITE);
	int c = min(puck.history.size(), length);
	for (int i = 0; i < c; i++)
		memcpy(hist + i, &puck.history[i], sizeof(PuckPosition)); 
	ReleaseMutex(puck.historyMutex);
	return c;
}

ObjectTracker *getPuckTracker() {
	return &track_puck;
}

ObjectTracker *getHomeGoalTracker() {
	return &track_goal1;
}

ObjectTracker *getAwayGoalTracker() {
	return &track_goal2;
}

CamCapture *getCamCapture() {
	return capture;
}

float getCameraFrequency() {
	return cameraFrequency;
}

bool isHomeGoal() {
	return puck.homeGoal;
}

bool isAwayGoal() {
	return puck.awayGoal;
}

bool hasPuckPosition() {
	return puck.hasPosition;
}

float getPuckSpeed() {
	if (puck.history.size() >= 2) {
		PuckPosition last = puck.history[puck.history.size() - 1];
		PuckPosition secondLast = puck.history[puck.history.size() - 2];

		float dx = last.x - secondLast.x;
		float dy = last.y - secondLast.y;

		float length = sqrt(dx * dx + dy * dy);
		float freq = getCameraFrequency();

		return length * freq / 1000000 * 3600;
	}
	return 0;
}