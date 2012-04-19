#ifndef __LIMITS_H__
#define __LIMITS_H__

namespace limits {
	bool isCommandOkay(int teamId, char *cmd);
	bool checkConstructive();
	bool isOkayHomeGoal();
	bool isOkayAwayGoal();
	void update();
	void init();
	void faceOff();	// TODO: When to call faceOff? Manual? Automatic (how?)?
}

#endif //__LIMITS_H__