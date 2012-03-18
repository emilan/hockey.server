#ifndef __MICROCONTROLLERS_H__
#define __MICROCONTROLLERS_H__

bool initializeMicroControllers(void(*)(unsigned char*, unsigned char*));
void calibrateMicroControllers();

void sendCommandsToHomeMicroController(char *msg, int length);
void sendCommandsToAwayMicroController(char *msg, int length);

#endif //__MICROCONTROLLERS_H__