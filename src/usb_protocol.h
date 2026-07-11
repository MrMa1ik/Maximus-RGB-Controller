#ifndef USB_PROTOCOL_H
#define USB_PROTOCOL_H

#include <stdint.h>
#include <Adafruit_TinyUSB.h>

extern Adafruit_USBD_WebUSB usb_web;

void usbProtocolInit();

enum ResponseTransport {
    RESPONSE_SERIAL,
    RESPONSE_HID
};

void setResponseTransport(ResponseTransport transport);
void processCommand(const char* command);
void sendResponse(const char* text);

enum BinaryCommand {
    CMD_SET_PIXELS        = 0x01,
    CMD_SIGNALRGB_CANVAS  = 0x02,
    CMD_SIGNALRGB_SHOW    = 0x03,
};

void processBinaryPacket(const uint8_t* data, uint16_t length);

#endif // USB_PROTOCOL_H