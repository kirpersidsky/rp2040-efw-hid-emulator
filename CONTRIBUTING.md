# Contributing

Contributions are welcome, especially reproducible observations from real EFW
hardware.

## Protocol evidence

Please distinguish clearly between a raw physical capture, a static-analysis
observation, an inference, and emulator-only behaviour. Do not submit guessed
protocol bytes as facts. Include the EFW model, firmware version, host
application/version, capture method, and the action that produced the traffic.

Do not upload vendor SDKs, proprietary application binaries, personally
identifying device serial numbers, or third-party captures without permission.
Open an issue first for large captures; a sanitized transcript may be enough.

## Firmware changes

Keep each command implementation small and evidence-backed. A change should
name its evidence source, reject unknown requests rather than fabricate a
success response, preserve UART diagnostics, and build with the Pico SDK.

The current public profile deliberately supports ASIAIR filter slots 1, 2, and
3 only.
