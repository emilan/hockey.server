#include "Limits.h"
#include "GameState.h"
#include <vector>

#define MAX_TRANSLATION_DIFF	5
#define MAX_PLAYERS_IN_MOVEMENT 2

using namespace std;

typedef struct {
	unsigned char teamId;
	unsigned char playerId;
	unsigned char destinationTranslation;
} ApprovedCommand;

vector<ApprovedCommand> approvedCommands = vector<ApprovedCommand>();

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
		Team *pTeam = teamFromId(it->teamId);
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