#include "render_scheduler.h"
#include <Arduino.h>
#include "controller_state.h"
#include "led_engine.h"
#include "effect_engine.h"

static volatile bool renderNeeded = false;
static uint32_t lastLocalUpdate = 0;

void renderSchedulerInit() {
    lastLocalUpdate = millis();
    renderNeeded = false;
}

void renderSchedulerUpdate() {
    if (currentMode == CONTROLLER_SIGNALRGB) {
        if (renderNeeded) {
            ledCommitCanvas();
            renderNeeded = false;
        }
    } else {
        // Run offline/startup hardware effect calculations on Core 0 at 60 FPS
        if (millis() - lastLocalUpdate >= 16) {
            lastLocalUpdate = millis();
            updateEffects();
            showAll();
        }
    }
}

void renderRequest() {
    renderNeeded = true;
    controllerSignalActivity();
}