#include "TeamConnection.h"
#include "Gametime.h"
#include "Limits.h"

#include <iostream>
#include <process.h>
#include <Windows.h>
#include <fstream>

// Class methods
TeamConnection::TeamConnection(Connection source, char *name) {
	connection = source;
	socket = new UDPSocket();
	lastAlive = 0;
	this->name = name;
}

TeamConnection::~TeamConnection() {
	if (socket != NULL) {
		delete socket;
		socket = NULL;
	}
}

void TeamConnection::send(int *toSend, const int bufLength){ //skickar speltillstånd till spelare
	char msg[4*39]; //antal chars som beskriver speltillståndet

	for (int i=0; i<bufLength; i++) { //dekonstruerar ett intfält till ett charfält
		int x = toSend[i];
		int j = i * 4;

		msg[j++] = (unsigned char) ((x >> 0) & 0xff);  // läser en byte i taget i varje int
		msg[j++] = (unsigned char) ((x >> 8) & 0xff);
		msg[j++] = (unsigned char) ((x >> 16) & 0xff);
		msg[j++] = (unsigned char) ((x >> 24) & 0xff);

	}

	sendBytes(msg, 4 * bufLength);
}

void TeamConnection::sendBytes(char *msg, const int bufLength) {
	socket->sendTo(msg, bufLength, connection.adress, connection.port);
}

bool TeamConnection::isFromSource(Connection source) {
	return connection == source;
}

void TeamConnection::reportAlive() {
	lastAlive = getGametime();
}

bool TeamConnection::isAlive() {
	int currentTime = getGametime();
	return (currentTime - lastAlive) < 10000;
}

char *TeamConnection::getName() {
	return this->name;
}

bool Connection::operator==(Connection b){ 
	return b.adress==adress && b.port==port;
}

// Instances
namespace teamconnectionns {
	UDPSocket *listeningSocket = NULL;

	TeamConnection* homeTeamConnection = NULL;
	TeamConnection* awayTeamConnection = NULL;

	HANDLE networkReceiverThreadHandle;
	HANDLE clientsAliveThreadHandle;

	void(*homeCommandsReceived)(char *, int) = NULL;
	void(*awayCommandsReceived)(char *, int) = NULL;

	bool listening;
}
using namespace teamconnectionns;

TeamConnection *teamFromConnection(Connection *pConnection) {
	if (homeTeamConnection->isFromSource(*pConnection)) 
		return homeTeamConnection;
	else if(awayTeamConnection->isFromSource(*pConnection))
		return awayTeamConnection;
	return NULL;
}

TeamConnection* getHomeTeamConnection() {
	return homeTeamConnection;
}

TeamConnection* getAwayTeamConnection() {
	return awayTeamConnection;
}

unsigned __stdcall networkReceiverThread(void *param) {
	//###### Player comands are saved here
	char homeCmd[30];
	char awayCmd[30];
	//######

	if (homeTeamConnection == NULL || awayTeamConnection == NULL) {
		cerr << "teams not defined" << endl;
		exit(1);
	}

	Connection source = Connection();
	char buf[BUFLENGTH];	 // läst meddelande
	char msg[BUFLENGTH/4+1]; // formaterat medelande
	char err[BUFLENGTH/4+1]; // formaterat svar, misslyckade kommandon
	
	/*
	ofstream myfile = ofstream();	// DEBUGVERKTYG
	myfile.open ("commands.txt");	// öpnnar fil där kommandon sparas
	*/

	// TODO: These shouldn't be here!
	//pHomeSerial->write(NULL, 0);		// slutar kalibrara mikrokontrollerna när spelet börjar
	//pAwaySerial->write(NULL, 0);

	while(listening) {// recieves commands and passes them on to correct microprocessor
		int rcvBytes = listeningSocket->recvFrom(buf,BUFLENGTH,source.adress,source.port); //tar emot meddelande
		TeamConnection *teamC = teamFromConnection(&source);
		if (teamC != NULL) {
			if (rcvBytes == 1) {					
				if (buf[0] == 'N')					// heartbeat protocol
					teamC->reportAlive();
			}
			else if ((rcvBytes % (5 * 4)) == 0) {	// contains command(s)
				int index = 0;
				int errIndex = 0;
				int cmdIndex = 0;
				for (int cmdIndex = 0; cmdIndex < rcvBytes / 5 / 4; cmdIndex++) {
					//myfile << getGametime() << "\t";	//sparar speltiden när kommandot mottogs

					char cmd[5];
					for (int i = 0; i < 5; i++) {		//läser intfält som ett charfält av kommandon
						cmd[i] = buf[cmdIndex * 5 * 4 + i * 4];
						//myfile << (int)(i == 3 ? (signed char)msg[i] : (unsigned char)msg[i]) << "\t";
					}
					// one command should now be in msg
					if (limits::isCommandOkay(teamC == homeTeamConnection ? 0 : 1, cmd)) {
						memcpy(msg + index, cmd, 5);
						index += 5;
						//myfile << "+";
					}
					else {
						memcpy(err + errIndex, cmd, 5);
						errIndex += 5;
						//myfile << "-";
					}
					//myfile<<endl;
				}

				if (errIndex > 0) {
					int errMessage[BUFLENGTH/4 + 1];
					for (int i = 0; i < errIndex; i++) 
						errMessage[i] = (int)(i % 5 == 3 ? (signed char)err[i] : (unsigned char)err[i]);
					teamC->send(errMessage, errIndex);
				}
				
				//skickar vidare kommandot beroende på vartifrån meddelandet kom ifrån
				if (teamC == homeTeamConnection && homeCommandsReceived != NULL)
					homeCommandsReceived(msg, index);
				else if(teamC == awayTeamConnection && awayCommandsReceived != NULL)
					awayCommandsReceived(msg, index);
			}
		}
	}
}

