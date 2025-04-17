# Belvedere

![Everyone's favorite snarky British butler who ran an American househould better than the dad ever could](mr-belvedere.jpg)

Belvedere is a cross-platform HID device monitoring application that allows you to bind keyboard events to system commands. It's particularly useful for managing keyboard LED states and other keyboard-related functionality.

## Features

- Cross-platform support (macOS and Linux)
- Monitor specific keyboard keycodes
- Execute commands based on key events
- Hot-reload configuration without restarting
- Support for QMK custom keycodes

## Requirements

- C compiler (GCC or Clang)
- CMake 3.10 or higher
- libhidapi
- libuv

### Installing Dependencies

#### macOS

```bash
brew install cmake hidapi libuv
```

#### Ubuntu/Debian

```bash
sudo apt-get install cmake libhidapi-dev libuv1-dev
```

#### Fedora

```bash
sudo dnf install cmake hidapi-devel libuv-devel
```

#### Arch Linux

```bash
sudo pacman -S cmake hidapi libuv
```

## Building

### Using Make

```bash
make
```

This will create a `build` directory and build the project inside it.

### Using CMake Directly

```bash
mkdir build
cd build
cmake ..
make
```

### Build Options

- `ENABLE_DEBUG`: Enable debug build (default: OFF)
- `BUILD_TESTS`: Build test suite (default: ON)
- `ENABLE_COVERAGE`: Enable code coverage (default: OFF)

Example:

```bash
cmake -DENABLE_DEBUG=ON -DBUILD_TESTS=ON ..
```

## Testing

Run the test suite:

```bash
make test
```

Generate code coverage report:

```bash
make coverage
```

## Installation

Install the application:

```bash
make install
```

Uninstall the application:

```bash
make uninstall
```

## Configuration

Belvedere uses a configuration file located at `~/.config/belvedere/config`. The configuration file has the following format:

```
[general]
setleds = /usr/local/bin/setleds
monitored_keycodes = 57,71,83,111,112

[0x5043/0x54a3]
target = "X.Tips*"
111 = +scroll
112 = -scroll

[0x5262/0x4e4b]
target = "Trackball*"
57 = ^caps
83 = ^num
71 = ^scroll
```

### Configuration Options

#### General Section

- `setleds`: Path to the setleds command (default: `/usr/local/bin/setleds`)
- `monitored_keycodes`: Comma-separated list of keycodes to monitor

#### Device Sections

Each device section is identified by its vendor ID and product ID in hexadecimal format: `[0xVID/0xPID]`

- `target`: Target device name (used with setleds)
- Key bindings: `keycode = mode+led`

### Key Binding Modes

- `^`: Toggle LED state
- `+`: Turn LED on
- `-`: Turn LED off

## Usage

Start Belvedere:

```bash
belvedere
```

Enable debug logging:

```bash
belvedere -v
```

Reload configuration:

```bash
belvedere --reload
```

Or send a SIGHUP signal to a running instance:

```bash
kill -HUP $(pgrep belvedere)
```

## License

This project is licensed under the MIT License - see the LICENSE file for details.

## Contributing

Contributions are welcome! Please feel free to submit a Pull Request.