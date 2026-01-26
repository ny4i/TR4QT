# TR4QT Future Refactoring Plan

**Created**: 2026-01-23
**Last Updated**: 2026-01-26
**Consolidated From**: REFACTORING_STATUS.md, REFACTORING_RECOMMENDATIONS.md, POST_REFACTORING_SUMMARY.md, EXTRACTION_ARCHITECTURE.md, GitHub Issue #62

---

## Executive Summary

MainWindow is at **4,624 lines** (1.54X over the 3,000 STOP limit). Significant progress has been made:
- Phase 1: 11 manager classes extracted (17.4% reduction from 6,560)
- Phase 2A: unique_ptr conversion complete, onLogQSO already refactored
- IntegrityService deemed unnecessary (DataIntegrityManager handles business logic)

**Current State**: Phase 2A complete. Phase 2B/2C pending (service extractions).

---

## Architecture Target

| Metric | Current | Target | Gap |
|--------|---------|--------|-----|
| MainWindow lines | 4,624 | <3,000 | -1,624 lines |
| Business logic in UI | Minimal | No | Continue extraction |
| Service test coverage | ~60% | >90% | +30% |
| SQL in MainWindow | Zero | Zero | Complete |

---

## Completed Refactoring (Phase 1 & 2A)

### Manager Classes Extracted (11 total, 4,484 lines)

**Controllers** (`/src/controllers/`):
1. QSOLogger (383 lines) - QSO validation, scoring, duplicate checking
2. DataIntegrityManager (322 lines) - Integrity checks, rescore operations
3. ContestManager (211 lines) - Contest activation, configuration
4. ImportExportManager (355 lines) - ADIF/Cabrillo import/export
5. DownloadManager (406 lines) - CTY.dat, LOTW, SCP downloads
6. RadioManager (206 lines) - Radio control, auto-reconnect
7. BandSwitchingManager (173 lines) - Band navigation, AUTO S&P
8. CWMessageManager (170 lines) - Function keys, CW templates

**UI Managers** (`/src/ui/managers/`):
9. MenuManager (416 lines) - Menu bar creation
10. SettingsManager (150 lines) - Window geometry, fonts, themes
11. WindowManager (202 lines) - Auxiliary window management

### Services Created

12. **MaintenanceService** (175 lines) - Clear log workflow with backup
13. **QSOLoggingService** - Complete logging workflow orchestration
14. **QSOLoggingCoordinator** - Post-logging actions (UDP, backup, integrity)

### Phase 2A Completions (2026-01-26)

- **unique_ptr conversion**: 17 service pointers converted to `std::unique_ptr`
  - Better memory safety, RAII resource management
  - No functional change, 36 lines removed from explicit deletes
  - Qt parent-managed widgets kept as raw pointers (Qt handles deletion)

- **onLogQSO refactoring** (already complete before this session):
  - onLogQSO is now ~30 lines
  - Delegates to QSOLoggingService which returns LogQSOResult
  - Clean separation: validation/logging in service, UI updates in MainWindow

### Other Completions

- ErrorField enum added to QSOLoggingService
- DatabaseTransaction RAII wrapper
- RSTValidator consolidation
- BandConstants single source of truth
- UIDefaults namespace for constants
- 100% DialogHelper compliance
- Pre-commit hooks for pattern enforcement

---

## Deferred/Rejected Extractions

### IntegrityService (Rejected - Not Needed)

**Status**: Rejected (analyzed 2026-01-26)
**Rationale**: DataIntegrityManager already handles all business logic properly

**Analysis**:
The existing architecture correctly separates concerns:
- **DataIntegrityManager**: Business logic
  - `quickIntegrityCheck()` - Count-based integrity check
  - `fullIntegrityCheck()` - Comprehensive validation
  - `rescoreContestSilent()` - Rescore all QSOs
- **MainWindow**: UI orchestration only
  - Confirmation dialogs
  - Result display
  - Status updates
  - Model wiring (get QSOs, update rows)

Creating IntegrityService would add indirection without benefit. The ~80 lines of UI orchestration in MainWindow is appropriate - it's UI code, not business logic.

### SessionController (Rejected)

**Rationale**: Big-bang approach too risky. Incremental façade services preferred.
**Alternative**: Continue extracting small, focused services.

### StatusNotifier Pattern (Rejected)

**Rationale**: Over-engineering for current needs. Qt signals work well.
**Alternative**: Enhance existing signal/slot patterns.

### Dependency Injection Container (Rejected)

**Rationale**: Qt idioms (singletons, parent-child ownership) work well.
**Alternative**: Manual DI where beneficial for testing.

---

## Phase 2B/2C: Remaining Service Extractions

### Priority 1: Enhance WindowManager for Window Orchestration

**Status**: Pending
**Effort**: 3-4 hours
**Impact**: -80 lines from MainWindow

**Current Problem**:
WindowManager manages window state but MainWindow still has:
- Window creation logic
- Window show/hide orchestration
- Window state coordination

**Enhancement**:
- Add window creation to WindowManager
- Add cross-window communication
- Centralize window state management

---

### Priority 2: StationInfoService Enhancement

**Status**: Pending
**Effort**: 1 day
**Impact**: -200 lines from MainWindow (revised from 350)

