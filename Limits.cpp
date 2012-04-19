#include "Limits.h"
#include "Team.h"
#include "Puck.h"
#include "Gametime.h"
#include "Mask.h"

#include <vector>

#define MAX_TRANSLATION_DIFF	5
#define MAX_PLAYERS_IN_MOVEMENT 2

using namespace std;

namespace limits_private {
	typedef struct {
		unsigned char teamId;
		unsigned char playerId;
		unsigned char destinationTranslation;
	} ApprovedCommand;

	vector<ApprovedCommand> approvedCommands = vector<ApprovedCommand>();

	void removePlayerCommands(int teamId, int playerId);
	bool isCommandOkayMaxInMovement(int teamId, char *cmd);
}

void limits_private::removePlayerCommands(int teamId, int playerId) {
	for (vector<ApprovedCommand>::iterator it = approvedCommands.begin(); it != approvedCommands.end(); ) {
		if (it->teamId == teamId && it->playerId == playerId)
			it = approvedCommands.erase(it);
		else ++it;
	}
}

bool limits_private::isCommandOkayMaxInMovement(int teamId, char *cmd) {
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

bool limits::isCommandOkay(int teamId, char *cmd) {
	if (!limits_private::isCommandOkayMaxInMovement(teamId, cmd))
		return false;
	return true;
}

namespace limits_private {
	unsigned char players[2] = {0, 0};
	int constructiveTime = 0;
}

// From SBHF:
// Efter det att pucken är möjlig att nå måste den tävlande snarast uträtta något konstruktivt,
// dock senast inom 5 sekunder.
bool limits::checkConstructive() {
	PuckPosition puckPos = getPuckPosition();
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
		if (limits_private::players[0] != 0 && limits_private::players[1] == 0)
			res = getGametime() - limits_private::constructiveTime < 5000;
		else {
			limits_private::constructiveTime = getGametime();
			res = true;
		}
	}
	else if (tmpPlayers[1] != 0 && tmpPlayers[0] == 0) {
		if (limits_private::players[1] != 0 && limits_private::players[0] == 0)
			res = getGametime() - limits_private::constructiveTime < 5000;
		else {
			limits_private::constructiveTime = getGametime();
			res = true;
		}
	}
	else res = true;
	

	limits_private::players[0] = tmpPlayers[0];
	limits_private::players[1] = tmpPlayers[1];

	return res;
}

namespace limits_private {
	int faceOffTime;
	bool isInvalidGoalOnlyCenterHome;
	bool isInvalidGoalOnlyCenterAway;
	Mask *pHomeCenterInvalidGoalMask;
	Mask *pAwayCenterInvalidGoalMask;
	bool isOkayGoalWithinThreeSeconds();
}

// Sets face off time to now and resets data
void limits::faceOff() {
	limits_private::faceOffTime = getGametime();
	limits_private::isInvalidGoalOnlyCenterHome = true;
	limits_private::isInvalidGoalOnlyCenterAway = true;
}

void limits::init() {
	limits_private::isInvalidGoalOnlyCenterHome = true;
	limits_private::isInvalidGoalOnlyCenterAway = true;
	limits_private::pHomeCenterInvalidGoalMask = new Mask("invalidgoalhomecenter.png");
	limits_private::pAwayCenterInvalidGoalMask = new Mask("invalidgoalawaycenter.png");
}

/* 
 * Har pucken hela tiden befunnit sig inom räckhåll för centern, tills den kommer in i målgården,
 * för att sedan gå i mål, är mål ogiltigt även efter tre sekunder. 
 */
void limits::update() {
	if (limits_private::isInvalidGoalOnlyCenterHome || limits_private::isInvalidGoalOnlyCenterAway) {
		PuckPosition puckPos = getPuckPosition();
		if ((puckPos.x != 0) && (puckPos.y != 0)) {
			if (limits_private::isInvalidGoalOnlyCenterHome && 
				!limits_private::pHomeCenterInvalidGoalMask->contains(puckPos.x, puckPos.y)) {
				limits_private::isInvalidGoalOnlyCenterHome = false;
				cout << "puck left home center invalid goal mask" << endl;
			}
			if (limits_private::isInvalidGoalOnlyCenterAway && 
				!limits_private::pAwayCenterInvalidGoalMask->contains(puckPos.x, puckPos.y)) {
				limits_private::isInvalidGoalOnlyCenterAway = false;
				cout << "puck left away center invalid goal mask" << endl;
			}
		}
	}
}

// Mål som görs inom tre sekunder efter matchstart eller tekning är ogiltiga.
bool limits_private::isOkayGoalWithinThreeSeconds() {
	if (getGametime() - faceOffTime < 3000) {
		cout << "goal disqualified because goal was made too soon after face off" << endl;
		return false;
	}
	return true;
}

/* Från SBHF:
 * Mål som görs inom tre sekunder efter matchstart eller tekning är ogiltiga.
 * Har pucken hela tiden befunnit sig inom räckhåll för centern, tills den kommer in i målgården,
 * för att sedan gå i mål, är mål ogiltigt även efter tre sekunder. 
 *  Detta gäller även efter målvakts- eller målbursretur. 
 */
bool limits::isOkayHomeGoal() {
	if (limits_private::isInvalidGoalOnlyCenterHome) {
		cout << "goal disqualified because the puck never left the home team's center" << endl;
		return false;
	}
	return limits_private::isOkayGoalWithinThreeSeconds();
}

bool limits::isOkayAwayGoal() {
	if (limits_private::isInvalidGoalOnlyCenterAway) {
		cout << "goal disqualified because the puck never left the away team's center" << endl;
		return false;
	}
	return limits_private::isOkayGoalWithinThreeSeconds();
}