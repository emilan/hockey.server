#include "CameraCalibration.h"
#include "Puck.h"
#include "ObjectTracker.h"
#include "CamCapture.h"

#include "cv.h"
#include "highgui.h"

#include <iostream>

using namespace std;

#define WND_TOOLBAR	"toolbar"

void calibrateCamera() {
	cout << "Camera Calibrator" << endl;
	cout << "s - save color to file" << endl
		 << "q - quit calibration" << endl;

	// startar bildhantering
	initializeTracking(); 

	// definierar max/min på möjliga färgvärden
	int hmin = 0
	   ,hmax = 255
	   ,smin = 0
	   ,smax = 255
	   ,vmin = 0
	   ,vmax = 255;

	// skapar en toolbar med sliders och binder dem till färgvärden.
	cvNamedWindow(WND_TOOLBAR, 1);
    cvCreateTrackbar("H Min", WND_TOOLBAR, &hmin, 360, 0);
    cvCreateTrackbar("H Max", WND_TOOLBAR, &hmax, 360, 0);
    cvCreateTrackbar("S Min", WND_TOOLBAR, &smin, 256, 0);
	cvCreateTrackbar("S Max", WND_TOOLBAR, &smax, 256, 0);
    cvCreateTrackbar("V Min", WND_TOOLBAR, &vmin, 256, 0);
    cvCreateTrackbar("V Max", WND_TOOLBAR, &vmax, 256, 0);
	
	bool calibrating=true;
	ObjectTracker *pTrackPuck = getPuckTracker();
	CamCapture *pCamCapture = getCamCapture();

	pTrackPuck->setCalibrationMode(true);//track_puck objektet sätts i kalibreringsmode -> den visar bilder
	IplImage* frame= cvCreateImage(IMAGESIZE, 8, 3); //skapar minnesplats för bild
	CvPoint2D32f* puck=new CvPoint2D32f();//skapar position för puck
	
	while(calibrating){
		pTrackPuck->setColor(cvScalar(hmin, smin, vmin), cvScalar(hmax, smax, vmax)); //uppdatera bildhanteringen med ev. nya värden
		pCamCapture->myQueryFrame(frame);//ta ny bild
		
		pTrackPuck->trackObject(frame, puck);//gör bildhantering
		
		switch(cvWaitKey(1)){ //uppdaterar visade bilder och väntar på tangent input
			case 's': //sparar nuvarande värger till fil och avslutar
				pTrackPuck->saveColor();
				calibrating=false;
				break;
			case 'q':
				calibrating=false;
				break;
		}
	}
	pTrackPuck->setCalibrationMode(false);
	cvDestroyWindow(WND_TOOLBAR);
}
