# Public protocol scope

The firmware is a narrow compatibility profile, not a complete specification
of the ZWO EFW protocol.

## Implemented behaviour

- USB HID identity compatible with the observed EFW device class.
- Feature-report initialization/status exchanges required by the tested
  ASIAIR workflow.
- Time-compressed final-idle transitions for ASIAIR filter slots 1, 2, and 3.
- UART logging of incoming Feature reports and the selected response profile.

The three implemented transitions are based on physical USB observations of an
EFW-S-0 firmware 3.9, then validated against ASIAIR using this RP2040
firmware.

## Deliberate limitations

- Slots 4–7 are not implemented, even though the captured status frame carries
  a seven-slot value. Do not select them in ASIAIR.
- There is no physical motor, calibration, persistent position storage,
  movement timing, moving-state report, reset, firmware update, or
  configuration-write implementation.
- A slot transition changes the emulated status immediately. This is an
  emulator simplification, not a claim about real-wheel timing.
- Compatibility with every EFW model or ASIAIR version is not claimed.

Unknown requests are rejected rather than emulated speculatively.
