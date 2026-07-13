#ifndef EFFECT_ENGINE_H
#define EFFECT_ENGINE_H

#include <stdint.h>

enum EffectType {
    EFFECT_OFF = 0,
    EFFECT_STATIC = 1,
    EFFECT_BREATHING = 2,
    EFFECT_FLASHING = 3,
    EFFECT_DOUBLE_FLASHING = 4,
    EFFECT_COLOR_PULSE = 5,
    EFFECT_COLOR_SHIFT = 6,
    EFFECT_COLOR_WAVE = 7,
    EFFECT_MARQUEE = 8,
    EFFECT_RAINBOW_WAVE = 9,
    EFFECT_VISOR = 10,
    EFFECT_RAINBOW_FLASHING = 11,
    EFFECT_COLOR_RING_DOUBLE_FLASHING = 12,
    EFFECT_STACK = 13,
    EFFECT_FIRE = 14,
    EFFECT_LIGHTNING = 15,
    EFFECT_METEOR = 16,
    EFFECT_COLOR_RING = 17,
    EFFECT_PLANETARY = 18,
    EFFECT_DOUBLE_METEOR = 19,
    EFFECT_ENERGY = 20,
    EFFECT_BLINK = 21,
    EFFECT_CLOCK = 22,
    EFFECT_SEQUENTIAL_GLOW = 23, // Register sequential wipe
    EFFECT_COUNT = 24,
    EFFECT_RAINBOW = 9
};

void updateEffects();
void setEffect(uint8_t output, EffectType effect);
void clearOutput(uint8_t output);

#endif // EFFECT_ENGINE_H