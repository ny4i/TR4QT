# TR4QT Architecture Checkpoints

**PURPOSE**: Mandatory checks that Claude MUST execute at specific workflow points.

**ENFORCEMENT**: These are HARD CONSTRAINTS. Task fails if checkpoints don't pass.

---

## CHECKPOINT 1: Task Start (MANDATORY)

**TRIGGER**: Before implementing ANY feature or code change

**ALGORITHM**:
```
EXECUTE:
  1. Parse user request to identify target files
  2. FOR EACH file in target_files:
       a. Run: wc -l <file>
       b. Determine status: GREEN/YELLOW/RED
       c. IF RED: Execute REFUSAL_PROTOCOL
       d. IF YELLOW: Execute WARNING_PROTOCOL
  3. IF all GREEN: PROCEED

THRESHOLDS (normal classes):
  - GREEN: < 1000 lines
  - YELLOW: 1000-1499 lines
  - RED: >= 1500 lines

THRESHOLDS (MainWindow / main UI entry point):
  - GREEN: < 2000 lines
  - YELLOW: 2000-2499 lines
  - RED: >= 3000 lines

REFUSAL_PROTOCOL (RED):
  1. Output refusal message (see .claude/REFUSAL_TEMPLATES.md)
  2. Output extraction plan (docs/EXTRACTION_ARCHITECTURE.md)
  3. Log violation to .claude/VIOLATIONS.md
  4. WAIT FOR user_override OR user_starts_extraction
  5. DO NOT implement feature

WARNING_PROTOCOL (YELLOW):
  1. Output warning message
  2. Propose: Implement feature in NEW service class instead
  3. Show benefit: Keeps file under 1500 limit
  4. Ask: "Implement in new service, or add to existing file?"
  5. IF user chooses existing: PROCEED with caution
  6. IF user chooses new service: Design service first
```

**EXECUTION COMMAND**:
```bash
# Check MainWindow size
wc -l src/ui/MainWindow.cpp

# Check all large files
bash scripts/check_architecture.sh
```

**OUTPUT FORMAT**:
```
📊 File Health Check: src/ui/MainWindow.cpp
├─ Current: 5,050 lines
├─ Threshold: 3,000 (MainWindow STOP limit)
├─ Status: 🔴 RED (1.7X over limit)
├─ Estimated change: +50 lines
├─ After change: 5,100 lines
└─ Action: 🚫 BLOCKED - See .claude/REFUSAL_TEMPLATES.md
```

---

## CHECKPOINT 2: Before Commit (MANDATORY)

**TRIGGER**: Before running `git commit`

**ALGORITHM**:
```
EXECUTE:
  1. Run all tests: cd build && ctest --output-on-failure
  2. IF tests fail: BLOCK commit, show failures
  3. Run architecture check: bash scripts/check_architecture.sh
  4. IF violations found: BLOCK commit, show violations
  5. Check for forbidden patterns:
     - SQL queries in UI classes (grep)
     - Hardcoded hex colors (grep)
     - setParent(nullptr) calls (grep)
  6. IF forbidden patterns found: BLOCK commit
  7. IF all checks pass: PROCEED with commit

FORBIDDEN PATTERNS:
  1. SQL in UI:
     - Pattern: src/ui/**/*.cpp containing "db.exec" or "QSqlQuery"
     - Action: BLOCK, recommend Repository class

  2. Hardcoded colors:
     - Pattern: /*.cpp containing '= "#[0-9A-Fa-f]{6}"' (not in ThemeManager.cpp)
     - Action: BLOCK, recommend ThemeManager::color()

  3. setParent(nullptr):
     - Pattern: /*.cpp containing "setParent(nullptr)"
     - Action: BLOCK, recommend deleteLater() or hide()
```

**EXECUTION COMMAND**:
```bash
# Run pre-commit checks
bash scripts/pre-commit-checks.sh
```

---

## CHECKPOINT 3: After Extraction (MANDATORY)

**TRIGGER**: After extracting services from MainWindow

**ALGORITHM**:
```
EXECUTE:
  1. Verify MainWindow LOC reduced
     - Before: Read from .claude/METRICS.md
     - After: wc -l src/ui/MainWindow.cpp
     - Reduction: Before - After
     - IF reduction < expected: WARN (incomplete extraction)

  2. Verify extracted service has tests
     - Check: tests/test_{service_name}.cpp exists
     - IF missing: BLOCK (no tests = no extraction)

  3. Verify tests pass
     - Run: cd build && ctest --output-on-failure
     - IF fail: ROLLBACK extraction

  4. Update metrics
     - Log to .claude/METRICS.md
     - Record: date, service name, LOC reduction, test coverage

  5. IF all pass: Mark extraction COMPLETE
     - Commit with message including LOC reduction
     - Update docs/EXTRACTION_ARCHITECTURE.md progress
```

**SUCCESS CRITERIA**:
- MainWindow LOC reduced by expected amount
- Extracted service has >80% test coverage
- All tests pass (old + new)
- No regressions introduced

---

## CHECKPOINT 4: Weekly Architecture Review (RECOMMENDED)

**TRIGGER**: Once per week, start of session

**ALGORITHM**:
```
EXECUTE:
  1. Generate architecture health report
     - Run: bash scripts/architecture_report.sh
     - Shows: File sizes, test coverage, god classes, trends

  2. Compare to last week
     - Read: .claude/METRICS.md (last 2 entries)
     - Calculate: ΔLines, ΔCoverage, ΔGodClasses

  3. IF metrics worsening:
     - Output: Warning with trend analysis
     - Recommend: Pause features, focus on refactoring
     - Set goal: Reduce MainWindow by X lines this week

  4. IF metrics improving:
     - Output: Progress report
     - Continue: Current extraction plan

  5. Update metrics log
     - Append to .claude/METRICS.md
```

**OUTPUT FORMAT**:
```
📊 TR4QT Architecture Health Report (2026-01-13)

God Classes:
  ❌ src/ui/MainWindow.cpp: 5,050 lines (1.7X over 3,000 limit)

Trend (vs. last week):
  📈 MainWindow: -514 lines (better - extraction ongoing)
  📉 Test coverage: +5% (better)
  ✅ Technical debt decreasing

Recommendation:
  🔧 Continue extraction: StationInfoService next (target: -350 lines)
  🎯 Goal: MainWindow < 3,000 lines
```

---

## Using These Checkpoints

### For Claude (AI Agent):

1. **At task start**: Read this file, execute CHECKPOINT 1
2. **Before commit**: Execute CHECKPOINT 2
3. **After extraction**: Execute CHECKPOINT 3
4. **Weekly**: Execute CHECKPOINT 4

### For Human (Developer):

1. Trust but verify - spot check Claude followed checkpoints
2. Override when necessary: `OVERRIDE: <reason>`
3. Update thresholds if needed (document why)
4. Add new checkpoints as patterns emerge

---

## Checkpoint Failure Log

**Location**: `.claude/VIOLATIONS.md`

**Format**:
```markdown
## Violation: {Date}
- File: {file}
- Lines: {lines}
- Limit: {threshold}
- Task: {user_request}
- Action: BLOCKED
- Resolution: {override | extraction | cancelled}
```

---

## Success Metrics

Track progress in `.claude/METRICS.md`:

- MainWindow LOC reduction over time
- Test coverage increase
- Extraction completion (services created)
- Architecture violations (should trend to 0)

**Goal**: MainWindow < 3,000 lines (40% reduction from 5,050)
