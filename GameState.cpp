#include <vector>
#include <process.h>
#include "GameState.h"
#include "TeamConnection.h"
#include "Team.h"
#include "CamCapture.h"
#include "ObjectTracker.h"
#include "cv.h"
#include "highgui.h"
#include "GameState.h"
#include "SerialConnection.h"
#include "Gametime.h"
#include <iostream>
#include <fstream>
#define DISTANCETOGOAL 300
using namespace std;
bool running=false;
bool paused=false;
SerialConnection* homeSerial;
SerialConnection* awaySerial;

Team homeTeam;
Team awayTeam;

CamCapture* capture;
ObjectTracker track_puck;
Puck puck;
//int gameTime=0;
//int gameStartTime=0;
bool trackingInitialized=false;
bool micConInitialized=false;
bool initializeTracking(){
	if(!trackingInitialized){
	
		capture = new CamCapture();
		
		if (!capture->initCam()){
			fprintf(stderr,"Could not initialize capturing...\n");
			return false;
		}
		try{
			/*skapar ett bildbehandlings objekt somletar efter grönt på spelplanen, se Objecttracker.cpp*/
			track_puck = ObjectTracker(cvLoadImage("playField.bmp",0),"green");
		}catch(exception e){
			cout<<"no mask or color information"<<endl;
			return false;
		}
		trackingInitialized=true;

		//tar en bild och använder den för att skapa en puckrepresentation
		IplImage* frame = cvCreateImage(IMAGESIZE,8,3);
		capture->myQueryFrame(frame);
		puck=Puck(frame);
	}
	return true;
}
bool initializeMicroControllers(){
	if(homeSerial==NULL){//skapar en seriell anslutning till en mikrokontroller om den inte redan finns
		homeSerial=new SerialConnection("COM3");
		//homeSerial->write(NULL,0,"a");//ber mikrokontrollern att kalibrera sig
	}
	
	if(awaySerial==NULL){
		awaySerial=new SerialConnection("COM4");//likadant för andra laget
		//awaySerial->write(NULL,0,"a");
	}
	
	if(!homeSerial->isAvailable()||!awaySerial->isAvailable()){//kollar om anslutning finns med mikrokontroller
		cout<<"error: could not connect to microprocessors"<<endl;
		cout<<"please connect them to COM ports 3 and 4 and shut down all other processes communicating with them"<<endl;
 		return false;
	}
	return true;
}
unsigned __stdcall cameraThread(void* param){
	CvPoint2D32f* puckPoint=new CvPoint2D32f();
	IplImage* frame = cvCreateImage(IMAGESIZE,8,3);//skapar en buffer för en bild
	int t0=0,t1=0,t2=0,t3=0;//DEBUGVERKTYG

	while(running){
		t0=clock();
		//fyller buffren med en ny bild
		capture->myQueryFrame(frame);//40ms
		t1=clock();
		//hanterar bilden
		track_puck.trackObject(frame,puckPoint);//20ms
		t2=clock();
		//uppdaterar puckens position med ny data
		puck.updatePosition(puckPoint);
		t3=clock();
		//float cycle=(GetTickCount()-t);
		//cout<<t0-t1<<"\t"<<t1-t2<<"\t"<<t3-t2<<endl;
		//

	}
	return NULL;
}

