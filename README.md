Creality K2 Status Light
An ESP32-powered LED progress indicator for Creality K2 3D printers.

Creality K2 Status Light is a DIY project that uses addressable LEDs to provide an easy-to-see indication of print progress. A browser-based control panel helps with setup and configuration.
Project status: Work in progress. Features and hardware instructions will be updated as the firmware evolves.

Features
- Print progress visualisation: display printing progress using addressable LEDs.
- Web interface: access settings from a browser on your local network.
- Wi-Fi configuration: configure the ESP32 network connection.
- Multilingual interface: English and Polish user-interface options.
- LED testing: test the progress-light behaviour during setup.
Additional lighting effects, language options and LED layout settings are under development. Check the latest release for what is actually supported.
Hardware
- ESP32 development board compatible with the firmware
- Compatible addressable LED strip(s)
- Power supply sized for the chosen LEDs
- Suitable wires and connectors
- Creality K2 printer on the same network (for network-based progress reporting)
Important: Exact ESP32 board, LED model, GPIO pin assignments, LED voltage, and power wiring must be confirmed against the firmware and your hardware before powering the circuit. Do not power a high-current LED strip directly from an ESP32 GPIO pin. Make sure the controller and LED power supply share a common ground when required by the wiring design.
Installation
1. Download the firmware from this repository: [`K2_Status_Light_v2_1.ino`](./K2_Status_Light_v2_1.ino).
2. Install Arduino IDE.
3. Install the ESP32 board package in Arduino IDE and select the correct ESP32 board and serial port.
4. Install the libraries required by the firmware. Adafruit NeoPixel is used by the current sketch; check the #include lines for any other dependencies.
5. Review the firmware configuration, especially the LED pin, LED count, and any network settings.
6. Connect the hardware according to the verified wiring information for your board and LED strip.
7. Compile and upload the sketch to the ESP32.
8. Open the device's web interface and complete its Wi-Fi and printer configuration as supported by your firmware version.
The exact first-time Wi-Fi access method and printer connection settings will be documented after they are verified against the published sketch.

How it works
The ESP32 drives an addressable LED strip to represent print progress. The LED bar provides a visual status indicator that can be seen without opening the printer's screen. Configuration is available through a browser-based interface.
Screenshots and wiring
Photos of the finished installation, screenshots of the web interface, and a verified wiring diagram are coming soon.
Roadmap
- [ ] Publish a verified GPIO and power wiring diagram
- [ ] Add photos and web-interface screenshots
- [ ] Document exact printer communication and initial Wi-Fi setup
- [ ] Document selectable 12/24 LED configurations, once implemented and tested
- [ ] Document pulse effects and looping progress tests, once implemented and tested
- [ ] Expand translations to German and Spanish, once implemented and tested
- [ ] Publish versioned releases
Contributing
Suggestions, bug reports, and improvements are welcome. Please use the repository's Issues tab to report a problem or propose an enhancement.
Disclaimer
This is an independent community project and is not affiliated with or endorsed by Creality. Build and use it at your own risk. Verify all electrical connections and power requirements before switching on.
License
See the LICENSE file if a license has been added to the repository. Until a license is provided, do not assume the source code is licensed for unrestricted reuse.
