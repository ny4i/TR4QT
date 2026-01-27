# TR4QT Architecture Violations Log

This file logs all architecture rule violations.

**Purpose**: Track when rules are violated, why, and how they were resolved.

**Format**:
- Date/Time
- Violation type
- File and details
- Action taken (blocked, overridden, cancelled)
- Resolution notes

---

## Initial State (2026-01-12)

**Baseline violations discovered:**

### Violation: MainWindow God Class
- **File**: src/ui/MainWindow.cpp
- **Lines**: 5,564
- **Limit**: 1,500
- **Overage**: 4,064 lines (271% over)
- **Status**: ❌ PRE-EXISTING (accumulated over time)
- **Action**: Extraction plan created (docs/EXTRACTION_ARCHITECTURE.md)
- **Resolution**: IN PROGRESS - Phased extraction starting 2026-01-12

**Notes**: This violation existed before checkpoint system was created. The checkpoint system will prevent this from happening again.

---

## How to Use This Log

**When Claude blocks a task:**
1. Violation automatically logged by `scripts/check_architecture.sh`
2. Includes: date, type, file, details
3. Awaits user decision (extract, override, cancel)

**User overrides:**
Format:
```
User overrode violation with: OVERRIDE: {reason}
```

**Resolution:**
When violation is fixed:
```
Resolution (YYYY-MM-DD): {how it was fixed}
```

---

## Future Violations Will Appear Below

<!-- Violations logged by check_architecture.sh will be appended here -->

## Resolved Violations

### SQL in UI Class — RESOLVED (2026-01-27)
- **Original (2026-01-12)**: 10 SQL queries in MainWindow.cpp, 1 in BackupRestoreDialog.cpp
- **Resolution**: All SQL extracted to Repository/Service classes during phased extraction
- **Verification**: `grep -r 'db\.execute\|QSqlQuery' src/ui/` returns zero hits
- **Status**: ✅ RESOLVED — zero SQL in UI layer

### MainWindow God Class — ONGOING
- **Original (2026-01-12)**: 5,564 lines
- **Current (2026-01-27)**: 4,512 lines (19% reduction)
- **Limit**: 3,000 (MainWindow-specific threshold)
- **Status**: ⚠️ Still 1.5X over limit, but business logic extracted to services
- **Note**: Remaining code is primarily UI setup and window orchestration (see docs/FUTURE_REFACTORING.md)

