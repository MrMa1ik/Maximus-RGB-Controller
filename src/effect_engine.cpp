#include "effect_engine.h"
#include "led_engine.h"
#include "storage.h"
#include "controller_state.h" // Added for global currentMode tracking
#include <FastLED.h>

struct LocalEffectState {
    uint8_t heat[MAX_LEDS_PER_OUTPUT]; // Heat buffer for Fire
    int32_t position;                  // Positional trackers
    int32_t direction;                 // Direction/bounce trackers
};

static LocalEffectState states[NUM_CHANNELS];

// File-scope global tracking variables for the Sequential Glow effect
static uint8_t globalWipeState = 0; // 0 = Wipe, 1 = Hold, 2 = Breathing
static uint32_t globalEffectStartTime = 0;
static uint32_t globalWipeCompleteTime = 0;
static uint32_t globalBreatheStartTime = 0;
static uint8_t lastGlobalEffect = 255;
static ControllerMode lastMode = CONTROLLER_SIGNALRGB;

void setEffect(uint8_t output, EffectType effect) {
    if (output >= NUM_CHANNELS) return;
    storageSetEffect(output, (uint8_t)effect);
    
    // Reset the global wipe state immediately when any effect command is applied
    globalWipeState = 0;
    globalEffectStartTime = millis();
}

void clearOutput(uint8_t output) {
    if (output >= NUM_CHANNELS) return;
    memset(leds[output], 0, sizeof(leds[output]));
}

