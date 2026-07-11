#include "effect_engine.h"
#include "led_engine.h"
#include "storage.h"
#include <FastLED.h>

struct LocalEffectState {
    uint8_t heat[MAX_LEDS_PER_OUTPUT]; // Heat buffer for Fire
    int32_t position;                  // Positional trackers
    int32_t direction;                 // Direction/bounce trackers
};

static LocalEffectState states[NUM_CHANNELS];

void setEffect(uint8_t output, EffectType effect) {
    if (output >= NUM_CHANNELS) return;
    storageSetEffect(output, (uint8_t)effect);
}

void clearOutput(uint8_t output) {
    if (output >= NUM_CHANNELS) return;
    memset(leds[output], 0, sizeof(leds[output]));
}

void updateEffects() {
    for (uint8_t ch = 0; ch < NUM_CHANNELS; ch++) {
        uint16_t ledCount = storageGetLedCount(ch);
        uint8_t brightness = storageGetBrightness(ch);
        uint8_t effect = storageGetEffect(ch);
        uint8_t speed = storageGetEffectSpeed(ch);
        
        uint8_t r, g, b;
        storageGetEffectColor(ch, &r, &g, &b);
        CRGB baseColor = CRGB(r, g, b);

        // Apply Global Brightness limit dynamically per channel
        CRGB colorAdjusted = baseColor;
        colorAdjusted.nscale8_video(brightness);

        // Safe 64-bit temporal base to prevent the 4.67-hour 32-bit overflow bug
        uint64_t timeBase = (uint64_t)millis() * speed;

        switch (effect) {
            case EFFECT_STATIC: { // 1. Static
                for (uint16_t i = 0; i < ledCount; i++) {
                    leds[ch][i] = colorAdjusted;
                }
                break;
            }
            case EFFECT_BREATHING: { // 2. Breathing
                // Added a +1 offset to prevent breathing from stalling at speed 0
                uint8_t breathe = beatsin8((speed / 4) + 1, 10, brightness);
                CRGB col = baseColor;
                col.nscale8_video(breathe);
                for (uint16_t i = 0; i < ledCount; i++) {
                    leds[ch][i] = col;
                }
                break;
            }
            case EFFECT_FLASHING: { // 3. Flashing
                bool flashOn = (timeBase / 2000) % 2;
                for (uint16_t i = 0; i < ledCount; i++) {
                    leds[ch][i] = flashOn ? colorAdjusted : CRGB(CRGB::Black);
                }
                break;
            }
            case EFFECT_DOUBLE_FLASHING: { // 4. Double Flashing
                uint32_t t = timeBase / 2000;
                uint32_t phase = t % 6;
                bool flashOn = (phase == 0 || phase == 2);
                for (uint16_t i = 0; i < ledCount; i++) {
                    leds[ch][i] = flashOn ? colorAdjusted : CRGB(CRGB::Black);
                }
                break;
            }
            case EFFECT_COLOR_PULSE: { // 5. Color Pulse
                uint8_t pulse = beatsin8((speed / 4) + 1, 20, brightness);
                uint8_t hue = (timeBase / 10000) % 256;
                CRGB col = CHSV(hue, 255, pulse);
                for (uint16_t i = 0; i < ledCount; i++) {
                    leds[ch][i] = col;
                }
                break;
            }
            case EFFECT_COLOR_SHIFT: { // 6. Color Shift
                uint8_t hue = (timeBase / 4000) % 256;
                CRGB col = CHSV(hue, 255, brightness);
                for (uint16_t i = 0; i < ledCount; i++) {
                    leds[ch][i] = col;
                }
                break;
            }
            case EFFECT_COLOR_WAVE: { // 7. Color Wave
                uint8_t offset = (timeBase / 4000) % 256;
                for (uint16_t i = 0; i < ledCount; i++) {
                    leds[ch][i] = CHSV(offset + (i * 256 / ledCount), 255, brightness);
                }
                break;
            }
            case EFFECT_MARQUEE: { // 8. Marquee
                uint16_t step = (timeBase / 3000) % 6;
                for (uint16_t i = 0; i < ledCount; i++) {
                    leds[ch][i] = ((i % 6) == step) ? colorAdjusted : CRGB(CRGB::Black);
                }
                break;
            }
            case EFFECT_RAINBOW_WAVE: { // 9. Rainbow Wave
                uint8_t initialHue = (timeBase / 4000) % 256;
                fill_rainbow(leds[ch], ledCount, initialHue, 256 / ledCount);
                for (uint16_t i = 0; i < ledCount; i++) {
                    leds[ch][i].nscale8_video(brightness);
                }
                break;
            }
            case EFFECT_VISOR: { // 10. Visor
                uint16_t width = 8;
                uint16_t range = ledCount > width ? ledCount - width : 1;
                uint16_t pos = beatsin16((speed / 8) + 1, 0, range);
                for (uint16_t i = 0; i < ledCount; i++) {
                    if (i >= pos && i < pos + width) {
                        leds[ch][i] = colorAdjusted;
                    } else {
                        leds[ch][i].fadeToBlackBy(40);
                    }
                }
                break;
            }
            case EFFECT_RAINBOW_FLASHING: { // 11. Rainbow Flashing
                bool flashOn = (timeBase / 2000) % 2;
                uint8_t hue = (timeBase / 1000) % 256;
                CRGB col = flashOn ? CRGB(CHSV(hue, 255, brightness)) : CRGB(CRGB::Black);
                for (uint16_t i = 0; i < ledCount; i++) {
                    leds[ch][i] = col;
                }
                break;
            }
            case EFFECT_COLOR_RING_DOUBLE_FLASHING: { // 12. Color Ring Double Flashing
                uint32_t t = timeBase / 2000;
                uint32_t phase = t % 6;
                bool flashOn = (phase == 0 || phase == 2);
                uint8_t hue = (timeBase / 800) % 256;
                for (uint16_t i = 0; i < ledCount; i++) {
                    if (flashOn) {
                        uint16_t center = ledCount / 2;
                        uint16_t dist = abs((int)i - (int)center);
                        uint8_t val = sin8((dist * 8) - (timeBase / 400));
                        CRGB col = CHSV(hue + dist, 255, val);
                        col.nscale8_video(brightness);
                        leds[ch][i] = col;
                    } else {
                        leds[ch][i] = CRGB(CRGB::Black);
                    }
                }
                break;
            }
            case EFFECT_STACK: { // 13. Stack (Slowing divisor increased to 8000 for gradual, smooth step curves)
                uint32_t cycle = (timeBase / 8000) % (ledCount * 10);
                uint16_t activeTarget = ledCount - 1 - (cycle / ledCount) % ledCount;
                uint16_t movingPos = cycle % ledCount;
                for (uint16_t i = 0; i < ledCount; i++) {
                    if (i > activeTarget || i == movingPos) {
                        leds[ch][i] = colorAdjusted;
                    } else {
                        leds[ch][i] = CRGB(CRGB::Black);
                    }
                }
                break;
            }
            case EFFECT_FIRE: { // 14. Fire
                for (uint16_t i = 0; i < ledCount; i++) {
                    uint8_t cooldown = random8(0, ((55 * 10) / ledCount) + 2);
                    if (states[ch].heat[i] > cooldown) {
                        states[ch].heat[i] -= cooldown;
                    } else {
                        states[ch].heat[i] = 0;
                    }
                }
                for (uint16_t k = ledCount - 1; k >= 2; k--) {
                    states[ch].heat[k] = (states[ch].heat[k - 1] + states[ch].heat[k - 2] + states[ch].heat[k - 2]) / 3;
                }
                if (random8() < 120) {
                    uint8_t y = random8(7);
                    if (y < ledCount) {
                        states[ch].heat[y] = qadd8(states[ch].heat[y], random8(160, 255));
                    }
                }
                for (uint16_t j = 0; j < ledCount; j++) {
                    CRGB color = HeatColor(states[ch].heat[j]);
                    color.nscale8_video(brightness);
                    leds[ch][j] = color;
                }
                break;
            }
            case EFFECT_LIGHTNING: { // 15. Lightning
                if (states[ch].position == 0) {
                    states[ch].position = random16(100, 1000 - speed * 3);
                    states[ch].direction = random8(2, 6); // flash count
                } else {
                    states[ch].position--;
                }
                if (states[ch].position < 10 && states[ch].direction > 0) {
                    bool flashOn = (states[ch].position % 2 == 0);
                    CRGB strikeColor = flashOn ? CRGB(200, 220, 255) : CRGB(CRGB::Black);
                    strikeColor.nscale8_video(brightness);
                    for (uint16_t i = 0; i < ledCount; i++) {
                        leds[ch][i] = strikeColor;
                    }
                    if (states[ch].position == 0) {
                        states[ch].direction--;
                        states[ch].position = 4;
                    }
                } else {
                    for (uint16_t i = 0; i < ledCount; i++) {
                        leds[ch][i] = CRGB(CRGB::Black);
                    }
                }
                break;
            }
            case EFFECT_METEOR: { // 16. Meteor
                for (uint16_t i = 0; i < ledCount; i++) {
                    if (random8() < 40) {
                        leds[ch][i].fadeToBlackBy(40);
                    }
                }
                uint16_t meteorPos = (timeBase / 2000) % ledCount;
                for (uint16_t i = 0; i < meteorPos; i++) {
                    if (meteorPos - i < 6) {
                        CRGB col = colorAdjusted;
                        col.nscale8_video(255 - (meteorPos - i) * 40);
                        leds[ch][i] = col;
                    }
                }
                break;
            }
            case EFFECT_COLOR_RING: { // 17. Color Ring (Calibrated Speed Curve)
                uint16_t center = ledCount / 2;
                uint16_t maxDist = ledCount / 2;
                uint16_t radius = (timeBase / 2000) % (maxDist + 5);
                uint8_t hue = (timeBase / 4000) % 256;
                for (uint16_t i = 0; i < ledCount; i++) {
                    uint16_t dist = abs((int)i - (int)center);
                    if (dist == radius) {
                        leds[ch][i] = CHSV(hue, 255, brightness);
                    } else if (dist < radius) {
                        leds[ch][i].fadeToBlackBy(30);
                    } else {
                        leds[ch][i] = CRGB(CRGB::Black);
                    }
                }
                break;
            }
            case EFFECT_PLANETARY: { // 18. Planetary (Calibrated Speed Curve)
                uint16_t orbit1 = (timeBase / 2000) % ledCount;
                uint16_t orbit2 = ledCount - 1 - ((timeBase / 3000) % ledCount);
                for (uint16_t i = 0; i < ledCount; i++) {
                    leds[ch][i] = CRGB(CRGB::Black);
                }
                if (orbit1 < ledCount) leds[ch][orbit1] = colorAdjusted;
                if (orbit2 < ledCount) {
                    CRGB col2 = CHSV((timeBase / 5000) % 256, 255, brightness);
                    leds[ch][orbit2] = col2;
                }
                break;
            }
            case EFFECT_DOUBLE_METEOR: { // 19. Double Meteor
                for (uint16_t i = 0; i < ledCount; i++) {
                    leds[ch][i].fadeToBlackBy(30);
                }
                uint16_t pos1 = (timeBase / 2000) % ledCount;
                uint16_t pos2 = ledCount - 1 - ((timeBase / 2000) % ledCount);
                if (pos1 < ledCount) leds[ch][pos1] = colorAdjusted;
                if (pos2 < ledCount) leds[ch][pos2] = colorAdjusted;
                break;
            }
            case EFFECT_ENERGY: { // 20. Energy (Calibrated Speed Curve)
                uint32_t xoffset = timeBase / 250;
                uint8_t hue = (timeBase / 5000) % 256;
                for (uint16_t i = 0; i < ledCount; i++) {
                    uint8_t noise = inoise8(i * 40, xoffset);
                    CRGB col = CHSV(hue + (noise / 4), 255, noise);
                    col.nscale8_video(brightness);
                    leds[ch][i] = col;
                }
                break;
            }
            case EFFECT_BLINK: { // 21. Blink
                bool oddOn = (timeBase / 2000) % 2;
                for (uint16_t i = 0; i < ledCount; i++) {
                    bool odd = (i % 2 != 0);
                    leds[ch][i] = (odd == oddOn) ? colorAdjusted : CRGB(CRGB::Black);
                }
                break;
            }
            case EFFECT_CLOCK: { // 22. Clock (Dynamically Scaled virtual timer)
                // Maps speed to a virtual clock multiplier (Speed 128 = real-time, 255 = 2x speed, 0 = freeze)
                uint64_t virtualMillis = (uint64_t)millis() * speed / 128;
                uint32_t secondHand = (virtualMillis / 1000) % ledCount;
                uint32_t minuteHand = (virtualMillis / 20000) % ledCount;
                for (uint16_t i = 0; i < ledCount; i++) {
                    if (i == secondHand) {
                        CRGB colSec = CRGB::Red;
                        colSec.nscale8_video(brightness);
                        leds[ch][i] = colSec;
                    } else if (i == minuteHand) {
                        leds[ch][i] = colorAdjusted;
                    } else {
                        leds[ch][i] = CRGB(CRGB::Black);
                    }
                }
                break;
            }
            case EFFECT_OFF:
            default: {
                for (uint16_t i = 0; i < ledCount; i++) {
                    leds[ch][i] = CRGB(CRGB::Black);
                }
                break;
            }
        }

        // FIXED: Clear inactive trailing pixels beyond the configured hardware count to prevent garbage/leftovers
        for (uint16_t i = ledCount; i < MAX_LEDS_PER_OUTPUT; i++) {
            leds[ch][i] = CRGB(CRGB::Black);
        }
    }
}