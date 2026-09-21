# Teensy 4.1 C

Minimal bare-metal C project for the Teensy 4.1 using CMake and Ninja. It
does not depend on the Arduino or Teensyduino core.

## Requirements

- CMake 3.20 or newer
- Ninja
- The complete Arm GNU Toolchain for Embedded (`arm-none-eabi-gcc`, newlib,
  `arm-none-eabi-objcopy`, `arm-none-eabi-objdump`, and `arm-none-eabi-size`)
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
./scripts/configure
./scripts/compile
```

Configuration also generates `build/compile_commands.json`. The `.clangd`
file points clangd at that database and selects the ARM target, so run the
CMake configure command once before opening the project in an editor.

## Format and lint

Install LLVM to use the project formatting and linting scripts:

```sh
./scripts/format
./scripts/format --check
./scripts/lint
```

`./scripts/format` formats the project C sources and headers in place.

## Serial monitor

Run `./scripts/monitor` to print USB serial output from the Teensy. It finds
the `/dev/cu.usbmodem*` device automatically and reconnects after uploads or
board resets. Stop it with `Ctrl-C`.
`--check` verifies formatting without changing files. Linting uses the ARM
compile database, so run `./scripts/configure` first after changing CMake
configuration.

If the complete ARM toolchain is in another location, set its executable
prefix during configuration:

```sh
cmake --fresh --preset teensy41 \
  -DARM_GCC_PREFIX=/path/to/arm-gnu-toolchain/bin/arm-none-eabi-
```

The default build produces `build/teensy41`, `build/teensy41.s`, and
`build/teensy41.hex`. The assembly dump is refreshed automatically after a
successful link.

The `bin` target produces a raw binary:

```sh
cmake --build build --target bin
```

With `teensy_loader_cli` installed, press the Teensy program button and run:

```sh
cmake --build build --target flash
```

The application entry point is `src/main.c`. The platform implementation is
under `sdk/`, with public headers in `sdk/include/teensy/`.

The SDK currently provides GPIO, GPIO interrupts, PWM, ADC, clocks, timing,
UART, I2C, USB CDC, a WDOG3 (RTWDOG) watchdog, and a compact `printf`
implementation. Include `<teensy/watchdog.h>` to start, refresh, inspect, or
disable the watchdog. `printf` output is sent only to USB CDC after the host
has enumerated the device.
