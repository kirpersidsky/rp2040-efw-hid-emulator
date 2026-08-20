#ifndef TUSB_CONFIG_H_
#define TUSB_CONFIG_H_

#include "tusb_option.h"

#define CFG_TUSB_MCU            OPT_MCU_RP2040
#define CFG_TUSB_OS             OPT_OS_PICO
#define CFG_TUSB_RHPORT0_MODE   (OPT_MODE_DEVICE | OPT_MODE_FULL_SPEED)

#define CFG_TUD_ENDPOINT0_SIZE  64
#define CFG_TUD_HID             1
#define CFG_TUD_HID_EP_BUFSIZE  64

#endif
