#ifndef SETTINGS_H
#define SETTINGS_H

#include <stdint.h>

uint16_t settingsGetLedCount(uint8_t output);
void settingsSetLedCount(uint8_t output, uint16_t count);

uint8_t settingsGetBrightness(uint8_t output);
void settingsSetBrightness(uint8_t output, uint8_t brightness);

uint8_t settingsGetEffect(uint8_t output);
void settingsSetEffect(uint8_t output, uint8_t effect);

uint8_t settingsGetEffectSpeed(uint8_t output);
void settingsSetEffectSpeed(uint8_t output, uint8_t speed);

void settingsGetEffectColor(uint8_t output, uint8_t* r, uint8_t* g, uint8_t* b);
void settingsSetEffectColor(uint8_t output, uint8_t r, uint8_t g, uint8_t b);

void settingsGetEffectColor2(uint8_t output, uint8_t* r, uint8_t* g, uint8_t* b);
void settingsSetEffectColor2(uint8_t output, uint8_t r, uint8_t g, uint8_t b);

#endif // SETTINGS_H