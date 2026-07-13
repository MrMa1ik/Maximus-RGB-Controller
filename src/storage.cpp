#include "storage.h"
#include <Arduino.h>
#include <EEPROM.h>

static StorageLayout activeConfig;
static bool needsSave = false;
static uint32_t lastChangeTime = 0;

void storageInit() {
    EEPROM.begin(sizeof(StorageLayout));
    EEPROM.get(0, activeConfig);

    if (activeConfig.magic != STORAGE_MAGIC || activeConfig.version != STORAGE_VERSION) {
        storageFactoryDefaults();
        storageSave();
    }
}

void storageSave() {
    activeConfig.magic = STORAGE_MAGIC;
    activeConfig.version = STORAGE_VERSION;
    EEPROM.put(0, activeConfig);
    EEPROM.commit();
    needsSave = false;
}

void storageFactoryDefaults() {
    activeConfig.magic = STORAGE_MAGIC;
    activeConfig.version = STORAGE_VERSION;
    for (uint8_t i = 0; i < MAX_OUTPUTS; i++) {
        activeConfig.channels[i].ledCount = MAX_LEDS_PER_OUTPUT;
        activeConfig.channels[i].brightness = DEFAULT_BRIGHTNESS;
        activeConfig.channels[i].effect = 9; // Default to "Rainbow wave"
        activeConfig.channels[i].speed = DEFAULT_SPEED;
        
        // Default Primary Color: Cyan
        activeConfig.channels[i].r = 0;
        activeConfig.channels[i].g = 155;
        activeConfig.channels[i].b = 222;

        // Default Secondary Color: Magenta
        activeConfig.channels[i].r2 = 255;
        activeConfig.channels[i].g2 = 0;
        activeConfig.channels[i].b2 = 255;
    }
}

uint16_t storageGetLedCount(uint8_t output) {
    if (output >= MAX_OUTPUTS) return MAX_LEDS_PER_OUTPUT;
    return activeConfig.channels[output].ledCount;
}

void storageSetLedCount(uint8_t output, uint16_t count) {
    if (output >= MAX_OUTPUTS) return;
    if (activeConfig.channels[output].ledCount != count) {
        activeConfig.channels[output].ledCount = count;
        storageTriggerAutoSave();
    }
}

uint8_t storageGetBrightness(uint8_t output) {
    if (output >= MAX_OUTPUTS) return DEFAULT_BRIGHTNESS;
    return activeConfig.channels[output].brightness;
}

void storageSetBrightness(uint8_t output, uint8_t brightness) {
    if (output >= MAX_OUTPUTS) return;
    if (activeConfig.channels[output].brightness != brightness) {
        activeConfig.channels[output].brightness = brightness;
        storageTriggerAutoSave();
    }
}

uint8_t storageGetEffect(uint8_t output) {
    if (output >= MAX_OUTPUTS) return 9;
    return activeConfig.channels[output].effect;
}

void storageSetEffect(uint8_t output, uint8_t effect) {
    if (output >= MAX_OUTPUTS) return;
    if (activeConfig.channels[output].effect != effect) {
        activeConfig.channels[output].effect = effect;
        storageTriggerAutoSave();
    }
}

uint8_t storageGetEffectSpeed(uint8_t output) {
    if (output >= MAX_OUTPUTS) return DEFAULT_SPEED;
    return activeConfig.channels[output].speed;
}

void storageSetEffectSpeed(uint8_t output, uint8_t speed) {
    if (output >= MAX_OUTPUTS) return;
    if (activeConfig.channels[output].speed != speed) {
        activeConfig.channels[output].speed = speed;
        storageTriggerAutoSave();
    }
}

void storageGetEffectColor(uint8_t output, uint8_t* r, uint8_t* g, uint8_t* b) {
    if (output >= MAX_OUTPUTS) {
        *r = 0; *g = 155; *b = 222;
        return;
    }
    *r = activeConfig.channels[output].r;
    *g = activeConfig.channels[output].g;
    *b = activeConfig.channels[output].b;
}

void storageSetEffectColor(uint8_t output, uint8_t r, uint8_t g, uint8_t b) {
    if (output >= MAX_OUTPUTS) return;
    if (activeConfig.channels[output].r != r || 
        activeConfig.channels[output].g != g || 
        activeConfig.channels[output].b != b) {
        activeConfig.channels[output].r = r;
        activeConfig.channels[output].g = g;
        activeConfig.channels[output].b = b;
        storageTriggerAutoSave();
    }
}

void storageGetEffectColor2(uint8_t output, uint8_t* r, uint8_t* g, uint8_t* b) {
    if (output >= MAX_OUTPUTS) {
        *r = 255; *g = 0; *b = 255;
        return;
    }
    *r = activeConfig.channels[output].r2;
    *g = activeConfig.channels[output].g2;
    *b = activeConfig.channels[output].b2;
}

void storageSetEffectColor2(uint8_t output, uint8_t r, uint8_t g, uint8_t b) {
    if (output >= MAX_OUTPUTS) return;
    if (activeConfig.channels[output].r2 != r || 
        activeConfig.channels[output].g2 != g || 
        activeConfig.channels[output].b2 != b) {
        activeConfig.channels[output].r2 = r;
        activeConfig.channels[output].g2 = g;
        activeConfig.channels[output].b2 = b;
        storageTriggerAutoSave();
    }
}

void storageTriggerAutoSave() {
    needsSave = true;
    lastChangeTime = millis();
}

void storageUpdateAutoSave() {
    if (needsSave && (millis() - lastChangeTime >= AUTO_SAVE_DELAY_MS)) {
        storageSave();
    }
}