#include "Puck.h"
#include "ObjectTracker.h"
#include "CamCapture.h"
#include "Limits.h"
#include "HockeyGame.h"
#include "Gametime.h"

#include "cv.h"

#include <process.h>

#include <vector>

using namespace std;

#define DISTANCETOGOAL	315
#define PUCKHISTORY		100

namespace puck_private {
	CamCapture *capture;
	ObjectTracker track_puck;
	ObjectTracker track_goal1;
	ObjectTracker track_goal2;

	vector<puck::Position> history;
	HANDLE historyMutex;
	bool hasPosition;
	bool homeGoal;
	bool awayGoal;
	bool goalDetected;

	CvPoint2D32f goal1;
	CvPoint2D32f goal2;

	bool trackingInitialized = false;
	volatile bool trackingPuck = false;
	volatile bool doneTracking = false;
	HANDLE cameraThreadHandle;
	void(*homeGoalMade)(void) = NULL;
	void(*awayGoalMade)(void) = NULL;

	float cameraFrequency;

	void updatePosition(CvPoint2D32f* point);
	void addHistory(puck::Position pos);
	unsigned __stdcall cameraThread(void* param);

	bool cameraVisible = false;
	void camera_mouse_callback(int event, int x, int y, int flags, void* param);
	puck::Position translateCoordinates(int x, int y);
}

// private functions

void puck_private::addHistory(puck::Position pos) {
	WaitForSingleObject(historyMutex, INFINITE);
	history.push_back(pos);
	if (history.size() > PUCKHISTORY)
		history.erase(history.begin());
	ReleaseMutex(historyMutex);
}

void puck_private::updatePosition(CvPoint2D32f* ps){
	//gör linjär transformation pixelkordinatrer -> millimeter med hjälp utav målens position
	hasPosition = !(ps->x == 0 && ps->y == 0);
	static int becameInvisibleTime = -1;
	if (hasPosition) {
		puck::Position pos = translateCoordinates(ps->x, ps->y);
		addHistory(pos);
		homeGoal = awayGoal = goalDetected = false;

		limits::update();
		becameInvisibleTime = -1;
	}
	else {	// Puck becomes invisible when in goal. This code checks if the puck's trajectory is towards one of the goals.
		if (becameInvisibleTime == -1) {
			becameInvisibleTime = getGametime();
			cout << "t" << becameInvisibleTime << endl;
		}
		else if (!goalDetected && (history.size() >= 2) && (getGametime() - becameInvisibleTime > 100)) {
			// TODO: Don't use just two last points (only if it doesn't work)
			// TODO: Either put something on top of goals or allow detection of puck in goal (because sometimes it succeeds)
			puck::Position last = history[history.size() - 1];
			
			// TODO: From automatic goal detection instead?
			float awayGoallineX = 284;
			float goallineMinY = -50;
			float goallineMaxY = 43;
			float homeGoallineX = -284;

			puck::Position secondLast = history[history.size() - 2];
			int i = history.size() - 3;
			while ((secondLast.x == last.x || secondLast.y == last.y) && (i >= 0)) {
				secondLast = history[i];
				cout << "<" << i << endl;
				i--;
			}	

			cout << last.x << "," << last.y << "\t" << secondLast.x << "," << secondLast.y << endl;

			// Between center of field and finnish goal line and moving towards finnish goal
			if (last.x < awayGoallineX + 5 && last.x > 0 && secondLast.x < last.x) {
				// y = (y2 - y1) / (x2 - x1) * (x - x1) + y1
				float y = 1.0f * (last.y - secondLast.y) / (last.x - secondLast.x) * (awayGoallineX - secondLast.x) + secondLast.y;
				if (y > goallineMinY && y < goallineMaxY) {
					// Goal detected. Ask limits if okay goal
					if (limits::isOkayHomeGoal()) {
						homeGoal = true;
						awayGoal = false;
						if (homeGoalMade != NULL)
							homeGoalMade();
					}
					else homeGoal = awayGoal = false;
					goalDetected = true;
					hockeygame::pauseGame();	// TODO: Perhaps a bit against my principles of modularity... Should be in some kind of goal event...
				}
				else homeGoal = awayGoal = false;
			}
			// Between center and swedish goal line and moving towards swedish goal
			else if (last.x > homeGoallineX - 5 && last.x < 0 && secondLast.x > last.x) {
				float y = 1.0f * (last.y - secondLast.y) / (last.x - secondLast.x) * (homeGoallineX - secondLast.x) + secondLast.y;
				cout << y << endl;
				if (y > goallineMinY && y < goallineMaxY) {
					// Goal detected. Ask limits if okay goal
					if (limits::isOkayAwayGoal()) {
						homeGoal = false;
						awayGoal = true;
						if (awayGoalMade != NULL)
							awayGoalMade();
					}
					else homeGoal = awayGoal = false;
					goalDetected = true;
					hockeygame::pauseGame();	// TODO: Perhaps a bit against my principles of modularity... Should be in some kind of goal event...
				}
				else homeGoal = awayGoal = false;
			}
		}
	}
}

puck::Position puck_private::translateCoordinates(int x, int y) {
	puck::Position pos;
	pos.x = x - (puck_private::goal1.x + puck_private::goal2.x) / 2;
	pos.y = y - (puck_private::goal1.y + puck_private::goal2.y) / 2;

	float v1x = puck_private::goal2.x - puck_private::goal1.x;
	float v1y = puck_private::goal2.y - puck_private::goal1.y; 
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
	return pos;
}

