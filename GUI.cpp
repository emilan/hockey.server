#include "GUI.h"
#include "Team.h"

#include "cv.h"
#include "highgui.h"

#include <process.h>

#define WIDTH	800
#define HEIGHT	400
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
	CvScalar colorFinland = cvScalar(255, 255, 255, 255);
	CvScalar colorSweden = cvScalar(0, 255, 255, 255);
	cvCopy(pImgBg, pImg, NULL);
	for (int i = 0; i < 2; i++) {
		Team *pTeam = getTeamById(i);
		CvScalar playerColor = i == 0 ? colorSweden : colorFinland;
		CvPoint playerFaces = i == 0 ? cvPoint(1, -1) : cvPoint(-1, 1);	// TODO: Examine how players club rotates with his location

		for (int j = 0; j < 6; j++) {
			Player * pPlayer = pTeam->getPlayer(j);
			PlayerLocation loc = pPlayer->getCurrentLocation();
			float rot = - pPlayer->getCurrentRotation() / 255.0f * 2 * PI + PI / 2;	// vridning medsols axel -> vridning motsols spelare
			CvPoint playerCenter = cvPoint(WIDTH / 2 - loc.x, HEIGHT / 2 + loc.y);
			
			int playerClubLength = 41;
			CvPoint playerClub = 
				cvPoint(playerCenter.x + playerFaces.x * playerClubLength * cos (rot), 
						playerCenter.y + playerFaces.y * playerClubLength * sin (rot));

			cvCircle(pImg, playerCenter, 15, playerColor, 3);
			cvLine(pImg, playerCenter, playerClub, playerColor, 3);
		}
	}
	char buf[20];
	sprintf_s(buf, "FPS: %f", fps);
	cvPutText(pImg, buf, cvPoint(0, 20), &cvFont(1), cvScalar(255, 255, 255, 255));
	cvShowImage("test", pImg);
	cvWaitKey(1);
}

unsigned __stdcall drawingThread(void* param){
	while (true) {
		draw();
	}
}

void startDrawing() {
	pImg = cvCreateImage(cvSize(WIDTH, HEIGHT), 8, 3);
	pImgBg = cvCreateImage(cvSize(WIDTH, HEIGHT), 8, 3);
	cvRectangle(pImgBg, cvPoint(0, 0), cvPoint(WIDTH, HEIGHT), cvScalar(0, 0, 0, 0), CV_FILLED);
	for (int i = 0; i < 2; i++) {
		Team *pTeam = getTeamById(i);
		for (int j = 0; j < 6; j++) {
			Player * pPlayer = pTeam->getPlayer(j);
			for (int k = 0; k < 256; k++) {
				PlayerLocation loc = pPlayer->getLocation(k);
				cvCircle(pImgBg, cvPoint(WIDTH / 2 - loc.x, HEIGHT / 2 + loc.y), 2, cvScalar(128, 128, 128, 255), CV_FILLED);
			}
		}
	}

	_beginthreadex(NULL, 0, drawingThread, NULL, 0, NULL);
}