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

# Opt-in link-time optimization: -flto on every compile unit - the SDK
# library included - and at the link.  OFF by default; both reference
# SDKs (pico, Teensyduino) also build without LTO.
option(TEENSY_LTO "Enable link-time optimization (cross-module inlining)" OFF)

# Compile/link setup shared by the main application target and every
# example; callers append their own -Wl,-Map when they want one.
function(teensy_target_options target)
    cmake_parse_arguments(OPT "WITH_MAP" "MAPFILE" "" ${ARGN})
    add_executable(${target} ${OPT_UNPARSED_ARGUMENTS})
    target_link_libraries(${target} PRIVATE teensy_sdk)

    target_compile_definitions(${target} PRIVATE ${TEENSY_DEFINITIONS})
    set(TEENSY_LTO_FLAGS "")
    if(TEENSY_LTO)
        set(TEENSY_LTO_FLAGS -ffat-lto-objects)
        set(TEENSY_LTO_LINK_FLAGS -flto -flto-partition=none)
    endif()
    target_compile_options(${target} PRIVATE
        ${TEENSY_CPU_FLAGS}
        ${TEENSY_SYSROOT_INCLUDE_FLAG}
        ${TEENSY_GCC_INCLUDE_FLAG}
        -Wall
        -Wextra
        -ffunction-sections
        -fdata-sections
        -Os
        ${TEENSY_LTO_FLAGS})

    target_link_options(${target} PRIVATE
        ${TEENSY_CPU_FLAGS}
        ${TEENSY_LTO_LINK_FLAGS}
        -nostdlib
        -nostartfiles
        -nodefaultlibs
        "-T${TEENSY_LINKER_SCRIPT}"
        -Wl,--gc-sections
        -Wl,--relax
        ${TEENSY_LTO_FLAGS}
        # One RWX load segment is intentional: the ITCM image is loaded from
        # flash and copied to RAM at startup, so text/code/data share a
        # segment (same layout as Teensyduino binaries).  Silence the
        # binutils 2.39+ RWX warning.
        -Wl,--no-warn-rwx-segments)

    target_link_libraries(${target} PRIVATE gcc)
    set_property(TARGET ${target} APPEND PROPERTY LINK_DEPENDS "${TEENSY_LINKER_SCRIPT}")
    if(OPT_WITH_MAP AND OPT_MAPFILE)
        target_link_options(${target} PRIVATE
            "-Wl,-Map,${CMAKE_CURRENT_BINARY_DIR}/${OPT_MAPFILE}")
    endif()
endfunction()

# Per-target HEX/BIN generation (main and examples).
function(teensy_add_hex_target target)
    set(hex "${CMAKE_CURRENT_BINARY_DIR}/${target}.hex")
    set(bin "${CMAKE_CURRENT_BINARY_DIR}/${target}.bin")
    add_custom_command(OUTPUT "${hex}"
        COMMAND "${ARM_OBJCOPY}" -O ihex -R .eeprom
            "$<TARGET_FILE:${target}>" "${hex}"
        DEPENDS ${target}
        VERBATIM)
    add_custom_command(OUTPUT "${bin}"
        COMMAND "${ARM_OBJCOPY}" -O binary
            "$<TARGET_FILE:${target}>" "${bin}"
        DEPENDS ${target}
        VERBATIM)
    add_custom_target(${target}-hex DEPENDS "${hex}")
    add_custom_target(${target}-bin DEPENDS "${bin}")

    if(TARGET vendored_loader_cli)
        add_custom_target(flash-${target} DEPENDS ${target}-hex vendored_loader_cli)
    else()
        add_custom_target(flash-${target} DEPENDS ${target}-hex)
    endif()
endfunction()

find_program(ARM_SIZE arm-none-eabi-size)
find_program(ARM_OBJCOPY arm-none-eabi-objcopy)
find_program(ARM_OBJDUMP arm-none-eabi-objdump)
find_program(TEENSY_LOADER_CLI teensy_loader_cli)

# The vendored CLI adds macOS soft-reboot (no button press needed); it is
# built from tools/teensy_loader_cli with a custom_target below and preferred
# over whatever PATH provides.  The binary itself is not tracked in git.
set(TEENSY_LOADER_CLI_LOCAL
    "${CMAKE_CURRENT_SOURCE_DIR}/tools/teensy_loader_cli/teensy_loader_cli")

if(CMAKE_HOST_APPLE)
    # Apple's system "MacOSX.sdk" can ship .tbd files this toolchain cannot
    # parse (the SDK 27, which lists arm64e.x1-macos).  Pick the newest SDK
    # whose tbd files cmake can actually read.
    file(GLOB _sdk_candidates
        "/Library/Developer/CommandLineTools/SDKs/MacOSX*.sdk"
        "/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX*.sdk")
    set(_usable_sdks "")
    foreach(candidate IN LISTS _sdk_candidates)
        if(EXISTS "${candidate}/usr/lib/libSystem.B.tbd")
            file(READ "${candidate}/usr/lib/libSystem.B.tbd" _tbd LIMIT 8000)
            if(NOT _tbd MATCHES "x1-macos")
                list(APPEND _usable_sdks "${candidate}")
            endif()
        endif()
    endforeach()
    if(_usable_sdks)
        list(SORT _usable_sdks)
        list(REVERSE _usable_sdks)
        list(GET _usable_sdks 0 TEENSY_LOADER_SDK)
    endif()

    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/tools/teensy_loader_cli/Makefile")
        add_custom_command(
            OUTPUT "${CMAKE_CURRENT_SOURCE_DIR}/tools/teensy_loader_cli/teensy_loader_cli"
            COMMAND make OS=MACOSX "SDK=${TEENSY_LOADER_SDK}"
            WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/tools/teensy_loader_cli"
            DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/tools/teensy_loader_cli/teensy_loader_cli.c"
            COMMENT "Building vendored teensy_loader_cli"
            VERBATIM)
        add_custom_target(vendored_loader_cli
            DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/tools/teensy_loader_cli/teensy_loader_cli")
        set(TEENSY_LOADER_CLI "${TEENSY_LOADER_CLI_LOCAL}"
            CACHE FILEPATH "teensy_loader_cli executable" FORCE)
    endif()
endif()

# Main application (src/main.c).
teensy_target_options(teensy41 WITH_MAP MAPFILE teensy41.map src/main.c)
set_property(TARGET teensy41 APPEND PROPERTY LINK_DEPENDS "${TEENSY_LINKER_SCRIPT}")

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
    teensy_add_hex_target(teensy41)

    if(TEENSY_LOADER_CLI)
        add_custom_target(flash
            COMMAND "${TEENSY_LOADER_CLI}" --mcu=TEENSY41 -svw
                "${CMAKE_CURRENT_BINARY_DIR}/teensy41.hex"
            DEPENDS teensy41-hex
            USES_TERMINAL
            VERBATIM)
        if(TARGET vendored_loader_cli)
            add_dependencies(flash vendored_loader_cli)
        endif()
    else()
        message(STATUS "teensy_loader_cli not found; the flash target is unavailable")
    endif()
else()
    message(STATUS "arm-none-eabi-objcopy not found; HEX/BIN targets are unavailable")
endif()

# One example per feature; targets: example-<name>, flash-<name>.
add_subdirectory(examples)
