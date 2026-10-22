# Arduino source

## Historical source

[`legacy/K2_Status_Light_v2_1.ino`](legacy/K2_Status_Light_v2_1.ino) was recovered **unaltered** from the repository history (commit `d6295dec`). It is an **old v2.1 sketch**, not the source code for the current v2.15 OTA image.

The sketch references `WiFi.h`, `WebServer.h`, `HTTPClient.h`, `Preferences.h`, `ESPmDNS.h`, `ArduinoJson.h`, `Adafruit_NeoPixel.h` and `Update.h`. ArduinoJson and Adafruit NeoPixel need to be installed separately in a suitable Arduino ESP32 environment.

**Do not flash this file expecting v2.15 behaviour.** Its default admin password placeholder must be changed, its setup AP password is hardcoded, and its LED GPIO (4), 24 LED count and GRBW pixel type are historical settings that must be checked against the exact wiring. The sketch has not been rebuilt or tested during this restoration.

## Current firmware

The repository has v2.15 compiled firmware in `Software/`, but the matching `v2.15` source has **not** been recovered. Upload the exact source used to build that binary before labeling it reproducible/open for modification. Compiled binaries cannot be reliably reversed into the original Arduino project.
