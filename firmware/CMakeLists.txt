cmake_minimum_required(VERSION 3.13)

include(pico_sdk_import.cmake)

project(zwo_efw_emulator C CXX ASM)

pico_sdk_init()

add_executable(zwo_efw_emulator
    src/main.c
    src/usb_descriptors.c
)

target_include_directories(zwo_efw_emulator PRIVATE src)

target_link_libraries(zwo_efw_emulator PRIVATE
    pico_stdlib
    tinyusb_board
    tinyusb_device
)

# UART logging prevents a second USB interface from changing the HID device.
pico_enable_stdio_uart(zwo_efw_emulator 1)
pico_enable_stdio_usb(zwo_efw_emulator 0)

pico_add_extra_outputs(zwo_efw_emulator)
