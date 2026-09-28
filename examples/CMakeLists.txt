# One example per feature.  Each builds example-<name>, a hex at
# example-<name>.hex, and a flash-<name> target using the vendored loader.

set(TEENSY_EXAMPLES
    blink
    gpio
    pwm
    timers
    rtc
    temperature
    nvram
    dma
    trng
    uart)

foreach(name IN LISTS TEENSY_EXAMPLES)
    teensy_target_options(example-${name}
        "${CMAKE_CURRENT_SOURCE_DIR}/${name}/${name}.c")
    teensy_add_hex_target(example-${name})
endforeach()
