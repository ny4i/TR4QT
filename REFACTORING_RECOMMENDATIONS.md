# TR4QT Code Refactoring Review

**Generated**: 2025-12-30
**Review Scope**: Core application code in src/ (11,445 lines)
**Overall Quality**: Good foundation with room for improvement

---

## Executive Summary

Reviewed TR4QT codebase focusing on ui/, contests/, exchanges/, data/, and utils/ directories. The codebase demonstrates good architecture with a clean plugin-based contest system and proper separation of concerns. However, there are several opportunities to reduce code duplication, extract magic numbers, and improve maintainability.

**Key Findings**:
- ✅ Strong contest plugin architecture (excellent)
- ✅ Good namespace organization and logging patterns
- ⚠️ Incomplete DialogHelper migration (211 QMessageBox calls vs. 66 DialogHelper calls)
- ⚠️ Significant code duplication in contest classes (RST validation, parsing logic)
- ⚠️ Multiple hard-coded values that should be constants

---

## Critical Issues

### Issue 1: QMessageBox Direct Usage Violates Logging Policy ⚠️ CRITICAL

**Priority**: HIGH
**Effort**: 2-3 hours
**Impact**: Logging, debugging, support

**Problem**:
CLAUDE.md mandates ALL dialogs MUST use DialogHelper for automatic logging, but 211 instances of direct `QMessageBox` calls remain (vs. only 66 `DialogHelper` calls). This bypasses the logging infrastructure and creates inconsistent user experience.

**Current Code**:
```cpp
// Multiple files still using QMessageBox directly
QMessageBox::about(this, "About TR4QT", ...);
QMessageBox msgBox(this);
msgBox.setIcon(QMessageBox::Information);
msgBox.exec();
```

**Recommendation**:
```cpp
// Create DialogHelper::about() wrapper
// In DialogHelper.h:
static void about(QWidget* parent, const QString& title, const QString& text);

// In DialogHelper.cpp:
void DialogHelper::about(QWidget* parent, const QString& title, const QString& text) {
    LOG_INFO("DialogHelper", QString("About dialog: %1").arg(title));
    QMessageBox::about(parent, title, text);
}

// Update all call sites:
DialogHelper::about(this, "About TR4QT", ...);
```

**Files to Update**:
- src/ui/MainWindow.cpp (lines 1266-1280, 202-221)
- src/ui/dialogs/ADIFImportDialog.cpp (line 121)
- src/ui/dialogs/ContestChooserDialog.cpp
- src/ui/dialogs/PreferencesDialog.cpp
- src/ui/widgets/DXClusterWindow.cpp
- src/ui/dialogs/EditQSODialog.cpp
- src/ui/dialogs/BackupRestoreDialog.cpp
- src/ui/dialogs/ExportPreviewDialog.cpp
- src/ui/dialogs/RadioConfigDialog.cpp

---

## High Priority Refactoring

### Issue 2: Duplicate RST Validation Logic (6 Contest Files)

**Priority**: HIGH
**Effort**: 30 minutes
**Impact**: Code duplication (90 lines), DRY principle

**Problem**:
Identical 15-line `isValidRST()` method duplicated across 6 contest classes (CQWWContest, CQWPXContest, ARRLDXContest, IARUHFContest, etc.). Same validation logic = 90 total lines of duplication.

**Current Code**:
```cpp
// CQWWContest.cpp lines 109-124
bool CQWWContest::isValidRST(const QString& rst, ModeType mode) {
    if (mode == ModeType::CW || mode == ModeType::CWR) {
        QRegularExpression re("^[1-5][1-9][1-9]$");
        return re.match(rst).hasMatch();
    } else {
        QRegularExpression re("^[1-5][1-9][1-9]?$");
        return re.match(rst).hasMatch();
    }
}

// IDENTICAL in CQWPXContest.cpp, ARRLDXContest.cpp, and 3 others
```

**Recommendation**:
```cpp
// Create src/contests/RSTValidator.h
namespace TR4QT {

class RSTValidator {
public:
    static bool isValid(const QString& rst, ModeType mode) {
        if (mode == ModeType::CW || mode == ModeType::CWR) {
            QRegularExpression re("^[1-5][1-9][1-9]$");
            return re.match(rst).hasMatch();
        } else {
            QRegularExpression re("^[1-5][1-9][1-9]?$");
            return re.match(rst).hasMatch();
        }
    }

    static QString getDefault(ModeType mode) {
        return (mode == ModeType::CW || mode == ModeType::CWR) ? "599" : "59";
    }
};

} // namespace TR4QT
```

