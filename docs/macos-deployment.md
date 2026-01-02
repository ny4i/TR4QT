# macOS App Deployment Guide

**CRITICAL**: This section documents the complete process for creating distributable macOS app bundles. These lessons apply to ANY Mac app, not just TR4QT.

## The Problem: Homebrew Dependencies Don't Work on Other Macs

When you build a Qt app on macOS with Homebrew Qt/libraries, the app works on YOUR Mac but crashes on other Macs. Why?

1. **Absolute Paths**: Homebrew libraries have absolute path IDs like `/opt/homebrew/opt/qt/lib/QtCore.framework`
2. **Missing Dependencies**: macdeployqt doesn't follow complete dependency chains
3. **Invalid Signatures**: Modifying bundled libraries with install_name_tool invalidates code signatures
4. **Incomplete Bundling**: macdeployqt misses transitive dependencies (e.g., bundles libbrotlidec but not libbrotlicommon)

## Complete Deployment Workflow

This is the exact order of operations needed in `.github/workflows/build.yml` (or local deployment):

```bash
cd build/src

# 1. Bundle Qt frameworks and plugins with macdeployqt
$(brew --prefix qt@6)/bin/macdeployqt tr4qt.app -verbose=2

# 2. CRITICAL: Check for missing dependencies
# macdeployqt is incomplete - it WILL miss dependencies
# Run this check and manually copy any missing libraries:
find tr4qt.app/Contents/Frameworks -type f -exec otool -L {} \; | \
  grep "/opt/homebrew" | \
  grep -v "@rpath" | \
  sort -u

# Example output showing missing libraries:
#   /opt/homebrew/opt/brotli/lib/libbrotlicommon.1.dylib
#   /opt/homebrew/opt/dbus/lib/libdbus-1.3.dylib

# 3. Copy ALL missing dependencies into Frameworks/
cp /opt/homebrew/opt/brotli/lib/libbrotlicommon.1.dylib tr4qt.app/Contents/Frameworks/
cp -R /opt/homebrew/opt/qtbase/lib/QtDBus.framework tr4qt.app/Contents/Frameworks/
cp /opt/homebrew/opt/dbus/lib/libdbus-1.3.dylib tr4qt.app/Contents/Frameworks/

# 3B. CRITICAL: Copy TLS plugins (macdeployqt DOES NOT bundle these!)
# Qt6 requires TLS plugins for HTTPS connections
# Without these, all HTTPS downloads fail with "No functional TLS backend was found"
mkdir -p tr4qt.app/Contents/PlugIns/tls
cp /opt/homebrew/opt/qtbase/share/qt/plugins/tls/*.dylib tr4qt.app/Contents/PlugIns/tls/
# This copies:
#   - libqsecuretransportbackend.dylib (macOS native TLS - recommended)
#   - libqopensslbackend.dylib (OpenSSL TLS backend)
#   - libqcertonlybackend.dylib (certificate-only backend)

# 4. CRITICAL: Fix ALL library IDs and dependencies to use @rpath
# There are TWO types of paths to fix:
#   A) Library ID: What the library calls itself (install_name_tool -id)
#   B) Dependencies: What other libraries it references (install_name_tool -change)

# 4A. Fix library IDs
# Check EVERY manually copied library:
otool -L tr4qt.app/Contents/Frameworks/libbrotlicommon.1.dylib | head -2
# If you see: /opt/homebrew/opt/brotli/lib/libbrotlicommon.1.dylib
# Then you MUST fix it:
install_name_tool -id "@rpath/libbrotlicommon.1.dylib" \
  tr4qt.app/Contents/Frameworks/libbrotlicommon.1.dylib

# Repeat for ALL manually copied libraries and frameworks:
install_name_tool -id "@rpath/libdbus-1.3.dylib" \
  tr4qt.app/Contents/Frameworks/libdbus-1.3.dylib

install_name_tool -id "@rpath/QtDBus.framework/Versions/A/QtDBus" \
  tr4qt.app/Contents/Frameworks/QtDBus.framework/Versions/A/QtDBus

# 4B. Fix library dependencies (references to OTHER libraries)
# Check if any bundled library references absolute paths:
otool -L tr4qt.app/Contents/Frameworks/QtDBus.framework/Versions/A/QtDBus
# If you see dependencies like: /opt/homebrew/opt/dbus/lib/libdbus-1.3.dylib
# Then you MUST fix the dependency reference:
install_name_tool -change \
  "/opt/homebrew/opt/dbus/lib/libdbus-1.3.dylib" \
  "@rpath/libdbus-1.3.dylib" \
  tr4qt.app/Contents/Frameworks/QtDBus.framework/Versions/A/QtDBus

# 5. Ensure executable has @rpath set
install_name_tool -add_rpath "@executable_path/../Frameworks" \
  tr4qt.app/Contents/MacOS/tr4qt || true

# 6. Sign everything with ad-hoc signatures (CRITICAL: order matters!)
# install_name_tool invalidates signatures, so we MUST re-sign

# Sign all dylibs in Frameworks/
for lib in tr4qt.app/Contents/Frameworks/*.dylib; do
  if [ -f "$lib" ]; then
    codesign -s - -f "$lib"
  fi
done

# Sign all Qt frameworks
for framework in tr4qt.app/Contents/Frameworks/*.framework; do
  if [ -d "$framework" ]; then
    codesign -s - -f "$framework"
  fi
done

# Sign all plugins
find tr4qt.app/Contents/PlugIns -name "*.dylib" -exec codesign -s - -f {} \;

# Sign the executable
codesign -s - -f tr4qt.app/Contents/MacOS/tr4qt

# Sign the entire app bundle
codesign -s - -f tr4qt.app

# 7. Create DMG with hdiutil (NOT macdeployqt -dmg!)
# macdeployqt -dmg BREAKS signatures even after signing
mkdir -p dmg-contents
cp -R tr4qt.app dmg-contents/
hdiutil create -volname "AppName" -srcfolder dmg-contents -ov -format UDZO app.dmg
rm -rf dmg-contents
```

