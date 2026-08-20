#include <string.h>

#include "tusb.h"

#include "usb_descriptors.h"

enum {
    ITF_NUM_HID,
    ITF_NUM_TOTAL,
};

enum {
    EPNUM_HID_OUT = 0x01,
    EPNUM_HID_IN  = 0x81,
    CONFIG_TOTAL_LEN = TUD_CONFIG_DESC_LEN + TUD_HID_INOUT_DESC_LEN,
};

enum {
    STRID_LANGID = 0,
    STRID_MANUFACTURER,
    STRID_PRODUCT,
    STRID_SERIAL,
};

static const uint8_t hid_report_descriptor[] = {
    0x06, 0x00, 0xFF, 0x09, 0x01, 0xA1, 0x01,

    0x85, 0x01, 0x95, 0x0F, 0x75, 0x08, 0x26, 0xFF, 0x00, 0x15, 0x00,
    0x09, 0x01, 0x81, 0x02,
    0x85, 0x02, 0x95, 0x0F, 0x75, 0x08, 0x26, 0xFF, 0x00, 0x15, 0x00,
    0x09, 0x01, 0x81, 0x02,
    0x85, 0x03, 0x95, 0x0F, 0x75, 0x08, 0x26, 0xFF, 0x00, 0x15, 0x00,
    0x09, 0x01, 0x91, 0x02,
    0x85, 0x04, 0x95, 0x0F, 0x75, 0x08, 0x26, 0xFF, 0x00, 0x15, 0x00,
    0x09, 0x01, 0x91, 0x02,

    0xC0,
};

static const tusb_desc_device_t device_descriptor = {
    .bLength = sizeof(tusb_desc_device_t),
    .bDescriptorType = TUSB_DESC_DEVICE,
    .bcdUSB = 0x0200,
    .bDeviceClass = 0xEF,
    .bDeviceSubClass = 0x02,
    .bDeviceProtocol = 0x01,
    .bMaxPacketSize0 = CFG_TUD_ENDPOINT0_SIZE,
    .idVendor = 0x03C3,
    .idProduct = 0x1F01,
    .bcdDevice = 0x0100,
    .iManufacturer = STRID_MANUFACTURER,
    .iProduct = STRID_PRODUCT,
    .iSerialNumber = STRID_SERIAL,
    .bNumConfigurations = 0x01
};

static const uint8_t configuration_descriptor[] = {
    TUD_CONFIG_DESCRIPTOR(1, ITF_NUM_TOTAL, 0, CONFIG_TOTAL_LEN, 0, 100),
    TUD_HID_INOUT_DESCRIPTOR(ITF_NUM_HID, STRID_LANGID, HID_ITF_PROTOCOL_NONE,
                             sizeof(hid_report_descriptor), EPNUM_HID_OUT, EPNUM_HID_IN,
                             64, 1)
};

static const char *string_descriptors[] = {
    (const char[]) { 0x09, 0x04 },
    "ZWO",
    "ZWO Device",
    "123456"
};

const uint8_t *tud_descriptor_device_cb(void) {
    return (const uint8_t *) &device_descriptor;
}

const uint8_t *tud_descriptor_configuration_cb(uint8_t index) {
    (void) index;
    return configuration_descriptor;
}

const uint8_t *tud_hid_descriptor_report_cb(uint8_t instance) {
    (void) instance;
    return hid_report_descriptor;
}

const uint16_t *tud_descriptor_string_cb(uint8_t index, uint16_t langid) {
    static uint16_t descriptor[32];
    uint8_t count = 0;

    (void) langid;
    if (index == STRID_LANGID) {
        memcpy(&descriptor[1], string_descriptors[STRID_LANGID], 2);
        count = 1;
    } else {
        if (index >= (sizeof(string_descriptors) / sizeof(string_descriptors[0]))) {
            return NULL;
        }

        const char *string = string_descriptors[index];
        count = (uint8_t) strlen(string);
        if (count > 31) count = 31;

        for (uint8_t i = 0; i < count; ++i) {
            descriptor[1 + i] = string[i];
        }
    }

    descriptor[0] = (uint16_t) ((TUSB_DESC_STRING << 8) | (2 * count + 2));
    return descriptor;
}
