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
## Violation: 2026-01-12 10:35:29
- Type: God Class (File Size)
- File: src/ui/MainWindow.cpp
- Lines: 5564
- Limit: 1500
- Overage: 4064 lines (270.9% over)
- Status: ❌ RED

## Violation: 2026-01-12 10:35:29
- Type: SQL in UI Class
- Details: SQL queries found in src/ui/ directory
- Lines:
```
src/ui/MainWindow.cpp:3104:        QSqlQuery query = db.execute(
src/ui/MainWindow.cpp:3392:    QSqlQuery checkpointQuery = db.execute("PRAGMA wal_checkpoint(PASSIVE)", {});
src/ui/MainWindow.cpp:3396:    QSqlQuery countQuery = db.execute(
src/ui/MainWindow.cpp:3444:    QSqlQuery dbQuery = db.execute(
src/ui/MainWindow.cpp:3508:    QSqlQuery unknownBandQuery = db.execute(
src/ui/MainWindow.cpp:3538:        QSqlQuery lowercaseQuery = db.execute(
src/ui/MainWindow.cpp:3583:    QSqlQuery versionQuery = db.execute("PRAGMA user_version", {});
src/ui/MainWindow.cpp:3602:    QSqlQuery columnsQuery = db.execute("PRAGMA table_info(qsos)", {});
src/ui/MainWindow.cpp:3640:        QSqlQuery sampleQuery = db.execute(
src/ui/MainWindow.cpp:3999:    QSqlQuery query = db.execute("SELECT contest_id, contest_name, start_time, contest_type FROM contests LIMIT 1", {});
```

## Violation: 2026-01-12 10:59:26
- Type: God Class (File Size)
- File: src/ui/MainWindow.cpp
- Lines: 5564
- Limit: 1500
- Overage: 4064 lines (270.9% over)
- Status: ❌ RED

## Violation: 2026-01-12 10:59:26
- Type: SQL in UI Class
- Details: SQL queries found in src/ui/ directory
- Lines:
```
src/ui/MainWindow.cpp:3104:        QSqlQuery query = db.execute(
src/ui/MainWindow.cpp:3392:    QSqlQuery checkpointQuery = db.execute("PRAGMA wal_checkpoint(PASSIVE)", {});
src/ui/MainWindow.cpp:3396:    QSqlQuery countQuery = db.execute(
src/ui/MainWindow.cpp:3444:    QSqlQuery dbQuery = db.execute(
src/ui/MainWindow.cpp:3508:    QSqlQuery unknownBandQuery = db.execute(
src/ui/MainWindow.cpp:3538:        QSqlQuery lowercaseQuery = db.execute(
src/ui/MainWindow.cpp:3583:    QSqlQuery versionQuery = db.execute("PRAGMA user_version", {});
src/ui/MainWindow.cpp:3602:    QSqlQuery columnsQuery = db.execute("PRAGMA table_info(qsos)", {});
src/ui/MainWindow.cpp:3640:        QSqlQuery sampleQuery = db.execute(
src/ui/MainWindow.cpp:3999:    QSqlQuery query = db.execute("SELECT contest_id, contest_name, start_time, contest_type FROM contests LIMIT 1", {});
```

## Violation: 2026-01-12 11:40:01
- Type: God Class (File Size)
- File: src/ui/MainWindow.cpp
- Lines: 5564
- Limit: 1500
- Overage: 4064 lines (270.9% over)
- Status: ❌ RED

## Violation: 2026-01-12 11:40:01
- Type: SQL in UI Class
- Details: SQL queries found in src/ui/ directory
- Lines:
```
src/ui/MainWindow.cpp:3104:        QSqlQuery query = db.execute(
src/ui/MainWindow.cpp:3392:    QSqlQuery checkpointQuery = db.execute("PRAGMA wal_checkpoint(PASSIVE)", {});
src/ui/MainWindow.cpp:3396:    QSqlQuery countQuery = db.execute(
src/ui/MainWindow.cpp:3444:    QSqlQuery dbQuery = db.execute(
src/ui/MainWindow.cpp:3508:    QSqlQuery unknownBandQuery = db.execute(
src/ui/MainWindow.cpp:3538:        QSqlQuery lowercaseQuery = db.execute(
src/ui/MainWindow.cpp:3583:    QSqlQuery versionQuery = db.execute("PRAGMA user_version", {});
src/ui/MainWindow.cpp:3602:    QSqlQuery columnsQuery = db.execute("PRAGMA table_info(qsos)", {});
src/ui/MainWindow.cpp:3640:        QSqlQuery sampleQuery = db.execute(
src/ui/MainWindow.cpp:3999:    QSqlQuery query = db.execute("SELECT contest_id, contest_name, start_time, contest_type FROM contests LIMIT 1", {});
```

