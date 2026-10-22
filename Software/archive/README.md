# Firmware archive — original binary builds

This folder preserves additional firmware images from historical K2 Status Light releases. **Do not set the active updater manifest to these archived filenames.** The live manifest at [`../update.json`](../update.json) continues to point to **v2.15**.

## Archived OTA binaries

| Version | OTA binary | Source code |
| --- | --- | --- |
| 2.3 | [v2.3 .bin](K2_Status_Light_v2_3_OTA.bin) | [v2.3 .ino](../../Arduino/archive/K2_Status_Light_v2_3.ino) |
| 2.4 | [v2.4 .bin](K2_Status_Light_v2_4_OTA.bin) | [v2.4 .ino](../../Arduino/archive/K2_Status_Light_v2_4.ino) |
| 2.5 | [v2.5 .bin](K2_Status_Light_v2_5_OTA.bin) | [v2.5 .ino](../../Arduino/archive/K2_Status_Light_v2_5.ino) |
| 2.6 | [v2.6 .bin](K2_Status_Light_v2_6_OTA.bin) | [v2.6 .ino](../../Arduino/archive/K2_Status_Light_v2_6.ino) |
| 2.7 | [v2.7 .bin](K2_Status_Light_v2_7_OTA.bin) | [v2.7 .ino](../../Arduino/archive/K2_Status_Light_v2_7.ino) |
| 2.8 | [v2.8 .bin](K2_Status_Light_v2_8_OTA.bin) | [v2.8 .ino](../../Arduino/archive/K2_Status_Light_v2_8.ino) |
| 2.9 | [v2.9 .bin](K2_Status_Light_v2_9_OTA.bin) | [v2.9 .ino](../../Arduino/archive/K2_Status_Light_v2_9.ino) |
| 2.10 | [v2.10 .bin](K2_Status_Light_v2_10_OTA.bin) | [v2.10 .ino](../../Arduino/archive/K2_Status_Light_v2_10.ino) |
| 2.11 | [v2.11 .bin](K2_Status_Light_v2_11_OTA.bin) | [v2.11 .ino](../../Arduino/archive/K2_Status_Light_v2_11.ino) |
| 2.13 | [v2.13 .bin](K2_Status_Light_v2_13_OTA.bin) | [v2.13 .ino](../../Arduino/archive/K2_Status_Light_v2_13.ino) |
| 2.16 (archived newer build) | [v2.16 .bin](K2_Status_Light_v2_16_OTA.bin) | [v2.16 .ino](../../Arduino/archive/K2_Status_Light_v2_16.ino) |

For completeness, the root [Software folder](../) already contains v2.12, v2.14, and v2.15 OTA binaries with matching source under `Arduino/archive/`.

**Additional historical USB installer:** [K2_Status_Light_v2_3_USB.bin](K2_Status_Light_v2_3_USB.bin) — combined ESP32-C3 USB image from the v2.3 package. Use it **only** with hardware/flash settings verified against the [installation notes](../../docs/INSTALLATION.md); flashing may erase your configuration.

## Why each version was released

See the [detailed changelog](../../CHANGELOG.md) or the [Polish notes](../../docs/CHANGELOG_PL.md). Reasons are based on README files supplied in the original versioned packages, not a fresh hardware test.

## Archive integrity

The original bytes were recovered from GitHub history and versioned ZIP packages. Recovered Git blobs were checked against the originals before being added here. Binary files are not to be edited by hand; publishing a new update requires generating a new binary, version and checksum.

## Safety

Files labelled `_OTA.bin` are OTA images intended for compatible installed firmware, **not** guaranteed factory flash images. An outdated binary can omit fixes introduced in later versions. Do not connect an ESP32/LED directly to the printer's 24 V supply.
