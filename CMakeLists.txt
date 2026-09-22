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
find_program(CMAKE_MAKE_PROGRAM_EXE NAMES make)

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
        # Prefer the locally built binary; fall back to PATH versions.
        set(TEENSY_LOADER_CLI "${TEENSY_LOADER_CLI_LOCAL}"
            CACHE FILEPATH "teensy_loader_cli executable" FORCE)
    endif()
endif()

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
            COMMAND "${TEENSY_LOADER_CLI}" --mcu=TEENSY41 -svw "${TEENSY41_HEX}"
            DEPENDS hex
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
