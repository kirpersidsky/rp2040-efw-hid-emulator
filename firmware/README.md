# Firmware

RP2040 firmware for ZWO EFW USB HID emulator.

Current state:

Empty implementation.

Previous experimental TinyUSB code was removed.

Development phases:

1. USB enumeration
2. HID report handling
3. HID traffic logging
4. ASIAIR initialization analysis
5. Minimal EFW command emulation

Do not implement commands before HID traffic is understood.
# Firmware

RP2040/TinyUSB firmware for the public ZWO EFW HID compatibility profile.

Build from the repository root:

```sh
cmake -S firmware -B build -DPICO_SDK_PATH=/path/to/pico-sdk
cmake --build build
```

The flashable image is normally `build/zwo_efw_emulator.uf2`.

If CMake reports that `picotool` is unavailable, install a Pico SDK-compatible
`picotool` and build again. A temporary build with `-DPICO_NO_PICOTOOL=1`
produces ELF/BIN/HEX but may not create UF2.

The implemented profile is limited to ASIAIR slots 1–3. See
[`../docs/PROTOCOL_SCOPE.md`](../docs/PROTOCOL_SCOPE.md) before changing
firmware behaviour.
