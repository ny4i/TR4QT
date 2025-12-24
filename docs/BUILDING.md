# Building TR4QT

This guide covers building TR4QT on Windows and Linux platforms (including Ubuntu, Debian, and Raspberry Pi).

## Table of Contents

- [Prerequisites](#prerequisites)
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
- **Qt 6.2** or later (Qt 6.5+ recommended)
- **Hamlib** 4.0 or later

## Building on Linux

### Ubuntu/Debian

#### Install Dependencies

```bash
# Update package lists
sudo apt update

# Install build tools
sudo apt install -y build-essential cmake git

# Install Qt 6 development packages
sudo apt install -y qt6-base-dev qt6-base-dev-tools \
                     libqt6core6 libqt6gui6 libqt6widgets6 \
                     libqt6network6 libqt6sql6 libqt6serialport6

# Install Hamlib
sudo apt install -y libhamlib-dev libhamlib-utils

# Optional: Install SQLite (usually included with Qt)
sudo apt install -y libsqlite3-dev
```

**Note for Ubuntu 20.04 LTS and Debian 11:**
Qt 6 may not be available in the default repositories. You have two options:

1. **Add Qt PPA** (Ubuntu only):
   ```bash
   sudo add-apt-repository ppa:okirby/qt6-backports
   sudo apt update
   sudo apt install qt6-base-dev
   ```

2. **Install Qt from Qt Online Installer**:
   - Download from https://www.qt.io/download-qt-installer
   - Install to `/opt/Qt` or `~/Qt`
   - Add to PATH: `export PATH=/opt/Qt/6.5.3/gcc_64/bin:$PATH`

#### Build TR4QT

```bash
# Clone the repository
git clone https://github.com/ny4i/TR4QT.git
cd TR4QT

# Create build directory
mkdir build
cd build

# Configure with CMake
cmake ..

# If Qt is installed in a custom location:
# cmake -DCMAKE_PREFIX_PATH=/opt/Qt/6.5.3/gcc_64 ..

# Build (use -j for parallel compilation)
make -j$(nproc)

# Optional: Run tests
ctest

# Install (optional)
sudo make install
```

#### Running TR4QT

```bash
# From build directory
./src/tr4qt

# Or if installed
tr4qt
```

### Raspberry Pi

Building on Raspberry Pi (Raspbian/Raspberry Pi OS) is similar to Ubuntu/Debian, with some considerations:

#### Raspberry Pi 4/5 (64-bit)

```bash
# Install dependencies (same as Ubuntu/Debian)
sudo apt update
sudo apt install -y build-essential cmake git
sudo apt install -y qt6-base-dev libhamlib-dev

# Build (may take 10-15 minutes)
mkdir build && cd build
cmake ..
make -j4  # Use all 4 cores
```

#### Raspberry Pi 3/Zero (32-bit)

For older Pi models with limited RAM:

```bash
# Reduce parallel jobs to avoid out-of-memory errors
make -j2

# Or build single-threaded if RAM is very limited
make
```

**Performance Note:** TR4QT runs well on Raspberry Pi 4 and 5. On Pi 3 and older, expect slower performance, especially with large logs or multiple windows open.

### Generic Linux

For other Linux distributions:

1. **Install Qt 6**:
   - Fedora: `sudo dnf install qt6-qtbase-devel`
   - Arch: `sudo pacman -S qt6-base`
   - OpenSUSE: `sudo zypper install qt6-base-devel`

2. **Install Hamlib**:
   - Fedora: `sudo dnf install hamlib-devel`
   - Arch: `sudo pacman -S hamlib`
   - OpenSUSE: `sudo zypper install hamlib-devel`

3. **Build** as shown above

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

# Install Qt 6
pacman -S mingw-w64-x86_64-qt6-base \
          mingw-w64-x86_64-qt6-serialport

# Install Hamlib
pacman -S mingw-w64-x86_64-hamlib
```

#### Build TR4QT

```bash
# Clone repository
git clone https://github.com/ny4i/TR4QT.git
cd TR4QT

# Create build directory
mkdir build
cd build

# Configure and build
cmake -G "MinGW Makefiles" ..
mingw32-make -j$(nproc)

# Run
./src/tr4qt.exe
```

### Using Visual Studio

#### Install Requirements

1. **Visual Studio 2019 or later** (Community Edition is free)
   - Include "Desktop development with C++" workload

2. **Qt 6** from Qt Online Installer
   - Download from https://www.qt.io/download-qt-installer
   - Install MSVC 2019 64-bit component
   - Default location: `C:\Qt\6.5.3\msvc2019_64`

3. **CMake**
   - Download from https://cmake.org/download/
   - Add to PATH during installation

4. **Hamlib for Windows**
   - Download pre-built binaries from https://github.com/Hamlib/Hamlib/releases
   - Extract to `C:\hamlib`
   - Or build from source

#### Build TR4QT

Open **Developer Command Prompt for VS 2019**:

```cmd
# Clone repository
git clone https://github.com/ny4i/TR4QT.git
cd TR4QT

# Create build directory
mkdir build
cd build

# Configure (adjust paths as needed)
cmake -G "Visual Studio 16 2019" -A x64 ^
      -DCMAKE_PREFIX_PATH=C:\Qt\6.5.3\msvc2019_64 ^
      -DHAMLIB_INCLUDE_DIR=C:\hamlib\include ^
      -DHAMLIB_LIBRARY=C:\hamlib\lib\hamlib.lib ^
      ..

# Build
cmake --build . --config Release

# Run
.\src\Release\tr4qt.exe
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

### Qt 6 Migration Issues

TR4QT uses Qt 6, which has breaking changes from Qt 5:

- **Removed Qt::MidButton** → Use `Qt::MiddleButton`
- **QDateTime changes** → Requires `QTimeZone` parameter
- **Signal deprecations** → `stateChanged` → `checkStateChanged` (Qt 6.9+)

If you see warnings about deprecated APIs, these are non-fatal but should be addressed.

### Missing Qt Modules

If CMake fails to find Qt modules:

```bash
# Linux: Set Qt6_DIR
export Qt6_DIR=/usr/lib/x86_64-linux-gnu/cmake/Qt6

# Windows (MSYS2): Usually automatic
# Windows (Visual Studio):
set Qt6_DIR=C:\Qt\6.5.3\msvc2019_64\lib\cmake\Qt6

# Then re-run cmake
cmake ..
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

TR4QT requires Qt 6.2 minimum. Recommended versions:

- **Qt 6.5.3** - Stable, well-tested
- **Qt 6.6.x** - Latest LTS (Long Term Support)
- **Qt 6.7.x** - Current stable

Avoid Qt 6.0 and 6.1 (many bugs fixed in 6.2).

## Linux Window Manager Issues

TR4QT is tested on various Linux window managers. Here are known issues and solutions:

### GNOME / Mutter

**Issue:** Window geometry not saved correctly

**Solution:** This is a known Qt/GNOME issue. Workaround:
```bash
# Run with X11 instead of Wayland
export QT_QPA_PLATFORM=xcb
./tr4qt
```

**Issue:** High DPI scaling problems

**Solution:**
```bash
# Let Qt handle scaling
export QT_AUTO_SCREEN_SCALE_FACTOR=1
./tr4qt
```

### KDE Plasma / KWin

**Issue:** Dock windows (Band Map, DX Cluster) not floating correctly

**Solution:** KDE usually handles Qt windows well, but if issues occur:
```bash
# Disable compositor temporarily
qdbus org.kde.KWin /Compositor suspend
```

### Xfce / LXDE

**Issue:** Dialog windows too small or misaligned

**Solution:** These lightweight WMs generally work well. Ensure you have:
```bash
sudo apt install libqt6x11extras6  # If available
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

**General Issue:** Some Qt features don't work well on Wayland yet

**Solution:** Force X11 mode:
```bash
# For single run
QT_QPA_PLATFORM=xcb ./tr4qt

# Or set in .desktop file
Exec=env QT_QPA_PLATFORM=xcb /usr/local/bin/tr4qt
```

### X11 vs Wayland Detection

To check which you're using:
```bash
echo $XDG_SESSION_TYPE
# Output: x11 or wayland
```

## Troubleshooting

### CMake Can't Find Qt

```bash
# Linux
sudo apt install qt6-base-dev-tools
export CMAKE_PREFIX_PATH=/usr/lib/x86_64-linux-gnu/cmake/Qt6

# Windows (MSYS2)
export CMAKE_PREFIX_PATH=/mingw64/lib/cmake

# Windows (Visual Studio)
cmake -DCMAKE_PREFIX_PATH=C:\Qt\6.5.3\msvc2019_64 ..
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
sudo ldconfig  # Update library cache

# Windows: Download pre-built or build with MSYS2
pacman -S mingw-w64-x86_64-hamlib
```

### Build Fails with "undefined reference"

Missing library. Check that all dependencies are installed:
```bash
# Linux
ldd ./src/tr4qt

# Windows (MSYS2)
ldd ./src/tr4qt.exe
```

### Runtime Error: "libQt6Core.so.6: cannot open shared object file"

Qt libraries not in library path:

```bash
# Linux: Add to LD_LIBRARY_PATH
export LD_LIBRARY_PATH=/opt/Qt/6.5.3/gcc_64/lib:$LD_LIBRARY_PATH

# Or install Qt system-wide
sudo apt install qt6-base-dev

# Windows (MSYS2): Usually automatic
# Windows (Visual Studio): Copy Qt DLLs to exe directory or add to PATH
set PATH=C:\Qt\6.5.3\msvc2019_64\bin;%PATH%
```

### Application Crashes on Startup

1. **Check Qt platform plugin:**
   ```bash
   export QT_DEBUG_PLUGINS=1
   ./tr4qt
   ```

2. **Run with verbose logging:**
   ```bash
   ./tr4qt --verbose
   ```

3. **Use debugger:**
   ```bash
   gdb ./tr4qt
   run
   bt  # backtrace on crash
   ```

### Serial Port Permission Denied (Linux)

Radio connection fails with permission error:

```bash
# Add user to dialout group
sudo usermod -a -G dialout $USER

# Log out and log back in, then verify
groups | grep dialout

# Or use sudo (not recommended for regular use)
sudo ./tr4qt
```

### High Memory Usage

Large contest logs can use significant memory. For Raspberry Pi or low-RAM systems:

1. Close unused windows (Band Map, DX Cluster)
2. Reduce log table display (only show recent 100 QSOs)
3. Export and clear old contests

## Build Options

### Debug Build

```bash
cmake -DCMAKE_BUILD_TYPE=Debug ..
make
```

### Release Build (Optimized)

```bash
cmake -DCMAKE_BUILD_TYPE=Release ..
make
```

### Enable Tests

```bash
cmake -DBUILD_TESTING=ON ..
make
ctest --output-on-failure
```

### Custom Install Prefix

```bash
cmake -DCMAKE_INSTALL_PREFIX=/opt/tr4qt ..
make
sudo make install
```

## Packaging

### Linux .deb Package (Ubuntu/Debian)

```bash
# Install packaging tools
sudo apt install checkinstall

# Build and create package
cd build
sudo checkinstall --pkgname=tr4qt \
                  --pkgversion=2.40.2 \
                  --pakdir=.. \
                  make install
```

### Linux .rpm Package (Fedora/RHEL)

```bash
# Install packaging tools
sudo dnf install rpm-build

# Create package
cd build
sudo checkinstall --type=rpm make install
```

### Windows Installer

Use **Inno Setup** or **WiX Toolset** to create an installer:

1. Build in Release mode
2. Copy exe and all Qt/Hamlib DLLs to installer directory
3. Use `windeployqt` to gather Qt dependencies:
   ```cmd
   windeployqt tr4qt.exe
   ```
4. Create installer script and compile

## Getting Help

- **GitHub Issues:** https://github.com/ny4i/TR4QT/issues
- **Discussions:** https://github.com/ny4i/TR4QT/discussions
- **Documentation:** https://github.com/ny4i/TR4QT/tree/master/docs

## Quick Reference

### One-Line Builds

**Ubuntu/Debian:**
```bash
sudo apt update && sudo apt install -y build-essential cmake qt6-base-dev libhamlib-dev && git clone https://github.com/ny4i/TR4QT.git && cd TR4QT && mkdir build && cd build && cmake .. && make -j$(nproc)
```

**Fedora:**
```bash
sudo dnf install -y cmake qt6-qtbase-devel hamlib-devel && git clone https://github.com/ny4i/TR4QT.git && cd TR4QT && mkdir build && cd build && cmake .. && make -j$(nproc)
```

**Arch Linux:**
```bash
sudo pacman -S --needed base-devel cmake qt6-base hamlib && git clone https://github.com/ny4i/TR4QT.git && cd TR4QT && mkdir build && cd build && cmake .. && make -j$(nproc)
```

**Windows (MSYS2):**
```bash
pacman -S --needed base-devel mingw-w64-x86_64-toolchain mingw-w64-x86_64-cmake mingw-w64-x86_64-qt6-base mingw-w64-x86_64-hamlib && git clone https://github.com/ny4i/TR4QT.git && cd TR4QT && mkdir build && cd build && cmake -G "MinGW Makefiles" .. && mingw32-make -j$(nproc)
```
