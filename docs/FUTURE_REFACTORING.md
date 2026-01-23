# TR4QT Future Refactoring Plan

**Created**: 2026-01-23
**Consolidated From**: REFACTORING_STATUS.md, REFACTORING_RECOMMENDATIONS.md, POST_REFACTORING_SUMMARY.md, EXTRACTION_ARCHITECTURE.md, GitHub Issue #62

---

## Executive Summary

MainWindow remains at **4,652 lines** (1.55X over the 3,000 STOP limit). While significant progress was made extracting 11 manager classes (17.4% reduction from 6,560), further extraction is required to reach the architectural target of <3,000 lines.

**Current State**: Phase 1 refactoring (manager extraction) complete. Phase 2 (service layer) pending.

---

## Architecture Target

| Metric | Current | Target | Gap |
|--------|---------|--------|-----|
| MainWindow lines | 4,652 | <3,000 | -1,652 lines |
| Business logic in UI | Yes | No | Full extraction |
| Service test coverage | ~60% | >90% | +30% |
| SQL in MainWindow | Some | Zero | Complete removal |

---

## Completed Refactoring (Phase 1)

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

### Services Created (Session 2026-01-23)

12. **MaintenanceService** (175 lines) - Clear log workflow with backup

### Other Completions

- ErrorField enum added to QSOLoggingService
- DatabaseTransaction RAII wrapper
- RSTValidator consolidation
- BandConstants single source of truth
- UIDefaults namespace for constants
- 100% DialogHelper compliance
- Pre-commit hooks for pattern enforcement

---

## Phase 2: Pending Service Extractions

### Priority 1: Move onLogQSO Orchestration to LoggingCoordinator

