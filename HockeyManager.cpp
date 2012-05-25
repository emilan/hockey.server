#include "HockeyManager.h"
#include "HockeyGame.h"
#include "Puck.h"

#include <iostream>
#include <conio.h>

using namespace std;

int main() {
	if (!hockeygame::initialize())
		return 1;

	bool quit = false;
	greeter();	//skriver ut välkomsttext
	showHelp();	// skriver ut hjälp

	while(!quit) {
		switch(getch()) {	//lyssnar efter input från tangetbord
		case 'i':
			cout << "starting..." << endl;
			hockeygame::startGame();
			break;
		case 'p':
			cout << "pausing" << endl;
			hockeygame::pauseGame();
			break;
		case 'r':
			cout << "resuming" << endl;
			hockeygame::resumeGame();
			break;
		case 's':
			cout << "stopping" << endl;
			hockeygame::stopGame();
			break;
		case 'f':
			cout << "face off!" << endl;
			hockeygame::faceOff();
			break;
		case 'q':
			quit = true;	// avslutar loopen
			cout << "quiting" << endl;
			break;
		case 'a':
			hockeygame::calibrateCamera();
			break;
		case 'h':
			showHelp();
			break;
		case 'c':
			hockeygame::calibrateMicroControllers();
			break;
		case 'm':
			puck::toggleCamera();
			break;
		default:
			;
		}
	}
}

void showHelp() {
	cout << "i - start game" << endl
		 << "p - pause game" << endl
		 << "s - stop game" << endl
		 << "q - quit game" << endl
		 << "h - show this help" << endl
		 << "c - calibrate microcontrollers" << endl
		 << "a - calibrate image processing" << endl
		 << "f - face off" << endl	// TODO: Automatic?
		 << "m - toggle camera" << endl
		 ;
}

void greeter() {
	cout << "Welcome to the Hockey Server program" << endl;
}
