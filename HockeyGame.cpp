#include "HockeyGame.h"
#include "TeamConnection.h"
#include "Gametime.h"
#include "Puck.h"
#include "GUI.h"
#include "Team.h"
#include "MicroControllers.h"
#include "CameraCalibration.h"
#include "Limits.h"

#include <iostream>		// For console output
#include <fstream>		// For logging to file (measurements.txt)

using namespace std;

bool running = false;
bool paused = false;

void microControllersRead(unsigned char *homeStatus, unsigned char *awayStatus) {
	Team *pHomeTeam = getHomeTeam();
	Team *pAwayTeam = getAwayTeam();

	// Update teams
	pHomeTeam->update(homeStatus);
	pAwayTeam->update(awayStatus);

	acquireTeamConnections();
	TeamConnection *pHomeTeamConnection = getHomeTeamConnection();
	TeamConnection *pAwayTeamConnection = getAwayTeamConnection();
	// Send new state to AI
	if ((pHomeTeamConnection != NULL) && 
		(pAwayTeamConnection != NULL)) 
	{
		const int MESSAGELENGTH=29;
		int homeMessage[MESSAGELENGTH];
		int awayMessage[MESSAGELENGTH];
	
		puck::Position puck = puck::getPosition();
		int gametime = getGametime();

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

		pHomeTeamConnection->send(homeMessage, MESSAGELENGTH);
		pAwayTeamConnection->send(awayMessage, MESSAGELENGTH);
	}
	releaseTeamConnections();

// DEBUG log measurements
#ifdef DEBUG
	/*static ofstream measurementsLog = ofstream();
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

	measurementsLog << endl;*/
#endif
}

void homeGoalMade() {
	getHomeTeam()->goalMade();
	char homeCommands[30];
	for (int i = 0; i < 6; i++) {
		homeCommands[i * 5 + 0] = i;
		homeCommands[i * 5 + 1] = 100;
		homeCommands[i * 5 + 2] = 50;
		homeCommands[i * 5 + 3] = 127;
		homeCommands[i * 5 + 4] = 0;
	}
	char awayCommands[30];
	for (int i = 0; i < 6; i++) {
		awayCommands[i * 5 + 0] = i;
		awayCommands[i * 5 + 1] = 100;
		awayCommands[i * 5 + 2] = 10;
		awayCommands[i * 5 + 3] = 0;
		awayCommands[i * 5 + 4] = 0;
	}
	sendCommandsToHomeMicroController(homeCommands, 30);
	sendCommandsToAwayMicroController(awayCommands, 30);
}

void awayGoalMade() {
	getAwayTeam()->goalMade();
	char awayCommands[30];
	for (int i = 0; i < 6; i++) {
		awayCommands[i * 5 + 0] = i;
		awayCommands[i * 5 + 1] = 100;
		awayCommands[i * 5 + 2] = 50;
		awayCommands[i * 5 + 3] = 127;
		awayCommands[i * 5 + 4] = 0;
	}
	char homeCommands[30];
	for (int i = 0; i < 6; i++) {
		homeCommands[i * 5 + 0] = i;
		homeCommands[i * 5 + 1] = 100;
		homeCommands[i * 5 + 2] = 10;
		homeCommands[i * 5 + 3] = 0;
		homeCommands[i * 5 + 4] = 0;
	}
	sendCommandsToHomeMicroController(homeCommands, 30);
	sendCommandsToAwayMicroController(awayCommands, 30);
}

bool hockeygame::initialize() {
	cout << "Initializing camera and micro controllers, please be patient..." << endl;

	if (!initPlayerPositions()) {
		cout << "Failed reading player positions!" << endl;
		return false;
	}
	else if (!puck::initializeTracking(homeGoalMade, awayGoalMade)) {
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
		pauseGametime();
		//starts the threads
		
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
		limits::toggleRejectAllCommands();

		cout << "paused" << endl;
	}
	else cout << "can't pause game" << endl;
}

void hockeygame::resumeGame(){
	if(running&&paused){
		paused=false;
		resumeGametime();
		limits::toggleRejectAllCommands();

		cout << "resumed" << endl;
	}else{
		cout<<"can't resume game"<<endl;
	}
}

void hockeygame::calibrateCamera() {
	puck::stopTracking();
	::calibrateCamera();	// startar bildkalibrerings program
	puck::startTracking();
}

void hockeygame::calibrateMicroControllers() {
	::calibrateMicroControllers();
}

void hockeygame::faceOff() {
	if (paused) {
		hockeygame::resumeGame();
		limits::faceOff();
	}
	else cout << "face off must be done from paused state" << endl;
}