# Creality K2 Status Light

**DIY ESP32-C3 LED print-progress and status indicator for Creality K2 printers.**

![Project status](https://img.shields.io/badge/status-development-orange)
![Current OTA](https://img.shields.io/badge/current%20OTA-v2.15-blue)
![Archived build](https://img.shields.io/badge/archived-v2.16-informational)
![Platform](https://img.shields.io/badge/platform-ESP32--C3-green)

> **Current published update: v2.15.** Build **v2.16** has been archived with its source code but is **not** advertised for automatic installation. All old versions remain available for comparison and recovery where compatible.

## About

K2 Status Light uses an ESP32-C3 board, addressable LEDs, and a local browser interface to visualise Creality K2 printing progress. Its firmware communicates with the printer through the local network, offers status lighting and effects, and provides Wi-Fi and device configuration.

Features documented in the historical firmware include:
- Progress display on 12 or 24 addressable LEDs (in pairs).
- Configurable colours, brightness, flashing patterns, and effects.
- Browser-based configuration, preview, and LED test tools.
- Wi-Fi onboarding via `K2-LED-Setup` when needed.
- English, Polish, Spanish, and German UI in developed versions.
- Manual OTA updates and a GitHub version-check workflow.

Specific functions depend on the installed firmware version. See the [change history](CHANGELOG.md).

## Firmware and source

| Item | Download |
| --- | --- |
| **Current OTA v2.15** | [Download .bin](Software/K2_Status_Light_v2_15_OTA.bin) |
| **Source code v2.15** | [View .ino](Arduino/archive/K2_Status_Light_v2_15.ino) |
| **Archived build v2.16** | [OTA .bin](Software/archive/K2_Status_Light_v2_16_OTA.bin) · [source .ino](Arduino/archive/K2_Status_Light_v2_16.ino) |
| **All historical versions** | [Firmware archive](Software/archive/README.md) · [Source archive](Arduino/README.md) |
| **Why each version was created** | [Detailed changelog](CHANGELOG.md) |
| **OTA update manifest** | [Software/update.json](Software/update.json) |

The archive contains available original Arduino sources for v2.0, v2.1 and v2.3–v2.16, and OTA binaries from v2.3 through v2.16. **v2.2 remains unavailable** as its earlier ZIP could not be retrieved intact.

## Hardware used by the archived firmware

The v2.15 source and the packaged installation notes target:
- **ESP32-C3 SuperMini / ESP32C3 Dev Module**, 4 MB flash.
- **SK6812 RGBW** addressable LEDs, with data on **GPIO4** in the reference configuration.
- **24 LEDs maximum** in the original code, with 12/24 display layouts.
- Onboard blue status LED on **GPIO8** (active LOW on the referenced board).
- Printer network interface through **Moonraker** (default port 7125 in earlier builds).

**These are firmware settings, not a verified physical wiring diagram for every board.** Verify voltage, current, level shifting, grounds, and the GPIOs on your specific ESP32 and LED strip. Never connect printer 24 V directly to the ESP32 or 5 V LEDs. See [hardware notes](docs/HARDWARE.md).

## Installation

**Existing installation:** use the device's built-in update page with the correct compatible `_OTA.bin` file. Do not disconnect power during the update. If the firmware uses the GitHub manifest, it currently advertises **2.15**.

**New ESP32 device:** see the [first-time installation guide](docs/INSTALLATION.md) for the historically packaged v2.3 USB image and the Arduino sketch option. USB flashing can erase saved settings and requires the correct chip/flash/partition settings. Firmware has not been retested on every possible board.

**Security:** several archived versions have no web-panel authentication, and the setup AP has a published default password. Use the device only on a trusted LAN; never expose its admin/OTA interface directly to the internet.

## Repository folders

- [`Arduino/archive/`](Arduino/archive/) — original source sketches by version.
- [`Arduino/legacy/`](Arduino/legacy/) — recovered v2.1 sketch from Git history.
- [`Software/`](Software/) — current update manifest and selected original OTA files.
- [`Software/archive/`](Software/archive/) — additional earlier OTA and USB images.
- [`K2/`](K2/) — planned 3D printable cases, LED bar mounts, cable guides, and diffusers.
- [`docs/`](docs/) — installation, hardware notes, release checklist and licensing.

## Documentation

- [Version-by-version changelog](CHANGELOG.md)
- [Planned improvements and feature requests](docs/ROADMAP.md)
- [Polish version history](docs/CHANGELOG_PL.md)
- [Installation and first run](docs/INSTALLATION.md)
- [Hardware and wiring](docs/HARDWARE.md)
- [OTA publishing checklist](docs/RELEASING.md)
- [3D design roadmap](K2/README.md)
- [License and prior MIT versions](docs/LICENSING.md)
- [Contributions](CONTRIBUTING.md) · [Security](SECURITY.md)

## License

**Private, non-commercial home use only** under the current [Personal Home Use License](LICENSE); other uses require prior written permission. This is **source-available**, not OSI open source. Material previously distributed under MIT retains existing MIT permissions. See [licensing details](docs/LICENSING.md).

## Disclaimer

Independent community project; not affiliated with or endorsed by Creality. Firmware archives and their notes are preserved for reference; archived binaries and first-flash images should only be used on verified compatible hardware and are provided without warranty.
