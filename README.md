# TR4QT - Amateur Radio Contest Logger

> **⚠️ DEVELOPMENT VERSION - NOT READY FOR CONTEST USE**
> This software is under active development and should **NOT** be used for actual contests.
> Features are incomplete, bugs are expected, and data loss may occur.
> Wait for an official stable release before using in production.

Cross-platform Qt/C++ port of TR4W (Delphi contest logger) with centralized radio control via hamlib.

## Features (Phase 1 - In Progress)

### Core Logging
- ✅ Real-time QSO logging with validation
- ✅ Duplicate checking (per-band-mode, all-band-mode, contest-specific rules)
- ✅ Exchange prediction and memory (learns from previous QSOs)
- ✅ Auto-increment serial numbers
- ✅ Zone lookup from CTY.DAT (CQ/ITU zones)
- ✅ Operator callsign tracking
- ✅ QSO editing and deletion

### Radio Control
- ✅ Hamlib integration (direct linking, not rigctld)
- ✅ Tested with Elecraft K4, Icom IC-7610/IC-7300
- ✅ Frequency/mode/band monitoring
- ✅ Manual band selection (when radio disconnected)
- ✅ Auto-band detection from frequency

### Contest Support
- ✅ CQ WW DX Contest (CW & SSB)
- ✅ CQ WPX Contest (CW & SSB)
- ✅ Winter Field Day
- ✅ ARRL Field Day
- ✅ ARRL Sweepstakes
- ✅ Contest-specific scoring and multipliers
- ✅ Real-time score calculation
- ✅ Band summary grid with QSO/multiplier counts

### Country/Geographic Data
- ✅ CTY.DAT parser with thread-safe reload
- ✅ Auto-download of fresh CTY.DAT files
- ✅ DXCC entity lookup by callsign
- ✅ US call area zone logic
- ✅ Geographic utilities (distance/bearing calculations)
- ✅ Native map viewer for multipliers

### DX Cluster & Spotting
- ✅ DX Cluster client (Telnet)
- ✅ Band Map window with spot filtering
- ✅ LOTW user database with auto-download
- ✅ Filter spots by LOTW users
- ✅ Click-to-tune from band map

### Import/Export
- ✅ **ADIF import** with validation and CTY.DAT lookup
- ✅ ADIF export
- ✅ Cabrillo export
- ✅ Export preview dialogs

### Multiplier Tracking
- ✅ Per-contest multiplier rules
- ✅ Per-band vs all-band multiplier scopes
- ✅ Needs display widget (worked/confirmed status)
- ✅ Visual multiplier indicators

### Data Management
- ✅ SQLite database for portable log storage
- ✅ Automatic database migrations
- ✅ Backup and restore functionality
- ✅ Multiple contest support (switch between contests)

### Network Features
- ✅ UDP broadcast support (N1MM-compatible format)
- ✅ Web server for remote monitoring
- ✅ Real-time statistics via HTTP

### User Interface
- ✅ Dark/light theme support
- ✅ Configurable fonts and sizes
- ✅ Statistics window (real-time contest stats)
- ✅ Resizable/dockable windows
- ✅ Keyboard shortcuts (Alt-B/Alt-V for band up/down)
- ✅ macOS menu integration

### Utilities
- ✅ Send Morse dialog (via radio)
- ✅ Preferences dialog (radio, logging, display, network, hamlib debug)
- ✅ Emergency ADIF logging (when database fails)
- ✅ Comprehensive logging system with file output

## Priority Contests

1. **CQ WW DX Contest** (CW & SSB)
   - Exchange: RST + CQ Zone
   - Multipliers: DXCC Countries + CQ Zones (per band)

2. **CQ WPX Contest** (CW & SSB)
   - Exchange: RST + Serial Number (auto-increment)
   - Multipliers: Callsign prefixes (all bands)

3. **Winter Field Day**
   - Exchange: Class + Section
   - Multipliers: ARRL/RAC Sections

## Requirements

- Qt 6.5+ (with modules: Core, Widgets, Sql, Network, HttpServer, SerialPort, Svg, Xml, Quick, Qml, ShaderTools, PrintSupport, Concurrent)
- Hamlib 4.0+
- CMake 3.16+
- C++17 compiler

## Building from Source

### Linux (Ubuntu/Debian)

```bash
# Install build tools
sudo apt install build-essential cmake git

# Install Qt6 development packages
sudo apt install qt6-base-dev qt6-base-private-dev \
    qt6-httpserver-dev qt6-serialport-dev qt6-svg-dev \
    qt6-declarative-dev qt6-shadertools-dev \
    libqt6sql6-sqlite

# Install Hamlib
sudo apt install libhamlib-dev

# Build
git clone https://github.com/ny4i/TR4QT.git
cd TR4QT
cmake -B build
cmake --build build -j$(nproc)
```

### macOS (Homebrew)

```bash
brew install qt@6 hamlib cmake
cmake -B build
cmake --build build -j$(sysctl -n hw.ncpu)
```

### Windows (MinGW)

See `docs/windows-deployment.md` for detailed Windows build instructions.

## Running

```bash
# Linux
./build/src/tr4qt

# macOS
./build/src/tr4qt.app/Contents/MacOS/tr4qt
```

## Pre-built Downloads

Pre-built binaries are available from [Releases](https://github.com/ny4i/TR4QT/releases):
- **macOS**: `.dmg` (signed and notarized)
- **Windows**: `.exe` installer or `.zip` portable
- **Linux x86_64**: `.AppImage`
- **Raspberry Pi (ARM64)**: `.AppImage`

## Architecture

- **Radio Control**: Hamlib C library (direct linking, NOT rigctld)
- **Database**: SQLite for portable, cross-platform storage
- **UI**: Qt6 Widgets with Model/View pattern
- **Scoring**: Strategy pattern for contest-specific rules

## Reference

Based on [TR4W](http://tr4w.net/) (Delphi)

## Acknowledgments

Thanks to [n1mm_view](https://github.com/n1kdo/n1mm_view) for the use of the ARRL section shapefiles.

Special thanks to the developers of [wfview](https://gitlab.com/eliggett/wfview) for their pioneering work on Icom network protocol implementation and [QLog](https://github.com/foldynl/QLog) for their Qt-based amateur radio logging applications. Their open-source projects provided valuable reference material and inspiration for TR4QT's Icom network support and the gridline display respectively.

## License

GPL v3 - See LICENSE file for details.

Copyright (C) 2024-2026 Thomas M. Schaefer, NY4I