**Usage**:
```cpp
#include "RSTValidator.h"

bool firstIsRST = RSTValidator::isValid(first, m_mode);
QString rst = RSTValidator::getDefault(m_mode);
```

---

### Issue 3: Hard-Coded RST Default Values (15+ Locations)

**Priority**: HIGH
**Effort**: 15 minutes (depends on Issue 2)
**Impact**: Code duplication, magic strings

**Problem**:
Magic strings "599" and "59" appear 15+ times with duplicate ternary logic:
```cpp
qso.rstSent = (qso.mode == ModeType::CW) ? "599" : "59";  // MainWindow.cpp:1872
qso.rstReceived = (qso.mode == ModeType::CW) ? "599" : "59";  // MainWindow.cpp:1884
QString rst = (mode == ModeType::CW || mode == ModeType::CWR) ? "599" : "59";  // InitialExchangeManager.cpp:147
// + 12 more locations
```

**Recommendation**:
Use `RSTValidator::getDefault()` from Issue 2:
```cpp
qso.rstSent = RSTValidator::getDefault(qso.mode);
qso.rstReceived = RSTValidator::getDefault(qso.mode);
```

---

### Issue 4: Duplicate Reconnection Logic (MainWindow + DXClusterWindow)

**Priority**: MEDIUM
**Effort**: 2 hours
**Impact**: Code duplication, maintainability

**Problem**:
Nearly identical auto-reconnect code in 2 classes with hard-coded values (10 attempts, 10000ms interval).

**Current Code**:
```cpp
// MainWindow.h
static constexpr int MAX_RADIO_RECONNECT_ATTEMPTS = 10;

// MainWindow.cpp lines 156-174
m_radioReconnectTimer->setInterval(10000);  // 10 seconds
connect(m_radioReconnectTimer, &QTimer::timeout, this, [this]() {
    if (m_radioAutoReconnect && m_radioReconnectAttempts < MAX_RADIO_RECONNECT_ATTEMPTS) {
        m_radioReconnectAttempts++;
        // ... complex reconnection logic ...
    }
});

// DXClusterWindow - IDENTICAL pattern with different variable names
```

**Recommendation**:
Extract to reusable `ReconnectionManager` helper class in `src/utils/ReconnectionManager.{h,cpp}` with configurable retry strategy.

---

### Issue 5: Magic Numbers for UI Dimensions (20+ Locations)

**Priority**: MEDIUM
**Effort**: 1 hour
**Impact**: UI consistency, maintainability

**Problem**:
Hard-coded UI dimensions scattered throughout MainWindow.cpp:
```cpp
setMinimumSize(800, 600);
resize(1024, 768);
m_qsoTableView->setColumnWidth(QSOTableModel::ColBand, qMax(60, ...));
m_callsignEntry->setMinimumWidth(150);
int newWpm = qMin(currentWpm + increment, 60);  // Max 60 WPM
int newWpm = qMax(currentWpm - increment, 5);   // Min 5 WPM
```

**Recommendation**:
Add UI constants to `src/core/Constants.h`:
```cpp
// UI Window Dimensions
constexpr int MAIN_WINDOW_MIN_WIDTH = 800;
constexpr int MAIN_WINDOW_MIN_HEIGHT = 600;
constexpr int MAIN_WINDOW_DEFAULT_WIDTH = 1024;
constexpr int MAIN_WINDOW_DEFAULT_HEIGHT = 768;

// QSO Table Column Widths
constexpr int COL_WIDTH_BAND = 60;
constexpr int COL_WIDTH_DATE = 80;
constexpr int COL_WIDTH_CALLSIGN = 100;

// CW Speed Limits
constexpr int CW_SPEED_MIN_WPM = 5;
constexpr int CW_SPEED_MAX_WPM = 60;
```

---

## Medium Priority Improvements

### Issue 6: Duplicate Exchange Parsing Logic (All Contest Classes)

**Effort**: 3 hours
**Impact**: 50+ lines per contest class

All contest classes have similar `parseReceivedExchange()` methods with duplicate "smart detection" logic for RST vs other fields. Extract common logic to `ExchangeParser` utility class.

