# Claude Development Notes for TR4QT

This file contains important reminders and conventions for Claude when working on TR4QT.

## ⚠️ CRITICAL REMINDERS

### Always Bump Version Before Building
**NEVER build without updating the version constant first!**

Before making any code changes:
1. Edit `/src/core/Constants.h`
2. Increment `APP_VERSION` appropriately
3. Update the comment with what's changing
4. Then make your code changes
5. Build with `make tr4qt -j4`
6. Commit with version in message

This prevents confusion about which version is running.

## Version Management

**CRITICAL**: The version number must be updated with every release commit.

Location: `/src/core/Constants.h`
```cpp
constexpr const char* APP_VERSION = "X.Y.Z";
```

Convention:
- **Major (X)**: Major features or breaking changes
- **Minor (Y)**: New features, enhancements
- **Patch (Z)**: Bug fixes, small improvements

Update process:
1. Increment version in Constants.h
2. Update comment to describe the change
3. Include version in commit message: "Feature description - vX.Y.Z"
4. Rebuild: `cmake --build build`

Example:
```cpp
constexpr const char* APP_VERSION = "2.58.0";  // Zone lookup from cty.dat, manual band selection fixes
```

## Build Process

Standard build:
```bash
cmake --build build
```

Clean rebuild:
```bash
rm -rf build && cmake -B build && cmake --build build
```

Run after build:
```bash
# IMPORTANT: Always kill any running instances before starting a new one
pkill -9 tr4qt
./build/src/tr4qt.app/Contents/MacOS/tr4qt
```

**CRITICAL**: Always `pkill -9 tr4qt` before running the application to avoid multiple instances running simultaneously.

## Testing

Run tests:
```bash
cd build && ctest --output-on-failure
```

## Git Workflow

Commit message format:
```
Brief description of change - vX.Y.Z

Detailed explanation of what changed and why.

Changes:
- Bullet point list of specific changes
- Include file locations if helpful

🤖 Generated with [Claude Code](https://claude.com/claude-code)

Co-Authored-By: Claude Sonnet 4.5 <noreply@anthropic.com>
```

## Pre-Release Checklist

**CRITICAL**: Before creating any release tag, ALWAYS run through this checklist!

Release tags trigger GitHub Actions builds for all platforms. A failed build wastes time and creates confusion.

### 1. Verify Qt Module Dependencies Match
Compare CMakeLists.txt with Windows CI configuration:

```bash
# Check what Qt components TR4QT requires
grep -A 10 "find_package(Qt6 REQUIRED COMPONENTS" CMakeLists.txt

# Check what Windows CI installs
grep -A 5 "modules:" .github/workflows/build.yml
```

**Rule**: Every Qt component in CMakeLists.txt (except base modules) must be listed in the Windows CI `modules:` line.

