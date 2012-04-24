#include "CameraCalibration.h"
#include "Puck.h"
#include "ObjectTracker.h"
#include "CamCapture.h"

#include "cv.h"
#include "highgui.h"

#include <iostream>

using namespace std;

void calibrateCamera(CamCapture *pCamCapture, ObjectTracker *pTrackCalib, char *name) {
	bool calibrating=true;

	CvScalar cvMin = pTrackCalib->getMinColor();
	CvScalar cvMax = pTrackCalib->getMaxColor();

	// definierar max/min på möjliga färgvärden
	int hmin = cvMin.val[0]
	   ,hmax = cvMax.val[0]
	   ,smin = cvMin.val[1]
	   ,smax = cvMax.val[1]
	   ,vmin = cvMin.val[2]
	   ,vmax = cvMax.val[2];

	// skapar en toolbar med sliders och binder dem till färgvärden.
	cvNamedWindow(name, 1);
    cvCreateTrackbar("H Min", name, &hmin, 360, 0);
    cvCreateTrackbar("H Max", name, &hmax, 360, 0);
    cvCreateTrackbar("S Min", name, &smin, 256, 0);
	cvCreateTrackbar("S Max", name, &smax, 256, 0);
    cvCreateTrackbar("V Min", name, &vmin, 256, 0);
    cvCreateTrackbar("V Max", name, &vmax, 256, 0);
	
	pTrackCalib->setCalibrationMode(true);//track_puck objektet sätts i kalibreringsmode -> den visar bilder
	IplImage* frame= cvCreateImage(IMAGESIZE, 8, 3); //skapar minnesplats för bild
	CvPoint2D32f* puck=new CvPoint2D32f();//skapar position för puck
	
	while(calibrating){
		pTrackCalib->setColor(cvScalar(hmin, smin, vmin), cvScalar(hmax, smax, vmax)); //uppdatera bildhanteringen med ev. nya värden
		pCamCapture->myQueryFrame(frame);//ta ny bild
		
		pTrackCalib->trackObject(frame, puck);//gör bildhantering
		
		switch(cvWaitKey(1)){ //uppdaterar visade bilder och väntar på tangent input
			case 's': //sparar nuvarande värger till fil och avslutar
				pTrackCalib->saveColor();
				calibrating=false;
				break;
			case 'q':
				calibrating=false;
				break;
		}
	}
	pTrackCalib->setCalibrationMode(false);
	cvDestroyWindow(name);
	cvReleaseImage(&frame);
}

void calibrateCamera() {
	cout << "Camera Calibrator (puck, home goal, away goal)" << endl;
	cout << "s - save current calibration, go to next" << endl
		 << "q - cancel current calibration, go to next" << endl;

	CamCapture *pCamCapture = puck::getCamCapture();
	calibrateCamera(pCamCapture, puck::getPuckTracker(), "puck");
	calibrateCamera(pCamCapture, puck::getHomeGoalTracker(), "home goal");
	calibrateCamera(pCamCapture, puck::getAwayGoalTracker(), "away goal");

	IplImage* frame= cvCreateImage(IMAGESIZE, 8, 3); //skapar minnesplats för bild
	pCamCapture->myQueryFrame(frame);//ta ny bild
	puck::trackGoals(frame);
	cvReleaseImage(&frame);

	cout << "Camera calibration done!" << endl;
}
