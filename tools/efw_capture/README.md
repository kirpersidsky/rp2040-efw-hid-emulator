# ZWO EFW capture collector — specification

## Purpose

This directory specifies a future standalone collector for a volunteer who owns
a physical ZWO EFW. The target device is USB VID `03C3`, PID `1F01`.

The intended workflow is simple: connect the EFW to macOS or Windows, run one
binary, then send us its one JSON result file. The JSON file is the only
required output artifact.

The collector is not a USB sniffer. It records the host HID API calls it makes,
their exact arguments, API return status, elapsed duration, and API-visible
bytes. Its JSON shape is defined by `capture_schema.json`.

## Required captured data

The future collector must write:

* capture timestamp and collector/platform version information;
* HID API/backend name;
* device VID/PID and manufacturer/product strings if exposed by the OS;
* raw HID report descriptor, its length, and SHA-256;
* an ordered record of every SetReport/GetReport API operation, including
  report type, report ID, client-buffer/data-buffer lengths, exact bytes,
  API result, error code/domain, and duration.

The device serial number is deliberately excluded from the default schema
result. A future serial collection mode, if needed, requires an explicit
opt-in design and separate review.

## Safety boundary

The ordinary collector invocation must not send a command that could move or
calibrate the wheel, change position, write device configuration, or update
firmware. This first repository change implements no collector and sends no
HID report.

The only proposed future default active profile is documented in `SAFETY.md`.
Its command bytes are a **local static-analysis fact** from
`libEFWFilter.dylib`; they are not presented as a physical EFW trace.

## Buffer recording rule

HID platforms differ in whether a report ID is represented separately or in
the client data buffer. A result must preserve both views when available:

* `client_buffer_hex` is the caller's ID-inclusive representation;
* `data_buffer_hex` is the pointer/data region supplied to or returned by the
  OS HID API; and
* `api_bytes_hex` is the exact byte sequence observed for that API operation.

The matching lengths must be recorded even when client and data buffers differ.
This records the API contract without claiming a captured USB wire packet.

## Current status

This directory contains specification documents and a schema only. There is no
executable collector, no packaged dependency, and no physical-device action.

## macOS read-only collector

The stage-2 macOS collector is in [macos/](macos/). It reads metadata and the
raw HID descriptor only; it neither opens the device nor implements the active
profile described above.
