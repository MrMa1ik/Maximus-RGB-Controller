#ifndef STORAGE_H
#define STORAGE_H

#include <stdint.h>
#include "config.h"

struct ChannelConfig {
    uint16_t ledCount;
    uint8_t brightness;
    uint8_t effect;
    uint8_t speed;
    uint8_t r;
    uint8_t g;
    uint8_t b;
};

struct StorageLayout {
    uint32_t magic;
    uint32_t version;
    ChannelConfig channels[MAX_OUTPUTS];
};

void storageInit();
void storageSave();
void storageFactoryDefaults();

// Accessor methods
uint16_t storageGetLedCount(uint8_t output);
void storageSetLedCount(uint8_t output, uint16_t count);

uint8_t storageGetBrightness(uint8_t output);
void storageSetBrightness(uint8_t output, uint8_t brightness);

uint8_t storageGetEffect(uint8_t output);
void storageSetEffect(uint8_t output, uint8_t effect);

uint8_t storageGetEffectSpeed(uint8_t output);
void storageSetEffectSpeed(uint8_t output, uint8_t speed);

void storageGetEffectColor(uint8_t output, uint8_t* r, uint8_t* g, uint8_t* b);
void storageSetEffectColor(uint8_t output, uint8_t r, uint8_t g, uint8_t b);

void storageTriggerAutoSave();
void storageUpdateAutoSave();

#endif // STORAGE_H