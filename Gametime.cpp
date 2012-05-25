#include "Gametime.h"

#include <time.h>

#define TIME clock()

namespace gametimens{
	int startTime;
	int totalPausetime = 0;
	int pauseStartTime = TIME;
}

using namespace gametimens;

void startGametime() {
	startTime = TIME;
	totalPausetime = 0;
	pauseStartTime = 0;
}

void pauseGametime() {
	if (pauseStartTime == 0) 
		pauseStartTime = TIME;
}

void resumeGametime() {
	if (pauseStartTime != 0) {
		totalPausetime += TIME-pauseStartTime;
		pauseStartTime=0;
	}
}

int getGametime() {
	if (pauseStartTime != 0)
		return pauseStartTime - startTime - totalPausetime;
	return TIME - startTime - totalPausetime;
}