# Creality K2 Status Light

**A DIY ESP32-powered LED status and print-progress light for Creality K2 printers.**

![Project status](https://img.shields.io/badge/status-development-orange)
![Firmware](https://img.shields.io/badge/OTA%20firmware-v2.15-blue)
![Platform](https://img.shields.io/badge/platform-ESP32-green)

> **Development project:** The published repository currently contains compiled OTA firmware, but not the Arduino source sketch. Hardware pins, supported ESP32 board variants, connection procedures and specific firmware features must be checked against the actual firmware before use.

## Overview

Creality K2 Status Light is a community-built LED indicator designed to show print progress at a glance. The project is based on an ESP32 controller, addressable LEDs, and a browser-based configuration interface.

**Project goals**
- Display printer progress on LED light bars.
- Offer LED effects and tests through a local web interface.
- Make setup and Wi-Fi configuration convenient.
- Provide English and Polish interface options.
- Support versioned firmware updates.

The functions listed above describe the project's intended capabilities; individual firmware builds may differ.

## Downloads

The newest firmware package currently referenced by the update manifest is **v2.15**:

| File | Purpose |
| --- | --- |
| [v2.15 OTA firmware](Software/K2_Status_Light_v2_15_OTA.bin) | Current package linked by update manifest |
| [v2.14 OTA firmware](Software/K2_Status_Light_v2_14_OTA.bin) | Older build |
| [v2.12 OTA firmware](Software/K2_Status_Light_v2_12_OTA.bin) | Older build |
| [Update manifest](Software/update.json) | Machine-readable version, filename, size and SHA-256 |

**Important:** These files are named `_OTA.bin` and are intended for an *already installed and compatible* OTA-capable firmware. **Do not assume they are suitable for flashing a blank ESP32 over USB.** A verified initial-installation firmware and exact flashing instructions have not yet been published.

## Hardware

- A supported ESP32 development board (exact model to be confirmed).
- Compatible addressable LED strip(s).
- A properly rated power supply for the LEDs and controller.
- Suitable cabling, connectors, and protection as appropriate.
- Creality K2 printer accessible on the required local network.

See [Hardware and wiring](docs/HARDWARE.md) for electrical safety notes and the information still needed to publish an accurate wiring diagram.

## Setup

### Already running an earlier compatible version

1. Confirm that your hardware is running an existing build of **this** project and has an OTA update feature.
2. Open its web interface and follow the update workflow provided by that firmware.
3. If it uses `Software/update.json`, the current manifest selects version **2.15**.
4. Do not disconnect power during an update. Keep a tested recovery method available.

The exact update UI and steps depend on the installed firmware. This documentation does **not** assert compatibility with unrelated ESP32 firmware.

### New build / first-time installation

A full USB installation guide cannot be verified yet because the source code, board selection, GPIO assignment and initial flash package are not present in the repository. **Please do not wire or flash hardware based on guessed values.** See [Installation status](docs/INSTALLATION.md).

## Firmware updates

`Software/update.json` contains the advertised version and firmware filename plus expected file size and SHA-256 digest. Firmware publishers should keep all four values in sync with the uploaded binary. See [Release and OTA checklist](docs/RELEASING.md).

## Documentation

- [Installation and first-run status](docs/INSTALLATION.md)
- [Hardware, wiring and power](docs/HARDWARE.md)
- [Release and OTA publishing checklist](docs/RELEASING.md)
- [Changes and firmware history](CHANGELOG.md)
- [How to contribute](CONTRIBUTING.md)
- [Security and responsible reporting](SECURITY.md)

## Screenshots and diagrams

Photos of the LED bars, web interface and a **verified** wiring diagram can be added under `docs/images/` when available.

## Roadmap

- [ ] Publish reviewed Arduino source code and dependency versions.
- [ ] Document the exact board, GPIO assignments and number/type of LEDs.
- [ ] Provide a tested first-time USB flashing procedure.
- [ ] Confirm network onboarding, printer connectivity and web UI behaviours per build.
- [ ] Add photos, screenshots and wiring diagrams.
- [ ] Publish GitHub Releases with notes and checksums.

## Contributing

Feedback and issue reports are welcome. Please see [CONTRIBUTING.md](CONTRIBUTING.md). Only repository maintainers and explicitly invited collaborators can directly push to this repository.

## License

This repository currently contains an [MIT license](LICENSE), which **allows commercial use** subject to its conditions. A private-use-only or non-commercial restriction would require a different licensing decision; merely describing the project as DIY does not change its legal permissions.

## Disclaimer

An independent community project, not affiliated with or endorsed by Creality. Electrical assembly, third-party firmware flashing, and printer integration are performed at your own risk. Verify component ratings and connections before applying power.
