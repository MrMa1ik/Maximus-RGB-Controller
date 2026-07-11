#include <Arduino.h>
#include "led_engine.h"
#include "usb_protocol.h"
#include "controller_state.h"
#include "render_scheduler.h"
#include "storage.h"
#include <Adafruit_TinyUSB.h>

// Core 0 (Main Core): Handles Startup Animations, Local Effects, and Parallel PIO rendering
void setup() {
    Serial.end(); // Completely disable and remove the CDC ACM COM port!
    
    TinyUSBDevice.setManufacturerDescriptor("Maximus");
    TinyUSBDevice.setProductDescriptor("Maximus RGB Controller");
    
    storageInit();
    ledInit();
    controllerStateInit();
    renderSchedulerInit();
}

void loop() {
    controllerStateUpdate();
    renderSchedulerUpdate();
}

// Core 1 (Second Core): Dedicated to WebUSB Bulk Ingestion
void setup1() {
    usbProtocolInit();
}

void loop1() {
    static uint8_t packetBuffer[3009];
    static uint16_t packetIndex = 0;
    
    static char textBuffer[64];
    static uint8_t textIndex = 0;
    
    int availableBytes = usb_web.available();
    if (availableBytes > 0) {
        uint8_t tempBuffer[256];
        int bytesToRead = (availableBytes < 256) ? availableBytes : 256;
        int readBytes = usb_web.read(tempBuffer, bytesToRead);
        
        for (int i = 0; i < readBytes; i++) {
            uint8_t c = tempBuffer[i];
            
            // 1. If currently accumulating a text command
            if (textIndex > 0) {
                if (c == '\n' || c == '\r') {
                    textBuffer[textIndex] = '\0';
                    processCommand(textBuffer);
                    textIndex = 0;
                } else if (textIndex < sizeof(textBuffer) - 1) {
                    textBuffer[textIndex++] = (char)c;
                }
                continue;
            }
            
            // 2. If currently accumulating a binary canvas packet
            if (packetIndex > 0) {
                packetBuffer[packetIndex++] = c;
                if (packetIndex >= 3009) {
                    processBinaryPacket(packetBuffer, 3009);
                    packetIndex = 0;
                }
                continue;
            }
            
            // 3. We are at index 0. Determine packet type by the first byte:
            if (c == 0x03) {
                // Start of a binary canvas packet
                packetBuffer[0] = c;
                packetIndex = 1;
            } 
            else if (c == 'B' || c == 'S' || c == 'C' || c == 'E' || c == 'D' || c == 'P' || c == 'I') {
                // Start of an ASCII text command (BRIGHTNESS, SPEED, COLOR, EFFECT, DEFAULTS, PING, INFO)
                textBuffer[0] = (char)c;
                textIndex = 1;
            }
        }
    }
}