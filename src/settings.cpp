#include "settings.h"
#include "storage.h"

uint16_t settingsGetLedCount(uint8_t output) {
    return storageGetLedCount(output);
}

void settingsSetLedCount(uint8_t output, uint16_t count) {
    storageSetLedCount(output, count);
}

uint8_t settingsGetBrightness(uint8_t output) {
    return storageGetBrightness(output);
}

void settingsSetBrightness(uint8_t output, uint8_t brightness) {
    storageSetBrightness(output, brightness);
}

uint8_t settingsGetEffect(uint8_t output) {
    return storageGetEffect(output);
}

void settingsSetEffect(uint8_t output, uint8_t effect) {
    storageSetEffect(output, effect);
}

uint8_t settingsGetEffectSpeed(uint8_t output) {
    return storageGetEffectSpeed(output);
}

void settingsSetEffectSpeed(uint8_t output, uint8_t speed) {
    storageSetEffectSpeed(output, speed);
}

void settingsGetEffectColor(uint8_t output, uint8_t* r, uint8_t* g, uint8_t* b) {
    storageGetEffectColor(output, r, g, b);
}

void settingsSetEffectColor(uint8_t output, uint8_t r, uint8_t g, uint8_t b) {
    storageSetEffectColor(output, r, g, b);
}

void settingsGetEffectColor2(uint8_t output, uint8_t* r, uint8_t* g, uint8_t* b) {
    storageGetEffectColor2(output, r, g, b);
}

void settingsSetEffectColor2(uint8_t output, uint8_t r, uint8_t g, uint8_t b) {
    storageSetEffectColor2(output, r, g, b);
}