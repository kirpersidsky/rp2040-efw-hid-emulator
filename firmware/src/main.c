#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "bsp/board_api.h"
#include "pico/stdlib.h"
#include "tusb.h"

#include "usb_descriptors.h"

// Capture-backed only: data bytes after Feature Report ID 3.
static const uint8_t captured_status_request_prefix[] = {0x7E, 0x5A, 0x02, 0x01};

// Capture-backed only: initial slot-1 data bytes after Feature Report ID 1.
// TinyUSB prepends the requested ID for a control GET_REPORT with a nonzero ID.
static const uint8_t captured_status_response_data[] = {
    0x7E, 0x5A, 0x01, 0x01, 0x00, 0x01, 0x01, 0x01,
    0x07, 0x00, 0x00, 0x00, 0x00, 0x2B, 0x01,
};

// Capture-backed only: final idle slot-2 data bytes after Feature Report ID 1.
static const uint8_t captured_slot2_final_idle_status_response_data[] = {
    0x7E, 0x5A, 0x01, 0x01, 0x00, 0x02, 0x02, 0x02,
    0x07, 0x00, 0x00, 0x00, 0x00, 0x2B, 0x01,
};

static const uint8_t captured_slot2_transition_request_data[] = {
    0x7E, 0x5A, 0x01, 0x02, 0x02, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

// Capture-backed only: final idle slot-3 data bytes after Feature Report ID 1.
static const uint8_t captured_slot3_final_idle_status_response_data[] = {
    0x7E, 0x5A, 0x01, 0x01, 0x00, 0x03, 0x03, 0x03,
    0x07, 0x00, 0x00, 0x00, 0x00, 0x2B, 0x01,
};

static const uint8_t captured_slot3_transition_request_data[] = {
    0x7E, 0x5A, 0x01, 0x02, 0x03, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

static const uint8_t captured_slot1_transition_request_data[] = {
    0x7E, 0x5A, 0x01, 0x02, 0x01, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

// Multi-source evidence only: data bytes after Feature Report ID 3.
static const uint8_t captured_handshake_request_prefix[] = {0x7E, 0x5A, 0x02, 0x04};

// Cynthion frame supplied by the source as a labeled "handshake" response;
// these are the data bytes after Feature Report ID 1. TinyUSB prepends 01,
// producing: 01 7E 5A 04 03 00 09 00 45 46 57 2D 53 2D 30 00.
static const uint8_t captured_handshake_response_data[] = {
    0x7E, 0x5A, 0x04, 0x03, 0x00, 0x09, 0x00, 0x45,
    0x46, 0x57, 0x2D, 0x53, 0x2D, 0x30, 0x00,
};

static const uint8_t captured_serial_request_data[] = {
    0x7E, 0x5A, 0x02, 0x0C, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

static const uint8_t captured_serial_response_data[] = {
    0x7E, 0x5A, 0x0C, 0x01, 0x0F, 0x02, 0x01, 0x02,
    0x00, 0x07, 0x03, 0xDC, 0xEF, 0x2B, 0x01,
};

static const uint8_t captured_alias_request_data[] = {
    0x7E, 0x5A, 0x02, 0x0D, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

static const uint8_t captured_alias_response_data[] = {
    0x7E, 0x5A, 0x0D, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00,
};

_Static_assert(sizeof(captured_serial_response_data) == 15u,
               "serial response must contain 15 data bytes");
_Static_assert(sizeof(captured_alias_response_data) == 11u,
               "alias response must contain 11 data bytes");
_Static_assert(sizeof(captured_slot2_final_idle_status_response_data) == 15u,
               "slot-2 final idle status must contain 15 data bytes");
_Static_assert(sizeof(captured_slot2_transition_request_data) == 15u,
               "slot-2 transition request must contain 15 data bytes");
_Static_assert(sizeof(captured_slot3_final_idle_status_response_data) == 15u,
               "slot-3 final idle status must contain 15 data bytes");
_Static_assert(sizeof(captured_slot3_transition_request_data) == 15u,
               "slot-3 transition request must contain 15 data bytes");
_Static_assert(sizeof(captured_slot1_transition_request_data) == 15u,
               "slot-1 transition request must contain 15 data bytes");

typedef enum {
    CAPTURED_RESPONSE_NONE,
    CAPTURED_RESPONSE_STATUS_0201,
    CAPTURED_RESPONSE_HANDSHAKE_0204,
    CAPTURED_RESPONSE_SERIAL_020C,
    CAPTURED_RESPONSE_ALIAS_020D,
} captured_response_t;

typedef enum {
    CAPTURED_STATUS_PROFILE_SLOT_1,
    CAPTURED_STATUS_PROFILE_SLOT_2,
    CAPTURED_STATUS_PROFILE_SLOT_3,
} captured_status_profile_t;

static uint32_t report_sequence;
static captured_response_t captured_response_armed;
static captured_status_profile_t captured_status_profile;

static void log_bytes(const uint8_t *data, uint16_t length) {
    for (uint16_t i = 0; i < length; ++i) {
        printf("%02X%s", (unsigned int)data[i],
               (i + 1u == length) ? "" : " ");
    }
}

static const char *log_report_type(hid_report_type_t report_type) {
    switch (report_type) {
        case HID_REPORT_TYPE_INPUT:
            return "input";
        case HID_REPORT_TYPE_OUTPUT:
            return "output";
        case HID_REPORT_TYPE_FEATURE:
            return "feature";
        default:
            return "other";
    }
}

static void log_report(const char *callback, uint8_t instance, uint8_t report_id,
                       hid_report_type_t report_type, uint16_t length,
                       const uint8_t *bytes) {
    const uint32_t timestamp_ms = to_ms_since_boot(get_absolute_time());
    const uint32_t sequence = ++report_sequence;

    // Log the callback's Report ID exactly as supplied. Do not infer an ID
    // from payload bytes or from a USB control transfer not exposed here.
    // Use %u/%X with explicit unsigned-int casts: these specifiers are
    // supported by Pico's compact printf and match every variadic argument.
    printf("[t=%ums seq=%u] %s instance=%u callback_report_id=0x%02X "
           "report_type=%s report_type_raw=%u payload_length=%u payload=",
           (unsigned int)timestamp_ms, (unsigned int)sequence, callback,
           (unsigned int)instance, (unsigned int)report_id,
           log_report_type(report_type), (unsigned int)report_type,
           (unsigned int)length);
    if (bytes == NULL || length == 0) {
        printf("<none>");
    } else {
        log_bytes(bytes, length);
    }
    printf("\r\n");
}

int main(void) {
    board_init();
    stdio_init_all();
    tusb_init();

    printf("EFW HID initialized: only capture-backed 02 01, 02 04, 02 0C, and 02 0D responses are enabled\r\n");

    while (true) {
        tud_task();
    }
}

void tud_hid_set_report_cb(uint8_t instance, uint8_t report_id,
                           hid_report_type_t report_type,
                           const uint8_t *buffer, uint16_t bufsize) {
    log_report("tud_hid_set_report_cb", instance, report_id, report_type,
               bufsize, buffer);

    // For a control SET_REPORT with Report ID 3, TinyUSB has already removed
    // the ID before invoking this callback. Do not require observed padding:
    // the callback can expose only the report-data portion.
    if (report_type == HID_REPORT_TYPE_FEATURE && report_id == 0x03 &&
        bufsize >= sizeof(captured_status_request_prefix) &&
        memcmp(buffer, captured_status_request_prefix,
               sizeof(captured_status_request_prefix)) == 0) {
        captured_response_armed = CAPTURED_RESPONSE_STATUS_0201;
        printf("EFW pairing: capture-backed 02 01 request armed\r\n");
        return;
    }

    // This is deliberately a four-byte command match, not a generic 02
    // prefix: only the observed 02 04 request can arm the handshake replay.
    if (report_type == HID_REPORT_TYPE_FEATURE && report_id == 0x03 &&
        bufsize == 15u &&
        memcmp(buffer, captured_handshake_request_prefix,
               sizeof(captured_handshake_request_prefix)) == 0) {
        captured_response_armed = CAPTURED_RESPONSE_HANDSHAKE_0204;
        printf("EFW pairing: 02 04 handshake request armed\r\n");
        return;
    }

    if (report_type == HID_REPORT_TYPE_FEATURE && report_id == 0x03 &&
        bufsize == sizeof(captured_serial_request_data) &&
        memcmp(buffer, captured_serial_request_data,
               sizeof(captured_serial_request_data)) == 0) {
        captured_response_armed = CAPTURED_RESPONSE_SERIAL_020C;
        printf("EFW pairing: 02 0C serial request armed\r\n");
        return;
    }

    if (report_type == HID_REPORT_TYPE_FEATURE && report_id == 0x03 &&
        bufsize == sizeof(captured_alias_request_data) &&
        memcmp(buffer, captured_alias_request_data,
               sizeof(captured_alias_request_data)) == 0) {
        captured_response_armed = CAPTURED_RESPONSE_ALIAS_020D;
        printf("EFW pairing: 02 0D alias request armed\r\n");
        return;
    }

    if (report_type == HID_REPORT_TYPE_FEATURE && report_id == 0x03 &&
        bufsize == sizeof(captured_slot2_transition_request_data) &&
        memcmp(buffer, captured_slot2_transition_request_data,
               sizeof(captured_slot2_transition_request_data)) == 0) {
        captured_status_profile = CAPTURED_STATUS_PROFILE_SLOT_2;
        captured_response_armed = CAPTURED_RESPONSE_NONE;
        printf("EFW state: 01 02 02 accepted; selected capture-backed slot-2 final-idle status (time-compressed, no response armed)\r\n");
        return;
    }

    if (report_type == HID_REPORT_TYPE_FEATURE && report_id == 0x03 &&
        bufsize == sizeof(captured_slot3_transition_request_data) &&
        memcmp(buffer, captured_slot3_transition_request_data,
               sizeof(captured_slot3_transition_request_data)) == 0) {
        captured_status_profile = CAPTURED_STATUS_PROFILE_SLOT_3;
        captured_response_armed = CAPTURED_RESPONSE_NONE;
        printf("EFW state: 01 02 03 accepted; selected capture-backed slot-3 final-idle status (time-compressed, no response armed)\r\n");
        return;
    }

    if (report_type == HID_REPORT_TYPE_FEATURE && report_id == 0x03 &&
        bufsize == sizeof(captured_slot1_transition_request_data) &&
        memcmp(buffer, captured_slot1_transition_request_data,
               sizeof(captured_slot1_transition_request_data)) == 0) {
        captured_status_profile = CAPTURED_STATUS_PROFILE_SLOT_1;
        captured_response_armed = CAPTURED_RESPONSE_NONE;
        printf("EFW state: 01 02 01 accepted; selected capture-backed slot-1 final-idle status (time-compressed, no response armed)\r\n");
        return;
    }

    // An unknown request cannot retain a prior pairing.
    captured_response_armed = CAPTURED_RESPONSE_NONE;
    printf("EFW pairing: unknown request rejected; no response armed\r\n");
}

uint16_t tud_hid_get_report_cb(uint8_t instance, uint8_t report_id,
                               hid_report_type_t report_type,
                               uint8_t *buffer, uint16_t reqlen) {
    log_report("tud_hid_get_report_cb", instance, report_id, report_type,
               reqlen, NULL);

    // TinyUSB writes Report ID 01 into the control response. ASIAIR's
    // observed callback request length is 16; returning 15 data bytes
    // produces a 16-byte USB status frame with the report ID.
    if (report_type == HID_REPORT_TYPE_FEATURE && report_id == 0x01 &&
        captured_response_armed == CAPTURED_RESPONSE_STATUS_0201 &&
        reqlen >= sizeof(captured_status_response_data)) {
        const uint8_t *status_response_data = captured_status_response_data;
        const char *status_profile_name = "slot-1";
        if (captured_status_profile == CAPTURED_STATUS_PROFILE_SLOT_2) {
            status_response_data = captured_slot2_final_idle_status_response_data;
            status_profile_name = "slot-2";
        } else if (captured_status_profile == CAPTURED_STATUS_PROFILE_SLOT_3) {
            status_response_data = captured_slot3_final_idle_status_response_data;
            status_profile_name = "slot-3";
        }
        memcpy(buffer, status_response_data, sizeof(captured_status_response_data));
        captured_response_armed = CAPTURED_RESPONSE_NONE;
        printf("EFW GetReport: capture-backed %s status returned (15 data bytes)\r\n",
               status_profile_name);
        return sizeof(captured_status_response_data);
    }

    if (report_type == HID_REPORT_TYPE_FEATURE && report_id == 0x01 &&
        captured_response_armed == CAPTURED_RESPONSE_HANDSHAKE_0204 &&
        reqlen >= sizeof(captured_handshake_response_data)) {
        memcpy(buffer, captured_handshake_response_data,
               sizeof(captured_handshake_response_data));
        captured_response_armed = CAPTURED_RESPONSE_NONE;
        printf("EFW GetReport: 02 04 capture-backed handshake returned (15 data bytes)\r\n");
        return sizeof(captured_handshake_response_data);
    }

    if (report_type == HID_REPORT_TYPE_FEATURE && report_id == 0x01 &&
        captured_response_armed == CAPTURED_RESPONSE_SERIAL_020C &&
        reqlen >= sizeof(captured_serial_response_data)) {
        memcpy(buffer, captured_serial_response_data,
               sizeof(captured_serial_response_data));
        captured_response_armed = CAPTURED_RESPONSE_NONE;
        printf("EFW GetReport: 02 0C capture-backed serial returned (15 data bytes)\r\n");
        return sizeof(captured_serial_response_data);
    }

    if (report_type == HID_REPORT_TYPE_FEATURE && report_id == 0x01 &&
        captured_response_armed == CAPTURED_RESPONSE_ALIAS_020D &&
        reqlen >= sizeof(captured_alias_response_data)) {
        memcpy(buffer, captured_alias_response_data,
               sizeof(captured_alias_response_data));
        captured_response_armed = CAPTURED_RESPONSE_NONE;
        printf("EFW GetReport: 02 0D capture-backed alias returned (11 data bytes)\r\n");
        return sizeof(captured_alias_response_data);
    }

    captured_response_armed = CAPTURED_RESPONSE_NONE;
    printf("EFW GetReport: unknown or unpaired request rejected; returning 0 data bytes\r\n");
    return 0;
}
