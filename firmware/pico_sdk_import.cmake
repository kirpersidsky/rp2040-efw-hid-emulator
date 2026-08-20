if (DEFINED ENV{PICO_SDK_PATH} AND NOT PICO_SDK_PATH)
    set(PICO_SDK_PATH "$ENV{PICO_SDK_PATH}")
endif ()

if (NOT PICO_SDK_PATH OR NOT EXISTS "${PICO_SDK_PATH}/pico_sdk_init.cmake")
    message(FATAL_ERROR
        "Set PICO_SDK_PATH to the root of a Raspberry Pi Pico SDK checkout.")
endif ()

include("${PICO_SDK_PATH}/pico_sdk_init.cmake")
