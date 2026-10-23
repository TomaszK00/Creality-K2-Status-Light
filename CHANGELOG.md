# Firmware changelog

## v2.29 — test candidate (23 October 2026)

- Make mobile navigation buttons, card spacing, typography and form controls consistent across the panel.
- Keep the sticky LED preview compact when scrolling and make stage controls clearer on narrow phones.
- Expand opened brightness and Pulse controls across the LED card; keep six palette colours per row where space permits.
- CSS parsing, interaction checks and GitHub Actions compilation passed; physical device testing is still pending.

## v2.28 — test candidate (23 October 2026)

- Mobile Network, Printer, Update and FAQ panels open full screen and close with their Close buttons; Language remains a popup.
- LED brightness uses an expandable 0–100% slider, and Pulse uses an expandable checkbox with a 0–5000 ms slider in every stage.
- Keep the selected effect visible in each mobile row and shorten the Test mode label so Repeat test fits beside it.
- JavaScript UI checks passed. GitHub Actions compiled the OTA binary; device and printer testing remains pending.

## v2.27 — test candidate (23 October 2026)

- Split Network and Printer into separate panels; Wi-Fi Save & Connect uses one button.
- Add live printer readout, multiple discovery results, disconnect, rescan, and local Moonraker scanning.
- Show 0–100% printer discovery progress and mobile language flags.
- Rename panel and OTA links to LayerGlow; keep a legacy filename alias for older updater compatibility.
- Compiled with GitHub Actions; requires testing on the physical ESP32-C3 and printer.

Release descriptions are based on the README files included with the original firmware packages and on preserved repository history. They document **why each build was prepared**, not a claim that every feature was independently tested on physical hardware.

**Published automatic OTA target:** v2.29 test candidate via [`Software/update.json`](Software/update.json). v2.16 is archived as a newer, not-yet-promoted build.

| Version | Why it was created / main changes | Files |
| --- | --- | --- |
| **2.16** | Improved Wi-Fi scanning while in `K2-LED-Setup` AP mode; suspend reconnect attempts during scans; distinguish no networks from scan failures; added internal save/restore backup for LED configurations. **Archived, not advertised by current OTA manifest.** | [source](Arduino/archive/K2_Status_Light_v2_16.ino) · [OTA](Software/archive/K2_Status_Light_v2_16_OTA.bin) |
| **2.15** | Fixed Wi-Fi recovery: auto-start AP after ~8 seconds without home Wi-Fi, background reconnect attempts, radio recovery, delayed AP shutdown and automatic reconnection after changing networks. | [source](Arduino/archive/K2_Status_Light_v2_15.ino) · [OTA](Software/K2_Status_Light_v2_15_OTA.bin) |
| **2.14** | Red **Update** button with white lettering when a GitHub update is detected. New effects: comet, traveling fill and alternating LED pairs. | [source](Arduino/archive/K2_Status_Light_v2_14.ino) · [OTA](Software/K2_Status_Light_v2_14_OTA.bin) |
| **2.13** | Added independently timed **Random Pairs** LED effect with adjustable speed, also reflected in LED preview and test. | [source](Arduino/archive/K2_Status_Light_v2_13.ino) · [OTA](Software/archive/K2_Status_Light_v2_13_OTA.bin) |
| **2.12** | Changed onboard blue LED's update-available signal to two long flashes followed by one short flash; documented pattern in multilingual FAQ. | [source](Arduino/archive/K2_Status_Light_v2_12.ino) · [OTA](Software/K2_Status_Light_v2_12_OTA.bin) |
| **2.11** | Added per-status **LED On** toggles and automatic saving; removed the edges-center-edges effect with settings migration; improved phone layout. | [source](Arduino/archive/K2_Status_Light_v2_11.ino) · [OTA](Software/archive/K2_Status_Light_v2_11_OTA.bin) |
| **2.10** | Addressed failures detecting GitHub updates when `update.json` was missing; added fallback binary discovery and new onboard LED Wi-Fi/update signals. A manifest was published alongside this version. | [source](Arduino/archive/K2_Status_Light_v2_10.ino) · [OTA](Software/archive/K2_Status_Light_v2_10_OTA.bin) |
| **2.9** | Streamlined update panel, added explicit **Update** dialog, refreshed header/footer layout and improved GitHub update checking. | [source](Arduino/archive/K2_Status_Light_v2_9.ino) · [OTA](Software/archive/K2_Status_Light_v2_9_OTA.bin) |
| **2.8** | Introduced GitHub manifest-based updates (version, binary size and SHA-256 verification); expanded/finished interface translations and animated effects. | [source](Arduino/archive/K2_Status_Light_v2_8.ino) · [OTA](Software/archive/K2_Status_Light_v2_8_OTA.bin) |
| **2.7** | Automatic web-panel refresh after OTA, persistent connection warnings and new centre/edge/wave-style LED effects. | [source](Arduino/archive/K2_Status_Light_v2_7.ino) · [OTA](Software/archive/K2_Status_Light_v2_7_OTA.bin) |
| **2.6** | Reworked dashboard and LED settings layout; added individual lighting effects per status (including Knight Rider), toasts and clearer progress display. | [source](Arduino/archive/K2_Status_Light_v2_6.ino) · [OTA](Software/archive/K2_Status_Light_v2_6_OTA.bin) |
| **2.5** | Added per-status pulse settings, automatic LED progress test, reset of full LED settings and clearer OTA progress notices. | [source](Arduino/archive/K2_Status_Light_v2_5.ino) · [OTA](Software/archive/K2_Status_Light_v2_5_OTA.bin) |
| **2.4** | Enabled automatic saving of LED settings, restoration of default colours and clearer instructions for changing Wi-Fi. | [source](Arduino/archive/K2_Status_Light_v2_4.ino) · [OTA](Software/archive/K2_Status_Light_v2_4_OTA.bin) |
| **2.3** | Expanded 12/24 LED configuration, pulsation, looping tests and status LED; packaged a first-install USB image alongside OTA and four language options. | [source](Arduino/archive/K2_Status_Light_v2_3.ino) · [OTA](Software/archive/K2_Status_Light_v2_3_OTA.bin) · [USB](Software/archive/K2_Status_Light_v2_3_USB.bin) |
| **2.2** | An earlier ZIP is listed in the personal file archive but its original bytes could not be retrieved; no faithful copy or verified change description is published here. | *Unavailable* |
| **2.1** | Added browser Wi-Fi network selection, English/Polish controls, web OTA upload and LED preview/test functionality. | [source](Arduino/legacy/K2_Status_Light_v2_1.ino) |
| **2.0** | Initial ESP32-C3 web/network and Moonraker print-status prototype, intended for testing without physical LED strip. | [source](Arduino/archive/K2_Status_Light_v2_0.ino) |

## Important release distinctions

- **OTA binaries** (`_OTA.bin`) update compatible already-installed firmware; they are **not automatically safe for first-time USB flashing**.
- The **v2.3 USB image** is provided as a historical combined flash image intended for ESP32-C3 4 MB at address `0x0`. Using it can erase saved Wi-Fi/printer configuration. Read [installation instructions](docs/INSTALLATION.md) before proceeding.
- Old binaries are saved under `Software/archive/`; previously published v2.12/v2.14/v2.15 images stay in `Software/`. The automatic update manifest is deliberately unchanged.
- Firmware notes are from original package documentation. Testing of each compiled image on real user hardware has **not** been independently re-performed during archiving.
- Earlier copies distributed under MIT preserve their existing MIT license rights. See [licensing notes](docs/LICENSING.md).
