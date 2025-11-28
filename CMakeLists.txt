cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# initalize pico_sdk from installed location
# (note this can come from environment, CMake cache etc)
set(PICO_SDK_PATH "/Users/oein/oein/pico/pico-sdk")

# Pull in Pico SDK (must be before project)
include(pico_sdk_import.cmake)

project(projectx C CXX ASM)

set(CFG_TUD_HID 1)
set(CFG_TUD_ENABLED 1)
set(PICO_CXX_ENABLE_EXCEPTIONS 1)
set(PICO_CXX_ENABLE_RTTI 1)

# initialize the Raspberry Pi Pico SDK
pico_sdk_init()


add_executable(projectx
    src/main.cpp
    src/usb_descriptors.c
    src/tusb_config.h
)


# pico_generate_pio_header(projectx ${CMAKE_CURRENT_LIST_DIR}/src/WS2812/WS2812.pio)
# pico_generate_pio_header(projectx ${CMAKE_CURRENT_LIST_DIR}/src/WS2812/WS2812.pio)
pico_set_program_name(projectx "IIDX")
pico_set_program_version(projectx "1.0")

# Make sure TinyUSB can find tusb_config.h
target_include_directories(projectx PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}/src)

# Add pico_stdlib library which aggregates commonly used features
target_link_libraries(projectx PUBLIC pico_stdlib pico_unique_id tinyusb_device tinyusb_board hardware_pio hardware_spi hardware_adc)

pico_enable_stdio_usb(projectx 1)
pico_enable_stdio_uart(projectx 0)
# create map/bin/hex/uf2 file in addition to ELF.
pico_add_extra_outputs(projectx)