# CI Build Environment Guide for Qt Cross-Platform Projects

This document captures the complete CI/CD build environment setup for TR4QT, focusing on lessons learned about Windows MinGW + Qt and macOS Qt builds. **These patterns apply to ANY Qt cross-platform project.**

## Table of Contents
1. [Windows CI Setup (MinGW + Qt)](#windows-ci-setup-mingw--qt)
2. [macOS CI Setup (Homebrew + Qt)](#macos-ci-setup-homebrew--qt)
3. [Common Pitfalls and Solutions](#common-pitfalls-and-solutions)
4. [Deployment Workflows](#deployment-workflows)
5. [Testing and Verification](#testing-and-verification)

---

## Windows CI Setup (MinGW + Qt)

### Environment Overview
- **Runner**: `windows-latest` (currently Windows Server 2022)
- **Qt Installation**: `jurplel/install-qt-action@v4`
- **Compiler**: MinGW (bundled with Qt)
- **Shell**: Git Bash (via `shell: bash`)

### Critical Configuration

#### 1. Installing Qt with install-qt-action

```yaml
- name: Install Qt
  uses: jurplel/install-qt-action@v4
  with:
    version: '6.7.2'              # Qt version
    host: windows
    target: desktop
    arch: win64_mingw             # MUST specify MinGW arch
    tools: 'tools_mingw1310'      # CRITICAL - installs MinGW compiler
    modules: 'qtwebsockets qthttpserver'  # Additional Qt modules
    cache: true                   # Cache Qt installation for faster builds
```

**Key Points:**
- `arch: win64_mingw` - Uses MinGW compiler (free, open-source)
- `tools: 'tools_mingw1310'` - **MUST** install MinGW tools explicitly
- `modules` - List additional Qt modules beyond base (Core, Gui, Widgets, Network, Sql)
- Base modules are always included, don't list them

#### 2. Qt Installation Paths

After `install-qt-action` runs, Qt is installed at:
- **Qt Base**: `D:\a\<REPO>\Qt\<VERSION>\mingw_64`
  - Example: `D:\a\TR4QT\Qt\6.7.2\mingw_64`
- **MinGW Tools**: `D:\a\<REPO>\Qt\Tools\mingw1310_64\bin`
- **Environment Variable**: `QT_ROOT_DIR` points to Qt base directory

**CRITICAL**: These paths are NOT in standard locations like `C:\Qt`. They're in the GitHub Actions workspace.

#### 3. Finding MinGW at Runtime (THE TRICKY PART)

**Problem**: MinGW path changes with Qt version and runner environment.

**Wrong Approach** (will fail):
```bash
# ❌ Hardcoded path - breaks when Qt version changes
MINGW_BIN="C:/Qt/Tools/mingw1310_64/bin"

# ❌ Assumes runner.temp location - wrong on GitHub Actions
MINGW_BIN="${{ runner.temp }}/../Qt/Tools/mingw1310_64/bin"
```

**Correct Approach** (derive from QT_ROOT_DIR):
```bash
# ✅ Derive MinGW path from QT_ROOT_DIR environment variable
# QT_ROOT_DIR = D:\a\TR4QT\Qt\6.7.2\mingw_64
# Go up 2 levels to get D:\a\TR4QT\Qt
QT_BASE=$(dirname $(dirname "${{ env.QT_ROOT_DIR }}"))
echo "DEBUG: QT_ROOT_DIR=${{ env.QT_ROOT_DIR }}"
echo "DEBUG: QT_BASE=$QT_BASE"

# Find MinGW directory (handles version changes automatically)
MINGW_BIN=$(find "$QT_BASE/Tools" -type d -name "mingw*_64" -print -quit 2>/dev/null || echo "")
if [ -n "$MINGW_BIN" ]; then
  MINGW_BIN="$MINGW_BIN/bin"
fi

# Validate MinGW was found
if [ ! -d "$MINGW_BIN" ] || [ -z "$MINGW_BIN" ]; then
  echo "ERROR: MinGW not found at $MINGW_BIN"
  exit 1
fi
echo "Using MinGW from: $MINGW_BIN"
```

**Why This Works:**
1. Uses `QT_ROOT_DIR` set by install-qt-action (always correct)
2. Derives Qt base directory (2 levels up)
3. Searches for `mingw*_64` (version-agnostic)
4. Validates directory exists before proceeding
5. Fails fast with clear error if not found

#### 4. Required MinGW Runtime DLLs

Windows executables built with MinGW **require** these runtime DLLs to run:

```bash
# CRITICAL - Copy these or app won't launch on user's machine
cp "$MINGW_BIN/libgcc_s_seh-1.dll" "$DEST/"      # GCC runtime (SEH = 64-bit)
cp "$MINGW_BIN/libstdc++-6.dll" "$DEST/"         # C++ standard library
cp "$MINGW_BIN/libwinpthread-1.dll" "$DEST/"     # POSIX threads for Windows
```

**Different libgcc variants:**
- `libgcc_s_seh-1.dll` - 64-bit Windows (most common)
- `libgcc_s_dw2-1.dll` - 32-bit Windows (DWARF-2 exception handling)
- `libgcc_s_sjlj-1.dll` - 32-bit Windows (setjmp/longjmp exception handling)

**Always check which variant exists** in the MinGW directory:
```bash
if [ -f "$MINGW_BIN/libgcc_s_seh-1.dll" ]; then
  cp "$MINGW_BIN/libgcc_s_seh-1.dll" "$DEST/"
elif [ -f "$MINGW_BIN/libgcc_s_dw2-1.dll" ]; then
  cp "$MINGW_BIN/libgcc_s_dw2-1.dll" "$DEST/"
else
  echo "ERROR: No libgcc variant found"
  exit 1
fi
```

#### 5. CMake Configuration for MinGW

```yaml
- name: Configure CMake
  run: |
    cmake -B build -G "MinGW Makefiles" \
      -DCMAKE_BUILD_TYPE=Release \
      -DCMAKE_PREFIX_PATH="${{ env.QT_ROOT_DIR }}"
```

**Key Points:**
- `-G "MinGW Makefiles"` - Use MinGW generator (NOT Visual Studio)
- `-DCMAKE_PREFIX_PATH` - Point to Qt installation directory
- Use `${{ env.QT_ROOT_DIR }}` from install-qt-action

#### 6. Building with MinGW

```yaml
- name: Build
  run: cmake --build build --config Release --parallel
```

**Simple and reliable** - CMake handles the MinGW toolchain.

---

## macOS CI Setup (Homebrew + Qt)

### Environment Overview
- **Runner**: `macos-latest` (currently macOS 14)
- **Qt Installation**: Homebrew (`brew install qt@6`)
- **Compiler**: Clang (Xcode Command Line Tools)
- **Shell**: Bash (default)

### Critical Configuration

#### 1. Installing Qt via Homebrew

```yaml
- name: Install Dependencies
  run: |
    brew install qt@6
    brew install libusb
```

**Key Points:**
- `brew install qt@6` installs **ALL Qt modules** (no need to specify modules individually)
- Qt is installed to `/opt/homebrew/opt/qt@6` (Apple Silicon) or `/usr/local/opt/qt@6` (Intel)
- Homebrew handles all dependencies automatically

#### 2. Qt Installation Paths

Homebrew installs Qt to:
- **Qt Base**: `/opt/homebrew/opt/qt@6` (Apple Silicon) or `/usr/local/opt/qt@6` (Intel)
- **Frameworks**: `/opt/homebrew/opt/qt@6/lib/*.framework`
- **Plugins**: `/opt/homebrew/opt/qt@6/share/qt/plugins`
- **Binaries**: `/opt/homebrew/opt/qt@6/bin`

Use `brew --prefix qt@6` to get the installation prefix dynamically.

#### 3. CMake Configuration for macOS

```yaml
- name: Configure CMake
  run: |
    cmake -B build \
      -DCMAKE_BUILD_TYPE=Release \
      -DCMAKE_PREFIX_PATH=$(brew --prefix qt@6) \
      -DHAMLIB_ROOT=/usr/local
```

**Key Points:**
- `-DCMAKE_PREFIX_PATH=$(brew --prefix qt@6)` - Dynamic path to Qt
- Works on both Apple Silicon and Intel Macs
- Homebrew CMake integration is excellent

#### 4. Building Hamlib from Source (CRITICAL for macOS)

**Problem**: Homebrew Hamlib bottles are **code-signed**, which causes issues when bundled into unsigned apps.

**Solution**: Build Hamlib from source as unsigned:

```yaml
- name: Build Hamlib from source (unsigned)
  run: |
    curl -L -o hamlib.tar.gz "https://github.com/Hamlib/Hamlib/releases/download/$HAMLIB_VERSION/hamlib-$HAMLIB_VERSION.tar.gz"
    tar -xzf hamlib.tar.gz
    cd hamlib-$HAMLIB_VERSION
    ./configure --prefix=/usr/local --disable-shared --enable-static
    make -j$(sysctl -n hw.ncpu)
    sudo make install
```

**Why This Works:**
- `--disable-shared --enable-static` - Static linking (no dylib bundling issues)
- Unsigned binaries don't conflict with app bundle signatures
- Consistent behavior across all Macs

---

## Common Pitfalls and Solutions

### 1. Windows: Missing libgcc_s_seh-1.dll

**Error**: "The program can't start because libgcc_s_seh-1.dll is missing from your computer"

**Cause**: MinGW runtime DLLs not copied to deployment directory

**Solution**: See [Required MinGW Runtime DLLs](#4-required-mingw-runtime-dlls)

### 2. Windows: "interface" is a reserved keyword

**Error**:
```
error: expected ',' or '...' before 'struct'
void sendDiscoveryMessage(const QNetworkInterface& interface);
```

**Cause**: Windows COM headers define `#define interface struct`

**Solution**: Rename parameter to `netInterface`, `iface`, or `networkInterface`

### 3. Windows: Qt Module Not Found (Qt6WebSockets, Qt6HttpServer, etc.)

**Error**: `Could not find a package configuration file provided by "Qt6WebSockets"`

**Cause**: Module not listed in `install-qt-action` modules parameter

**Solution**: Add to `modules:` in install-qt-action configuration:
```yaml
modules: 'qtwebsockets qthttpserver qtwebengine'
```

**Note**: macOS doesn't have this issue because `brew install qt@6` installs ALL modules.

### 4. macOS: App Launches on Build Mac but Crashes on User's Mac

**Error**: App crashes immediately with dyld errors about missing libraries

**Cause**: Homebrew library paths are absolute (`/opt/homebrew/...`), not bundled

**Solution**: Use macdeployqt + manual library copying + install_name_tool (see macOS deployment guide in CLAUDE.md)

### 5. Windows: MinGW Path Not Found in CI

**Error**: `ERROR: No libgcc variant found in /bin`

**Cause**: Hardcoded or wrong MinGW path

**Solution**: Use the dynamic MinGW path detection from [Finding MinGW at Runtime](#3-finding-mingw-at-runtime-the-tricky-part)

### 6. Both Platforms: SQL Plugin Not Found

**Error**: "QSQLITE driver not loaded" or database operations fail silently

**Cause**: Missing `qsqlite.dll` (Windows) or `libqsqlite.dylib` (macOS) plugin

**Solution**:
- **Windows**: Copy `qsqlite.dll` to `sqldrivers/` subdirectory
- **macOS**: macdeployqt should handle this, but verify with `ls -la app.app/Contents/PlugIns/sqldrivers/`

### 7. Both Platforms: HTTPS Downloads Fail (TLS Backend Missing)

**Error**: "No functional TLS backend was found"

**Cause**: Qt TLS plugins not bundled (windeployqt/macdeployqt **FORGET THESE**)

**Solution**:
- **Windows**: Manually copy `tls/*.dll` from Qt plugins directory
- **macOS**: Manually copy `/opt/homebrew/opt/qt@6/share/qt/plugins/tls/*.dylib` to `app.app/Contents/PlugIns/tls/`

**Critical files:**
- `qopensslbackend` - OpenSSL TLS backend
- `qschannelbackend` (Windows) - Native Windows TLS
- `qsecuretransportbackend` (macOS) - Native macOS TLS

---

## Deployment Workflows

### Windows Deployment Checklist

After building, before creating installer:

1. **Copy Qt DLLs** (explicit list):
   - Qt6Core.dll, Qt6Gui.dll, Qt6Widgets.dll, Qt6Network.dll
   - Qt6Sql.dll, Qt6HttpServer.dll, Qt6WebSockets.dll
   - Qt6PrintSupport.dll, Qt6Concurrent.dll

2. **Copy MinGW Runtime DLLs**:
   - libgcc_s_seh-1.dll (or appropriate variant)
   - libstdc++-6.dll
   - libwinpthread-1.dll

3. **Copy Qt Plugins**:
   - `platforms/qwindows.dll` (CRITICAL - app won't launch without this)
   - `sqldrivers/qsqlite.dll` (database access)
   - `tls/*.dll` (HTTPS support)
   - `styles/qwindowsvistastyle.dll` (native Windows appearance)
   - `imageformats/*.dll` (PNG, JPG support)

4. **Create qt.conf**:
   ```ini
   [Paths]
   Plugins = .
   ```

5. **Verify Deployment**:
   ```bash
   ls -lh *.dll
   ls -lh platforms/*.dll
   ls -lh sqldrivers/*.dll
   ls -lh tls/*.dll
   ```

### macOS Deployment Checklist

After building, before creating DMG:

1. **Run macdeployqt**:
   ```bash
   $(brew --prefix qt@6)/bin/macdeployqt app.app -verbose=2
   ```

2. **Copy TLS Plugins** (macdeployqt misses these):
   ```bash
   mkdir -p app.app/Contents/PlugIns/tls
   cp /opt/homebrew/opt/qt@6/share/qt/plugins/tls/*.dylib app.app/Contents/PlugIns/tls/
   ```

3. **Check for Missing Dependencies**:
   ```bash
   find app.app/Contents/Frameworks -type f -exec otool -L {} \; | \
     grep "/opt/homebrew" | \
     grep -v "@rpath" | \
     sort -u
   ```

4. **Fix Absolute Paths** (if any found):
   ```bash
   install_name_tool -id "@rpath/libname.dylib" app.app/Contents/Frameworks/libname.dylib
   install_name_tool -change "/opt/homebrew/path/lib.dylib" "@rpath/lib.dylib" app.app/Contents/Frameworks/framework
   ```

5. **Re-sign Everything**:
   ```bash
   # Sign dylibs
   for lib in app.app/Contents/Frameworks/*.dylib; do
     codesign -s - -f "$lib"
   done

   # Sign frameworks
   for framework in app.app/Contents/Frameworks/*.framework; do
     codesign -s - -f "$framework"
   done

   # Sign plugins
   find app.app/Contents/PlugIns -name "*.dylib" -exec codesign -s - -f {} \;

   # Sign app
   codesign -s - -f app.app
   ```

6. **Create DMG** (NOT with macdeployqt):
   ```bash
   mkdir -p dmg-contents
   cp -R app.app dmg-contents/
   hdiutil create -volname "AppName" -srcfolder dmg-contents -ov -format UDZO app.dmg
   ```

---

## Testing and Verification

### Local Testing Before CI

**Windows:**
```bash
# Test in clean environment (no Qt in PATH)
cd build/src
./app.exe --version

# Check DLL dependencies
ldd app.exe  # Git Bash
```

**macOS:**
```bash
# Test from DMG
hdiutil attach app.dmg
xattr -cr "/Volumes/AppName/app.app"  # Remove quarantine
"/Volumes/AppName/app.app/Contents/MacOS/app" --version

# Check for absolute paths (should be NONE)
find "/Volumes/AppName/app.app/Contents/Frameworks" -type f \
  -exec otool -L {} \; | grep "/opt/homebrew" | grep -v "@rpath"
```

### CI Testing Steps

After deployment, before uploading artifacts:

1. **Smoke Test** - Launch app with `--version`
2. **Verify Critical Files** - Check DLLs/frameworks exist
3. **Check Signatures** (macOS) - Verify code signing
4. **Test Installer** (Windows) - Install and run

Example:
```yaml
- name: Smoke Test Installer
  run: |
    # Install silently
    ./TR4QT-Setup.exe /S /D=C:\TestInstall

    # Test launch
    C:\TestInstall\tr4qt.exe --version

    # Verify plugins
    ls C:\TestInstall\platforms\qwindows.dll
    ls C:\TestInstall\sqldrivers\qsqlite.dll
```

---

## Summary: Key Lessons for ANY Qt CI Project

1. **Windows**: Always use `install-qt-action` with `tools: 'tools_mingw1310'`
2. **Windows**: Derive MinGW path from `QT_ROOT_DIR`, never hardcode
3. **Windows**: Explicitly copy ALL runtime DLLs (MinGW, Qt, plugins)
4. **macOS**: Build Hamlib (or other C libraries) from source as unsigned
5. **macOS**: Always manually copy TLS plugins (macdeployqt forgets them)
6. **macOS**: Use `install_name_tool` to fix absolute Homebrew paths to @rpath
7. **Both**: Never trust "automatic" deployment tools (windeployqt, macdeployqt) - verify everything
8. **Both**: Create smoke tests that run the deployed app in CI
9. **Both**: Document EVERY deployment step (future you will thank you)
10. **Both**: Version everything in source control (including workflow files)

---

## References

- [install-qt-action Documentation](https://github.com/jurplel/install-qt-action)
- [Qt for Windows - MinGW](https://doc.qt.io/qt-6/windows.html)
- [Qt for macOS](https://doc.qt.io/qt-6/macos.html)
- [GitHub Actions Runners](https://docs.github.com/en/actions/using-github-hosted-runners/about-github-hosted-runners)
- [TR4QT CLAUDE.md](../CLAUDE.md) - Deployment guides
- [TR4QT build.yml](../.github/workflows/build.yml) - Complete CI workflow

---

**Last Updated**: 2025-12-29
**TR4QT Version**: 3.7.2
**Author**: Claude Sonnet 4.5 (via Claude Code)
