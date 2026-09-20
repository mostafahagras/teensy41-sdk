cmake_minimum_required(VERSION 3.20)

project(teensy41 C)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(CheckIncludeFile)
check_include_file(stdint.h TEENSY_HAS_STDINT_H)
if(NOT TEENSY_HAS_STDINT_H)
    message(FATAL_ERROR
        "The selected ARM toolchain has no target C headers. Install the complete "
        "Arm GNU Toolchain for Embedded (including newlib), not the compiler-only "
        "Homebrew arm-none-eabi-gcc formula.")
endif()

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)

set(TEENSY_CPU_FLAGS
    -mcpu=cortex-m7
    -mthumb
    -mfpu=fpv5-d16
    -mfloat-abi=hard)

set(TEENSY_DEFINITIONS
    __IMXRT1062__
    ARDUINO_TEENSY41
    F_CPU=600000000)

execute_process(
    COMMAND "${CMAKE_C_COMPILER}" -print-sysroot
    OUTPUT_VARIABLE ARM_GCC_SYSROOT
    OUTPUT_STRIP_TRAILING_WHITESPACE
    RESULT_VARIABLE ARM_GCC_SYSROOT_RESULT)
if(ARM_GCC_SYSROOT_RESULT EQUAL 0 AND IS_DIRECTORY "${ARM_GCC_SYSROOT}/include")
    set(TEENSY_SYSROOT_INCLUDE_FLAG "-isystem${ARM_GCC_SYSROOT}/include")
endif()
execute_process(
    COMMAND "${CMAKE_C_COMPILER}" -print-file-name=include
    OUTPUT_VARIABLE ARM_GCC_INCLUDE
    OUTPUT_STRIP_TRAILING_WHITESPACE
    RESULT_VARIABLE ARM_GCC_INCLUDE_RESULT)
if(ARM_GCC_INCLUDE_RESULT EQUAL 0 AND IS_DIRECTORY "${ARM_GCC_INCLUDE}")
    set(TEENSY_GCC_INCLUDE_FLAG "-isystem${ARM_GCC_INCLUDE}")
endif()

set(TEENSY_LINKER_SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/sdk/linker/imxrt1062_t41.ld")

add_subdirectory(sdk)

add_executable(teensy41 src/main.c)
target_link_libraries(teensy41 PRIVATE teensy_sdk)

target_compile_definitions(teensy41 PRIVATE ${TEENSY_DEFINITIONS})
target_compile_options(teensy41 PRIVATE
    ${TEENSY_CPU_FLAGS}
    ${TEENSY_SYSROOT_INCLUDE_FLAG}
    ${TEENSY_GCC_INCLUDE_FLAG}
    -Wall
    -Wextra
    -ffunction-sections
    -fdata-sections
    -Os)

target_link_options(teensy41 PRIVATE
    ${TEENSY_CPU_FLAGS}
    -nostdlib
    -nostartfiles
    -nodefaultlibs
    "-T${TEENSY_LINKER_SCRIPT}"
    -Wl,--gc-sections
    -Wl,--relax
    "-Wl,-Map,${CMAKE_CURRENT_BINARY_DIR}/teensy41.map")

target_link_libraries(teensy41 PRIVATE gcc)
set_property(TARGET teensy41 APPEND PROPERTY LINK_DEPENDS "${TEENSY_LINKER_SCRIPT}")

find_program(ARM_SIZE arm-none-eabi-size)
find_program(ARM_OBJCOPY arm-none-eabi-objcopy)
find_program(TEENSY_LOADER_CLI teensy_loader_cli)

if(ARM_SIZE)
    add_custom_command(TARGET teensy41 POST_BUILD
        COMMAND "${ARM_SIZE}" "$<TARGET_FILE:teensy41>"
        VERBATIM)
endif()

if(ARM_OBJDUMP)
    set(TEENSY41_ASM "${CMAKE_CURRENT_BINARY_DIR}/teensy41.s")
    add_custom_command(TARGET teensy41 POST_BUILD
        COMMAND sh -c
            "\"${ARM_OBJDUMP}\" -d \"$<TARGET_FILE:teensy41>\" > \"${TEENSY41_ASM}\""
        BYPRODUCTS "${TEENSY41_ASM}"
        VERBATIM)
else()
    message(STATUS "arm-none-eabi-objdump not found; disassembly is unavailable")
endif()

if(ARM_OBJCOPY)
    set(TEENSY41_HEX "${CMAKE_CURRENT_BINARY_DIR}/teensy41.hex")
    set(TEENSY41_BIN "${CMAKE_CURRENT_BINARY_DIR}/teensy41.bin")

    add_custom_command(OUTPUT "${TEENSY41_HEX}"
        COMMAND "${ARM_OBJCOPY}" -O ihex -R .eeprom
            "$<TARGET_FILE:teensy41>" "${TEENSY41_HEX}"
        DEPENDS teensy41
        VERBATIM)
    add_custom_command(OUTPUT "${TEENSY41_BIN}"
        COMMAND "${ARM_OBJCOPY}" -O binary
            "$<TARGET_FILE:teensy41>" "${TEENSY41_BIN}"
        DEPENDS teensy41
        VERBATIM)

    add_custom_target(hex ALL DEPENDS "${TEENSY41_HEX}")
    add_custom_target(bin DEPENDS "${TEENSY41_BIN}")

    if(TEENSY_LOADER_CLI)
        add_custom_target(flash
            COMMAND "${TEENSY_LOADER_CLI}" --mcu=TEENSY41 -w "${TEENSY41_HEX}"
            DEPENDS hex
            USES_TERMINAL
            VERBATIM)
    else()
        message(STATUS "teensy_loader_cli not found; the flash target is unavailable")
    endif()
else()
    message(STATUS "arm-none-eabi-objcopy not found; HEX/BIN targets are unavailable")
endif()
