add_library(teensy_sdk OBJECT
    src/bootdata.c
    src/cache.c
    src/gpio.c
    src/clock.c
    src/i2c.c
    src/memory.c
    src/pwm.c
    src/printf.c
    src/printf_backend.c
    src/startup.c
    src/time.c
    src/uart.c
    src/usb_api.c
    src/usb.c
    src/usb_desc.c
    src/usb_serial.c)

target_include_directories(teensy_sdk
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/include")

target_compile_definitions(teensy_sdk PRIVATE ${TEENSY_DEFINITIONS} USB_SERIAL)
target_compile_options(teensy_sdk PRIVATE
    ${TEENSY_CPU_FLAGS}
    ${TEENSY_SYSROOT_INCLUDE_FLAG}
    ${TEENSY_GCC_INCLUDE_FLAG}
    -Wall
    -Wextra
    -ffunction-sections
    -fdata-sections
    -Os)
