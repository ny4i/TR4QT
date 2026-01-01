# Claude Development Notes for TR4QT

This file contains important reminders and conventions for Claude when working on TR4QT.

## 🏗️ BUILD PHILOSOPHY - APPLIES TO ALL PROJECTS

### Explicit > Implicit (when implicit is unreliable)

**NEVER use "automatic" deployment tools that don't work reliably.**

Common unreliable tools:
- ❌ `windeployqt` (Windows) - Silently fails to copy plugins, inconsistent
- ❌ `macdeployqt` (macOS) - Misses dependencies, gets paths wrong
- ❌ `linuxdeploy` (Linux) - Complex, hard to debug, inconsistent
- ❌ **ALL OF THEM** - None bundle Qt TLS plugins by default (critical for HTTPS!)

**Why these tools are problematic:**
1. **Silent failures** - Missing dependencies without clear errors
2. **False confidence** - "I used the official tool, so it should work"
3. **Wastes time** - Hours debugging what the tool "forgot"
4. **Not deterministic** - Same inputs can produce different outputs
5. **Hard to debug** - Don't know what they did vs. didn't do

**Instead: Be Explicit**

✅ **List every dependency explicitly**
✅ **Copy every file with clear commands**
✅ **Verify what was deployed**
✅ **Clear errors when something fails**

**Example (Windows):**
```bash
# BAD: Hope windeployqt works
windeployqt tr4qt.exe --sql  # Maybe copies SQL plugin? Maybe not?

# GOOD: Explicit
cp Qt6Core.dll Qt6Gui.dll Qt6Widgets.dll ...  # Know exactly what's copied
mkdir sqldrivers && cp qsqlite.dll sqldrivers/  # SQL plugin always included
ls -la sqldrivers/  # Verify it's there
```

**Example (macOS):**
```bash
# BAD: Trust macdeployqt to bundle everything
macdeployqt tr4qt.app
# Result: App launches but HTTPS fails with "No functional TLS backend was found"
# macdeployqt silently forgot TLS plugins!

# GOOD: Explicit TLS plugin bundling
macdeployqt tr4qt.app
mkdir -p tr4qt.app/Contents/PlugIns/tls
cp /opt/homebrew/opt/qtbase/share/qt/plugins/tls/*.dylib tr4qt.app/Contents/PlugIns/tls/
ls -la tr4qt.app/Contents/PlugIns/tls/  # Verify all 3 TLS backends present
# Result: App works perfectly, HTTPS downloads succeed
```

