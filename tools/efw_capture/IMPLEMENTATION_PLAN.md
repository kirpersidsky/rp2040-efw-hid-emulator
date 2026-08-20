# Implementation plan for the EFW collector

## Stage 1 — current

Documentation and `capture_schema.json` only. No executable collector, HID API
call, dependency, or firmware change is part of this stage.

## Stage 2 — macOS passive collector

Implement macOS read-only enumeration and raw HID report-descriptor dump. It
will perform no SetReport/GetReport operation.

## Stage 3 — macOS active `02 04` profile

Only after separate confirmation, implement the active profile specified in
`SAFETY.md`: Feature SetReport ID 3 using the official-library statically
observed `03 7E 5A 02 04` client-buffer prefix, followed by Feature GetReport
ID 1. The source is local static analysis of `libEFWFilter.dylib`, not a
physical EFW trace.

## Stage 4 — Windows collector

Implement Windows behavior with the same `capture_schema.json`, preserving the
native API's distinction between report ID, client buffer, and data buffer.

## Stage 5 — volunteer packaging

Package a standalone binary and concise instructions for a volunteer to submit
one JSON result file.

Before active EFW work, the macOS portion is tested on the available physical
EAF only in read-only modes (enumeration and descriptor retrieval). EAF data is
not confirmation of EFW protocol, report placement, or response bytes.
