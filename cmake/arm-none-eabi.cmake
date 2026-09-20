set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR arm)
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

set(ARM_GCC_PREFIX "arm-none-eabi-" CACHE STRING
    "Prefix or path prefix for the ARM GNU toolchain")

set(_arm_toolchain_hints)
if(APPLE AND ARM_GCC_PREFIX STREQUAL "arm-none-eabi-")
    file(GLOB _arm_toolchain_hints
        LIST_DIRECTORIES true
        "/Applications/ArmGNUToolchain/*/arm-none-eabi/bin"
        "$ENV{HOME}/.local/toolchains/*/bin")
    list(SORT _arm_toolchain_hints ORDER DESCENDING)
endif()

find_program(ARM_GCC_COMPILER NAMES "${ARM_GCC_PREFIX}gcc"
    HINTS ${_arm_toolchain_hints})
if(ARM_GCC_COMPILER)
    get_filename_component(ARM_GCC_BIN_DIR "${ARM_GCC_COMPILER}" DIRECTORY)
    find_program(ARM_GXX_COMPILER NAMES "${ARM_GCC_PREFIX}g++"
        HINTS "${ARM_GCC_BIN_DIR}")
    find_program(ARM_ASM_COMPILER NAMES "${ARM_GCC_PREFIX}gcc"
        HINTS "${ARM_GCC_BIN_DIR}")
    find_program(ARM_OBJCOPY NAMES "${ARM_GCC_PREFIX}objcopy"
        HINTS "${ARM_GCC_BIN_DIR}")
    find_program(ARM_OBJDUMP NAMES "${ARM_GCC_PREFIX}objdump"
        HINTS "${ARM_GCC_BIN_DIR}")
    find_program(ARM_SIZE NAMES "${ARM_GCC_PREFIX}size"
        HINTS "${ARM_GCC_BIN_DIR}")
else()
    set(ARM_GCC_COMPILER "${ARM_GCC_PREFIX}gcc")
    set(ARM_GXX_COMPILER "${ARM_GCC_PREFIX}g++")
    set(ARM_ASM_COMPILER "${ARM_GCC_PREFIX}gcc")
    set(ARM_OBJDUMP "${ARM_GCC_PREFIX}objdump")
endif()

set(CMAKE_C_COMPILER "${ARM_GCC_COMPILER}" CACHE FILEPATH "ARM C compiler" FORCE)
set(CMAKE_CXX_COMPILER "${ARM_GXX_COMPILER}" CACHE FILEPATH "ARM C++ compiler" FORCE)
set(CMAKE_ASM_COMPILER "${ARM_ASM_COMPILER}" CACHE FILEPATH "ARM assembler" FORCE)
