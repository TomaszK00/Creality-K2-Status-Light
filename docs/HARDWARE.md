# Hardware and wiring

**Status: verified wiring diagram not yet published.**

The project is intended for an ESP32, suitable addressable LEDs, and a properly rated power source. Exact wiring depends on the firmware and real components.

## Electrical safety

- **Never connect LED power current to a GPIO pin.** GPIO is a signal output, not a power source.
- Verify LED strip operating voltage, maximum current, supply polarity and ESP32 power requirements independently.
- Size the PSU and wiring for the actual load; include appropriate short-circuit protection.
- Many addressable LED configurations require a common signal reference (ground) between the controller and LED supply; use the verified design.
- Keep connections insulated and strain-relieved. Do not modify printer mains wiring.

## Details needed for the official wiring guide

| Parameter | Status |
| --- | --- |
| ESP32 exact board | To verify |
| Data GPIO for LED bar 1 | To verify |
| Data GPIO for LED bar 2 | To verify |
| LED model and voltage | To verify |
| Number of pixels per bar | To verify |
| Power wiring and protective components | To verify |
| Optional buttons and status LED | To verify |

A wiring drawing and photographs will be published after hardware and source have been reviewed.