**Base modules** (always installed, don't need to be listed):
- Core, Gui, Widgets, Network, Sql, PrintSupport, Concurrent, Test, OpenGL, Xml

**Additional modules** (must be explicitly listed):
- HttpServer (currently required by TR4QT)
- WebSockets, WebView, Multimedia, Charts, 3D, etc. (add if needed)

**macOS note**: `brew install qt@6` includes all modules, so macOS builds won't catch this issue.

### 2. Verify Last CI Build Passed
```bash
gh run list --limit 3
```

Both macOS and Windows builds must show ✓ success. If either failed, fix the issue before tagging.

### 3. Version Already Bumped
Confirm Constants.h has been updated in the latest commit:
```bash
grep APP_VERSION src/core/Constants.h
git log -1 --oneline
```

Version in Constants.h should match the commit message.

### 4. Build and Test Locally
```bash
cmake --build build
cd build && ctest --output-on-failure
```

All tests should pass. If any fail, investigate before releasing.

### 5. Check for Uncommitted Changes
```bash
git status
```

Should show "nothing to commit, working tree clean".

### 6. Create Release Tag (Only After Above Checks Pass)
```bash
# Lightweight tag (testing)
git tag v2.95.0
git push origin v2.95.0

# Annotated tag (official releases)
git tag -a v2.95.0 -m "Release v2.95.0 - WebServer pull model refactoring"
git push origin v2.95.0
```

### 7. Monitor GitHub Actions
```bash
gh run watch
```

Watch the triggered build. Both platforms must succeed for a valid release.

### Common Issues Caught By This Checklist:
- ✅ Missing Qt modules in Windows CI (e.g., HttpServer)
- ✅ Version mismatch between code and tag
- ✅ Tests failing on specific platforms
- ✅ Build errors from uncommitted changes
- ✅ CI configuration drift from CMakeLists.txt

**Remember**: Release tags are permanent in the git history. It's better to spend 2 minutes verifying than to create a broken release tag!

## Common Patterns

### Radio State
Radio frequency/band/mode is stored in `m_currentState` (RadioState struct):
- `frequencyA`: freq_t (Hz)
- `bandA`: BandType enum
- `modeA`: ModeType enum

### Country Lookup
```cpp
CountryData countryData = m_countryFile.lookup(callsign);
if (countryData.isValid()) {
    int zone = countryData.cqZone;
    QString country = countryData.name;
    QString continent = continentToString(countryData.continent);
}
```

### Exchange Prediction
Uses InitialExchangeManager singleton:
```cpp
QString prediction = InitialExchangeManager::instance().predictExchange(
    callsign,
    m_activeContest,
    m_currentState.modeA
);
```

Strategy priority:
1. Exchange memory (exact match)
2. CTY.DAT lookup (zone/country)
3. Contest defaults (RST)

### Database Operations
Always check for active contest:
```cpp
if (m_hasActiveContest) {
    QSORepository repo;
    if (!repo.saveQSO(qso, m_currentContestDbId)) {
        LOG_WARN("Tag", QString("Failed: %1").arg(repo.lastError()));
    }
}
```

### Dialog Messages
**CRITICAL**: ALL dialog messages MUST go through DialogHelper. Never use QMessageBox directly.

This ensures:
- All dialogs are automatically logged (message + user response)
- Text is selectable/copyable by default for error messages
- Consistent user experience across the application
- Easier debugging via log analysis

Use DialogHelper for all user-facing dialogs:
```cpp
// Question dialog
QMessageBox::StandardButton reply = DialogHelper::question(
    this,
    "Confirm Action",
    "Are you sure you want to proceed?"
);

// Information
DialogHelper::information(this, "Success", "Operation completed successfully.");

// Warning
DialogHelper::warning(this, "Warning", "This action cannot be undone.");

// Critical error
DialogHelper::critical(this, "Error", QString("Failed: %1").arg(errorMessage));
```

**NEVER** use QMessageBox::question/information/warning/critical directly. Always use DialogHelper.

Files:
- `/src/utils/DialogHelper.h` - Dialog wrapper with logging
- `/src/utils/DialogHelper.cpp` - Implementation

**This pattern applies to all projects**: If one dialog is logged, ALL dialogs must be logged via a common helper class.

## Known Issues / Limitations

### Database Threading (Deferred until Networking Implementation)
**Issue**: Database singleton is not thread-safe for concurrent writes (discovered 2025-12-27)

**Status**: Deferred until TCP networking implementation
- Single-operator use: SAFE (99% of users)
- Multi-threaded concurrent writes: CRASHES
- Test coverage: `tests/test_qso_load_performance.cpp` (concurrent tests disabled)

**Performance Verified**:
- Single-threaded: 6,800+ QSOs/second
- Transaction time: <1ms average
- Database throughput: 24M+ QSOs/hour theoretical max

**Solution Required Before**:
- TCP-based networked TR4QT (multi-station logging)
- True multi-operator concurrent logging

**Implementation Options** (documented in `src/data/Database.cpp`):
1. Connection pool (thread_local connections)
2. Serialized access (QMutex)
3. Message queue architecture (recommended for TCP server)

**Reference**: See comprehensive TODO in `/src/data/Database.cpp` line 13

## Architecture Notes

### Contest System
- Base class: `ContestBase` (abstract)
- Contest implementations: `CQWWContest`, `CQWPXContest`, `WinterFieldDayContest`
- Auto-registration: `REGISTER_CONTEST(ClassName, "ID")`
- Factory access: `ContestRegistry::instance().createContest("ID")`

### Exchange Fields
Contests define received/sent exchange fields:
```cpp
QList<ExchangeField> getReceivedExchangeFields() const override;
QList<ExchangeField> getSentExchangeFields() const override;
```

QSOTableModel adapts columns based on contest exchange fields.

### Duplicate Checking
Each contest defines duplicate rule:
```cpp
DuplicateCheckingRule getDuplicateCheckingRule() const override {
    return DuplicateCheckingRule::PerBandMode;
}
```

Options:
- `PerBandMode`: Same call/band/mode is dupe
- `AllBandMode`: Same call/mode across all bands
- `PerBand`: Same call/band (any mode)
- `AllBand`: Same call (once per contest)

## Key Files

### Core
- `/src/core/Constants.h` - **VERSION HERE**
- `/src/core/Types.h` - Enums and type definitions
- `/src/models/QSO.h` - QSO record structure

### UI
- `/src/ui/MainWindow.cpp` - Main application logic
- `/src/ui/models/QSOTableModel.cpp` - Log display

### Contests
- `/src/contests/ContestBase.h` - Contest interface
- `/src/contests/CQWWContest.cpp` - CQ WW implementation
- `/src/contests/CQWPXContest.cpp` - CQ WPX implementation

### Exchange System
- `/src/exchanges/InitialExchangeManager.cpp` - Exchange prediction
- `/src/data/ExchangeMemoryRepository.cpp` - Exchange memory

### Country Data
- `/src/utils/CountryFile.cpp` - CTY.DAT parser
- `~/.tr4qt/cty.dat` - Country file data

## Recent Changes (Session Summary)

### v2.57.x - Manual Band Selection
- Fixed focus issues (callsign field gets focus on startup)
- Fixed duplicate checking (band/mode stored as TEXT not enums)
- Implemented manual band selection when radio disconnected
- Added ALT-B/ALT-V (Option-B/Option-V on Mac) for band up/down
- Set frequency to band edge for manual selection (visual indicator)
- Fixed frequency display without radio connection

### v2.58.0 - Zone Lookup
- Implemented country/zone lookup from cty.dat when logging QSO
- Zones now auto-fill in exchange field (via InitialExchangeManager)
- Zones display in "Zn" column of QSO table
- Geographic fields populated: CQ Zone, ITU Zone, Country, Continent

## Known Issues / TODOs

From code comments:
- [ ] Calculate QSO points via contest (line 1385)
- [ ] Check for new multipliers (line 1386)
- [ ] Improve exchange prediction architecture (per plan file)

## macOS App Deployment Guide

**CRITICAL**: This section documents the complete process for creating distributable macOS app bundles. These lessons apply to ANY Mac app, not just TR4QT.

### The Problem: Homebrew Dependencies Don't Work on Other Macs

When you build a Qt app on macOS with Homebrew Qt/libraries, the app works on YOUR Mac but crashes on other Macs. Why?

1. **Absolute Paths**: Homebrew libraries have absolute path IDs like `/opt/homebrew/opt/qt/lib/QtCore.framework`
2. **Missing Dependencies**: macdeployqt doesn't follow complete dependency chains
3. **Invalid Signatures**: Modifying bundled libraries with install_name_tool invalidates code signatures
4. **Incomplete Bundling**: macdeployqt misses transitive dependencies (e.g., bundles libbrotlidec but not libbrotlicommon)

### Complete Deployment Workflow

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

### Critical Checks: Find ALL Absolute Paths

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

### Common macOS Deployment Issues

#### Issue 1: App Won't Launch on Other Mac
**Symptom**: DMG opens fine, app shows in Applications, but clicking it does nothing. No error, no UI, no dock icon.

**Diagnosis**:
```bash
# Check crash reports on the target Mac
ls -lt ~/Library/Logs/DiagnosticReports/ | grep tr4qt | head -1
# Look for: SIGKILL (Code Signature Invalid)
```

**Cause**: Invalid code signatures on bundled frameworks.

**Fix**: Sign EVERY component individually (not with `--deep`), AFTER running install_name_tool.

#### Issue 2: Missing Library Errors
**Symptom**: Crash report shows `Library not loaded: @rpath/libsomething.dylib`

**Diagnosis**:
```bash
# On the Mac where it built, check what the app links against
otool -L tr4qt.app/Contents/MacOS/tr4qt
otool -L tr4qt.app/Contents/Frameworks/QtGui.framework/Versions/A/QtGui
```

**Cause**: macdeployqt doesn't follow the complete dependency chain.

**Fix**: Manually copy the missing library into Frameworks/, fix its ID with install_name_tool, and re-sign.

#### Issue 3: Absolute Path Errors
**Symptom**: Crash report shows `Library not loaded: /opt/homebrew/opt/something/lib/libfoo.dylib`

**Diagnosis**:
```bash
# Check library IDs (the FIRST line of otool -L output)
otool -L tr4qt.app/Contents/Frameworks/libfoo.dylib | head -2
```

**Cause**: Library was copied into bundle but its install ID still points to absolute Homebrew path.

**Fix**: Use install_name_tool to change the ID to @rpath (see workflow above).

### Why Each Step Matters

1. **macdeployqt**: Bundles most Qt frameworks and plugins, but misses dependencies
2. **Manual copying**: macdeployqt doesn't know about non-Qt Homebrew dependencies
3. **install_name_tool**: Changes library IDs from absolute paths to @rpath so they work on any Mac
4. **Code signing**: macOS refuses to load libraries with invalid signatures; install_name_tool breaks signatures so we must re-sign
5. **Individual signing**: `codesign --deep` is unreliable; sign each component separately
6. **hdiutil for DMG**: macdeployqt -dmg breaks signatures; use hdiutil instead

### Testing a DMG Before Release

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

### Automation in CI/CD

The complete workflow is in `.github/workflows/build.yml` under the `build-macos` job, `Create App Bundle` step.

**Critical notes for CI**:
- Run on `macos-latest` (currently macOS 14)
- Build Hamlib from source as unsigned (Homebrew bottles are signed and cause issues)
- Don't use `macdeployqt -dmg`
- Always verify with otool before creating release

### Quick Reference: Essential Commands

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

## Mac-Specific Notes

Keyboard shortcuts:
- Qt::ALT maps to Option (⌥) on Mac
- Qt::CTRL maps to Command (⌘) on Mac
- Displayed automatically in menus with correct symbols

Path handling:
- Use QDir::homePath() for cross-platform home directory
- Config: `~/.tr4qt/`
- Logs: `~/.tr4qt/logs/`
- Backups: `~/.tr4qt/backups/`
