cmake_minimum_required(VERSION 3.13)

# initialize the SDK based on PICO_SDK_PATH
# note: this must happen before project()
include(pico_sdk_import.cmake)

project(picoOS)
enable_language( C CXX ASM )

# initialize the Raspberry Pi Pico SDK
pico_sdk_init()

if (TARGET tinyusb_device)

add_executable(picoOS
    picoOS.c
)

# Add pico_stdlib library which aggregates commonly used features
target_link_libraries(picoOS pico_stdlib pico_multicore hardware_adc)

pico_enable_stdio_usb(picoOS 1)
pico_enable_stdio_uart(picoOS 0)

# create map/bin/hex/uf2 file in addition to ELF.
pico_add_extra_outputs(picoOS)

elseif(PICO_ON_DEVICE)
    message(WARNING "not building hello_usb because TinyUSB submodule is not initialized in the SDK")
endif()
