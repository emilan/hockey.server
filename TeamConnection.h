#include "PracticalSocket.h"
#include <string> 
#define BUFLENGTH 255
#define PORT 60040

#ifndef __TEAM_CONNECTION_H__
#define __TEAM_CONNECTION_H__

struct Connection { //struct för att hålla reda på anslutnng
	std::string adress;
	unsigned short port;
	bool operator==(Connection b);
	Connection(){adress="";port=0;};
};

class TeamConnection { // klass för att sköta kommunikation med AI-modul
	Connection connection;
	UDPSocket* socket;
	int lastAlive;
	char *name;	
public:
	void send(int* buf,const int bufLength);
	void sendBytes(char *buf, const int bufLength);
	bool isFromSource(Connection source);
	void command(int com[BUFLENGTH],int length);
	void reportAlive();
	bool isAlive();
	char *getName();
	TeamConnection(Connection source, char *name);
	~TeamConnection();
};

void acquireTeamConnections();
void releaseTeamConnections();
TeamConnection* getHomeTeamConnection();
TeamConnection* getAwayTeamConnection();

bool setUpConnections(void(*stopFunction)(void), 
	void(*homeCmdsReceived)(char *msg, int length), 
	void(*awayCmdsReceived)(char *msg, int length));
void pauseListening();
void stopListening();
void resumeListening();

#endif //__TEAM_CONNECTION_H__