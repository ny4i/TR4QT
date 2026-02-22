# Building TR4QT

This guide covers building TR4QT from source on Linux, macOS, and Windows.

## Quick Start

**Clone the repository:**
```bash
git clone https://github.com/ny4i/TR4QT.git
cd TR4QT
```

Then follow platform-specific instructions below.

## Table of Contents

- [Prerequisites](#prerequisites)
- [Building on macOS](#building-on-macos)
- [Building on Linux](#building-on-linux)
  - [Ubuntu/Debian](#ubuntudebian)
  - [Raspberry Pi](#raspberry-pi)
  - [Generic Linux](#generic-linux)
- [Building on Windows](#building-on-windows)
- [Qt-Specific Issues](#qt-specific-issues)
- [Linux Window Manager Issues](#linux-window-manager-issues)
- [Troubleshooting](#troubleshooting)

## Prerequisites

All platforms require:

- **C++ Compiler** with C++17 support (GCC 7+, Clang 5+, MSVC 2017+)
- **CMake** 3.16 or later
- **Qt 6.5** or later (Qt 6.10+ recommended)
  - Required modules: Core, Gui, Widgets, Network, Sql, SerialPort, PrintSupport, Concurrent, Svg, Xml, Multimedia, HttpServer, Quick, QuickWidgets, Qml, ShaderTools
  - Optional: GuiPrivate (enables panadapter waterfall display)
- **Hamlib** 4.0 or later (4.7+ recommended)
- **Git** (to clone the repository)

**Linux additionally requires:**
- **ALSA development libraries** (`libasound2-dev`) — for MIDI/CW keyer support
- **PulseAudio development libraries** (`libpulse-dev`) — Qt Multimedia audio backend

**Bundled Dependencies** (no installation required):
- **QCustomPlot** 2.1.1 — plotting/statistics (included in source tree)
- **qtkeychain** — secure credential storage (fetched automatically by CMake)

## Building on macOS

### Install Dependencies

#### Using Homebrew (Recommended)

[Homebrew](https://brew.sh) is the easiest way to install dependencies on macOS.

```bash
# Install Homebrew if not already installed
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"

# Install build tools
brew install cmake git

# Install Qt 6 (includes all required modules)
brew install qt@6

# Install Hamlib
brew install hamlib

# Add Qt to PATH (add to ~/.zshrc for persistence)
export PATH="/opt/homebrew/opt/qt@6/bin:$PATH"
export CMAKE_PREFIX_PATH="/opt/homebrew/opt/qt@6:$CMAKE_PREFIX_PATH"
```

**Note for Intel Macs:** Qt and Hamlib will be in `/usr/local` instead of `/opt/homebrew`.

#### Using Qt Online Installer

Alternatively, install Qt from the official installer:

1. Download from https://www.qt.io/download-qt-installer
2. Install Qt 6.5+ with macOS component
3. Install Hamlib via Homebrew: `brew install hamlib`
4. Set CMAKE_PREFIX_PATH when building (see below)

### Build TR4QT

```bash
# Clone the repository
git clone https://github.com/ny4i/TR4QT.git
cd TR4QT

# Configure and build
cmake -B build
cmake --build build -j$(sysctl -n hw.ncpu)

# Optional: Run tests
cd build && ctest --output-on-failure

# The app bundle will be at: build/src/tr4qt.app
```

### Running TR4QT

```bash
# From project root
./build/src/tr4qt.app/Contents/MacOS/tr4qt

# Or double-click tr4qt.app in Finder (in build/src/)

# Kill any running instances first (graceful shutdown)
pkill tr4qt && sleep 1
./build/src/tr4qt.app/Contents/MacOS/tr4qt
```

### macOS-Specific Notes

**Keyboard Shortcuts:**
- Qt automatically maps shortcuts to macOS conventions:
  - `Qt::ALT` → Option key
  - `Qt::CTRL` → Command key

**Permissions:**
- **Serial Port Access:** Grant Terminal/IDE permission to access USB devices in System Settings → Privacy & Security → Files and Folders

**Code Signing:**
For local testing, ad-hoc signing is sufficient:
```bash
codesign --force --deep --sign - build/src/tr4qt.app
```

## Building on Linux

### Ubuntu/Debian

#### Install Dependencies

```bash
# Update package lists
sudo apt update

# Install build tools
sudo apt install -y build-essential cmake git pkg-config

# Install Qt 6 development packages
sudo apt install -y \
    qt6-base-dev qt6-base-dev-tools qt6-base-private-dev \
    libqt6serialport6-dev \
    libqt6svg6-dev \
    qt6-multimedia-dev \
    qt6-httpserver-dev \
    qt6-declarative-dev \
    qt6-shadertools-dev

# Install system libraries required by Qt/RtMidi
sudo apt install -y libasound2-dev libpulse-dev

# Install Hamlib
sudo apt install -y libhamlib-dev libhamlib-utils

# Optional: Install SQLite (usually included with Qt)
sudo apt install -y libsqlite3-dev
```

**Note:** Package names may vary slightly between Ubuntu/Debian versions. If a package is not found, search with `apt search qt6` to find the correct name for your distribution.

**Note for Ubuntu 22.04 LTS and Debian 11:**
Qt 6.5+ may not be available in the default repositories. Options:

1. **Install Qt from Qt Online Installer**:
   - Download from https://www.qt.io/download-qt-installer
   - Install to `/opt/Qt` or `~/Qt`
   - Set CMAKE_PREFIX_PATH when building

2. **Use a newer distribution** (Ubuntu 24.04+ or Debian 12+ recommended)

#### Build TR4QT

```bash
# Clone the repository
git clone https://github.com/ny4i/TR4QT.git
cd TR4QT

# Configure and build
cmake -B build
cmake --build build -j$(nproc)

# If Qt is installed in a custom location:
# cmake -B build -DCMAKE_PREFIX_PATH=/opt/Qt/6.10.2/gcc_64

# Optional: Run tests
cd build && ctest --output-on-failure
```

#### Running TR4QT

```bash
# From project root
./build/src/tr4qt
```

### Raspberry Pi

Building on Raspberry Pi 4/5 (64-bit Raspberry Pi OS or Debian) works the same as Ubuntu/Debian.

#### Raspberry Pi 4/5 (64-bit, recommended)

```bash
# Install dependencies (same as Ubuntu/Debian above)
sudo apt update
sudo apt install -y build-essential cmake git pkg-config \
    qt6-base-dev qt6-base-dev-tools qt6-base-private-dev \
    libqt6serialport6-dev libqt6svg6-dev \
    qt6-multimedia-dev qt6-httpserver-dev \
    qt6-declarative-dev qt6-shadertools-dev \
    libasound2-dev libpulse-dev \
    libhamlib-dev libhamlib-utils

# Configure and build (may take 10-15 minutes on Pi 5)
git clone https://github.com/ny4i/TR4QT.git
cd TR4QT
cmake -B build
cmake --build build -j4
```

**Note:** If Qt 6.5+ is not available in your Pi OS repositories, you can install Qt from source or use the `aqt` installer:
```bash
pip3 install aqtinstall
aqt install-qt linux desktop 6.10.2 linux_gcc_64 \
    -m qtmultimedia qthttpserver qtserialport qtshadertools \
    --outputdir /opt/Qt
```
Then build with: `cmake -B build -DCMAKE_PREFIX_PATH=/opt/Qt/6.10.2/gcc_64`

#### Older Raspberry Pi (Pi 3, limited RAM)

```bash
# Reduce parallel jobs to avoid out-of-memory errors
cmake --build build -j2

# Or build single-threaded if RAM is very limited
cmake --build build
```

**Performance Note:** TR4QT runs well on Raspberry Pi 4 and 5. On Pi 3 and older, expect slower performance, especially with large logs or multiple windows open.

### Generic Linux

For other Linux distributions, install the equivalent packages:

**Fedora:**
```bash
sudo dnf install -y cmake git gcc-c++ \
    qt6-qtbase-devel qt6-qtbase-private-devel \
    qt6-qtserialport-devel qt6-qtsvg-devel \
    qt6-qtmultimedia-devel qt6-qthttpserver-devel \
    qt6-qtdeclarative-devel qt6-qtshadertools-devel \
    alsa-lib-devel pulseaudio-libs-devel \
    hamlib-devel
```

**Arch Linux:**
```bash
sudo pacman -S --needed base-devel cmake git \
    qt6-base qt6-serialport qt6-svg \
    qt6-multimedia qt6-httpserver \
    qt6-declarative qt6-shadertools \
    alsa-lib libpulse \
    hamlib
```

**Then clone and build:**
```bash
git clone https://github.com/ny4i/TR4QT.git
cd TR4QT
cmake -B build
cmake --build build -j$(nproc)
```

## Building on Windows

### Using MSYS2 (Recommended)

MSYS2 provides a Unix-like environment for Windows with easy package management.

#### Install MSYS2

1. Download installer from https://www.msys2.org/
2. Run installer (default location: `C:\msys64`)
3. Open **MSYS2 MinGW 64-bit** terminal

#### Install Dependencies

```bash
# Update package database
pacman -Syu

# Install build tools
pacman -S --needed base-devel mingw-w64-x86_64-toolchain \
          mingw-w64-x86_64-cmake git

# Install Qt 6 modules
pacman -S mingw-w64-x86_64-qt6-base \
          mingw-w64-x86_64-qt6-serialport \
          mingw-w64-x86_64-qt6-svg \
          mingw-w64-x86_64-qt6-multimedia \
          mingw-w64-x86_64-qt6-httpserver \
          mingw-w64-x86_64-qt6-declarative \
          mingw-w64-x86_64-qt6-shadertools

# Install Hamlib
pacman -S mingw-w64-x86_64-hamlib
```

#### Build TR4QT

```bash
# Clone repository
git clone https://github.com/ny4i/TR4QT.git
cd TR4QT

# Configure and build
cmake -B build -G "MinGW Makefiles"
cmake --build build -j$(nproc)

# Run
./build/src/tr4qt.exe
```

### Using Visual Studio

#### Install Requirements

1. **Visual Studio 2019 or later** (Community Edition is free)
   - Include "Desktop development with C++" workload

2. **Qt 6** from Qt Online Installer
   - Download from https://www.qt.io/download-qt-installer
   - Install MSVC 2019 64-bit component
   - Select modules: SerialPort, Svg, Multimedia, HttpServer, Quick, ShaderTools

3. **CMake**
   - Download from https://cmake.org/download/
   - Add to PATH during installation

4. **Hamlib for Windows**
   - Download pre-built binaries from https://github.com/Hamlib/Hamlib/releases
   - Extract to `C:\hamlib`

#### Build TR4QT

Open **Developer Command Prompt for VS 2019**:

```cmd
git clone https://github.com/ny4i/TR4QT.git
cd TR4QT

cmake -B build -G "Visual Studio 16 2019" -A x64 ^
      -DCMAKE_PREFIX_PATH=C:\Qt\6.10.2\msvc2019_64 ^
      -DHAMLIB_INCLUDE_DIR=C:\hamlib\include ^
      -DHAMLIB_LIBRARY=C:\hamlib\lib\hamlib.lib

cmake --build build --config Release

.\build\src\Release\tr4qt.exe
```

### Using Qt Creator (Cross-platform)

Qt Creator provides an IDE that works on Windows, Linux, and macOS.

1. Open Qt Creator
2. File → Open File or Project
3. Select `CMakeLists.txt` in TR4QT directory
4. Configure build settings:
   - Choose kit (Desktop Qt 6.x.x)
   - Set build directory
5. Build → Build Project "TR4QT"
6. Run → Run

## Qt-Specific Issues

### Missing Qt Modules

If CMake fails with "Could not find a package configuration file provided by Qt6...", you are missing a required Qt module. Install the specific module mentioned in the error.

Common missing modules and their packages (Ubuntu/Debian):

| CMake Error | Ubuntu/Debian Package |
|---|---|
| Qt6HttpServer | `qt6-httpserver-dev` |
| Qt6Multimedia | `qt6-multimedia-dev` |
| Qt6SerialPort | `libqt6serialport6-dev` |
| Qt6Svg | `libqt6svg6-dev` |
| Qt6ShaderTools | `qt6-shadertools-dev` |
| Qt6Quick / Qt6Qml | `qt6-declarative-dev` |

If Qt is installed in a custom location:
```bash
# Linux
cmake -B build -DCMAKE_PREFIX_PATH=/opt/Qt/6.10.2/gcc_64

# Windows (Visual Studio)
cmake -B build -DCMAKE_PREFIX_PATH=C:\Qt\6.10.2\msvc2019_64
```

### Qt Plugins

On Linux, if the application fails to start with "Could not find the Qt platform plugin":

```bash
# Install platform plugins
sudo apt install qt6-qpa-plugins

# Or set environment variable
export QT_QPA_PLATFORM_PLUGIN_PATH=/usr/lib/x86_64-linux-gnu/qt6/plugins
```

### Qt Version Compatibility

TR4QT requires Qt 6.5 minimum. Recommended: Qt 6.10.x.

Avoid Qt 6.0–6.4 (missing required module APIs).

## Linux Window Manager Issues

TR4QT is tested on various Linux window managers. Here are known issues and solutions:

### GNOME / Mutter

**Issue:** Window geometry not saved correctly on Wayland

**Solution:**
```bash
# Run with X11 instead of Wayland
export QT_QPA_PLATFORM=xcb
./tr4qt
```

**Issue:** High DPI scaling problems

**Solution:**
```bash
export QT_AUTO_SCREEN_SCALE_FACTOR=1
./tr4qt
```

### i3 / Awesome / Other Tiling WMs

**Issue:** Floating windows tile incorrectly

**Solution:** Add TR4QT to floating window rules.

For **i3**, add to `~/.config/i3/config`:
```
for_window [class="tr4qt"] floating enable
for_window [title="DX Cluster"] floating enable
for_window [title="Band Map"] floating enable
for_window [title="Radio Control"] floating enable
```

For **Awesome**, add to `rc.lua`:
```lua
{ rule = { class = "tr4qt" },
  properties = { floating = true } }
```

### Wayland Issues

Some Qt features don't work well on Wayland yet. Force X11 mode:
```bash
# For single run
QT_QPA_PLATFORM=xcb ./tr4qt

# Or set in .desktop file
Exec=env QT_QPA_PLATFORM=xcb /usr/local/bin/tr4qt
```

To check which display server you're using:
```bash
echo $XDG_SESSION_TYPE
# Output: x11 or wayland
```

## Troubleshooting

### CMake Can't Find Qt

```bash
# Linux: Set CMAKE_PREFIX_PATH
export CMAKE_PREFIX_PATH=/usr/lib/x86_64-linux-gnu/cmake/Qt6
cmake -B build

# macOS (Homebrew)
export CMAKE_PREFIX_PATH="/opt/homebrew/opt/qt@6"
cmake -B build

# Windows (MSYS2): Usually automatic
# Windows (Visual Studio):
cmake -B build -DCMAKE_PREFIX_PATH=C:\Qt\6.10.2\msvc2019_64
```

### Hamlib Not Found

```bash
# Linux: Install from package manager
sudo apt install libhamlib-dev

# Or build from source
git clone https://github.com/Hamlib/Hamlib.git
cd Hamlib
./bootstrap
./configure
make
sudo make install
sudo ldconfig

# Windows (MSYS2)
pacman -S mingw-w64-x86_64-hamlib
```

### Build Fails with "undefined reference"

Missing library. Check that all dependencies are installed:
```bash
# Linux
ldd ./build/src/tr4qt

# Windows (MSYS2)
ldd ./build/src/tr4qt.exe
```

### Runtime Error: "libQt6Core.so.6: cannot open shared object file"

Qt libraries not in library path:
```bash
# Linux: Add to LD_LIBRARY_PATH
export LD_LIBRARY_PATH=/opt/Qt/6.10.2/gcc_64/lib:$LD_LIBRARY_PATH

# Or install Qt system-wide via package manager
```

### Serial Port Permission Denied (Linux)

Radio connection fails with permission error:

```bash
# Add user to dialout group
sudo usermod -a -G dialout $USER

# Log out and log back in, then verify
groups | grep dialout
```

### Application Crashes on Startup

1. **Check Qt platform plugin:**
   ```bash
   export QT_DEBUG_PLUGINS=1
   ./build/src/tr4qt
   ```

2. **Use debugger:**
   ```bash
   gdb ./build/src/tr4qt
   run
   bt  # backtrace on crash
   ```

## Build Options

### Debug Build

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build
```

### Release Build (Optimized)

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

### Enable Tests

```bash
cmake -B build -DBUILD_TESTING=ON
cmake --build build
cd build && ctest --output-on-failure
```

## Getting Help

- **GitHub Issues:** https://github.com/ny4i/TR4QT/issues
- **Discussions:** https://github.com/ny4i/TR4QT/discussions
- **Documentation:** https://github.com/ny4i/TR4QT/tree/master/docs
