#include "PracticalSocket.h"
#include <string> 
#define BUFLENGTH 255
#define PORT 60040
#ifndef CONNECTION
#define CONNECTION
struct Connection{//struct för att hålla reda på anslutnng
	std::string adress;
	unsigned short port;
	bool operator==(Connection b);
	Connection(){adress="";port=0;};
};
class TeamConnection{// klass för att sköta kommunikation med AI-modul
	Connection connection;
	UDPSocket* socket;
	int lastAlive;
	char *name;	
public:
	void send(int* buf,const int bufLength);
	void sendBytes(char *buf, const int bufLength);
	bool fromSource(Connection source);
	void command(int com[BUFLENGTH],int length);
	void reportAlive();
	bool isAlive();
	char *getName();
	TeamConnection(Connection source, char *name);
	~TeamConnection();
};
extern TeamConnection* homeTeamConnection;
extern TeamConnection* awayTeamConnection;

extern UDPSocket *listeningSocket;

unsigned __stdcall recieverThread(void* param);
unsigned __stdcall checkClientsProc(void *param);

#endif