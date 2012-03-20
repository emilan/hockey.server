#include "Puck.h"
#include "ObjectTracker.h"
#include "CamCapture.h"

#include "cv.h"

#include <process.h>

#include <vector>

using namespace std;

// Class methods
#define DISTANCETOGOAL	300
#define PUCKHISTORY		100

struct Puck {
	vector<PuckPosition> history;
	HANDLE historyMutex;

	CvPoint2D32f goal1;
	CvPoint2D32f goal2;
	Puck() {};
	Puck(IplImage* frame);
	
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

Puck::Puck(IplImage* frame) {
	historyMutex = CreateMutex(NULL, false, "readMutex");

	PuckPosition pos = {0, 0};
	addHistory(pos);
	ObjectTracker track_goal1 = ObjectTracker(cvLoadImage("goal1.bmp", 0), "red");	//object som letar efter ena målet
	ObjectTracker track_goal2 = ObjectTracker(cvLoadImage("goal2.bmp", 0), "red");	//object som letar efter andra målet
		
	goal1 = cvPoint2D32f(62, 117);	//skapar punkter med målens ungefärliga position
	goal2 = cvPoint2D32f(324, 116);
		
	//track_goal1.trackObject(frame, &goal1, false);	//hittar målens position
	//track_goal2.trackObject(frame, &goal2, false);
}

void Puck::updatePosition(CvPoint2D32f* ps){
	//gör linjär transformation pixelkordinatrer -> millimeter med hjälp utav målens position
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
}

// Instances, etc.
namespace puckns {
	CamCapture *capture;
	ObjectTracker track_puck;
	Puck puck;

	bool trackingInitialized = false;
	volatile bool trackingPuck = false;
	HANDLE cameraThreadHandle;
}
using namespace puckns;

unsigned __stdcall cameraThread(void* param) {
	CvPoint2D32f* puckPoint = new CvPoint2D32f();
	IplImage* frame = cvCreateImage(IMAGESIZE, 8, 3);//skapar en buffer för en bild
	int t0 = 0, t1 = 0, t2 = 0, t3 = 0; //DEBUGVERKTYG

	while (trackingPuck) {
		t0 = clock();
		//fyller buffren med en ny bild
		capture->myQueryFrame(frame); //40ms
		t1 = clock();
		//hanterar bilden
		track_puck.trackObject(frame, puckPoint);//20ms
		t2 = clock();
		//uppdaterar puckens position med ny data
		puck.updatePosition(puckPoint);
		t3 = clock();
		//float cycle=(GetTickCount()-t);
		//cout<<t0-t1<<"\t"<<t1-t2<<"\t"<<t3-t2<<endl;
	}
	return NULL;
}

bool initializeTracking() {
	if(!trackingInitialized) {
		capture = new CamCapture();
		
		if (!capture->initCam()) {
			fprintf(stderr, "Could not initialize capturing...\n");
			return false;
		}
		try {
			/*skapar ett bildbehandlings objekt somletar efter grönt på spelplanen, se Objecttracker.cpp*/
			track_puck = ObjectTracker(cvLoadImage("playField.bmp", 0), "green");
		} catch(exception e) {
			cout << "no mask or color information" << endl;
			return false;
		}
		trackingInitialized=true;

		//tar en bild och använder den för att skapa en puckrepresentation
		IplImage* frame = cvCreateImage(IMAGESIZE, 8, 3);
		capture->myQueryFrame(frame);
		puck = Puck(frame);
		
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

CamCapture *getCamCapture() {
	return capture;
}
