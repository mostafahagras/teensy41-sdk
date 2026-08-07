# Teensy 4.1 C

Minimal bare-metal C project for the Teensy 4.1 using CMake and Ninja. It
does not depend on the Arduino or Teensyduino core.

## Requirements

- CMake 3.20 or newer
- Ninja
- The complete Arm GNU Toolchain for Embedded (`arm-none-eabi-gcc`, newlib,
  `arm-none-eabi-objcopy`, and `arm-none-eabi-size`)
- `clangd` for editor language support
- `teensy_loader_cli` for the optional `flash` target

## Build

On macOS, install the complete Arm toolchain archive in a user-owned
directory:

```sh
mkdir -p "$HOME/.local/toolchains"
curl -L 'https://developer.arm.com/-/media/Files/downloads/gnu/15.2.rel1/binrel/arm-gnu-toolchain-15.2.rel1-darwin-arm64-arm-none-eabi.tar.xz' \
  | tar -xJ -C "$HOME/.local/toolchains"
```

Do not use the Homebrew formula named `arm-none-eabi-gcc`: it is compiler-only,
is built with `--without-headers`, and does not include newlib. The CMake
toolchain file automatically detects the archive under
`$HOME/.local/toolchains`.

```sh
cmake --preset teensy41
cmake --build --preset teensy41
```

Configuration also generates `build/compile_commands.json`. The `.clangd`
file points clangd at that database and selects the ARM target, so run the
CMake configure command once before opening the project in an editor.

If the complete ARM toolchain is in another location, set its executable
prefix during configuration:

```sh
cmake --fresh --preset teensy41 \
  -DARM_GCC_PREFIX=/path/to/arm-gnu-toolchain/bin/arm-none-eabi-
```

The default build produces `build/teensy41.elf` and `build/teensy41.hex`.
The `bin` target produces a raw binary:

```sh
cmake --build build --target bin
```

With `teensy_loader_cli` installed, press the Teensy program button and run:

```sh
cmake --build build --target flash
```

The application entry point is `src/main.c`. Hardware startup and memory
initialization are in `src/startup.c`; the Teensy 4.1 boot metadata is in
`src/bootdata.c`. This starter does not initialize USB, GPIO, or the CPU
clock; add those pieces to the application as needed.