void puck_private::camera_mouse_callback(int event, int x, int y, int flags, void* param) {
	switch( event ){
		case CV_EVENT_MOUSEMOVE: 
			break;

		case CV_EVENT_LBUTTONDOWN:
			break;

		case CV_EVENT_LBUTTONUP:
			puck::Position pos = translateCoordinates(x, y);
			cout << pos.x << "," << pos.y << endl;
			break;
	}
}

void puck::toggleCamera() {
	puck_private::cameraVisible = !puck_private::cameraVisible;
	if (!puck_private::cameraVisible) 
		cvDestroyWindow("camera");
}

unsigned __stdcall puck_private::cameraThread(void* param) {
	doneTracking = false;
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
		if (puck_private::cameraVisible) {
			cvShowImage("camera", frame);
			cvSetMouseCallback("camera", puck_private::camera_mouse_callback);
			cvWaitKey(1);
		}
		//hanterar bilden
		track_puck.trackObject(frame, puckPoint);//20ms
		//uppdaterar puckens position med ny data
		updatePosition(puckPoint);
	}
	doneTracking = true;
	return NULL;
}

// public functions

void puck::trackGoals(IplImage *pFrame) {
	puck_private::track_goal1.trackObject(pFrame, &puck_private::goal1);	//hittar målens position
	puck_private::track_goal2.trackObject(pFrame, &puck_private::goal2);
}

bool puck::initializeTracking(void(*homeGoalMade)(void), void(*awayGoalMade)(void)) {
	if(!puck_private::trackingInitialized) {
		limits::init();
		puck_private::homeGoalMade = homeGoalMade;
		puck_private::awayGoalMade = awayGoalMade;
		puck_private::historyMutex = CreateMutex(NULL, FALSE, NULL);

		puck::Position pos = {0, 0};
		puck_private::addHistory(pos);
		
		puck_private::goal1 = cvPoint2D32f(62, 117);	//skapar punkter med målens ungefärliga position
		puck_private::goal2 = cvPoint2D32f(324, 116);

		puck_private::goalDetected = false;


		puck_private::capture = new CamCapture();
		
		if (!puck_private::capture->initCam()) {
			fprintf(stderr, "Could not initialize capturing...\n");
			return false;
		}
		try {
			/*skapar ett bildbehandlings objekt somletar efter grönt på spelplanen, se Objecttracker.cpp*/
			puck_private::track_puck = ObjectTracker(cvLoadImage("playField.bmp", 0), "green");
			puck_private::track_goal1 = ObjectTracker(cvLoadImage("goal1.bmp", 0), "goal1");	//object som letar efter ena målet
			puck_private::track_goal2 = ObjectTracker(cvLoadImage("goal2.bmp", 0), "goal2");	//object som letar efter andra målet
		} catch(exception e) {
			cout << "no mask or color information" << endl;
			return false;
		}
		puck_private::trackingInitialized=true;

		//tar en bild och använder den för att skapa en puckrepresentation
		IplImage* frame = cvCreateImage(IMAGESIZE, 8, 3);
		puck_private::capture->myQueryFrame(frame);
		trackGoals(frame);
		
		puck::startTracking();
	}
	return true;
}

void puck::startTracking() {
	if (!puck_private::trackingPuck) {
		puck_private::trackingPuck = true;
		_beginthreadex(NULL, 0, puck_private::cameraThread, NULL, 0, NULL);
	}
}

void puck::stopTracking() {
	if (puck_private::trackingPuck)
		puck_private::trackingPuck = false;
	while (!puck_private::doneTracking)
		Sleep(1);
}

puck::Position puck::getPosition() {
	WaitForSingleObject(puck_private::historyMutex, INFINITE);
	puck::Position pos = puck_private::history.back();
	ReleaseMutex(puck_private::historyMutex);
	return pos;
}

int puck::getHistory(puck::Position *hist, unsigned int length) {
	WaitForSingleObject(puck_private::historyMutex, INFINITE);
	int c = min(puck_private::history.size(), length);
	for (int i = 0; i < c; i++)
		memcpy(hist + i, &puck_private::history[i], sizeof(puck::Position)); 
	ReleaseMutex(puck_private::historyMutex);
	return c;
}

ObjectTracker *puck::getPuckTracker() {
	return &puck_private::track_puck;
}

ObjectTracker *puck::getHomeGoalTracker() {
	return &puck_private::track_goal1;
}

ObjectTracker *puck::getAwayGoalTracker() {
	return &puck_private::track_goal2;
}

CamCapture *puck::getCamCapture() {
	return puck_private::capture;
}

float puck::getCameraFrequency() {
	return puck_private::cameraFrequency;
}

bool puck::isHomeGoal() {
	return puck_private::homeGoal;
}

bool puck::isAwayGoal() {
	return puck_private::awayGoal;
}

bool puck::hasPosition() {
	return puck_private::hasPosition;
}

float puck::getSpeed() {
	float res = 0;
	WaitForSingleObject(puck_private::historyMutex, INFINITE);
	if (puck_private::history.size() >= 2) {
		puck::Position last = puck_private::history[puck_private::history.size() - 1];
		puck::Position secondLast = puck_private::history[puck_private::history.size() - 2];

		float dx = last.x - secondLast.x;
		float dy = last.y - secondLast.y;

		float length = sqrt(dx * dx + dy * dy);
		float freq = getCameraFrequency();

		res = length * freq / 1000000 * 3600;
	}
	ReleaseMutex(puck_private::historyMutex);
	return res;
}
