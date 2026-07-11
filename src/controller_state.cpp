#include "controller_state.h"
#include <Arduino.h>
#include "config.h"
#include "storage.h"

volatile ControllerMode currentMode = CONTROLLER_STARTUP;
static volatile uint32_t lastActivityTime = 0;

void controllerStateInit() {
    currentMode = CONTROLLER_STARTUP;
    lastActivityTime = millis();
}

void controllerStateUpdate() {
    storageUpdateAutoSave();

    // Fallback if SignalRGB host stops sending updates
    if (currentMode == CONTROLLER_SIGNALRGB) {
        if (millis() - lastActivityTime > SIGNALRGB_TIMEOUT_MS) {
            currentMode = CONTROLLER_STARTUP;
        }
    }
}

void controllerSignalActivity() {
    currentMode = CONTROLLER_SIGNALRGB;
    lastActivityTime = millis();
}