**Responsibility**: Station callsign, operator, location management

Extract from MainWindow:
- Station info display updates
- Operator change handling (OPON command)
- Grid square lookups
- Country/zone display

StationInfoService already exists - enhance it to handle more responsibilities.

---

### Priority 3: FrequencyInputService Enhancement

**Status**: Pending
**Effort**: 4-6 hours
**Impact**: -100 lines from MainWindow

**Responsibility**: VFO entry, frequency parsing, band detection

Currently minimal - enhance to handle:
- VFO A/B text entry
- Frequency parsing and validation
- Band edge detection
- Manual band selection when no radio

---

### Priority 4: SpotProcessingService Enhancement

**Status**: Pending
**Effort**: 4-6 hours
**Impact**: -80 lines from MainWindow

**Responsibility**: DX spot handling, bandmap integration

Extract:
- Spot click handling
- Spot to QSO field population
- Worked/needed status calculation

---

## Code Quality Targets

### Constants Extraction (100% Complete)

- BandConstants.h - Band edge frequencies
- UIDefaults namespace - Window/widget dimensions
- Zone validation constants
- CW speed constants
- Integrity check thresholds
- AUTO S&P sensitivity constants

### Pattern Compliance

| Pattern | Status | Enforcement |
|---------|--------|-------------|
| DialogHelper for all dialogs | 100% | Pre-commit hook |
| No setParent(nullptr) | 100% | Pre-commit hook |
| No hardcoded colors | 100% | Pre-commit hook |
| No SQL in UI classes | 100% | Manual review |
| Named constants for magic numbers | 95% | Review |
| unique_ptr for owned services | 100% | Code review |

---

## Testing Strategy

### Service Tests (Required for Each Extraction)

Each new service MUST have:
- Unit tests for individual methods
- Integration tests with mocked dependencies
- >80% coverage before extraction is "complete"

**Existing Test Files**:
- `tests/test_data_integrity.cpp` - DataIntegrityManager tests
- `tests/test_qso_logging.cpp` - QSOLoggingService tests

**Test Files to Create**:
- `tests/test_window_manager.cpp`
- `tests/test_station_info_service.cpp`

### MainWindow Integration Tests

After extraction:
- Verify MainWindow correctly delegates to services
- Verify UI updates after service calls
- Use mocked services for isolation

---

## Success Metrics

### Architecture Goals

- [ ] MainWindow < 3,000 lines (currently 4,624)
- [x] No business logic in event handlers (onLogQSO is 30 lines, delegates)
- [x] No SQL queries in UI classes (all SQL in repositories)
- [ ] All event handlers < 50 lines
- [ ] Service test coverage > 90%

### Code Quality Goals

- [x] Zero magic numbers in UI code (UIDefaults namespace)
- [x] All services independently testable
- [x] Clear ownership model (unique_ptr for owned, raw for Qt-managed)
- [x] RAII for all resource management (unique_ptr conversion complete)

---

## Implementation Sequence

### Phase 2B: Service Enhancement (1-2 weeks)

1. **Priority 1**: Enhance WindowManager (4 hours)
   - Consolidate window orchestration
   - -80 lines from MainWindow

2. **Priority 2**: StationInfoService enhancement (1 day)
   - Extract station info management
   - -200 lines from MainWindow

3. **Priority 3**: FrequencyInputService enhancement (4-6 hours)
   - Extract frequency/band logic
   - -100 lines from MainWindow

4. **Priority 4**: SpotProcessingService enhancement (4-6 hours)
   - Extract spot handling
   - -80 lines from MainWindow

**Total Estimated Reduction**: ~460 lines
**Projected MainWindow**: ~4,164 lines

### Phase 3: Final Push (Future)

5. Additional extractions to reach <3,000 target
6. Identify large methods (>50 lines) for extraction
7. Continue incremental improvement

---

## Progress Tracking

| Date | Task | Lines Removed | MainWindow Total |
|------|------|---------------|------------------|
| 2026-01-09 | Phase 1 complete (11 managers) | 1,140 | 5,420 |
| 2026-01-23 | ErrorField enum, MaintenanceService | 12 | 4,652 |
| 2026-01-26 | unique_ptr conversion | 36 | 4,624 |
| 2026-01-26 | IntegrityService analysis | 0 | 4,624 (not needed) |
| - | WindowManager enhancement | ~80 | - |
| - | StationInfoService enhancement | ~200 | - |
| - | FrequencyInputService enhancement | ~100 | - |
| - | SpotProcessingService enhancement | ~80 | - |

---

## Document Consolidation

This document supersedes:
- ~~REFACTORING_STATUS.md~~ (historical, keep for reference)
- ~~REFACTORING_RECOMMENDATIONS.md~~ (historical, keep for reference)
- ~~POST_REFACTORING_SUMMARY.md~~ (historical, keep for reference)

The following remain active:
- **EXTRACTION_ARCHITECTURE.md** - Detailed service designs
- **CHECKPOINTS.md** - Enforcement rules
- **CLAUDE.md** - Project-specific instructions

---

**Last Updated**: 2026-01-26
**Next Review**: After completing Phase 2B
