# ZWO EFW USB HID Emulator for ASIAIR

An experimental Raspberry Pi Pico / RP2040 USB HID compatibility profile that ASIAIR recognizes as a ZWO EFW. It provides three virtual filter slots for a fixed physical dualband
filter: no filter wheel or filter exchange is required.

The intended use case is fast optical systems (approximately f/4 and faster), where the converging beam can shift a narrowband filter’s effective bandpass and produce different
best-focus positions for the Ha and OIII-dominated channels. ASIAIR can autofocus once through the dual-band reference slot, then apply calibrated ZWO EAF offsets when switching
between the virtual Ha and OIII slots.

This project was built because ASIAIR's hardware integration depends on
vendor-specific USB identities and HID behaviour. A custom passive filter
selector therefore cannot normally participate in ASIAIR's filter/offset
workflow. The Pico presents the observed EFW USB identity and replays only
evidence-backed HID behaviour needed by that workflow.

It is an interoperability experiment, not an official product and not a
complete EFW implementation. Read [NOTICE.md](NOTICE.md) and
[docs/PROTOCOL_SCOPE.md](docs/PROTOCOL_SCOPE.md) first.

## What works

Validated with ASIAIR and a real ZWO EAF:

- ASIAIR recognizes the Pico as a ZWO EFW.
- ASIAIR initialization completes.
- Filter slots **1**, **2**, and **3** can be selected from ASIAIR and
  Autorun/Imaging Plan.
- ASIAIR filter names and EAF filter offsets are stored by ASIAIR and applied
  after a slot change.
- A real EAF moves by the configured offset during the tested Autorun workflow.

The emulator has no motor or physical wheel. Slot changes are intentionally
time-compressed status changes. Slots 4–7 must not be selected.

## Hardware

- Raspberry Pi Pico (RP2040)
- USB data cable
- ASIAIR host
- Optional USB-UART adapter for diagnostics
- Optional real ZWO EAF for filter-offset autofocus workflows

See [hardware/wiring.md](hardware/wiring.md) for UART wiring.

## Build

Install the Raspberry Pi Pico SDK and make its path available:

```sh
cmake -S firmware -B build -DPICO_SDK_PATH=/path/to/pico-sdk
cmake --build build
```

The expected artifact is:

```text
build/zwo_efw_emulator.uf2
```

If your environment lacks `picotool`, install a Pico SDK-compatible version
and rebuild. A temporary build with `-DPICO_NO_PICOTOOL=1` can validate the
firmware but may not generate UF2.

## Flash the Pico

1. Unplug the Pico.
2. Hold **BOOTSEL** while reconnecting it by USB.
3. Copy `build/zwo_efw_emulator.uf2` to the mounted `RPI-RP2` drive.
4. Reconnect the Pico to ASIAIR.

Optional UART logging uses GP0/TX at 115200 baud:

```sh
screen /dev/cu.usbserial-XXXX 115200
```

## ASIAIR setup

1. Connect the flashed Pico to ASIAIR and add/detect the EFW.
2. Use only ASIAIR filter slots **1–3**.
3. Rename those filters in ASIAIR if desired.
4. Configure EAF filter offsets relative to slot 1, which is the reference
   filter. For example: dual-band/reference in slot 1, Ha-focused channel in
   slot 2, OIII-focused channel in slot 3.
5. Verify each transition manually before an unattended run.

### Repeated autofocus plan

For a session that must refocus periodically, use repeated Imaging Plan targets
whose sequence starts and ends on slot 1:

```text
autofocus on slot 1
slot 1: reference frames
slot 2: channel frames, ASIAIR applies offset 2
slot 3: channel frames, ASIAIR applies offset 3
slot 1: short return frame
```

Ending on slot 1 makes the next target's `Autofocus before each target start`
operate on the reference filter even if ASIAIR autofocuses before processing
the next target sequence. Confirm this order with a short daytime or test run
before unattended imaging.

## How the protocol work was done

The project deliberately does not invent protocol commands. Each public
firmware behaviour is constrained by a combination of static analysis,
ASIAIR-to-Pico UART observations, and physical USB observations of an
EFW-S-0 firmware 3.9. Raw vendor SDKs, raw third-party USB captures, and test
logs are intentionally excluded from this repository.

See [docs/PROTOCOL_SCOPE.md](docs/PROTOCOL_SCOPE.md) for scope and limits.

## Contributing

See [CONTRIBUTING.md](CONTRIBUTING.md). In particular, do not submit vendor
SDK material or a capture you are not allowed to share.

## License

This repository's original code and documentation are released under the
[MIT License](LICENSE).
