# Installation and first run

## Current publishing status

This repository currently publishes OTA binaries under `Software/` but **does not yet publish the Arduino source matching firmware v2.15 or a verified factory-flash package**. Do not use the OTA binary as though it were a complete USB-flash image.

## Existing compatible installation

1. Confirm that the device already runs K2 Status Light and supports its OTA update mechanism.
2. Connect to the device through its currently configured interface.
3. Use the firmware's built-in update function, following the prompts shown there.
4. Keep stable power throughout installation.

The [update manifest](../Software/update.json) currently advertises **2.15**.

## First-time installation — pending verification

To publish reproducible instructions, the following details are needed:

- Exact ESP32 board and module variant.
- Arduino IDE / ESP32 board package versions.
- Arduino sketch and required library versions.
- Pinout, number of pixels, LED chipset and power voltage.
- Firmware partition layout and initial flashing method.
- Network onboarding and Creality K2 connection steps.

Once these are verified, this page can be expanded into a step-by-step tutorial. Never guess electrical connections or upload methods.
