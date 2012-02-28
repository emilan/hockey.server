#include "TeamConnection.h"
#include <iostream>
#include <process.h>
#include "GameState.h"
#include <fstream>
#include "Gametime.h"
#include "HockeyGame.h"
#include "Limits.h"

UDPSocket *listeningSocket = NULL;

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
void TeamConnection::send(int* toSend,const int bufLength){//skickar speltillst�nd till spelare
	char msg[4*39];//antal chars som beskriver speltillst�ndet

	for (int i=0; i<bufLength; i++) {//dekonstruerar ett intf�lt till ett charf�lt
		int x = toSend[i];
		int j = i << 2;

		msg[j++] = (byte) ((x >> 0) & 0xff);  // l�ser en byte i taget i varje int

		msg[j++] = (byte) ((x >> 8) & 0xff);
		msg[j++] = (byte) ((x >> 16) & 0xff);
		msg[j++] = (byte) ((x >> 24) & 0xff);

	}
	/*for(int k=4*9;k<4*10;k++){
		int a=msg[k];
		cout<<a<<"\t";
	}
	cout<<endl;*/
	socket->sendTo(msg,4*bufLength,connection.adress,connection.port);//Skickar meddelandet till AI-modul

}

void TeamConnection::sendBytes(char *msg, const int bufLength) {
	socket->sendTo(msg,bufLength,connection.adress,connection.port);//Skickar meddelandet till AI-modul
}

bool TeamConnection::fromSource(Connection source){
	return connection==source;
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

bool Connection::operator==(Connection b){//likamed operator f�r anslitning
	return b.adress==adress&&b.port==port;
}

TeamConnection* homeTeamConnection=0;
TeamConnection* awayTeamConnection=0;

TeamConnection *teamFromConnection(Connection *pConnection) {
	if (homeTeamConnection->fromSource(*pConnection)) 
		return homeTeamConnection;
	else if(awayTeamConnection->fromSource(*pConnection))
		return awayTeamConnection;
	return NULL;
}


unsigned __stdcall recieverThread(void* sock){
	//###### Player comands are saved here
	char homeCmd[30];
	char awayCmd[30];
	//######
	if(homeTeamConnection==NULL||awayTeamConnection==NULL){
		cerr<<"teams not defined"<<endl;
		exit(1);
	}
	Connection source=Connection();
	UDPSocket* socket=(UDPSocket*) sock;
	char buf[BUFLENGTH];//l�st meddelande
	char msg[BUFLENGTH/4+1]; //formaterat medelande
	char err[BUFLENGTH/4+1]; //formaterat svar, misslyckade kommandon

	ofstream myfile=ofstream();		//DEBUGVERKTYG
	myfile.open ("commands.txt");	//�pnnar fil d�r kommandon sparas
	homeSerial->write(NULL,0);//slutar kalibrara mikrokontrollerna n�r spelet b�rjar
	awaySerial->write(NULL,0);
	for(;;){// recieves commands and passes them on to correct microprocessor
		int rcvBytes=socket->recvFrom(buf,BUFLENGTH,source.adress,source.port); //tar emot meddelande
		TeamConnection *teamC = teamFromConnection(&source);
		if (teamC != NULL) {
			if (rcvBytes == 1) {
				if (buf[0] == 'N')
					teamC->reportAlive();
			}
			else if ((rcvBytes % (5 * 4)) == 0) {
				int index = 0;
				int errIndex = 0;
				int cmdIndex = 0;
				for (int cmdIndex = 0; cmdIndex < rcvBytes / 5 / 4; cmdIndex++) {
					myfile << getGametime() << "\t";			//sparar speltiden n�r kommandot mottogs
					char cmd[5];
					for (int i = 0; i < 5; i++) {	//l�ser intf�lt som ett charf�lt av kommandon
						cmd[i] = buf[cmdIndex * 5 * 4 + i * 4];
						myfile << (int)(i == 3 ? (signed char)msg[i] : (unsigned char)msg[i]) << "\t";
					}
					// one command should now be in msg
					if (isCommandOkay(teamC == homeTeamConnection ? 0 : 1, cmd)) {
						memcpy(msg + index, cmd, 5);
						index += 5;
						myfile << "+";
					}
					else {
						memcpy(err + errIndex, cmd, 5);
						errIndex += 5;
						myfile << "-";
					}
					myfile<<endl;
				}

				if (errIndex > 0) {
					int errMessage[BUFLENGTH/4 + 1];
					for (int i = 0; i < errIndex; i++) 
						errMessage[i] = (int)(i % 5 == 3 ? (signed char)err[i] : (unsigned char)err[i]);
					teamC->send(errMessage, errIndex);
				}
				
				if (teamC == homeTeamConnection)//skickar vidare kommandot beroende p� vartifr�n meddelandet kom ifr�n
					homeSerial->write(msg, index);//skriver till mikrokontroller
				else if(teamC == awayTeamConnection)
					awaySerial->write(msg, index);			
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
	while (true) {
		if (!checkClient(homeTeamConnection))
			((HockeyGame *)param)->stopGame();
		if (!checkClient(awayTeamConnection))
			((HockeyGame *)param)->stopGame();
		Sleep(10000);
	}
}
