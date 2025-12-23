# TR4QT - Amateur Radio Contest Logger

Cross-platform Qt/C++ port of TR4W (Delphi contest logger) with centralized radio control via hamlib.

## Features (Phase 1 - In Progress)

- ✅ cty.dat country file parser with auto-download
- ✅ DXCC entity lookup
- 🚧 Radio control via hamlib (K4, IC-7610/7760 focus)
- 🚧 SQLite database for portable log storage
- 🚧 Contest support: CQ WW, CQ WPX, Winter Field Day

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

- Qt6 (6.5+)
- Hamlib (4.0+)
- CMake (3.16+)
- C++17 compiler

## Building

```bash
mkdir build
cd build
cmake ..
make
```

## Running

```bash
./build/src/tr4qt
```

## Architecture

- **Radio Control**: Hamlib C library (direct linking, NOT rigctld)
- **Database**: SQLite for portable, cross-platform storage
- **UI**: Qt6 Widgets with Model/View pattern
- **Scoring**: Strategy pattern for contest-specific rules

## Reference

Based on TR4W (Delphi) at `/Users/toms/projects/TR4W/`

## License

TBD
