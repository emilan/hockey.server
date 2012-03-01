#include "HockeyGame.h"
#include "PracticalSocket.h"
#include "TeamConnection.h"
#include <iostream>
#include "GameState.h"
#include "SerialConnection.h"
#include "Gametime.h"
using namespace std;

bool HockeyGame::setUpConnections(){//upprättar anslutning AI-moduler
	if(homeTeamConnection==NULL||awayTeamConnection==NULL){
		listeningSocket = new UDPSocket(PORT);//objekt för udp-kommunikation

		Connection homeSource=Connection();//objekt som innehåller information om anslutningar, se TeamConnection.cpp
		Connection awaySource=Connection();
		char* buf[BUFLENGTH];//skapar en buffert i minnet att lägga motagna meddelanden

		int handshake[]={1};//skapar ett handskaknings meddelande

		cout<<"waiting at homeplayer"<<endl;

		listeningSocket->recvFrom(buf,BUFLENGTH,homeSource.adress,homeSource.port);//Väntar på handskakning från AI-modul
		homeTeamConnection = new TeamConnection(homeSource, "Home team");//Sparar AImodulens plats
		homeTeamConnection->send(handshake,1);//Skickar hanskakning

		cout<<"home aquired at port "<<homeSource.port<<endl;
		cout<<"waiting at awayplayer"<<endl;

		handshake[0]=2;//ny hanskakning för andra laget
	
		listeningSocket->recvFrom(buf,BUFLENGTH,awaySource.adress,awaySource.port);
		awayTeamConnection = new TeamConnection(awaySource, "Away team");
		awayTeamConnection->send(handshake,4);
		cout<<"away aquired at port "<<awaySource.port<<endl;
		if(homeTeamConnection->fromSource(awaySource)){//kollar att int samma AI har anslutit igen, om så ge fel
			cout<<"team already aquired, next team must be on another port"<<endl;
			return false;
		}
		recieverThreadHandle=(HANDLE)_beginthreadex(NULL,0,recieverThread,(void*)listeningSocket,CREATE_SUSPENDED,NULL);//startar tråden pausad, se teamConnections.cpp
		clientsAliveThreadHandle = (HANDLE)_beginthreadex(NULL, 0, checkClientsProc, (void *)this, CREATE_SUSPENDED, NULL);
	}
	return true;
}
bool HockeyGame::setUpGamestate(){
	if(!initializeMicroControllers()){
		return false;
	}

	senderThreadHandle=(HANDLE)_beginthreadex(NULL,0,senderThread,NULL,CREATE_SUSPENDED,NULL);//skapar tråden pausad
	
	return true;
}
bool HockeyGame::setUpCamera(){
	if(!initializeTracking()){ //startar bildhantering, se gamestate.cpp, definierar track_puck
		return false;
	}
	cameraThreadHandle=(HANDLE)_beginthreadex(NULL,0,cameraThread,NULL,CREATE_SUSPENDED,NULL);//skapar kameratråden pausad
	return true;
}

bool HockeyGame::initPlayerPositions() {
	for (int i = 0; i < 2; i++) {
		Team *pTeam = teamFromId(i);
		for (int j = 0; j < 6; j++) {
			char fileName[20];
			sprintf(fileName, "player%d%d.txt", i, j);
			if (!pTeam->getPlayer(j)->readLocations(fileName))
				return false;
		}
	}
	return true;
}

bool HockeyGame::initializeGame(){
	//creates the gamethreads suspended
	if(!running){
		running=true;
		if (!initPlayerPositions()) {
			running = false;
			cout << "Failed reading player positions" << endl;
			return running;
		}
		if(!setUpCamera()){//avbryter starten om inte kameran kan startas
			running= false;
			return running;
		}
		if(!setUpGamestate()){
			running= false;
			return running;
		}
		if(!setUpConnections()){
			running= false;
			return running;
		}
		paused=true;
		
		cout<<"successfully initialized game: starting gamethreads"<<endl;
		startGametime();//definieras i Gametime.cpp
		//starts the threads
		resumeGame();
		
	}else{
		cout<<"can't initialize: game already running"<<endl;
	}
	return running;
}

void safeTerminateThread(HANDLE threadHandle) {
	if (threadHandle != NULL)
		TerminateThread(threadHandle, 0);
}

void HockeyGame::stopGame(){//avslutar spelet
	if(running){
		safeTerminateThread(senderThreadHandle);
		safeTerminateThread(recieverThreadHandle);
		safeTerminateThread(cameraThreadHandle);

		// TODO: Make/find delete and null-function
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

		running=false;
		cout << "stopped" << endl;
		
		safeTerminateThread(clientsAliveThreadHandle);
	}else{
		cout<<"can't stop game: game isn't running"<<endl;
	}
}

void safeSuspendThread(HANDLE handle) {
	if (handle != NULL)
		SuspendThread(handle);
}

void HockeyGame::pauseGame(){//pausa spelet
	if(running&&!paused){
		paused=true;
		pauseGametime();

		safeSuspendThread(senderThreadHandle);
		safeSuspendThread(recieverThreadHandle);
		safeSuspendThread(cameraThreadHandle);
		safeSuspendThread(clientsAliveThreadHandle);

		cout << "paused" << endl;
	}else{
		cout<<"can't pause game"<<endl;
	}
}

void safeResumeThread(HANDLE handle) {
	if (handle != NULL)
		ResumeThread(handle);
}

void HockeyGame::resumeGame(){
	if(running&&paused){
		paused=false;
		resumeGametime();

		safeResumeThread(senderThreadHandle);
		safeResumeThread(recieverThreadHandle);
		safeResumeThread(cameraThreadHandle);
		safeResumeThread(clientsAliveThreadHandle);

		cout << "resumed" << endl;
	}else{
		cout<<"can't resume game"<<endl;
	}
}