## Critical Checks: Find ALL Absolute Paths

**ALWAYS** run this check after macdeployqt to find libraries that need fixing:

```bash
# Check for any absolute Homebrew paths in ALL bundled libraries
cd tr4qt.app/Contents

# Check dylibs
for lib in Frameworks/*.dylib; do
  if [ -f "$lib" ]; then
    echo "=== $lib ==="
    otool -L "$lib" | head -3
  fi
done

# Check frameworks
for framework in Frameworks/*.framework; do
  if [ -d "$framework" ]; then
    binary="$framework/Versions/A/$(basename $framework .framework)"
    if [ -f "$binary" ]; then
      echo "=== $binary ==="
      otool -L "$binary" | head -5
    fi
  fi
done

# Or use this one-liner to find ALL absolute paths:
find Frameworks -type f -exec otool -L {} \; 2>/dev/null | \
  grep -E "^\s+/opt/homebrew" | \
  grep -v "@rpath" | \
  sort -u
```

**Expected output** (GOOD - uses @rpath):
```
@rpath/libbrotlicommon.1.dylib (compatibility version 1.0.0, current version 1.2.0)
@rpath/QtCore.framework/Versions/A/QtCore (compatibility version 6.0.0, current version 6.7.2)
```

**Problem output** (BAD - absolute path):
```
/opt/homebrew/opt/brotli/lib/libbrotlicommon.1.dylib (compatibility version 1.0.0, current version 1.2.0)
```

If you see absolute paths, you MUST fix them with install_name_tool as shown above.

## Common macOS Deployment Issues

### Issue 1: App Won't Launch on Other Mac
**Symptom**: DMG opens fine, app shows in Applications, but clicking it does nothing. No error, no UI, no dock icon.

**Diagnosis**:
```bash
# Check crash reports on the target Mac
ls -lt ~/Library/Logs/DiagnosticReports/ | grep tr4qt | head -1
# Look for: SIGKILL (Code Signature Invalid)
```

**Cause**: Invalid code signatures on bundled frameworks.

