#include "GUI.h"
#include "Team.h"
#include "Puck.h"

#include "cv.h"
#include "highgui.h"
#include "Limits.h"
#include "Gametime.h"

#include "MicroControllers.h"

#include <process.h>

// Currently the same as board width/height, but not required
#define WINDOWWIDTH		836
#define WINDOWHEIGHT	460
#define PI 3.14159265

IplImage *pImgBg;
IplImage *pImg;

void draw() {
	static clock_t tOld = 0;
	static unsigned char counter = 0;
	static double fps = 0;
	if (++counter == 20) {
		clock_t tNew = clock();
		fps = 20.0 / (1.0 * (tNew - tOld) / CLOCKS_PER_SEC);
		tOld = tNew;
		counter = 0;
	}
	cvCopy(pImgBg, pImg, NULL);

	puck::Position puck = puck::getPosition();
	
	CvScalar puckColor = cvScalar(0, 255, 0, 255);

	puck::Position hist[100];
	int historyLength = puck::getHistory(hist, 100);
	for (int i = 0; i < historyLength; i++) {
		int shade = 255 * i / 100;
		cvCircle(pImg, cvPoint(WINDOWWIDTH / 2 + hist[i].x, WINDOWHEIGHT / 2 + hist[i].y), 11, puck::hasPosition() ? cvScalar(0, shade, 0, 255) : cvScalar(0, 0, shade, 255));
	}

	CvScalar colorFinland = cvScalar(255, 255, 255, 255);
	CvScalar colorSweden = cvScalar(0, 255, 255, 255);

	bool constructive = limits::checkConstructive();
	for (int i = 0; i < 2; i++) {
		Team *pTeam = getTeamById(i);
		CvScalar teamColor = i == 0 ? colorSweden : colorFinland;
		CvPoint playerFaces = i == 0 ? cvPoint(1, -1) : cvPoint(-1, 1);	// TODO: Examine how players club rotates with his location

		for (int j = 0; j < 6; j++) {
			Player *pPlayer = pTeam->getPlayer(j);
			PlayerLocation loc = pPlayer->getCurrentLocation();
			float rot = - pPlayer->getCurrentRotation() / 255.0f * 2 * PI + PI / 2;	// vridning medsols axel -> vridning motsols spelare
			CvPoint playerCenter = cvPoint(WINDOWWIDTH / 2 + loc.x, WINDOWHEIGHT / 2 + loc.y);
			
			int playerClubLength = 41;
			CvPoint playerClub = 
				cvPoint(playerCenter.x + playerFaces.x * playerClubLength * cos (rot), 
						playerCenter.y + playerFaces.y * playerClubLength * sin (rot));

			CvScalar playerColor = puck::hasPosition() && pPlayer->canAccessCoordinate(puck.x, puck.y) ? 
				(constructive ? cvScalar(0, 255, 0) : cvScalar(0, 0, 255, 255)) : teamColor;

			cvCircle(pImg, playerCenter, 15, playerColor, 3);
			cvLine(pImg, playerCenter, playerClub, playerColor, 3);
		}
	}

	char buf[20];
	sprintf_s(buf, "GUI: %.2f Hz", fps);
	cvPutText(pImg, buf, cvPoint(0, 15), &cvFont(1), cvScalar(255, 255, 255, 255));

	sprintf_s(buf, "MC: %.2f Hz", getMicroControllerFrequency());
	cvPutText(pImg, buf, cvPoint(0, 30), &cvFont(1), cvScalar(255, 255, 255, 255));

	sprintf_s(buf, "Cam: %.2f Hz", puck::getCameraFrequency());
	cvPutText(pImg, buf, cvPoint(0, 45), &cvFont(1), cvScalar(255, 255, 255, 255));

	static float maxPuckSpeed = 0;
	float puckSpeed = puck::getSpeed();
	if (puckSpeed > maxPuckSpeed)
		maxPuckSpeed = puckSpeed;
	sprintf_s(buf, "Speed: %.2f km/h", puckSpeed);
	cvPutText(pImg, buf, cvPoint(150, 15), &cvFont(1), cvScalar(255, 255, 255, 255));

	sprintf_s(buf, "Max: %.2f km/h", maxPuckSpeed);
	cvPutText(pImg, buf, cvPoint(150, 30), &cvFont(1), cvScalar(255, 255, 255, 255));

	if (puck::isHomeGoal())
		cvPutText(pImg, "GOAL->", cvPoint(350, 260), &cvFont(3, 3), colorSweden);
	if (puck::isAwayGoal())
		cvPutText(pImg, "<-GOAL", cvPoint(350, 260), &cvFont(3, 3), colorFinland);
	
	sprintf_s(buf, "%d - %d", getHomeTeam()->getGoals(), getAwayTeam()->getGoals());
	cvPutText(pImg, buf, cvPoint(380, 30), &cvFont(2, 2), cvScalar(255, 255, 255, 255));

	sprintf_s(buf, "%4d", getGametime() / 1000);
	cvPutText(pImg, buf, cvPoint(760, 30), &cvFont(2, 2), cvScalar(255, 255, 255, 255));

	cvShowImage("Hockey Server Visualisation", pImg);
	cvWaitKey(1);
}

unsigned __stdcall drawingThread(void* param){
	while (true) {
		draw();
	}
}

void startDrawing() {
	pImg = cvCreateImage(cvSize(WINDOWWIDTH, WINDOWHEIGHT), 8, 3);
	pImgBg = cvCreateImage(cvSize(WINDOWWIDTH, WINDOWHEIGHT), 8, 3);
	//IplImage *pImgAreas = cvLoadImage("areas.png", CV_LOAD_IMAGE_UNCHANGED);
	cvRectangle(pImgBg, cvPoint(0, 0), cvPoint(WINDOWWIDTH, WINDOWHEIGHT), cvScalar(0, 0, 0, 0), CV_FILLED);
	//cvAdd(pImgAreas, pImgBg, pImgBg, 0);
	for (int i = 0; i < 2; i++) {
		Team *pTeam = getTeamById(i);
		for (int j = 0; j < 6; j++) {
			Player * pPlayer = pTeam->getPlayer(j);
			for (int k = 0; k < 256; k++) {
				PlayerLocation loc = pPlayer->getLocation(k);
				int shade = 32 + 7 * k / 8;
				cvCircle(pImgBg, cvPoint(WINDOWWIDTH / 2 + loc.x, WINDOWHEIGHT / 2 + loc.y), 2, cvScalar(shade, shade, shade, 255), CV_FILLED);
			}
		}
	}

	_beginthreadex(NULL, 0, drawingThread, NULL, 0, NULL);
}