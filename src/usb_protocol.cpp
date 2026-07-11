#include "usb_protocol.h"
#include <Arduino.h>
#include <cstring>
#include <cstdlib>
#include <cstdio>

#include "settings.h"
#include "storage.h"
#include "config.h"
#include "effect_engine.h"
#include "led_engine.h"
#include <FastLED.h>
#include "controller_state.h"
#include "render_scheduler.h"

Adafruit_USBD_WebUSB usb_web;

static char commandBuffer[64];
static uint8_t commandLength = 0;
static ResponseTransport currentTransport = RESPONSE_SERIAL;

void processCommand(const char* command);

static bool parseOutputValue(const char* args, int* output, int* value) {
    char* end;
    *output = strtol(args, &end, 10);
    if (*end != ' ') return false;
    *value = strtol(end + 1, nullptr, 10);
    return (*output >= 0 && *output < MAX_OUTPUTS);
}

static bool parseOutputColor(const char* args, int* output, int* red, int* green, int* blue) {
    char* end;
    *output = strtol(args, &end, 10);
    if (*end != ' ') return false;
    *red = strtol(end + 1, &end, 10);
    if (*end != ' ') return false;
    *green = strtol(end + 1, &end, 10);
    if (*end != ' ') return false;
    *blue = strtol(end + 1, nullptr, 10);
    return (*output >= 0 && *output < MAX_OUTPUTS);
}

static void sendOutputConfig(uint8_t output) {
    char response[120];
    uint8_t red, green, blue;

    settingsGetEffectColor(output, &red, &green, &blue);

    snprintf(response, sizeof(response),
             "OUT %u COUNT %u BRIGHTNESS %u EFFECT %u SPEED %u COLOR %u %u %u",
             output,
             settingsGetLedCount(output),
             settingsGetBrightness(output),
             settingsGetEffect(output),
             settingsGetEffectSpeed(output),
             red, green, blue);

    sendResponse(response);
}

void setResponseTransport(ResponseTransport transport) {
    currentTransport = transport;
}

void usbProtocolInit() {
    usb_web.begin();
}

void sendResponse(const char* text) {
    usb_web.println(text);
}

void processCommand(const char* command) {
    if (strncmp(command, "BRIGHTNESS ", 11) == 0) {
        int output, value;
        if (!parseOutputValue(command + 11, &output, &value)) return;
        value = (value < 0) ? 0 : (value > 255 ? 255 : value);
        settingsSetBrightness(output, (uint8_t)value);
        currentMode = CONTROLLER_STARTUP;
    }
    else if (strncmp(command, "COUNT ", 6) == 0) {
        int output, value;
        if (!parseOutputValue(command + 6, &output, &value)) return;
        value = (value < 1) ? 1 : (value > MAX_LEDS_PER_OUTPUT ? MAX_LEDS_PER_OUTPUT : value);
        settingsSetLedCount(output, (uint16_t)value);
        clearOutput(output);
        showAll();
        currentMode = CONTROLLER_STARTUP;
    }
    else if (strncmp(command, "EFFECT ", 7) == 0) {
        int output, effect;
        if (!parseOutputValue(command + 7, &output, &effect)) return;
        if (effect < EFFECT_OFF) effect = EFFECT_OFF;
        if (effect >= EFFECT_COUNT) effect = EFFECT_RAINBOW;

        setEffect(output, (EffectType)effect);
        currentMode = CONTROLLER_STARTUP;
    }
    else if (strncmp(command, "SPEED ", 6) == 0) {
        int output, value;
        if (!parseOutputValue(command + 6, &output, &value)) return;
        value = (value < 0) ? 0 : (value > 255 ? 255 : value);
        settingsSetEffectSpeed(output, (uint8_t)value);
        currentMode = CONTROLLER_STARTUP;
    }
    else if (strncmp(command, "COLOR ", 6) == 0) {
        int output, red, green, blue;
        if (!parseOutputColor(command + 6, &output, &red, &green, &blue)) return;
        red   = (red   < 0) ? 0 : (red   > 255 ? 255 : red);
        green = (green < 0) ? 0 : (green > 255 ? 255 : green);
        blue  = (blue  < 0) ? 0 : (blue  > 255 ? 255 : blue);

        settingsSetEffectColor(output, (uint8_t)red, (uint8_t)green, (uint8_t)blue);
        currentMode = CONTROLLER_STARTUP;
    }
    else if (strcmp(command, "SAVE") == 0) {
        storageSave();
    }
    else if (strcmp(command, "DEFAULTS") == 0) {
        storageFactoryDefaults();
        storageSave();
        clearAll();
        showAll();
        currentMode = CONTROLLER_STARTUP;
    }
}

void processBinaryPacket(const uint8_t* data, uint16_t length) {
    if (length == 0) return;

    if (length > 0 && data[0] == 0x00) {
        data++;
        length--;
    }

    if (length < 9) return;

    uint8_t command_id = data[0];

    if (command_id == CMD_SIGNALRGB_SHOW) {
        uint16_t ch_led_counts[MAX_OUTPUTS];
        for (uint8_t ch = 0; ch < MAX_OUTPUTS; ch++) {
            uint16_t offset = 1 + (ch * 2);
            ch_led_counts[ch] = (uint16_t)data[offset] | ((uint16_t)data[offset + 1] << 8);
        }

        const uint8_t* rgb_data = data + 9;
        uint16_t bytes_offset = 0;

        for (uint8_t ch = 0; ch < MAX_OUTPUTS; ch++) {
            uint16_t count = ch_led_counts[ch];
            if (count == 0) continue;

            for (uint16_t i = 0; i < count; i++) {
                if (i >= MAX_LEDS_PER_OUTPUT) break; 

                setPixelCanvas(ch, i, CRGB(rgb_data[bytes_offset + i * 3], 
                                           rgb_data[bytes_offset + i * 3 + 1], 
                                           rgb_data[bytes_offset + i * 3 + 2]));
            }
            bytes_offset += count * 3;
        }

        renderRequest();
    }
}