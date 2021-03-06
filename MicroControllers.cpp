#include "MicroControllers.h"
#include "SerialConnection.h"

#include <iostream>		// For console output
#include <ctime>

using namespace std;

namespace microcontrollersns {
	SerialConnection *homeSerial;
	SerialConnection *awaySerial;
	HANDLE senderThreadHandle;
	float microControllerFps = 0;
}
using namespace microcontrollersns;

unsigned __stdcall microControllerReadThread(void* param) {
	cout << "micro controller thread started" << endl;
	void(*newDataFunction)(unsigned char*, unsigned char*) = (void(*)(unsigned char*, unsigned char*))param;

	unsigned char homeStatus[100];   //borde inte dessa minskas???
	unsigned char awayStatus[100];
	
	while (true) { // reads from serial connections and updates team status
		static clock_t tOld = 0;
		static unsigned char counter = 0;
		if (++counter == 20) {
			clock_t tNew = clock();
			microControllerFps = 20.0f / (1.0f * (tNew - tOld) / CLOCKS_PER_SEC);
			tOld = tNew;
			counter = 0;
		}
		Sleep(16);

		homeSerial->read((char *)homeStatus);
		awaySerial->read((char *)awayStatus);

		if (newDataFunction != NULL)
			newDataFunction(homeStatus, awayStatus);
	}
	return NULL;
}

bool initializeMicroControllers(void(*newDataFunction)(unsigned char*, unsigned char*)) {
	// TODO: Search all com ports until you find the two microcontrollers (they should identify themselves as t0 and t1)
	if(homeSerial==NULL) //skapar en seriell anslutning till en mikrokontroller om den inte redan finns
		homeSerial = new SerialConnection("COM3");
	
	if(awaySerial == NULL)
		awaySerial = new SerialConnection("COM4");//likadant f�r andra laget
	
	if(!homeSerial->isAvailable() || !awaySerial->isAvailable()) { //kollar om anslutning finns med mikrokontroller
		cout << "error: could not connect to microprocessors" << endl;
		cout << "please connect them to COM ports 3 and 4 and shut down all other processes communicating with them" << endl;
 		return false;
	}
	senderThreadHandle = (HANDLE)_beginthreadex(NULL, 0, microControllerReadThread, newDataFunction, 0, NULL);
	return true;
}

void calibrateMicroControllers() {
	cout << "In order to start automatic calibration, you need to turn every player" << endl;
	cout << "such that he faces foward." << endl;
	system("pause");
	homeSerial->write(NULL, 0, "a");
	awaySerial->write(NULL, 0, "a");
	cout << "Automatic calibration has been initialized." << endl;
	cout << "Please wait for it to finish." << endl;
	system("pause");
	cout << "Calibration done!" << endl;
}

void sendCommandsToHomeMicroController(char *msg, int length) {
	homeSerial->write(msg, length);
}

void sendCommandsToAwayMicroController(char *msg, int length) {
	awaySerial->write(msg, length);
}

float getMicroControllerFrequency() {
	return microControllerFps;
}