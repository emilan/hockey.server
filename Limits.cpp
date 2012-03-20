#include "Limits.h"
#include "Team.h"
#include "Puck.h"
#include "Gametime.h"

#include <vector>

#define MAX_TRANSLATION_DIFF	5
#define MAX_PLAYERS_IN_MOVEMENT 2

using namespace std;

namespace limitsns {
	typedef struct {
		unsigned char teamId;
		unsigned char playerId;
		unsigned char destinationTranslation;
	} ApprovedCommand;

	vector<ApprovedCommand> approvedCommands = vector<ApprovedCommand>();
}
using namespace limitsns;

void removePlayerCommands(int teamId, int playerId) {
	// TODO: Some kind of bug here? Reproduce!
	for (vector<ApprovedCommand>::iterator it = approvedCommands.begin(); it != approvedCommands.end(); ) {
		if (it->teamId == teamId && it->playerId == playerId)
			it = approvedCommands.erase(it);
		else ++it;
	}
}

bool isCommandOkayMaxInMovement(int teamId, char *cmd) {
	// If speed == 0, remove all queued commands for that player and allow the speed 0-command
	if (cmd[1] == 0) {
		removePlayerCommands(teamId, cmd[0]);
		return true;
	}

	// Step 1: Remove finished commands
	for (vector<ApprovedCommand>::iterator it = approvedCommands.begin(); it != approvedCommands.end(); ) {
		Team *pTeam = getTeamById(it->teamId);
		Player *pPlayer = pTeam->getPlayer(it->playerId);
		unsigned char trans = pPlayer->getTrans();

		if (abs(it->destinationTranslation - trans) < MAX_TRANSLATION_DIFF)
			it = approvedCommands.erase(it);
		else ++it;
	}

	// Count number of other players in movement
	int c = 0;
	for (vector<ApprovedCommand>::iterator it = approvedCommands.begin(); it != approvedCommands.end(); ++it) {
		if (it->teamId == teamId && it->playerId != cmd[0])
			++c;
	}

	if (c < MAX_PLAYERS_IN_MOVEMENT) {
		removePlayerCommands(teamId, cmd[0]);
		ApprovedCommand command = { teamId
								  , cmd[0]
								  , cmd[2]};
		approvedCommands.push_back(command);
	}
}

bool isCommandOkay(int teamId, char *cmd) {
	if (!isCommandOkayMaxInMovement(teamId, cmd))
		return false;
	return true;
}

namespace limitsns {
	unsigned char players[2] = {0, 0};
	int constructiveTime = 0;
}

// From SBHF:
// Efter det att pucken är möjlig att nå måste den tävlande snarast uträtta något konstruktivt,
// dock senast inom 5 sekunder.
bool checkConstructive() {
// TODO: Rethink where coordinates are handled!
#define WIDTH	849
#define HEIGHT	460

	PuckPosition puckPos = getPuckPosition();
	puckPos.x = WIDTH / 2 + puckPos.x;
	puckPos.y = HEIGHT / 2 + puckPos.y;
	unsigned char tmpPlayers[2] = {0, 0};
	
	for (int t = 0; t < 2; t++) {
		Team *pTeam = getTeamById(t);
		for (int p = 0; p < 6; p++) {
			Player *pPlayer = pTeam->getPlayer(p);
			if (pPlayer->canAccessCoordinate(puckPos.x, puckPos.y)) 
				tmpPlayers[t] |= 1 << p;
		}
	}

	bool res;

	if (tmpPlayers[0] != 0 && tmpPlayers[1] == 0) {
		if (players[0] != 0 && players[1] == 0)
			res = getGametime() - constructiveTime < 5000;
		else {
			constructiveTime = getGametime();
			res = true;
		}
	}
	else if (tmpPlayers[1] != 0 && tmpPlayers[0] == 0) {
		if (players[1] != 0 && players[0] == 0)
			res = getGametime() - constructiveTime < 5000;
		else {
			constructiveTime = getGametime();
			res = true;
		}
	}
	else res = true;
	

	players[0] = tmpPlayers[0];
	players[1] = tmpPlayers[1];

	return res;
}