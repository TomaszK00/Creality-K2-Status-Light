# Hardware and wiring

**Reference information recovered from original firmware sources; a physical wiring diagram is still pending.**

The archived v2.15 source targets **ESP32-C3 SuperMini** and **SK6812 RGBW addressable LEDs**. Exact board revision and LED strip wiring must still be checked on the physical project.

| Parameter | Reference code / notes |
| --- | --- |
| Controller | ESP32-C3 SuperMini (Arduino target ESP32C3 Dev Module, 4 MB) |
| LED data output | **GPIO4** |
| LED chipset / colour order | **SK6812 RGBW**, `NEO_GRBW + NEO_KHZ800` in reference source |
| Configured LEDs | Up to **24**; UI permits **12 / 24** layouts |
| Onboard blue status LED | **GPIO8**, active LOW on referenced board |
| LED power | **Verify voltage/current against your physical strip**; not supplied by ESP32 GPIO |
| LED bar wiring / mounting | Full tested physical diagram not yet published |

The 12/24 setup means a total LED count and associated pair mapping in the firmware; it does not automatically prove the physical wiring topology of two separate strips.

## Electrical safety

- Verify the exact controller, LED model, voltage, pinout and supply polarity before connecting power.
- Use a power supply, wire gauge and overcurrent protection suitable for the LED load. A GPIO pin carries only control signal.
- For a 5 V SK6812 strip driven by a 3.3 V ESP32 signal, a suitable 74AHCT125-type level shifter and a series data resistor are often recommended; verify the real wiring design.
- The controller and LED strip generally need a common ground when using a shared data line.
- Insulate connections and provide strain relief; do not modify the printer's mains wiring.
- **Never apply printer 24 V directly to an ESP32, a 5 V LED strip or an unverified connection.**

## Still needed before publishing a final diagram

- Photos of the exact ESP32 board and both LED-bar connections.
- LED strip model, voltage, number of LEDs per physical bar, and wire colors.
- Power source and any converters, fuses or logic-level shifters.
- Button wiring, housing dimensions and attachment locations.

See the [Arduino source archive](../Arduino/README.md) and [future K2 enclosure designs](../K2/README.md).
