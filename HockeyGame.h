#ifndef __HOCKEYGAME_H__
#define __HOCKEYGAME_H__

namespace hockeygame {
	bool initialize();
	bool startGame();
	void stopGame();
	void pauseGame();
	void resumeGame();
	void calibrateCamera();
	void calibrateMicroControllers();
	void faceOff();
}

#endif //__HOCKEYGAME_H__