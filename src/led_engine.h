#ifndef LED_ENGINE_H
#define LED_ENGINE_H

#include <stdint.h>
#include <FastLED.h>
#include "config.h"

#define NUM_CHANNELS 4
#define LEDS_PER_CHANNEL 250

extern CRGB leds[MAX_OUTPUTS][MAX_LEDS_PER_OUTPUT];
extern CRGB canvasLeds[MAX_OUTPUTS][MAX_LEDS_PER_OUTPUT];
extern int claimed_sms[NUM_CHANNELS];

void ledInit();
void setPixel(uint8_t output, uint16_t led, CRGB color);

inline void setPixelCanvas(uint8_t output, uint16_t led, CRGB color) {
    if (output < MAX_OUTPUTS && led < MAX_LEDS_PER_OUTPUT) {
        canvasLeds[output][led] = color;
    }
}

void ledCommitCanvas();
void showAll();
void clearAll();

#endif // LED_ENGINE_H