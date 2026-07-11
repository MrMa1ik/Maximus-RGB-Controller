#ifndef CONFIG_H
#define CONFIG_H

#define MAXIMUS_FIRMWARE_VERSION 3

// 4 channels @ 250 LEDs per channel (1000 total)
#define MAX_OUTPUTS            4
#define MAX_LEDS_PER_OUTPUT    250

#define STORAGE_MAGIC          0x4D41584DUL
#define STORAGE_VERSION        4

#define AUTO_SAVE_DELAY_MS     5000
#define SIGNALRGB_TIMEOUT_MS   500

#define DEFAULT_BRIGHTNESS     120
#define DEFAULT_SPEED          128

#endif // CONFIG_H