void updateEffects() {
    // 1. Calculate boundaries and dynamic cumulative total for sequential effects
    uint16_t bounds[MAX_OUTPUTS];
    uint32_t totalLeds = 0;
    for (uint8_t i = 0; i < MAX_OUTPUTS; i++) {
        totalLeds += storageGetLedCount(i);
        bounds[i] = totalLeds;
    }

    // Reset global wipe states if we transition from real-time Canvas to Startup/Offline mode
    if (lastMode != currentMode) {
        if (currentMode == CONTROLLER_STARTUP) {
            globalWipeState = 0;
            globalEffectStartTime = millis();
        }
        lastMode = (ControllerMode)currentMode;
    }

    // Reset global wipe states if the reference effect is changed
    uint8_t currentEffect = storageGetEffect(0);
    if (lastGlobalEffect != currentEffect) {
        globalWipeState = 0;
        globalEffectStartTime = millis();
        lastGlobalEffect = currentEffect;
    }

    // FIXED: Declare variables at top-level function scope to guarantee visibility across all nested blocks
    uint16_t ledCount = 0;
    uint8_t brightness = 0;
    uint8_t effect = 0;
    uint8_t speed = 0;
    uint8_t r = 0, g = 0, b = 0;
    CRGB baseColor = CRGB::Black;
    CRGB colorAdjusted = CRGB::Black;
    CRGB color2Adjusted = CRGB::Black;
    uint64_t timeBase = 0;

    for (uint8_t ch = 0; ch < NUM_CHANNELS; ch++) {
        ledCount = storageGetLedCount(ch);
        brightness = storageGetBrightness(ch);
        effect = storageGetEffect(ch);
        speed = storageGetEffectSpeed(ch);
        
        storageGetEffectColor(ch, &r, &g, &b);
        baseColor = CRGB(r, g, b);

        // Apply Global Brightness limit dynamically per channel
        colorAdjusted = baseColor;
        colorAdjusted.nscale8_video(brightness);

        // Fetch custom secondary colors from active config registers
        uint8_t r2, g2, b2;
        storageGetEffectColor2(ch, &r2, &g2, &b2);
        CRGB baseColor2 = CRGB(r2, g2, b2);
        color2Adjusted = baseColor2;
        color2Adjusted.nscale8_video(brightness);

        // Safe 64-bit temporal base to prevent the 4.67-hour 32-bit overflow bug
        timeBase = (uint64_t)millis() * speed;

        switch (effect) {
            case EFFECT_STATIC: { // 1. Static
                for (uint16_t i = 0; i < ledCount; i++) {
                    leds[ch][i] = colorAdjusted;
                }
                break;
            }
            case EFFECT_BREATHING: { // 2. Breathing (Pulses smoothly between Color 1 and generated Color 2)
                uint8_t blendRatio = beatsin8((speed / 4) + 1, 0, 255);
                CRGB col = blend(colorAdjusted, color2Adjusted, blendRatio);
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
            case EFFECT_STACK: { // 13. Stack (Slowing divisor increased to 32000 for gradual, smooth step curves)
                uint32_t cycle = (timeBase / 32000) % (ledCount * 10);
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
            case EFFECT_METEOR: { // 16. Meteor (Cascades sequentially across GP2 -> GP3 -> GP4 -> GP6 over custom secondary background)
                if (totalLeds == 0) break;

                // Fill active channel length with your custom Hardware Color 2 (Secondary Color) as the background
                for (uint16_t i = 0; i < ledCount; i++) {
                    leds[ch][i] = color2Adjusted;
                }

                uint32_t meteorPos = (timeBase / 2000) % totalLeds;

                // Draw meteor head and decaying tail across the global sequence
                for (uint16_t idx = 0; idx < 6; idx++) {
                    int32_t g = (int32_t)meteorPos - idx;
                    while (g < 0) g += totalLeds;

                    // Check if absolute global index 'g' falls inside this channel 'ch'
                    bool onThisChannel = false;
                    uint16_t localIdx = 0;
                    if (ch == 0) {
                        if (g < bounds[0]) {
                            onThisChannel = true;
                            localIdx = g;
                        }
                    } else {
                        if (g >= bounds[ch - 1] && g < bounds[ch]) {
                            onThisChannel = true;
                            localIdx = g - bounds[ch - 1];
                        }
                    }

                    if (onThisChannel) {
                        CRGB col = colorAdjusted;
                        uint8_t fade = 255 - idx * 40;
                        col.nscale8_video(fade); // Fades trailing pixels
                        
                        // Blend the primary meteor colors over your secondary background
                        leds[ch][localIdx] = blend(color2Adjusted, col, fade);
                    }
                }
                break;
            }
            case EFFECT_COLOR_RING: { // 17. Color Ring
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
            case EFFECT_PLANETARY: { // 18. Planetary
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
            case EFFECT_ENERGY: { // 20. Energy
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
            case EFFECT_CLOCK: { // 22. Clock
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
            case EFFECT_SEQUENTIAL_GLOW: { // 23. Sequential Glow (Satisfying progressive loading/wipe effect, then transitions to infinite breathing)
                if (totalLeds == 0) break;

                uint32_t elapsed = millis() - globalEffectStartTime;
                uint64_t effectTimeBase = (uint64_t)elapsed * speed;
                
                // FIXED: Divisor increased from 4000 to 16000, halving the wipe speed
                uint32_t globalPos = effectTimeBase / 4000;

                // State boundary check: Transition to infinite breathing as soon as the wipe completes
                if (globalPos >= totalLeds && globalWipeState == 0) {
                    globalWipeState = 1;
                    globalWipeCompleteTime = millis();
                }

                if (globalWipeState == 0) {
                    // Phase 1: Progressive Wipe/Chase (Runs exactly once on boot/switch, now at half-speed)
                    for (uint16_t i = 0; i < ledCount; i++) {
                        // Map local LED index 'i' on channel 'ch' to absolute global LED index 'g'
                        uint32_t g = (ch == 0) ? i : (bounds[ch - 1] + i);

                        if (g <= globalPos) {
                            leds[ch][i] = colorAdjusted;
                        } else {
                            leds[ch][i] = CRGB(CRGB::Black);
                        }
                    }
                } else {
                    // Phase 2: Indefinite Breathing (Starts seamlessly at peak brightness, breathing speed is UNCHANGED, blends primary and complementary color)
                    uint32_t breatheElapsed = 0;
                    if (millis() > globalWipeCompleteTime) {
                        breatheElapsed = millis() - globalWipeCompleteTime;
                    }
                    uint64_t breatheTimeBase = (uint64_t)breatheElapsed * speed;
                    
                    // Aligns the breathing speed to match the default breathing speed (divisor remains 2000), starting at peak (64)
                    uint32_t phase = (breatheTimeBase / 2000) + 64;
                    uint8_t breatheVal = quadwave8(phase);

                    CRGB breathingColor = blend(colorAdjusted, color2Adjusted, breatheVal);

                    for (uint16_t i = 0; i < ledCount; i++) {
                        leds[ch][i] = breathingColor;
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