unsigned __stdcall senderThread(void* param){
	const int width = 800;
	const int height = 400;
	CvPoint center = cvPoint(width / 2, height / 2);
	IplImage *pImg = cvCreateImage(cvSize(width, height), 8, 3);
	IplImage *pImgBg = cvCreateImage(cvSize(width, height), 8, 3);
	cvRectangle(pImgBg, cvPoint(0, 0), cvPoint(width, height), cvScalar(0, 0, 0, 0), CV_FILLED);
	for (int i = 0; i < 2; i++) {
		Team *pTeam = teamFromId(i);
		for (int j = 0; j < 6; j++) {
			Player * pPlayer = pTeam->getPlayer(j);
			for (int k = 0; k < 256; k++) {
				PlayerLocation loc = pPlayer->getLocation(k);
				cvCircle(pImgBg, cvPoint(center.x - loc.x, center.y + loc.y), 2, cvScalar(128, 128, 128, 255), CV_FILLED);
			}
		}
	}

	cout<<"senderthread started"<<endl;
	const int MESSAGELENGTH=29;
	unsigned char homeStatus[100];   //borde inte dessa minskas???
	unsigned char awayStatus[100];
	int homeMessage[MESSAGELENGTH];
	int awayMessage[MESSAGELENGTH];
	ofstream myfile=ofstream();
	myfile.open ("measurements.txt");
	time_t msec = time(NULL) * 1000;
	
	int t=0;
	while(running){//bygger meddelanden och skickar dem till AI-modulerna
		//long t=clock();
		Sleep(10);
		//cout<<" cycletime: "<<clock()-t<<endl;
		//t =clock();
		int lengthHome=homeSerial->read((char *)homeStatus);
		int lengthAway=awaySerial->read((char *)awayStatus);

		homeTeam.update(homeStatus);
		awayTeam.update(awayStatus);

		
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
			Team *pTeam = teamFromId(i);
			for (int j = 0; j < 6; j++) {
				Player * pPlayer = pTeam->getPlayer(j);
				PlayerLocation loc = pPlayer->getCurrentLocation();
				cvCircle(pImg, cvPoint(center.x - loc.x, center.y + loc.y), 3, i == 0 ? colorSweden : colorFinland, CV_FILLED);
			}
		}
		char buf[20];
		sprintf_s(buf, "FPS: %f", fps);
		cvPutText(pImg, buf, cvPoint(0, 20), &cvFont(1), cvScalar(255, 255, 255, 255));
		cvShowImage("test", pImg);
		cvWaitKey(1);

		if ((homeTeamConnection != NULL) && 
			(awayTeamConnection != NULL)) {
			int index=0;
			awayMessage[index]   = awayTeam.getGoals();
			homeMessage[index++] = homeTeam.getGoals();

			awayMessage[index]   = homeTeam.getGoals();
			homeMessage[index++] = awayTeam.getGoals();

			awayMessage[index]=puck.x;
			homeMessage[index++]=puck.x;

			awayMessage[index]=puck.y;
			homeMessage[index++]=puck.y;

			awayMessage[index]=getGametime();
			homeMessage[index++]=getGametime();

			myfile<<getGametime()<<"\t";
			for(int i=0;i<12;i++){
				awayMessage[index]=(awayStatus[i]&0xff);//awayStatus[i];
				homeMessage[index++]=(homeStatus[i]&0xff);
			
				myfile << (int)homeStatus[i] << "\t";	
			}
		
			for(int i=0;i<12;i++){
				awayMessage[index]=(homeStatus[i]&0xff);
				homeMessage[index++]=(awayStatus[i]&0xff);//awayStatus[i];
			
				myfile << (int)awayStatus[i] << "\t";	
			}
		
			myfile<<endl;
			homeTeamConnection->send(awayMessage,MESSAGELENGTH);
			awayTeamConnection->send(homeMessage,MESSAGELENGTH);
		}
		//cout<<clock()-t<<endl;;
		
	}
	
	return NULL;
}
Puck::Puck(IplImage* frame){
		x=0;
		y=0;
		ObjectTracker track_goal1 = ObjectTracker(cvLoadImage("goal1.bmp",0),"red");//object som letar efter ena målet
		ObjectTracker track_goal2 = ObjectTracker(cvLoadImage("goal2.bmp",0),"red");//object som letar efter andra målet
		
		
		goal1=cvPoint2D32f(62,117);//skapar punkter med målens ungefärliga position
		goal2=cvPoint2D32f(324,116);
		
		

		track_goal1.trackObject(frame,&goal1,false);//hittar målens position
		track_goal2.trackObject(frame,&goal2,false);
	}
void Puck::updatePosition(CvPoint2D32f* ps){
	//gör linjär transformation pixelkordinatrer -> millimeter med hjälp utav målens position
 	x=ps->x-(goal1.x+goal2.x)/2;
	y=ps->y-(goal1.y+goal2.y)/2;
	//cout<<x<<", "<<y<<endl;
	float v1x=goal2.x-goal1.x;
	float v1y=goal2.y-goal1.y; 
	float v2x=v1y;
	float v2y=-v1x;
	float norm=sqrt(v1x*v1x+v1y*v1y);

	v1x=v1x/norm;
	v1y=v1y/norm;
	v2x=v2x/norm;
	v2y=v2y/norm;
	x=v1x*x+v1y*y;
	y=v2x*x+v2y*y;
	x*=2*DISTANCETOGOAL/norm;
	y*=2*DISTANCETOGOAL/norm;
	//cout<<goal1.x<<", "<<goal1.y<<endl;
	
}

Team *teamFromId(int id) {
	if (id == 0)
		return &homeTeam;
	else if (id == 1)
		return &awayTeam;
	return NULL;
}