# Installation and first run

## Supported reference configuration

Original packaged firmware and source files target **ESP32-C3 SuperMini / ESP32C3 Dev Module** (4 MB flash), with a SK6812 RGBW strip and local Moonraker communication to a Creality K2. Consult [hardware notes](HARDWARE.md) before connecting power.

The current OTA manifest points to **v2.15**, whose [Arduino source is available](../Arduino/archive/K2_Status_Light_v2_15.ino). The newer v2.16 is archived but not promoted as the default update.

## Option A — upgrade an already running compatible device

1. Confirm the installed firmware includes a functional OTA update page and that the board has sufficient OTA app partition space.
2. Open the device's web interface on the trusted local network.
3. Use the firmware update control to select the compatible [v2.15 OTA binary](../Software/K2_Status_Light_v2_15_OTA.bin), or follow the firmware's GitHub-update workflow.
4. Keep power stable throughout upload, validation and restart.

**Do not flash an OTA image directly at address 0x0** as if it were a full USB image. Do not install the archived v2.16 just because its version number is higher; it has not been selected for public auto-updates.

## Option B — first-time flash using the original v2.3 USB image

The [v2.3 historical package](../Software/archive/README.md) contains a **combined ESP32-C3 USB image** intended for a 4 MB-flash ESP32-C3. The accompanying original instructions specify flashing from offset `0x0`.

1. Install a trusted version of Espressif `esptool` on a computer and connect your ESP32-C3 using a **data-capable USB cable**.
2. Identify the correct serial port (replace `COM5` below).
3. Download [`K2_Status_Light_v2_3_USB.bin`](../Software/archive/K2_Status_Light_v2_3_USB.bin).
4. Only if you have verified the board model and backup/recovery procedure, run:

```powershell
py -m esptool --chip esp32c3 --port COM5 write-flash 0x0 K2_Status_Light_v2_3_USB.bin
```

This is preserved **historical guidance** from the v2.3 package, not a newly tested installation. Flashing a combined image can overwrite stored Wi-Fi/printer settings and is unsuitable for other chip/flash layouts.

After first installation, follow the AP setup flow and use compatible OTA updates to move to a newer version.

## Option C — compile and upload an Arduino sketch

1. Install **Arduino IDE 2.x**.
2. Install the **ESP32 boards package by Espressif Systems**.
3. Install **ArduinoJson v7** and **Adafruit NeoPixel** using Library Manager.
4. Download the desired original [Arduino source](../Arduino/README.md); for current v2.15 use [this sketch](../Arduino/archive/K2_Status_Light_v2_15.ino).
5. Select **ESP32C3 Dev Module**, 4 MB flash, USB CDC On Boot enabled if available, and an OTA-compatible layout such as **Minimal SPIFFS (1.9 MB APP with OTA)**.
6. Confirm GPIO4, LED model, power, printer IP and any network/security settings match your real build.
7. Compile, inspect warnings, and upload over USB. If changing partition layouts, expect to need reconfiguration afterwards.

Package and library versions may affect compilation; builds have not been recompiled and hardware-tested as part of repository restoration.

## First-time Wi-Fi configuration

Most historical builds provide the setup access point **`K2-LED-Setup`** with default password **`k2ledsetup`**, typically at `http://192.168.4.1`. Configure a 2.4 GHz home network and the printer connection from the web panel. Some older versions require a physical reset after changing Wi-Fi; v2.15 includes background reconnection.

**Security warning:** This is a documented default, not a strong secret. Several historical firmware versions lack web-panel authentication. Use only on a trusted LAN, change defaults where supported, and do not forward the ESP32 web interface to the public Internet.

## Hardware and safety notes

- GPIO4 is the SK6812 RGBW data output in the archived reference code; GPIO8 is the active-low onboard blue status LED on the referenced board.
- Use the correct LED voltage and suitably rated separate power wiring; GPIO is a signal, not a power supply.
- A 5 V SK6812 strip often benefits from proper 3.3 V-to-5 V data level shifting and suitable series resistor.
- **Never connect 24 V directly to the ESP32 or LED strip.**
- Validate pinout against the actual board and code before powering.

See [historical release notes](../CHANGELOG.md) and [archived binaries](../Software/archive/README.md).
