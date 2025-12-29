# Windows Build Issue - v3.7.0

## Status: ✅ RESOLVED (2025-12-29)

**Root Cause**: MinGW toolchain mismatch - Strawberry Perl MinGW 13.2.0 is incompatible with Qt 6.10.1 (built with MinGW 13.1.0)

**Solution**: Use Qt's bundled MinGW 13.1.0 at `C:\Qt\Tools\mingw1310_64\bin`

```bash
# Set PATH to use Qt's MinGW (not Strawberry Perl)
export PATH="/c/Qt/Tools/mingw1310_64/bin:$PATH"

# Clean build
rm -rf build
cmake -B build -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release \
  -DHAMLIB_ROOT=C:/projects/hamlib \
  -DCMAKE_PREFIX_PATH=C:/Qt/6.10.1/mingw_64

cmake --build build
```

**CI Status**: Already uses Qt's MinGW (configured in `.github/workflows/build.yml`)

---

## Original Problem (for reference)
**Error**: `undefined reference to '__imp___argc'` when linking on Windows

## Timeline
1. **v3.6.3** - Built successfully on Windows as GUI app (WIN32 flag) with Qt 6.10.1
2. **v3.7.0** - Added miniz library and bundled cty.dat
3. **Now** - Same build configuration fails with entry point error

## What Changed in v3.7.0
- Added `src/3rdparty/miniz/miniz.cpp` (renamed from miniz.c for C++ compilation)
- Added `resources/cty.dat` (99KB) to embedded resources
- Modified `CountryFileDownloader.cpp` to use miniz instead of system unzip
- Added first-run cty.dat extraction in `main.cpp`
- Added Hamlib debug checkbox in preferences
- Fixed scientific notation in frequency logging (10 locations)

## Build Environment (Windows)
- **Qt Version**: 6.10.1 (mingw_64)
- **MinGW**: Strawberry Perl MinGW 13.2.0 at `C:/Strawberry/c/bin/`
- **Hamlib**: `C:/projects/hamlib`
- **CMake Command Used**:
  ```bash
  cmake -B build -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release \
    -DHAMLIB_ROOT=C:/projects/hamlib \
    -DCMAKE_PREFIX_PATH=C:/Qt/6.10.1/mingw_64
  ```

## Error Details
```
C:/Strawberry/c/bin/../lib/gcc/x86_64-w64-mingw32/13.2.0/../../../../x86_64-w64-mingw32/bin/ld.exe:
C:/Qt/6.10.1/mingw_64/lib/libQt6EntryPoint.a(qtentrypoint_win.cpp.obj):qtentrypoint_win.cpp:
(.rdata$.refptr.__imp___argc[.refptr.__imp___argc]+0x0):
undefined reference to `__imp___argc'
```

## Analysis
The `__imp___argc` symbol is part of MinGW's C runtime and should be automatically available.
The error suggests a **MinGW toolchain mismatch** between:
- The MinGW used to build Qt 6.10.1 (likely Qt's bundled MinGW 13.1.0)
- The MinGW being used to compile TR4QT (Strawberry Perl MinGW 13.2.0)

## Attempted Fixes
1. ❌ Added Qt6::EntryPoint to find_package - Component not found
2. ❌ Made Qt6::EntryPoint optional - Still get error
3. ❌ Removed WIN32 flag (console mode) - User correctly noted v3.6.3 worked as GUI
4. ❌ Clean rebuild - Error persists
5. ✅ **Use Qt's bundled MinGW 13.1.0** - **THIS FIXED IT**

## Resolution
**Use Qt's bundled MinGW** instead of Strawberry Perl MinGW:
- Qt 6.10.1 was built with MinGW 13.1.0
- Strawberry Perl has MinGW 13.2.0 (incompatible at link time)
- Solution: `C:\Qt\Tools\mingw1310_64\bin` must be in PATH before Strawberry

The miniz.cpp and cty.dat were NOT the problem - it was purely a toolchain mismatch.

## Files to Check
- `src/CMakeLists.txt` - Lines 76 (miniz.cpp added), 182 (WIN32 flag)
- `CMakeLists.txt` - Line 31 (find_package Qt6)
- `resources/resources.qrc` - Line 13 (cty.dat added)
- `src/3rdparty/miniz/miniz.cpp` - Renamed from .c, might need extern "C"

## Success Criteria ✅ MET
Build completes successfully on Windows with:
- ✅ WIN32 flag (GUI application, no console window)
- ✅ All v3.7.0 features working (bundled cty.dat, miniz extraction)
- ✅ Qt's bundled MinGW 13.1.0 toolchain

## Reference Commits
- v3.6.3: `b3044ce` - Last known working Windows build
- v3.7.0 changes: `3a4ee89`, `c97d44e`, `9299a7a`
- Latest: `7905d43` - Restored WIN32 flag after debugging attempts
- Resolution verified: 2025-12-29 (local build successful with Qt MinGW)
