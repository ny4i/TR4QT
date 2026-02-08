# TR4QT Product Roadmap

**Generated**: 2026-01-01
**Purpose**: Single source of truth for all planned work, features, and improvements
**Version**: 3.15.0

---

## Table of Contents

1. [Executive Summary](#executive-summary)
2. [Now - Critical Items (0-2 weeks)](#now---critical-items-0-2-weeks)
3. [Next - High Priority (2-8 weeks)](#next---high-priority-2-8-weeks)
4. [Later - Medium Priority (2-6 months)](#later---medium-priority-2-6-months)
5. [Future - Low Priority (6+ months)](#future---low-priority-6-months)
6. [Completed Items](#completed-items)
7. [Implementation Notes](#implementation-notes)

---

## Executive Summary

### Current State (v3.15.0)
- ✅ Core logging system operational
- ✅ Radio control via Hamlib
- ✅ DX Cluster integration with band map
- ✅ 14 contest modules implemented
- ✅ Database persistence and backup system
- ✅ ADIF import/export
- ✅ UDP broadcasts (N1MM+ compatible)
- ✅ Geographic maps (ARRL Sections, US States)

### Key Metrics
- **Codebase**: ~11,500 lines (src/)
- **Test Coverage**: 15 test suites, 185+ tests
- **Refactoring Progress**: 15% complete (2 of 11 issues resolved)
- **Open TODO Items**: ~85 across code and plans

### Strategic Focus Areas
1. **Code Quality** (Refactoring, DialogHelper migration, constants extraction)
2. **User Experience** (In-app map viewer, backup settings UI, exchange enhancements)
3. **Feature Completeness** (Florida QSO Party, Super Check Partial, additional contests)
4. **Platform Support** (Windows/macOS deployment automation)

---

## Now - Critical Items (0-2 weeks)

### 1. ✅ Code Refactoring Quick Wins - COMPLETED ⚡
**Priority**: ~~CRITICAL~~ **COMPLETED**
**Effort**: ~40 minutes (estimated 2 hours)
**Impact**: Code quality, maintainability
**Status**: ✅ **All tasks complete** (2026-01-02)

#### Tasks
- [x] **Complete Issue 3**: Replace 4 remaining hard-coded RST values ✅
  - Files: `MainWindow.cpp` (3 instances), `InitialExchangeManager.cpp` (1 instance)
  - Change: Use `RSTValidator::getDefault(mode)`
  - Commit: `8015c66`

- [x] **Add Constants - Phase 1** ✅: Added to `Constants.h`
  - Zone validation constants (Issue 7): `CQ_ZONE_MIN/MAX (1-40)`, `ITU_ZONE_MIN/MAX (1-90)`
  - Integrity check thresholds (Issue 9): `INTEGRITY_CHECK_INTERVAL_MS (5 min)`, `INTEGRITY_CHECK_QSO_THRESHOLD (50)`
  - CW speed limits (Issue 5 partial): `CW_SPEED_MIN (5)`, `CW_SPEED_MAX (60)`, `CW_SPEED_DEFAULT (25)`
  - Commit: `8015c66`

- [x] **DialogHelper::about()** ✅: Already implemented and in use
  - All About dialogs in codebase use `DialogHelper::about()`
  - Consistent logging across all dialog interactions
  - No migration needed

**Reference**: `REFACTORING_STATUS.md` - Quick Wins section
**Completed**: 2026-01-02

---

### 2. Complete DialogHelper Migration 🔧 HIGH PRIORITY
**Priority**: HIGH (mandated by CLAUDE.md)
**Effort**: 2-3 hours
**Impact**: Logging compliance, debugging, consistency
**Status**: 41% complete (119/289 calls migrated)

#### Remaining Files (170 QMessageBox calls)
- [ ] `MainWindow.cpp` (~40+ calls)
- [ ] `PreferencesDialog.cpp`
- [ ] `DXClusterWindow.cpp`
- [ ] `RadioConfigDialog.cpp`
- [ ] `ADIFImportDialog.cpp`
- [ ] `ContestChooserDialog.cpp`
- [ ] `EditQSODialog.cpp`
- [ ] `BackupRestoreDialog.cpp`
- [ ] `ExportPreviewDialog.cpp`

#### Strategy
Migrate files as they're touched for other work (opportunistic refactoring)

**Reference**: `REFACTORING_STATUS.md` - Issue 1

---

### 3. Database Threading Documentation 📝 MEDIUM PRIORITY
**Priority**: MEDIUM
**Effort**: 1 hour
**Impact**: Future-proofing for networked TR4QT

#### Context
Database singleton is thread-safe for single-operator use but needs work before multi-operator TCP networking.

#### Tasks
- [ ] Document thread safety status in `Database.cpp` (already has comprehensive TODO)
- [ ] Add test cases for concurrent scenarios (disabled for now)
- [ ] Design message queue architecture for future TCP server

**Reference**: `CLAUDE.md` - Known Issues / Limitations section
**Deferred Until**: TCP networking implementation

---

## Next - High Priority (2-8 weeks)

### 4. Backup Settings UI Integration 💾
**Priority**: HIGH
**Effort**: 1-2 hours
**Impact**: User configurability
**Status**: Phase 3 pending

#### Current State
- ✅ BackupManager infrastructure complete
- ✅ Manual backup via Tools → Backup Log works
- ✅ Auto-backup hook in QSO logging flow
- ⏳ Settings hardcoded (not exposed to user)

#### Tasks
- [ ] Add 4 getter/setter pairs to `AppSettings.h/cpp`
  - `setAutoBackupEnabled()` / `getAutoBackupEnabled()`
  - `setAutoBackupInterval()` / `getAutoBackupInterval()`
  - `setBackupDirectory()` / `getBackupDirectory()`
  - `setMaxBackups()` / `getMaxBackups()`

- [ ] Add Backup tab to `PreferencesDialog`
  - Auto-backup checkbox
  - Interval spinner (1-1000 QSOs)
  - Max backups spinner (1-100)
  - Directory path + Browse button
  - Info section (last backup time, size, next scheduled)

- [ ] Update `MainWindow.cpp` lines 66-72 to load from settings

**Reference**: `BACKUP_INTEGRATION_TODO.md`

---

### 5. ✅ In-App Map Viewer Window - COMPLETED 🗺️
**Priority**: ~~HIGH~~ **COMPLETED**
**Status**: ✅ Implemented using native Qt graphics (NativeMapViewer)

#### Implementation Details
Chose **Option B** (Native Qt Graphics) instead of web-based approach:
- QGraphicsView-based rendering (no Qt WebEngine dependency)
- GeoJSON polygon display with Mercator projection
- Chloropleth coloring based on QSO counts
- Auto-refresh when QSO model changes
- Zoom and pan controls
- Self-contained dialog architecture

#### Completed Features
- ✅ ARRL Sections map (View → Sections Map)
- ✅ US States map for WAS tracking (View → States Map)
- ✅ Statistics display (worked/total/completion %)
- ✅ Worked list sidebar
- ✅ Menu integration in MainWindow
- ✅ Proper separation of concerns (NativeMapViewer is self-contained)

#### Implementation Files
- `src/ui/NativeMapViewer.h/cpp` - Self-contained map viewer dialog
- `src/ui/MainWindow.h/cpp` - Menu actions and integration

**Benefits Achieved**:
- ✅ No web server or browser dependency
- ✅ Works on all platforms (including MinGW Windows)
- ✅ Smaller distribution (no Qt WebEngine)
- ✅ Truly native Qt
- ✅ Good architecture (self-contained dialog)

---

### 6. Florida QSO Party Contest 🏆
**Priority**: HIGH
**Effort**: 4-6 hours
**Impact**: First QSO Party implementation (template for others)

#### Status
Plan complete, implementation ready to start

#### Key Features
- In-state vs out-of-state detection (via `StationInfo.state`)
- County-based multipliers (67 Florida counties)
- State-dependent exchanges
- Power multipliers (QRP ×3, Low ×2, High ×1)
- Band restrictions (40/20/15/10M only)
- Mode restrictions (Phone + CW only, no digital)

#### Files to Create
- [ ] `src/contests/FloridaQSOPartyContest.h`
- [ ] `src/contests/FloridaQSOPartyContest.cpp`
- [ ] `tests/test_flqp.cpp`

#### Files to Modify
- [ ] `src/core/Types.h` - Add `County` to `MultiplierType` enum (if not present)
- [ ] `src/CMakeLists.txt` - Add source files
- [ ] `tests/CMakeLists.txt` - Add test executable

**Reference**: Claude plan file (Florida QSO Party plan)

---

### 7. Static Map Export (PNG/JPG) 📸
**Priority**: LOW
**Effort**: 1-2 hours
**Impact**: Contest reports, social media sharing

#### Use Cases
- Contest reports (include map showing worked states/sections)
- Social media posts (share WAS progress)
- Print-friendly format
- Archive snapshots of progress

#### Implementation (Simpler with Native Maps)
Since we're using QGraphicsView (not web-based), export is much simpler:

#### Tasks
- [ ] Add "Export as Image..." button to NativeMapViewer dialog
- [ ] Use `QGraphicsView::grab()` or `QGraphicsScene::render()` to capture
- [ ] Save to user-selected file path (PNG/JPG)
- [ ] Optional: Export options dialog (size, format, include legend/stats)

**Simplified approach** - No need for QWebEngine, hidden rendering, or JS callbacks.

**Reference**: Native implementation makes this trivial compared to original plan

---

### 8. Super Check Partial (SCP) Database 🔍
**Priority**: MEDIUM
**Effort**: 4-6 days (6 phases)
**Impact**: Callsign auto-completion
**Status**: ✅ Detailed plan complete, ❌ Implementation not started

#### Goal
Real-time callsign lookup/autocomplete from master database (like TR4W)

#### Plan Summary (from task a4b23e4)
Comprehensive 12-section implementation plan created, covering:
- Database schema (`scp_callsigns` table in GlobalDatabase)
- SCPRepository (smart prefix+suffix matching <50ms)
- SCPDownloader (from supercheckpartial.com)
- SCPCallsignExtractor (augment from local logs)
- SCPMatcher (matching engine)
- UI integration (reuse duplicate warning label)
- Preferences tab (download, settings)

#### Implementation Phases (6 days)
- [ ] **Day 1**: Database foundation (schema, SCPRepository)
- [ ] **Day 2**: Download infrastructure (SCPDownloader, extractor)
- [ ] **Day 3**: Matching engine (SCPMatcher, UI integration)
- [ ] **Day 4**: Preferences integration (settings, SCP tab)
- [ ] **Day 5**: Local log augmentation
- [ ] **Day 6**: Polish & testing

#### Files to Create (12 new files)
- `src/data/SCPRepository.h/cpp`
- `src/utils/SCPDownloader.h/cpp`
- `src/utils/SCPCallsignExtractor.h/cpp`
- `src/utils/SCPMatcher.h/cpp`
- 4 test files

#### Files to Modify (8 files)
- `src/data/global_schema.sql` - Add scp_callsigns table
- `src/ui/MainWindow.h/cpp` - Integrate into onCallsignChanged()
- `src/ui/dialogs/PreferencesDialog.h/cpp` - Add SCP tab
- `src/utils/AppSettings.h/cpp` - Add SCP settings
- `src/CMakeLists.txt` - Add source files

**Reference**: Detailed plan in task a4b23e4 output (12 sections, production-ready design)

---

## Later - Medium Priority (2-6 months)

### 9. Additional Contest Implementations 🏅

#### 9.1 More QSO Parties (using Florida QP as template)
- [ ] California QSO Party (58 counties)
- [ ] Pennsylvania QSO Party (67 counties)
- [ ] New York QSO Party (62 counties)
- [ ] Texas QSO Party (254 counties!)
- [ ] 7QP (7 western states)

**Effort**: 3-4 hours each (template established)

---

### 10. Exchange System Enhancements 🔄
**Priority**: MEDIUM
**Effort**: 6-8 weeks (phased implementation)
**Impact**: TR4W-level sophistication

#### Phase 1: Critical Fixes (Week 1) ✅ IMMEDIATE VALUE
**Status**: NEEDS IMPLEMENTATION

- [ ] Add validation to `onLogQSO()` - call `validateReceivedExchange()`
- [ ] Populate `parsedExchange` field - call `parseReceivedExchange()`
- [ ] Extract RST from exchange or use mode-based defaults
- [ ] Add validation to `EditQSODialog`

**Impact**: Zero invalid exchanges enter database

#### Phase 2: Auto-Population & Exchange Memory (Weeks 2-3)
**Status**: PLANNED

- [ ] Create `InitialExchangeManager` class
- [ ] Create `ExchangeMemoryRepository` + database table
- [ ] Enhance `autoPopulateExchange()` with smart prediction
- [ ] Save exchanges to memory after logging
- [ ] Create master database system (`~/.tr4qt/master.db`)

**Impact**: 70% of exchanges auto-fill correctly

#### Phase 3: Real-Time Validation & Visual Feedback (Weeks 3-4)
**Status**: PLANNED

- [ ] Create `ExchangeValidator` class
- [ ] Add border color indicators (green/yellow/red)
- [ ] Debounced validation (<300ms feedback)
- [ ] Tooltip with error messages
- [ ] Optional status label below exchange field

**Impact**: Users see validation feedback while typing

#### Phase 4: Smart Navigation & Field Management (Weeks 4-5)
**Status**: PLANNED

- [ ] Auto-focus to exchange after 3+ chars in callsign
- [ ] Exchange overwrite mode (first keystroke replaces)
- [ ] Smart space insertion (`SmartExchangeParser`)
- [ ] Tab completion for sections (`QCompleter`)
- [ ] Add settings to `AppSettings`

**Impact**: <20 keystrokes per QSO (down from ~30)

#### Phase 5: Exchange Memory UI (Weeks 5-6)
**Status**: PLANNED

- [ ] Create `ExchangeMemoryDialog` - Tools → Exchange Memory Manager
- [ ] Memory table view with statistics
- [ ] Import/export to CSV
- [ ] Delete old entries (>1 year)
- [ ] Optional: Import TR4W TRMASTER.DTA

**Impact**: Exchange memory persists and improves over time

#### Phase 6: Advanced Features & DOM Files (Weeks 6-8)
**Status**: PLANNED

- [ ] JSON resource files for contest-specific data
- [ ] `ContestResourceLoader` class
- [ ] Smart multi-field parser with reordering
- [ ] Context-sensitive help (F1 key)
- [ ] Exchange statistics window with charts

**Impact**: TR4W-level sophistication

**Reference**: Claude plan file (Exchange Enhancement plan)

---

### 11. DXCC Entities Map 🌍
**Priority**: MEDIUM
**Effort**: 4-6 hours
**Impact**: World map visualization for DXCC progress
**Status**: Partially stubbed in NativeMapViewer

#### Current State
- ✅ `MapType::DXCC` enum exists in NativeMapViewer
- ✅ Title and basic structure ready
- ❌ No GeoJSON data loaded yet
- ❌ No DXCC entity mapping

#### Tasks
- [ ] Download Natural Earth Data (1:50m countries shapefile)
- [ ] Convert to GeoJSON format
- [ ] Create DXCC entity → country mapping logic
- [ ] Handle multi-DXCC countries (US = K, KH6, KL7, KP2, KP4, etc.)
- [ ] Add to NativeMapViewer's `loadGeoJSON()` switch statement
- [ ] Track QSOs per DXCC from `QSO.dxccEntity` field
- [ ] Add menu item: View → DXCC Map

**Note**: Uses existing NativeMapViewer infrastructure (no new class needed)

**Reference**: `TODO_MAPS.md` - DXCC Entities Map section

---

### 12. Additional Refactoring Items 🔧

#### 12.1 Extract ReconnectionManager (Issue 4)
**Effort**: 2 hours
**Impact**: Code duplication elimination

Duplicate reconnection logic in `MainWindow` and `DXClusterWindow` (10 attempts, 10s interval).

- [ ] Create `src/utils/ReconnectionManager.h/cpp`
- [ ] Configurable retry strategy
- [ ] Replace duplicate code in 2 classes

#### 12.2 Extract UI Dimension Constants (Issue 5)
**Effort**: 1 hour (remaining)
**Impact**: UI consistency

- [ ] Add to `Constants.h`:
  - `MAIN_WINDOW_MIN_WIDTH` / `MIN_HEIGHT`
  - `MAIN_WINDOW_DEFAULT_WIDTH` / `DEFAULT_HEIGHT`
  - `COL_WIDTH_*` constants for table columns
  - (CW speed already done in Phase 1)

#### 12.3 Extract ExchangeParser Utility (Issue 6)
**Effort**: 3 hours
**Impact**: ~50+ lines per contest class

All contest classes have similar `parseReceivedExchange()` with duplicate logic.

- [ ] Create `src/exchanges/ExchangeParser.h/cpp`
- [ ] Common "smart detection" logic for RST vs other fields
- [ ] Reduce contest class duplication

#### 12.4 Transaction Wrapper in QSORepository (Issue 8)
**Effort**: 1 hour
**Impact**: Boilerplate reduction

Transaction begin/commit/rollback pattern duplicated in 3+ methods.

- [ ] Extract to template wrapper method
- [ ] Reduce ~20 lines of boilerplate per method

#### 12.5 Color Definitions (Issue 10)
**Effort**: 1 hour
**Impact**: UI consistency

Hard-coded colors (`#ff6600`, `#666`) scattered throughout UI.

- [ ] Use `ThemeManager` or create color constants
- [ ] Update all call sites

#### 12.6 FontFactory Utility (Issue 11)
**Effort**: 1 hour
**Impact**: Code reuse

Font family "Monospace" hard-coded in multiple places.

- [ ] Create `src/utils/FontFactory.h/cpp`
- [ ] Centralize font creation logic

**Reference**: `REFACTORING_RECOMMENDATIONS.md` - Issues 4-11

---

### 13. DX Cluster Spot Persistence (Already Implemented?)
**Priority**: LOW (may already be done)
**Effort**: Verify implementation
**Status**: Plan exists, check if implemented

#### Features (per plan)
- ✅ In-memory storage (`QList<Spot>`)
- ✅ Database persistence on shutdown
- ✅ Spot aging with color coding
- ✅ Band switching shows stored spots
- ✅ 5-second refresh timer

#### Tasks
- [ ] Verify implementation status (check if plan was executed)
- [ ] Test shutdown persistence
- [ ] Test spot aging colors

**Reference**: Claude plan file (Spot persistence plan)

---

## Future - Low Priority (6+ months)

### 14. TCP Networking & Multi-Operator Support 🌐
**Priority**: LOW
**Effort**: 8-12 weeks
**Blockers**: Database threading (Issue deferred)

#### Features
- Multi-station networked logging
- Master/slave architecture
- Shared spot database
- Real-time score sync

#### Prerequisites
- Implement database thread safety (Connection pool or message queue)
- Design TCP protocol
- Security considerations (authentication, encryption)

**Reference**: `CLAUDE.md` - Known Issues / Database Threading

---

### 15. SO2R (Dual Radio) Support 📻
**Priority**: LOW
**Effort**: 4-6 weeks

#### Current State
Single radio only (`radioNr = 1` hardcoded in UDP broadcasts)

#### Tasks
- [ ] Dual `RadioController` instances
- [ ] Radio A/B switching
- [ ] Separate VFOs in UI
- [ ] Band map per radio
- [ ] SO2R-specific keyboard shortcuts

**Reference**: Code TODOs in `UdpBroadcastManager.cpp` lines 153, 233

---

### 16. Additional Menu Item Implementations 🎯
**Priority**: LOW (various)
**Effort**: 1-4 hours each

From `MainWindow.h` TODOs:

#### Band Map & Multipliers
- [ ] **Swap Mult View** (`onSwapMultView()`) - Toggle multiplier window layout
- [ ] **Missing Mults Report** (`onMissingMultsReport()`) - Show unworked mults

#### Log Management
- [ ] **View/Edit Log** (`onViewEditLog()`) - Dedicated log editing window
- [ ] **Clear Dupes** (`onClearDupes()`) - Remove duplicate QSOs
- [ ] **Add Note** (`onNote()`) - Add timestamped notes to log
- [ ] **Recall Last Entry** (`onRecallLast()`) - Undo last QSO
- [ ] **Delete Last QSO** (`onDeleteLastQSO()`) - Quick delete

#### CW Operations
- [ ] **WinKeyer Mode** (`onWKMode()`) - Re-initialize WinKeyer
- [ ] **Auto CQ** (`onAutoCQ()`) - Automated CQ calling
- [ ] **Auto CQ Resume** (`onAutoCQResume()`) - Resume auto CQ
- [ ] **Kill CW** (`onKillCW()`) - Stop CW transmission
- [ ] **Toggle Sidetone** (`onToggleSidetone()`)
- [ ] **Toggle Autosend** (`onToggleAutosend()`)
- [ ] **CW Speed** (`onCWSpeed()`) - Adjust WPM

#### Search & Navigation
- [ ] **Dupe Check** (`onDupeCheck()`) - Check if callsign is dupe
- [ ] **Search Log** (`onSearchLog()`) - Find QSOs by criteria

#### Other
- [ ] **Inc Number** (`onIncNumber()`) - Increment serial number
- [ ] **Initial Exchange** (`onInitialExchange()`) - Set default exchange
- [ ] **Initialize** (`onInitialize()`) - Reset for new session
- [ ] **Toggle Rigs** (`onToggleRigs()`) - SO2R radio switching
- [ ] **Edit SO2R** (`onEditSO2R()`) - SO2R configuration

**Reference**: `src/ui/MainWindow.h` lines 106-144

---

### 17. Contest-Specific Auto-Switching 🔀
**Priority**: LOW
**Effort**: 1 hour
**Depends On**: All maps implemented (Sections, States, DXCC)

Auto-select appropriate map based on active contest type:
- ARRL Sweepstakes → Sections map
- State QSO Parties → States map
- CQ WW / ARRL DX → DXCC map
- Field Day → Sections map

**Reference**: `TODO_MAPS.md` - Contest-Specific Maps section

---

## Completed Items ✅

### Refactoring
- ✅ **Issue 2**: Duplicate RST Validation Logic - RSTValidator created (~90 lines eliminated)
- ✅ **Code Refactoring Quick Wins** (2026-01-02)
  - Replaced 4 hard-coded RST values with `RSTValidator::getDefault()`
  - Added Constants Phase 1: Zone ranges, integrity thresholds, CW speed limits
  - DialogHelper::about() already implemented and in use
  - Effort: 40 minutes (estimated 2 hours)

### Infrastructure
- ✅ **CountryFile Thread Safety** - QReadWriteLock implemented (2025-12-30)
- ✅ **Database Backup System** - BackupManager with auto-backup and rotation
- ✅ **Geographic Maps** - ARRL Sections map (v2.98.1), US States map (v3.4.0)
- ✅ **Native Map Viewer** - QGraphicsView-based in-app map display (v3.x)

### Platform Support
- ✅ **Linux Build Disabled** - Permanently disabled in CI (2026-01-01)
- ✅ **Windows Deployment Script** - Explicit DLL deployment (`scripts/windows-deploy.bat`)
- ✅ **macOS Bundle Process** - Complete deployment workflow documented

### Documentation
- ✅ **CHANGELOG.md** - Complete git history from v1.5.0 to v3.15.0 (214 versions)
- ✅ **REFACTORING_STATUS.md** - Comprehensive evaluation of 11 refactoring issues

---

## Implementation Notes

### Code TODO Comments (From grep)

**High Priority**:
- `Database.cpp:13` - Threading issue documentation (comprehensive TODO exists)
- `MainWindow.cpp:1504-1505` - Initialize/reload database for new contest
- `MainWindow.cpp:1575` - Reload contest settings if changed
- `MainWindow.cpp:3950` - Add `getMyCounty()` to AppSettings for QSO Parties
- `StationInfo.h:48-49` - Add Power field to Preferences UI + AppSettings
- `TelnetClient.cpp:118` - Add support for other cluster types (v5, v4)

**Medium Priority**:
- `MainWindow.cpp:1764` - Get category info from contest dialog
- `MainWindow.cpp:1778` - Get actual score from scoring engine
- `MainWindow.cpp:5142-5143` - Check if spot is multiplier/worked
- `QSOTableModel.cpp:99` - Implement S&P indicator (Run vs Search & Pounce)
- `NativeMapViewer.cpp:337` - Proper Mercator projection (currently simple linear)
- `HamlibCWSender.cpp:157` - Integrate with `RadioController::waitForMorseComplete()`

**Low Priority** (3rd-party libraries):
- Various TODOs in `miniz.h/cpp` (zip library)
- Various TODOs in `qcustomplot.h/cpp` (plotting library)

---

### Dependencies & Relationships

```
Exchange Enhancements → Exchange Memory → SCP Database
                     ↓
             Florida QSO Party
                     ↓
         More QSO Parties (template)

In-App Map Viewer → Static Map Export
                 ↓
            DXCC Map
                 ↓
      Contest Auto-Switching

Refactoring Quick Wins → DialogHelper Migration
                      ↓
              Code Quality Baseline
                      ↓
          Advanced Refactoring Items

Backup Settings UI → (independent)
Database Threading → TCP Networking → Multi-Operator
```

---

### Risk Assessment

**High Risk** (Complex, Long Timeline):
- TCP Networking & Multi-Operator (database threading dependency)
- SO2R Support (significant UI changes)
- Exchange System Phase 6 (advanced features)

**Medium Risk** (Moderate Complexity):
- Florida QSO Party (first QSO Party implementation - template for others)
- DXCC Map (multi-entity mapping complexity)
- In-App Map Viewer (Qt WebEngine integration)

**Low Risk** (Well-Defined, Short Timeline):
- Refactoring Quick Wins (proven patterns)
- DialogHelper Migration (mechanical changes)
- Backup Settings UI (existing pattern)
- Static Map Export (QWebEngineView screenshot)
- Constants Extraction (straightforward)

---

### Success Metrics

**Code Quality**:
- Refactoring progress: 15% → 50% (6 of 11 issues resolved)
- DialogHelper coverage: 41% → 100%
- Test coverage: Maintain >80% for new code

**User Experience**:
- Exchange auto-fill rate: 0% → 70%
- Keystrokes per QSO: 30 → <20
- In-app features: Reduce external browser dependency

**Feature Completeness**:
- Contest count: 14 → 20+
- QSO Party support: 0 → 5+
- Map types: 2 → 3 (Sections, States, DXCC)

---

**END OF ROADMAP**

For detailed implementation plans, see:
- `REFACTORING_STATUS.md` - Code quality improvements
- `BACKUP_INTEGRATION_TODO.md` - Backup settings UI
- `TODO_MAPS.md` - Geographic map features
- Claude plan files - Detailed implementation plans (local only)