---

### Issue 7: Hard-Coded Zone Validation (CQ/ITU)

**Effort**: 20 minutes
**Impact**: Magic numbers

```cpp
if (!ok || zone < 1 || zone > 40) {  // CQ Zone validation
```

Add to Constants.h:
```cpp
constexpr int CQ_ZONE_MIN = 1;
constexpr int CQ_ZONE_MAX = 40;
constexpr int ITU_ZONE_MIN = 1;
constexpr int ITU_ZONE_MAX = 90;
```

---

### Issue 8: Duplicate Transaction Pattern in QSORepository

**Effort**: 1 hour
**Impact**: Boilerplate reduction

Transaction begin/commit/rollback pattern duplicated in 3+ methods (~20 lines each). Extract to template wrapper method.

---

### Issue 9: Hard-Coded Integrity Check Thresholds

**Effort**: 15 minutes
**Impact**: Configuration

```cpp
m_integrityCheckTimer->start(5 * 60 * 1000);  // Check every 5 minutes
if (m_qsosSinceLastIntegrityCheck >= 50) {  // Check after 50 QSOs
```

Add to Constants.h:
```cpp
constexpr int INTEGRITY_CHECK_INTERVAL_MS = 5 * 60 * 1000;
constexpr int INTEGRITY_CHECK_QSO_THRESHOLD = 50;
```

---

## Low Priority Suggestions

### Issue 10: Duplicate Color Definitions

Hard-coded color values (`#ff6600`, `#666`, etc.) scattered throughout UI code. Use ThemeManager or create color constants.

### Issue 11: Repetitive Font Creation

Font family "Monospace" hard-coded in many places. Create `FontFactory` utility.

---

## Positive Observations ✅

These patterns are **well-implemented** and should be **maintained**:

1. **Contest Registry System** - Clean plugin architecture with factory pattern
2. **Database Integrity Checks** - Excellent Tier 1 verification in QSORepository
3. **CountryFile Pointer Pattern** - Avoids duplicate loading (follows CLAUDE.md)
4. **Exchange Field Metadata** - Declarative contest rules
5. **Namespace Organization** - Consistent `TR4QT` namespace
6. **Logging Patterns** - Good use of LOG_* macros throughout

---

## Implementation Roadmap

### Phase 1: Critical (Priority 1)
- [ ] Issue 1: Complete QMessageBox → DialogHelper migration

### Phase 2: Quick Wins (1-2 hours total)
- [ ] Issue 2: Extract RSTValidator
- [ ] Issue 3: Use RSTValidator::getDefault()
- [ ] Issue 5: Extract UI dimension constants
- [ ] Issue 7: Add zone validation constants
- [ ] Issue 9: Add integrity check constants

### Phase 3: Code Quality (4-6 hours)
- [ ] Issue 4: Extract ReconnectionManager
- [ ] Issue 6: Create ExchangeParser utility
- [ ] Issue 8: Transaction wrapper in QSORepository

### Phase 4: Polish (1-2 hours)
- [ ] Issue 10: Consolidate color definitions
- [ ] Issue 11: Create FontFactory utility

**Total Estimated Effort**: 12-16 hours for all issues

---

## Impact Summary

**Code Reduction**: ~200-300 lines (duplicate elimination)
**Maintainability**: Centralized constants, reusable helpers
**Testability**: Isolated validators and parsers
**Compliance**: Full DialogHelper logging coverage
**Consistency**: UI dimensions and colors standardized

---

## Files Referenced

**Core**:
- src/core/Constants.h
- src/utils/DialogHelper.{h,cpp}

**UI**:
- src/ui/MainWindow.{h,cpp}
- src/ui/dialogs/ADIFImportDialog.cpp
- src/ui/dialogs/ContestChooserDialog.cpp
- src/ui/dialogs/PreferencesDialog.cpp
- src/ui/widgets/DXClusterWindow.{h,cpp}
- (+ 4 more dialog files)

**Contests**:
- src/contests/CQWWContest.cpp
- src/contests/CQWPXContest.cpp
- src/contests/ARRLDXContest.cpp
- src/contests/IARUHFContest.cpp
- (+ 2 more contest files)

**Data**:
- src/data/QSORepository.cpp

**Exchanges**:
- src/exchanges/InitialExchangeManager.cpp

---

**End of Review**
