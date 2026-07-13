<img width="905" height="163" alt="Maximus Logo" src="https://github.com/user-attachments/assets/0f59bed0-9b0b-4fc5-ad61-5086112c709e" />


# Maximus-RGB-Controller

The Maximus RGB Controller is a high-performance, DIY WS2812B addressable RGB lighting controller powered by the Raspberry Pi Pico (RP2040). It features custom multi-core firmware and native, zero-latency integration with SignalRGB using unlocked Raw USB (WinUSB) bulk endpoints.


## Features

* **4 Independent Channels:** Drives up to 250 LEDs per channel (1,000 LEDs total limit) in parallel.
* **Stable 40 FPS Canvas Streaming:** Uses raw WinUSB bulk transfers (bypassing slow HID polling and serial COM limitations) to transmit frames to the Pico. It gives constant 42-43 FPS with 1-1000 LEDs
* **22 Hardware-Level Offline Effects:** Runs local, non-blocking procedural effects (such as Fire, Rainbow, and Wave) computed on Core 0 when SignalRGB is closed or your PC is asleep.
* **Persistent Flash Storage:** Automatically saves your speed, brightness, and colors to the Pico's flash memory with a 5-second auto-save cooldown to protect the physical flash wear.
* **Dedicated Single-Interface Design:** Programmatically compiles out CDC Serial on boot, preventing Windows composite parent driver conflicts and ensuring stable USB discovery.
---

<img width="1327" height="380" alt="signalrgb-logo" src="https://github.com/user-attachments/assets/fe2eeddc-1ab0-48aa-8748-bd1ec0debb1a" />

## Hardware Pin Mappings

Connect the Data In (DI) pin of your WS2812B LED strips to the following Raspberry Pi Pico GPIO pins:

* **Channel 1:** GP2 
* **Channel 2:** GP3 
* **Channel 3:** GP4 
* **Channel 4:** GP5 

*Note: GPIO Pins are easily configurable in "pin_config.h".

---
If you found any coding flaws then blame Gemini
---
## Installation & Setup

  ALWAYS USE EXTERNAL POWER SUPPLY FOR POWERING THE LEDS
### 1. Flash the Firmware
1. Open the `/Firmware` directory in VS Code with the **PlatformIO** extension installed and compile the files.
2. Hold down the physical **BOOTSEL** button on your Pico board while plugging it into your computer.
3. Click **Upload** in PlatformIO. (The uploader will automatically flash the binary to the virtual `RPI-RP2` storage drive)
or Just drag and drop the `firmware.uf2` file (located in `.pio\build\pico\`) to the `RPI-RP2` storage deive.

### 2. Configure Windows Drivers (Zadig)
Because the serial COM port is completely disabled, Windows will see the Pico as a raw USB device:
1. Download and open **Zadig** as an Administrator.
2. Click Options > List All Devices
3. From the dropdown list, select **Maximus RGB Controller** (ensure it has no interface suffixes).
4. Select **WinUSB** as the target driver (on the right side of the green arrow).
5. Click **Install Driver** (or **Replace Driver**) to register the required Device Interface GUID in the Windows registry.

### 3. Install the SignalRGB Plugin
1. Locate your SignalRGB User Plugins folder (typically at C:\Users\Username\Documents\WhirlwindFX\Plugins\).
2. Copy your **`Maximus.js`** file directly into this directory.
3. Completely restart SignalRGB (or go to the Device Console and click **Force Reload**).
4. The controller will appear in your device list after restart.

### 4. Configure Layout
1. Open SignalRGB, go to Devices tab and locate your **Maximus RGB Controller**.
2. Click on the controller you will see the options. Configure your LED strips or fans over the 4 channels (up to 250 LEDs per channel).
