#include "HockeyGame.h"
#include "TeamConnection.h"
#include "Gametime.h"
#include "Puck.h"
#include "GUI.h"
#include "Team.h"
#include "MicroControllers.h"
#include "CameraCalibration.h"

#include <iostream>		// For console output
#include <fstream>		// For logging to file (measurements.txt)

using namespace std;

bool running = false;
bool paused = false;

void microControllersRead(unsigned char *homeStatus, unsigned char *awayStatus) {
	const int MESSAGELENGTH=29;
	int homeMessage[MESSAGELENGTH];
	int awayMessage[MESSAGELENGTH];

	TeamConnection *pHomeTeamConnection = getHomeTeamConnection();
	TeamConnection *pAwayTeamConnection = getAwayTeamConnection();

	Team *pHomeTeam = getHomeTeam();
	Team *pAwayTeam = getAwayTeam();

	PuckPosition puck = getPuckPosition();
	
	int gametime = getGametime();

	// Update teams
	getHomeTeam()->update(homeStatus);
	getAwayTeam()->update(awayStatus);

	// Send new state to AI
	if ((pHomeTeamConnection != NULL) && 
		(pAwayTeamConnection != NULL)) {
		int index=0;
		awayMessage[index]   = pAwayTeam->getGoals();
		homeMessage[index++] = pHomeTeam->getGoals();

		awayMessage[index]   = pHomeTeam->getGoals();
		homeMessage[index++] = pAwayTeam->getGoals();

		awayMessage[index]   = puck.x;
		homeMessage[index++] = puck.x;

		awayMessage[index]   = puck.y;
		homeMessage[index++] = puck.y;

		awayMessage[index]   = gametime;
		homeMessage[index++] = gametime;

		for(int i = 0; i < 12; i++) {
			awayMessage[index]   = (awayStatus[i] & 0xff);
			homeMessage[index++] = (homeStatus[i] & 0xff);
		}

		for(int i=0;i<12;i++){
			awayMessage[index]   = (homeStatus[i] & 0xff);
			homeMessage[index++] = (awayStatus[i] & 0xff);
		}

		pHomeTeamConnection->send(awayMessage, MESSAGELENGTH);
		pAwayTeamConnection->send(homeMessage, MESSAGELENGTH);
	}

// DEBUG log measurements
#ifdef DEBUG
	static ofstream measurementsLog = ofstream();
	static bool measurementsLogInitialized = false;
	if (!measurementsLogInitialized) {
		measurementsLog.open("measurements.txt");
		measurementsLogInitialized = true;
	}

	measurementsLog << gametime << "\t";

	for (int i = 0; i < 12; i++) 
		measurementsLog << (int)homeStatus[i] << "\t";	

	for (int i=0;i<12;i++)
		measurementsLog << (int)awayStatus[i] << "\t";	

	measurementsLog << endl;
#endif
}

bool hockeygame::initialize() {
	cout << "Initializing camera and micro controllers, please be patient..." << endl;

	if (!initPlayerPositions()) {
		cout << "Failed reading player positions!" << endl;
		return false;
	}
	else if (!initializeTracking()) {
		cout << "Failed to initialize camera!";
		return false;
	}
	else if (!initializeMicroControllers(microControllersRead)) {
		cout << "Failed to initialize micro controllers!";
		return false;
	}

	startDrawing();

	cout << endl;
	return true;
}

bool hockeygame::startGame() {
	if (!running) {
		running=true;
		if (!setUpConnections(stopGame, sendCommandsToHomeMicroController, sendCommandsToAwayMicroController)) {
			running= false;
			return running;
		}
		paused=true;
		
		cout << "successfully started game" << endl;
		startGametime();//definieras i Gametime.cpp
		//starts the threads
		resumeGame();
		
	}
	else cout << "can't start: game already running" << endl;
	return running;
}

void hockeygame::stopGame(){//avslutar spelet
	if (running) {
		stopListening();

		running=false;
		cout << "stopped" << endl;
	} 
	else cout << "can't stop game: game isn't running" << endl;
}

void hockeygame::pauseGame(){ //pausa spelet
	if (running && !paused) {
		paused=true;
		pauseGametime();
		pauseListening();

		// TODO: Unregister sending

		cout << "paused" << endl;
	}
	else cout << "can't pause game" << endl;
}

void hockeygame::resumeGame(){
	if(running&&paused){
		paused=false;
		resumeGametime();
		resumeListening();

		// TODO: Reregister for sending

		cout << "resumed" << endl;
	}else{
		cout<<"can't resume game"<<endl;
	}
}

void hockeygame::calibrateCamera() {
	stopTrackingPuck();
	::calibrateCamera();	// startar bildkalibrerings program
	startTrackingPuck();
}

void hockeygame::calibrateMicroControllers() {
	::calibrateMicroControllers();
}