bool checkClient(TeamConnection *pTeam) {
	char c = 'D';
	int i = 0;

	while (!pTeam->isAlive() && i < 5) {
		pTeam->sendBytes(&c, 1);
		i++;
		Sleep(1000);
	}
	if (!pTeam->isAlive()) {
		cout << pTeam->getName() << " not alive!" << endl;
		return false;
	}
	return true;
}

unsigned __stdcall checkClientsProc(void *param) {
	void(*stopFunction)(void) = (void(*)(void))param;

	while (listening) {
		if (!checkClient(homeTeamConnection))
			stopFunction();
		else if (!checkClient(awayTeamConnection))
			stopFunction();
		Sleep(10000);
	}
	return 0;
}

// upprättar anslutning till AI-moduler (spelprogramvara)
bool setUpConnections(void(*stopFunction)(void), 
	void(*homeCmdsReceived)(char *msg, int length), 
	void(*awayCmdsReceived)(char *msg, int length))
{
	if (homeTeamConnection==NULL || awayTeamConnection==NULL) {
		listeningSocket = new UDPSocket(PORT); // objekt för udp-kommunikation

		Connection homeSource = Connection(); // objekt som innehåller information om anslutningar, se TeamConnection.cpp
		Connection awaySource = Connection();
		char* buf[BUFLENGTH]; // skapar en buffert i minnet att lägga motagna meddelanden

		int handshake[] = {1}; // skapar ett handskaknings meddelande

		cout << "waiting at homeplayer" << endl;

		listeningSocket->recvFrom(buf, BUFLENGTH, homeSource.adress, homeSource.port);//Väntar på handskakning från AI-modul
		homeTeamConnection = new TeamConnection(homeSource, "Home team");//Sparar AImodulens plats
		homeTeamConnection->send(handshake, 1);//Skickar hanskakning

		cout << "home aquired at port " << homeSource.port << endl;
		cout << "waiting at awayplayer" << endl;

		handshake[0] = 2;//ny hanskakning för andra laget
	
		listeningSocket->recvFrom(buf, BUFLENGTH, awaySource.adress, awaySource.port);
		awayTeamConnection = new TeamConnection(awaySource, "Away team");
		awayTeamConnection->send(handshake, 4);
		cout << "away aquired at port "<< awaySource.port << endl;
		if (homeTeamConnection->isFromSource(awaySource)) { //kollar att int samma AI har anslutit igen, om så ge fel
			cout << "team already aquired, next team must be on another port" << endl;
			return false;
		}

		homeCommandsReceived = homeCmdsReceived;
		awayCommandsReceived = awayCmdsReceived;

		networkReceiverThreadHandle = (HANDLE)_beginthreadex(NULL, 0, networkReceiverThread, NULL, CREATE_SUSPENDED, NULL);
		clientsAliveThreadHandle = (HANDLE)_beginthreadex(NULL, 0, checkClientsProc, (void*)stopFunction, CREATE_SUSPENDED, NULL);
	}
	return true;
}

void pauseListening() {
	SuspendThread(networkReceiverThreadHandle);
	SuspendThread(clientsAliveThreadHandle);
}

void resumeListening() {
	listening = true;
	ResumeThread(networkReceiverThreadHandle);
	ResumeThread(clientsAliveThreadHandle);
}

void stopListening() {
	listening = false;
	// TODO: Is this really the best/safest way?
	TerminateThread(networkReceiverThreadHandle, 0);

	if (homeTeamConnection != NULL) {
		delete homeTeamConnection;
		homeTeamConnection = NULL;
	}
	if (awayTeamConnection != NULL) {
		delete awayTeamConnection;
		awayTeamConnection = NULL;
	}
	if (listeningSocket != NULL) {
		delete listeningSocket;
		listeningSocket = NULL;
	}
}