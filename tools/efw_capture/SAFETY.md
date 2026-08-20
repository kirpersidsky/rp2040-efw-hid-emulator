# Safety boundary for the future EFW collector

## Current stage

This stage contains documentation and a JSON Schema only. It sends nothing to
any device: no HID SetReport, no HID GetReport, and no other USB operation.

## Future default active profile — specification only

If separately approved, the future default active profile is limited to this
one already statically established official-library call sequence:

1. Feature SetReport, Report ID `03`, ID-inclusive client buffer
   `03 7E 5A 02 04` (with the configured report-length tail); then
2. Feature GetReport, Report ID `01`.

This is an intention for a future implementation, not code added by this
change. The byte sequence and IDs are sourced from local static analysis of
`libEFWFilter.dylib` (`cefw_static_callers_and_buffers.md`), not from a
physical EFW trace.

`02 01` and every other command are excluded, including as default candidates,
until a separate decision and review. This restriction includes commands that
may be described in third-party comments or test fixtures.

## Prohibited categories

The ordinary collector must never issue commands in these categories:

* wheel positioning or movement;
* calibration or homing;
* configuration/alias or other persistent settings writes; and
* firmware update or update-file transfer.

Any future expansion requires an explicit review that names the exact API
calls, bytes, data-buffer convention, source of evidence, and operator-facing
consent.
