# Development roadmap

This document records requested enhancements to **Creality K2 Status Light**. Items below are **planned / to verify**, not advertised as already implemented. The firmware archive and OTA release manifest remain authoritative for released features.

Last updated: 23 October 2026.

## Web interface and LED configuration

- [ ] Replace the LED colour-count dropdown with **+ / − controls** beside colour entries, allowing addition/removal of individual colours, up to **12**.
- [ ] Display a maximum of **6 colour entries per row** on wider screens; adapt the layout for mobile.
- [ ] On mobile, replace the always-visible language flags with a **Languages** menu immediately to the right of **FAQ**, with a list of supported languages.
- [ ] Improve mobile navigation so the top settings/menu header behaves consistently while scrolling, without hiding access to key controls.
- [ ] Verify that changes to colour palettes, effect settings, and LED-count selection persist through save and reboot.

## Network and printer status

- [ ] Expand **Network and printer status** to show meaningful connection diagnostics after joining home Wi-Fi: connection state, SSID, device IP address, subnet mask, gateway, DNS, signal strength, and connected printer host/IP and port, wherever available.
- [ ] Present separate Wi-Fi, printer API and update-server connection states; show clear failure messages.
- [ ] Support opening the settings panel by a **stable local hostname**, e.g. `http://k2-status-light.local/`, using mDNS where supported, even if the DHCP-assigned IP address changes.
- [ ] Make the hostname discoverable in the status panel; document that `.local` resolution depends on client/OS/network support and may not work across isolated networks.
- [ ] Never show saved Wi-Fi passwords or secret tokens in diagnostics, logs, exports or screenshots.

## Release requirements

- [ ] Implement and test each improvement in the current Arduino source.
- [ ] Test desktop and mobile layouts, 12/24 LED configurations, network disconnection/recovery, Wi-Fi scans, and printer status handling.
- [ ] Compile firmware for the documented ESP32-C3 target and check flash/OTA partition limits.
- [ ] Publish a tested OTA binary and release notes before changing `Software/update.json`.
- [ ] Update README, installation guides, translations and changelog when a feature is confirmed working.

## Related documents

- [README](../README.md)
- [Firmware history](../CHANGELOG.md)
- [Hardware and wiring](HARDWARE.md)
- [Release checklist](RELEASING.md)

Contributions are welcome as proposals via Issues or Pull Requests, but publishing changed copies or derivatives is subject to the repository license and existing third-party or earlier-license rights.
