# macOS read-only EFW collector

Build from this directory:

```sh
./build.sh
```

Connect exactly one physical ZWO EFW with VID `03C3` and PID `1F01`, then run:

```sh
./efw_capture_readonly --output ./efw_capture.json
```

The output location may be absolute or relative. Existing files are protected;
use `--force` only when deliberately replacing an existing JSON result.

The collector does not open the HID device and does not call HID SetReport or
GetReport. It only reads HID metadata and the report descriptor through
IOHIDManager/IOHIDDevice properties. It does not collect the USB serial number.

You can send the generated JSON file to us. Before sending it, you may inspect
it yourself; its default device object contains no serial number.