**Benefits of Explicit Deployment:**
- Deterministic (same result every time)
- Transparent (see exactly what's deployed)
- Debuggable (clear errors when missing)
- Maintainable (easy to add/remove dependencies)
- Reliable (no silent failures)

**This philosophy applies to ALL projects, not just TR4QT.**

If you encounter an "automatic" tool that doesn't work reliably:
1. Don't keep using it hoping it will work this time
2. Replace it with explicit, deterministic steps
3. Document exactly what needs to be deployed

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

### Adding New Qt Modules - Update CI Configuration!
**CRITICAL**: When adding new Qt modules to CMakeLists.txt, you MUST update the Windows CI configuration!

Why this matters:
- macOS: `brew install qt@6` includes ALL modules (won't catch the issue)
- Windows: `install-qt-action` only installs base modules + explicitly listed modules
- If you forget to update CI, Windows builds will fail with "Qt6ModuleName not found"

**Workflow when adding a new Qt module:**

1. Add to `/CMakeLists.txt`:
   ```cmake
   find_package(Qt6 REQUIRED COMPONENTS
       Core Widgets Network
       NewModule  # ← Added here
   )
   ```

2. Add to `/src/CMakeLists.txt`:
   ```cmake
   target_link_libraries(tr4qt PRIVATE
       Qt6::Core Qt6::Widgets
       Qt6::NewModule  # ← Added here
   )
   ```

3. **CRITICAL**: Add to `/.github/workflows/build.yml` Windows CI:
   ```yaml
   - name: Install Qt
     uses: jurplel/install-qt-action@v4
     with:
       modules: 'qtwebsockets qthttpserver qtnewmodule'  # ← Added here
   ```

**Base modules** (always installed, don't need to be listed in CI):
- Core, Gui, Widgets, Network, Sql, PrintSupport, Concurrent, Test, OpenGL, Xml

**Additional modules** (MUST be explicitly listed in CI):
- WebEngine, WebEngineWidgets, WebChannel, Positioning, WebSockets, HttpServer
- Multimedia, Charts, 3D, etc.

**Verification after adding:**
```bash
# Check CMakeLists.txt matches Windows CI
grep -A 10 "find_package(Qt6 REQUIRED COMPONENTS" CMakeLists.txt
grep "modules:" .github/workflows/build.yml
```

**Real example that broke CI:**
- Added Qt WebEngineWidgets for map viewer in CMakeLists.txt
- Forgot to add `qtwebengine qtwebchannel qtpositioning` to Windows CI
- macOS build succeeded (all modules available), Windows failed
- Fixed by adding modules to `install-qt-action` configuration

This prevents confusion about which version is running.

### Check for Existing Data Before Loading/Creating

**CRITICAL**: Before loading data from files or creating new instances, ALWAYS check if the data already exists in the application.

**The Problem**:
- Loading duplicate copies of data wastes memory
- Creates synchronization issues (which copy is authoritative?)
- Can cause race conditions if one copy is updated
- Violates Single Source of Truth principle

**Examples of what to check**:
- ✅ Does MainWindow already have `m_countryFile` loaded?
- ✅ Is there a GlobalDatabase with DXCC entities?
- ✅ Does a singleton already exist for this data?
- ✅ Can I get a pointer/reference to existing data?

**Bad Pattern** (what NOT to do):
```cpp
// ADIFImportDialog.cpp
void ADIFImportDialog::onImportClicked() {
    // ❌ BAD: Loading own copy of cty.dat from filesystem
    CountryFile countryFile;
    countryFile.loadFromFile("~/.tr4qt/cty.dat");

    // ❌ Creates duplicate, wastes memory, no sync with MainWindow's copy
}
```

**Good Pattern** (what TO do):
```cpp
// ADIFImportDialog.h
class ADIFImportDialog : public QDialog {
public:
    // ✅ GOOD: Receive pointer to existing data
    ADIFImportDialog(CountryFile* countryFile, QWidget* parent);

private:
    CountryFile* m_countryFile;  // Pointer to MainWindow's CountryFile
};

// Usage in MainWindow
ADIFImportDialog dialog(&m_countryFile, this);
```

**Checklist before loading/creating data**:
1. Search codebase for existing instances (`grep -r "CountryFile m_"`)
2. Check for singletons (`SomeClass::instance()`)
3. Check GlobalDatabase for tables with the data
4. Ask: "Who is the authoritative owner of this data?"
5. Pass pointer/reference instead of creating duplicate

**Real-world example**:
- **Issue**: ADIFImportDialog was loading cty.dat from filesystem
- **Problem**: MainWindow already had `m_countryFile` loaded
- **Fix**: Pass `&m_countryFile` to dialog constructor
- **Benefit**: No duplicate load, always uses same data, no sync issues

**Why this matters**:
- Thread safety: Multiple copies can get out of sync during reload
- Memory efficiency: Large files (like cty.dat) shouldn't be duplicated
- Single Source of Truth: One authoritative copy prevents conflicts
- Performance: Loading from disk is slow, reuse what's in memory

**This principle applies to ALL projects, not just TR4QT.**

### ALWAYS Use DialogHelper for User Dialogs

**CRITICAL**: ALL dialog messages MUST go through DialogHelper. Never use QMessageBox directly.

**Why this matters**:
- All dialogs are automatically logged (message + user response)
- Text is selectable/copyable by default for error messages
- Consistent user experience across the application
- Easier debugging via log analysis

**Pattern to follow**:
```cpp
// ❌ NEVER do this:
QMessageBox::question(this, "Title", "Message");
QMessageBox::information(this, "Title", "Message");
QMessageBox::warning(this, "Title", "Message");
QMessageBox::critical(this, "Title", "Error");

// ✅ ALWAYS use DialogHelper:
#include "../../utils/DialogHelper.h"

// Question dialog (returns QMessageBox::StandardButton)
QMessageBox::StandardButton reply = DialogHelper::question(
    this,
    "Confirm Action",
    "Are you sure you want to proceed?"
);
if (reply == QMessageBox::Yes) {
    // User clicked Yes
}

// Information
DialogHelper::information(this, "Success", "Operation completed successfully.");

// Warning
DialogHelper::warning(this, "Warning", "This action cannot be undone.");

// Critical error
DialogHelper::critical(this, "Error", QString("Failed: %1").arg(errorMessage));
```

**What DialogHelper does automatically**:
1. Logs the dialog message to debug log
2. Logs user's response (Yes/No/Cancel/etc.)
3. Makes text selectable in error dialogs
4. Consistent styling across the app

**Files**:
- `/src/utils/DialogHelper.h` - Dialog wrapper with logging
- `/src/utils/DialogHelper.cpp` - Implementation

**Note**: Using the return type `QMessageBox::StandardButton` is correct - that's what DialogHelper returns. The key is calling `DialogHelper::question()` instead of `QMessageBox::question()`.

**This pattern applies to all projects**: If one dialog is logged, ALL dialogs must be logged via a common helper class.

### ALWAYS Avoid Magic Numbers

**CRITICAL**: NEVER use literal numbers directly in code. Always use named constants.

**Why this matters**:
- Magic numbers are unclear: What does `12` mean? Field width? Buffer size? Timeout?
- Hard to maintain: Change one occurrence, might miss others
- Error-prone: Easy to use wrong value (typo, copy-paste)
- Poor readability: Named constants document intent

**Pattern to follow**:
```cpp
// ❌ BAD: Magic numbers
QString formatted = QString("%1 %2").arg(callsign, -12).arg(frequency, 10);
m_label->setMaximumHeight(45);
QTimer::singleShot(3000, this, &Widget::refresh);

// ✅ GOOD: Named constants
const int CALLSIGN_FIELD_WIDTH = 12;
const int FREQUENCY_FIELD_WIDTH = 10;
QString formatted = QString("%1 %2")
    .arg(callsign, -CALLSIGN_FIELD_WIDTH)
    .arg(frequency, FREQUENCY_FIELD_WIDTH);

const int LABEL_HEIGHT = m_label->fontMetrics().height() + 10;  // Font height + padding
m_label->setMaximumHeight(LABEL_HEIGHT);

const int REFRESH_INTERVAL_MS = 3000;  // 3 seconds
QTimer::singleShot(REFRESH_INTERVAL_MS, this, &Widget::refresh);
```

**Guidelines**:
1. **Name meaningfully**: `CALLSIGN_FIELD_WIDTH` not `WIDTH_1`
2. **Calculate when possible**: Use `fontMetrics().height()` instead of hardcoded pixels
3. **Add comments**: Explain what the value represents and why it's that value
4. **Use `const` or `constexpr`**: Prevent accidental modification
5. **Scope appropriately**: File-level for reused values, function-level for one-time use

**Common magic numbers to avoid**:
- Field widths and spacing (use named constants)
- Timeout values (name with units: `_MS`, `_SECONDS`)
- Buffer sizes (document why that size)
- Pixel dimensions (calculate from font metrics when possible)
- Array indices (use enum or named constants)

**Exceptions** (when literals are OK):
- Mathematical constants: `0`, `1`, `2` in formulas (e.g., `x * 2`)
- Boolean values: `true`, `false`
- Null/empty checks: `nullptr`, `0`, `-1` for "not found"
- Loop counters: `for (int i = 0; i < n; i++)`

**Real-world examples from TR4QT**:
```cpp
// ❌ Before: Magic 3
QString formatted = QString("%1%2 %3%4%5")
    .arg(freqStr, 10)
    .arg(QString(3, ' '))  // What is 3?
    .arg(callsign, -12);

// ✅ After: Named constant
const int CALLSIGN_INDENT = 3;  // Spaces between frequency and callsign
QString formatted = QString("%1%2 %3%4%5")
    .arg(freqStr, 10)
    .arg(QString(CALLSIGN_INDENT, ' '))
    .arg(callsign, -12);
```

**This principle applies to ALL projects, not just TR4QT.**

## Version Management

**CRITICAL**: The version number must be updated with every release commit IN FOUR PLACES!

Locations:
1. **Application**: `/src/core/Constants.h`
   ```cpp
   constexpr const char* APP_VERSION = "X.Y.Z";
   ```

2. **Windows Installer**: `/installer/tr4qt.nsi`
   ```nsis
   !define APPVERSION "X.Y.Z"
   ```

3. **macOS Bundle**: `/src/CMakeLists.txt`
   ```cmake
   MACOSX_BUNDLE_SHORT_VERSION_STRING "X.Y.Z"
   MACOSX_BUNDLE_BUNDLE_VERSION "X.Y.Z"
   ```

4. **Windows Resource File**: `/resources/tr4qt.rc`
   ```c
   #define VER_FILEVERSION             X,Y,Z,0
   #define VER_FILEVERSION_STR         "X.Y.Z.0\0"
   #define VER_PRODUCTVERSION          X,Y,Z,0
   #define VER_PRODUCTVERSION_STR      "X.Y.Z\0"
   ```

Convention:
- **Major (X)**: Major features or breaking changes
- **Minor (Y)**: New features, enhancements
- **Patch (Z)**: Bug fixes, small improvements

Update process:
1. Increment version in Constants.h
2. Update comment to describe the change
3. **Update version in tr4qt.nsi to match**
4. **Update version in CMakeLists.txt to match**
5. **Update version in tr4qt.rc to match (4 places!)**
6. **Update CHANGELOG.md**: Move items from `[Unreleased]` to new version section with date
7. Include version in commit message: "Feature description - vX.Y.Z"
8. Rebuild: `cmake --build build`

Example (Constants.h):
```cpp
constexpr const char* APP_VERSION = "3.8.1";  // Streamlined ADIF import with checkbox rescore
```

Example (tr4qt.nsi):
```nsis
!define APPVERSION "3.8.1"
```

Example (CMakeLists.txt):
```cmake
MACOSX_BUNDLE_SHORT_VERSION_STRING "3.8.1"
MACOSX_BUNDLE_BUNDLE_VERSION "3.8.1"
```

Example (tr4qt.rc):
```c
#define VER_FILEVERSION             3,8,1,0
#define VER_FILEVERSION_STR         "3.8.1.0\0"
#define VER_PRODUCTVERSION          3,8,1,0
#define VER_PRODUCTVERSION_STR      "3.8.1\0"
```

**Why all four files?**
- Constants.h: Used by the running application for "About" dialog, window titles, etc.
- tr4qt.nsi: Used by the Windows installer filename and registry entries
- CMakeLists.txt: Used by macOS Info.plist (shown in crash reports and About dialog)
- tr4qt.rc: Used by Windows .exe Properties → Details tab (File Version, Product Version, Company)
- If they don't match, users and crash reports show inconsistent versions

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

UI Enhancements:
- [ ] **DialogHelper Cmd-C Support**: Add keyboard copy (Cmd-C/Ctrl-C) support for dialog text
  - Current: Right-click → Copy works (text selectable)
  - Desired: Cmd-C/Ctrl-C keyboard shortcut for copying dialog text
  - Challenge: QLabel doesn't support keyboard copy shortcuts natively
  - Potential solutions:
    1. Replace QLabel with QTextEdit in custom QMessageBox subclass
    2. Event filter approach (attempted but caused stability issues)
    3. Use Qt's TextBrowserInteraction (attempted but didn't work reliably)
  - Status: Deferred - not a show-stopper, right-click copy is functional

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

## Windows App Deployment Guide

**CRITICAL**: This section documents the complete process for creating distributable Windows applications. These lessons apply to ANY Windows Qt app, not just TR4QT.

### The Problem: Missing DLL Hell

When you build a Qt app on Windows, the .exe works on YOUR machine but fails on other machines with "missing DLL" errors. Why?

1. **Qt DLLs not bundled**: Your dev machine has Qt in PATH, but users don't
2. **Silent omissions**: `windeployqt` doesn't copy everything (especially HttpServer, TLS plugins)
3. **Runtime dependencies**: MinGW runtime DLLs, Hamlib, libusb not included
4. **Plugin directories**: Qt requires plugins in specific subdirectories

### Complete Deployment Workflow

**Use the explicit deployment script:**

```batch
# From project root on Windows
.\scripts\windows-deploy.bat
```

**What the script does** (`scripts/windows-deploy.bat`):

1. **Copies Qt DLLs explicitly** (no windeployqt):
   - Qt6Core.dll, Qt6Gui.dll, Qt6Widgets.dll, Qt6Network.dll
   - Qt6Sql.dll, Qt6HttpServer.dll (windeployqt forgets this!)
   - Qt6PrintSupport.dll, Qt6Concurrent.dll, Qt6WebSockets.dll

2. **Copies MinGW runtime DLLs**:
   - libgcc_s_seh-1.dll
   - libstdc++-6.dll
   - libwinpthread-1.dll

3. **Copies Qt plugins** (CRITICAL - app won't run without these):
   - `platforms\qwindows.dll` - Windows GUI support
   - `sqldrivers\qsqlite.dll` - Database access
   - `tls\*.dll` - HTTPS connections (qopensslbackend, qschannelbackend, qcertonlybackend)
   - `styles\qwindowsvistastyle.dll` - Native Windows appearance

4. **Copies Hamlib and dependencies**:
   - libhamlib-4.dll (radio control)
   - libusb-1.0.dll (USB radio support)

5. **Creates qt.conf** - Tells Qt where to find plugins

6. **Verifies deployment** - Lists all deployed files and checks for missing critical components

### Manual Deployment (if script unavailable)

If you need to deploy manually:

```batch
cd build\src

# Qt Core DLLs
copy c:\Qt\6.10.1\mingw_64\bin\Qt6Core.dll .
copy c:\Qt\6.10.1\mingw_64\bin\Qt6Gui.dll .
copy c:\Qt\6.10.1\mingw_64\bin\Qt6Widgets.dll .
copy c:\Qt\6.10.1\mingw_64\bin\Qt6Network.dll .
copy c:\Qt\6.10.1\mingw_64\bin\Qt6Sql.dll .
copy c:\Qt\6.10.1\mingw_64\bin\Qt6HttpServer.dll .
copy c:\Qt\6.10.1\mingw_64\bin\Qt6PrintSupport.dll .
copy c:\Qt\6.10.1\mingw_64\bin\Qt6Concurrent.dll .
copy c:\Qt\6.10.1\mingw_64\bin\Qt6WebSockets.dll .

# MinGW runtime
copy c:\Qt\6.10.1\mingw_64\bin\libgcc_s_seh-1.dll .
copy c:\Qt\6.10.1\mingw_64\bin\libstdc++-6.dll .
copy c:\Qt\6.10.1\mingw_64\bin\libwinpthread-1.dll .

# Qt Plugins (CRITICAL!)
mkdir platforms
copy c:\Qt\6.10.1\mingw_64\plugins\platforms\qwindows.dll platforms\

mkdir sqldrivers
copy c:\Qt\6.10.1\mingw_64\plugins\sqldrivers\qsqlite.dll sqldrivers\

mkdir tls
copy c:\Qt\6.10.1\mingw_64\plugins\tls\*.dll tls\

# Hamlib
copy c:\projects\hamlib\bin\libhamlib-4.dll .
copy c:\projects\hamlib\bin\libusb-1.0.dll .

# Create qt.conf
echo [Paths] > qt.conf
echo Plugins = . >> qt.conf
```

### Common Windows Deployment Issues

#### Issue 1: "Qt6HttpServer.dll is missing"
**Symptom**: App fails to launch with popup about missing Qt6HttpServer.dll

**Cause**: `windeployqt` doesn't know about HttpServer module (it's not a standard Qt module)

**Fix**: Manually copy `Qt6HttpServer.dll` from Qt bin directory

#### Issue 2: "The application failed to start because no Qt platform plugin could be initialized"
**Symptom**: Black screen or immediate crash with this error message

**Diagnosis**:
```batch
# Check if platforms directory exists
dir platforms

# Check if qwindows.dll exists
dir platforms\qwindows.dll
```

**Cause**: Missing `platforms\qwindows.dll` plugin or missing qt.conf

**Fix**:
1. Copy `qwindows.dll` to `platforms\` subdirectory
2. Create qt.conf with `[Paths]` and `Plugins = .`

#### Issue 3: Database doesn't work (can't create/open logs)
**Symptom**: Application runs but can't create contests or open logs

**Diagnosis**: Check for SQL driver plugin
```batch
dir sqldrivers\qsqlite.dll
```

**Cause**: Missing `sqldrivers\qsqlite.dll` plugin

**Fix**: Copy qsqlite.dll to `sqldrivers\` subdirectory

#### Issue 4: HTTPS downloads fail (CTY.DAT, LOTW updates)
**Symptom**: Application runs but downloading country file or LOTW data fails with TLS errors

**Diagnosis**: Check for TLS plugins
```batch
dir tls\*.dll
```

**Cause**: Missing TLS backend plugins (windeployqt **always** forgets these!)

**Fix**: Copy all TLS plugins from `Qt\plugins\tls\` to `tls\` subdirectory:
- qopensslbackend.dll (OpenSSL TLS)
- qschannelbackend.dll (Windows native TLS - recommended)
- qcertonlybackend.dll (certificate-only)

### Why Each Step Matters

1. **Explicit DLL copying**: Know exactly what's deployed, no surprises
2. **Qt plugins in subdirectories**: Qt requires plugins in specific locations relative to .exe
3. **qt.conf file**: Tells Qt to look in current directory for plugins (not Qt installation)
4. **Verification**: Catch missing files before distribution, not after users report issues

### Testing a Deployment Before Distribution

```batch
# 1. Build the application
cmake --build build --config Release

# 2. Run deployment script
.\scripts\windows-deploy.bat

# 3. Test the deployed exe
cd build\src
tr4qt.exe --version

# 4. Verify all DLLs are present
dir *.dll
dir platforms\*.dll
dir sqldrivers\*.dll
dir tls\*.dll

# 5. Test on a clean Windows VM (best practice)
# Copy build\src\ folder to a machine WITHOUT Qt or dev tools installed
# Should run without any DLL errors
```

### Automation in CI/CD

The complete workflow is in `.github/workflows/build.yml` under the `build-windows` job.

**Critical notes for CI**:
- Build on `windows-latest` runner
- Use MinGW or MSVC (specify in CMAKE_PREFIX_PATH)
- Run `windows-deploy.bat` after build
- Create ZIP or installer with all files from deploy directory
- Test the packaged app on the runner before creating release

### Quick Reference: Windows Deployment Checklist

Before distributing a Windows build:

- [ ] All Qt6*.dll files present in exe directory
- [ ] All MinGW runtime DLLs present (libgcc, libstdc++, libwinpthread)
- [ ] `platforms\qwindows.dll` exists
- [ ] `sqldrivers\qsqlite.dll` exists
- [ ] `tls\*.dll` plugins exist (at least one TLS backend)
- [ ] `libhamlib-4.dll` and `libusb-1.0.dll` present
- [ ] `qt.conf` file created
- [ ] Tested `tr4qt.exe --version` from deployed directory
- [ ] Tested full GUI launch from deployed directory
- [ ] (Optional) Tested on clean Windows machine without Qt/dev tools

### Common Windows Build Errors

#### Error: "interface" is a reserved keyword
**Symptom**:
```
error: expected ',' or '...' before 'struct'
void sendDiscoveryMessage(const QNetworkInterface& interface);
                                                   ^~~~~~~~~
```

**Cause**: Windows COM headers define `#define interface struct`, which conflicts with parameter names

**Fix**: Rename the parameter to something else (e.g., `netInterface`, `iface`, `networkInterface`)

**Files to check**: Any file that includes Windows headers (directly or through Hamlib) and uses "interface" as an identifier

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