**Fix**: Sign EVERY component individually (not with `--deep`), AFTER running install_name_tool.

### Issue 2: Missing Library Errors
**Symptom**: Crash report shows `Library not loaded: @rpath/libsomething.dylib`

**Diagnosis**:
```bash
# On the Mac where it built, check what the app links against
otool -L tr4qt.app/Contents/MacOS/tr4qt
otool -L tr4qt.app/Contents/Frameworks/QtGui.framework/Versions/A/QtGui
```

**Cause**: macdeployqt doesn't follow the complete dependency chain.

**Fix**: Manually copy the missing library into Frameworks/, fix its ID with install_name_tool, and re-sign.

### Issue 3: Absolute Path Errors
**Symptom**: Crash report shows `Library not loaded: /opt/homebrew/opt/something/lib/libfoo.dylib`

**Diagnosis**:
```bash
# Check library IDs (the FIRST line of otool -L output)
otool -L tr4qt.app/Contents/Frameworks/libfoo.dylib | head -2
```

**Cause**: Library was copied into bundle but its install ID still points to absolute Homebrew path.

**Fix**: Use install_name_tool to change the ID to @rpath (see workflow above).

## Why Each Step Matters

1. **macdeployqt**: Bundles most Qt frameworks and plugins, but misses dependencies
2. **Manual copying**: macdeployqt doesn't know about non-Qt Homebrew dependencies
3. **install_name_tool**: Changes library IDs from absolute paths to @rpath so they work on any Mac
4. **Code signing**: macOS refuses to load libraries with invalid signatures; install_name_tool breaks signatures so we must re-sign
5. **Individual signing**: `codesign --deep` is unreliable; sign each component separately
6. **hdiutil for DMG**: macdeployqt -dmg breaks signatures; use hdiutil instead

## Testing a DMG Before Release

```bash
# 1. Download the DMG
gh release download v2.95.8 --pattern "*macOS.dmg"

# 2. Mount it
hdiutil attach TR4QT-v2.95.8-macOS.dmg

# 3. Remove quarantine (simulates user doing "Open Anyway")
xattr -cr "/Volumes/AppName/app.app"

# 4. Verify NO absolute paths exist
find "/Volumes/AppName/app.app/Contents/Frameworks" -type f \
  -exec otool -L {} \; 2>/dev/null | \
  grep -E "^\s+/opt/homebrew" | \
  grep -v "@rpath"
# Should return NOTHING. If it shows paths, those libraries will fail on other Macs.

# 5. Try to launch it
"/Volumes/AppName/app.app/Contents/MacOS/app" --version

# 6. Check for crashes
ls -lt ~/Library/Logs/DiagnosticReports/ | grep app | head -1
```

## Automation in CI/CD

The complete workflow is in `.github/workflows/build.yml` under the `build-macos` job, `Create App Bundle` step.

**Critical notes for CI**:
- Run on `macos-latest` (currently macOS 14)
- Build Hamlib from source as unsigned (Homebrew bottles are signed and cause issues)
- Don't use `macdeployqt -dmg`
- Always verify with otool before creating release

## Quick Reference: Essential Commands

```bash
# Check library dependencies
otool -L /path/to/library.dylib

# Check library install ID (first line of otool -L output)
otool -L /path/to/library.dylib | head -2

# Change library install ID to @rpath (what the library calls itself)
install_name_tool -id "@rpath/libname.dylib" /path/to/library.dylib

# Change library dependency to @rpath (what it references)
install_name_tool -change "/absolute/path/to/dependency.dylib" "@rpath/dependency.dylib" /path/to/library.dylib

# Add rpath to executable
install_name_tool -add_rpath "@executable_path/../Frameworks" /path/to/executable

# Sign with ad-hoc signature (for local distribution)
codesign -s - -f /path/to/file

# Verify code signature
codesign -vvv /path/to/file

# Find all Homebrew absolute paths in app bundle
find MyApp.app/Contents/Frameworks -type f -exec otool -L {} \; 2>/dev/null | \
  grep "/opt/homebrew" | grep -v "@rpath" | sort -u
```