**Status**: Pending (Task #3)
**Effort**: 1-2 days
**Impact**: -150 lines from MainWindow

**Current Problem**:
`MainWindow::onLogQSO()` is ~250 lines mixing:
- Command parsing (OPON, UDP)
- QSO validation
- Database persistence with retry
- Exchange memory updates
- Post-logging actions (UDP broadcast, backup, integrity)

**Solution**:
Move orchestration to `QSOLoggingCoordinator` (already designed in EXTRACTION_ARCHITECTURE.md):
- MainWindow calls `m_qsoLoggingService->logQSO(request)`
- Service returns `LogQSOResult` for UI to handle
- MainWindow updates UI based on result

**Files to Create/Modify**:
- Create `src/services/QSOLoggingCoordinator.cpp` (if not exists)
- Enhance `src/services/QSOLoggingService.cpp` to use coordinator
- Modify `src/ui/MainWindow.cpp` to delegate

---

### Priority 2: Extract Integrity Check Workflows to IntegrityService

**Status**: Pending (Task #4)
**Effort**: 4-6 hours
**Impact**: -100 lines from MainWindow

**Current Problem**:
Integrity checks scattered:
- `MainWindow::onIntegrityCheck()` - Timer-triggered
- `MainWindow::onRescore()` - Manual rescore
- `DataIntegrityManager` exists but MainWindow still has orchestration

**Solution**:
Create `IntegrityService`:
```cpp
class IntegrityService {
public:
    struct IntegrityResult {
        bool success;
        int issuesFound;
        int issuesFixed;
        QString report;
    };

    IntegrityResult runFullCheck(int contestDbId);
    IntegrityResult rescoreAllQSOs(int contestDbId);
    IntegrityResult checkAndFixDuplicates(int contestDbId);
};
```

**Files to Create**:
- `src/services/IntegrityService.h`
- `src/services/IntegrityService.cpp`

---

### Priority 3: Enhance WindowManager for Window Orchestration

**Status**: Pending (Task #5)
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

### Priority 4: Convert Raw Pointers to unique_ptr for RAII

**Status**: Pending (Task #6)
**Effort**: 4-6 hours
**Impact**: Better memory safety, no line reduction

**Current Problem**:
Raw pointers in MainWindow:
```cpp
ContestBase* m_activeContest;          // Should be unique_ptr
QTimer* m_radioReconnectTimer;         // Parent-owned, OK as raw
```

**Solution**:
- Convert service pointers to `std::unique_ptr`
- Keep Qt parent-managed widgets as raw pointers (Qt handles)
- Document ownership model clearly

---

## Phase 3: Future Extraction Targets

### StationInfoService (Target: -350 lines)

**Responsibility**: Station callsign, operator, location management

Extract from MainWindow:
- Station info display updates
- Operator change handling (OPON command)
- Grid square lookups
- Country/zone display

---

### FrequencyInputService Enhancement

**Responsibility**: VFO entry, frequency parsing, band detection

Currently minimal - enhance to handle:
- VFO A/B text entry
- Frequency parsing and validation
- Band edge detection
- Manual band selection when no radio

---

### SpotProcessingService Enhancement

**Responsibility**: DX spot handling, bandmap integration

Extract:
- Spot click handling
- Spot to QSO field population
- Worked/needed status calculation

---

## Deferred Items (Not Currently Planned)

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
| No SQL in UI classes | Partial | Manual review |
| Named constants for magic numbers | 95% | Review |

---

## Testing Strategy

### Service Tests (Required for Each Extraction)

Each new service MUST have:
- Unit tests for individual methods
- Integration tests with mocked dependencies
- >80% coverage before extraction is "complete"

**Test Files to Create**:
- `tests/test_integrity_service.cpp`
- `tests/test_qso_logging_coordinator.cpp` (if not exists)
- `tests/test_window_manager.cpp`

### MainWindow Integration Tests

After extraction:
- Verify MainWindow correctly delegates to services
- Verify UI updates after service calls
- Use mocked services for isolation

---

## Success Metrics

### Architecture Goals

- [ ] MainWindow < 3,000 lines (currently 4,652)
- [ ] No business logic in event handlers
- [ ] No SQL queries in UI classes
- [ ] All event handlers < 50 lines
- [ ] Service test coverage > 90%

### Code Quality Goals

- [ ] Zero magic numbers in UI code
- [ ] All services independently testable
- [ ] Clear ownership model (documented)
- [ ] RAII for all resource management

---

## Implementation Sequence

### Phase 2A: Quick Wins (1 week)

1. **Priority 4**: Convert to unique_ptr (4 hours)
   - No functional change, better safety

2. **Priority 3**: Enhance WindowManager (4 hours)
   - Consolidate window orchestration
   - -80 lines from MainWindow

### Phase 2B: Service Extraction (2 weeks)

3. **Priority 2**: IntegrityService (6 hours)
   - Extract integrity/rescore workflows
   - -100 lines from MainWindow

4. **Priority 1**: QSOLoggingCoordinator (1-2 days)
   - Major extraction of onLogQSO
   - -150 lines from MainWindow

### Phase 2C: Completion (2 weeks)

5. **StationInfoService** (1 day)
   - Extract station info management
   - -350 lines from MainWindow

**Total Estimated Reduction**: ~680 lines
**Projected MainWindow**: ~3,972 lines (still over, but improving)

### Phase 3: Final Push (Future)

6. Additional extractions to reach <3,000 target
7. Frequency input enhancement
8. Spot processing enhancement

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

## Progress Tracking

Update this section as work progresses:

| Date | Task | Lines Removed | MainWindow Total |
|------|------|---------------|------------------|
| 2026-01-09 | Phase 1 complete (11 managers) | 1,140 | 5,420 |
| 2026-01-23 | ErrorField enum, MaintenanceService | 12 | 4,652 |
| - | unique_ptr conversion | 0 | - |
| - | WindowManager enhancement | ~80 | - |
| - | IntegrityService | ~100 | - |
| - | QSOLoggingCoordinator | ~150 | - |

---

**Last Updated**: 2026-01-23
**Next Review**: After completing Phase 2A
