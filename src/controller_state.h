#ifndef CONTROLLER_STATE_H
#define CONTROLLER_STATE_H

#include <stdint.h>

enum ControllerMode {
    CONTROLLER_STARTUP,
    CONTROLLER_SIGNALRGB
};

extern volatile ControllerMode currentMode;

void controllerStateInit();
void controllerStateUpdate();
void controllerSignalActivity();

#endif // CONTROLLER_STATE_H