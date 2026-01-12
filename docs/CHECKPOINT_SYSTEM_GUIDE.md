# TR4QT Checkpoint System - Implementation Guide

**Created**: 2026-01-12
**Purpose**: Prevent god class recurrence via algorithmic AI guidance

---

## What Problem Does This Solve?

**Problem**: Despite having god class prevention rules in CLAUDE.md, MainWindow grew to 5,564 lines (3.7X over 1,500 limit).

**Root Cause**: CLAUDE.md was written for humans, not AI agents. It used:
- ❌ Subjective language ("consider refactoring")
- ❌ No explicit triggers ("remember to check")
- ❌ Advisory guidelines (no enforcement)
- ❌ No feedback loop (violations not visible)

**Solution**: Checkpoint system with:
- ✅ Algorithmic rules (IF/THEN logic)
- ✅ Explicit triggers (task start, before commit)
- ✅ Hard constraints (task fails if violated)
- ✅ Automated feedback (scripts log violations)

---

## System Architecture

### Files Created

```
TR4QT/
├── .claude/
│   ├── CHECKPOINTS.md           # Mandatory checks at workflow points
│   ├── REFUSAL_TEMPLATES.md     # Exact text for blocking violations
│   ├── VIOLATIONS.md            # Log of rule violations
│   └── METRICS.md               # Architecture health tracking
├── scripts/
│   └── check_architecture.sh    # Automated violation detection
├── docs/
│   ├── EXTRACTION_ARCHITECTURE.md  # Service extraction plan
│   └── CHECKPOINT_SYSTEM_GUIDE.md  # This file
├── tests/
│   ├── test_onLogQSO_specification.md  # Behavior analysis
│   └── test_mainwindow_logqso.cpp     # Integration tests (partial)
└── CLAUDE_V2.md                 # Rewritten rules for AI agents
```

### Workflow Integration

```
┌─────────────────┐
│  User: Add      │
│  feature X      │
└────────┬────────┘
         │
         v
┌─────────────────────────────────────────┐
│ CHECKPOINT 1: Task Start (MANDATORY)    │
│                                         │
│ 1. Parse user request                   │
│ 2. Identify target files                │
│ 3. FOR EACH file:                       │
│      wc -l <file>                       │
│      IF lines >= 1500: BLOCK            │
│ 4. Run: bash scripts/check_architecture.sh │
└────────┬────────────────────────────────┘
         │
         ├─ VIOLATIONS FOUND
         │  │
         │  v
         │  ┌───────────────────────────┐
         │  │ Output Refusal Template   │
         │  │ (.claude/REFUSAL_TEMPLATES.md) │
         │  ├───────────────────────────┤
         │  │ Options:                  │
         │  │ A) Extract services       │
         │  │ B) Override (logged)      │
         │  └───────────┬───────────────┘
         │              │
         │              v
         │         ┌─────────┐
         │         │  WAIT   │
         │         └─────────┘
         │
         └─ NO VIOLATIONS
            │
            v
        ┌──────────────┐
        │  PROCEED     │
        │  with task   │
        └──────────────┘
```

---

## How to Use

### For Claude (AI Agent)

**At start of EVERY task:**

1. **Read checkpoint file**:
   ```bash
   # Check file: .claude/CHECKPOINTS.md
   ```

2. **Execute task start check**:
   ```bash
   bash scripts/check_architecture.sh
   ```

3. **If violations found**:
   - Read: `.claude/VIOLATIONS.md`
   - Output: Appropriate template from `.claude/REFUSAL_TEMPLATES.md`
   - Wait: User decision (extract, override, cancel)

4. **If no violations**:
   - Proceed with feature implementation

**Example Claude response when blocked**:
```
📊 Architecture Check: src/ui/MainWindow.cpp

❌ TASK BLOCKED: Architecture Violation

**File:** src/ui/MainWindow.cpp
**Current size:** 5,564 lines
**Limit:** 1,500 lines
**Overage:** 4,064 lines (271% over limit)

**Requested task:** Add dark mode toggle to Settings
**Estimated addition:** +75 lines

**Why this is blocked:**
MainWindow is a god class. Adding more code increases technical debt.

**Required action:**
Extract services BEFORE adding features.

**Extraction plan:**
See: docs/EXTRACTION_ARCHITECTURE.md

Options:
A) Proceed with extraction (recommended) - Type 'yes'
B) Override and add technical debt - Type 'OVERRIDE: reason'

What would you like to do?
```

### For Human (Developer)

**Daily workflow:**

1. **Morning**: Review architecture health
   ```bash
   bash scripts/check_architecture.sh
   cat .claude/METRICS.md
   ```

2. **During development**: Trust checkpoints
   - Claude will refuse invalid tasks
   - Override if absolutely necessary: `OVERRIDE: reason`
   - All overrides logged to `.claude/VIOLATIONS.md`

3. **Before commit**: Verify clean
   ```bash
   bash scripts/check_architecture.sh
   cd build && ctest --output-on-failure
   ```

4. **Weekly**: Review metrics
   - Open `.claude/METRICS.md`
   - Check MainWindow LOC trend
   - Update extraction priorities

**Override format**:
```
User: OVERRIDE: Demo for customer, will refactor next sprint
```

**All overrides logged**:
```markdown
## Violation: 2026-01-12 14:30
- Type: God Class (Feature Added Despite Limit)
- File: src/ui/MainWindow.cpp
- Task: Add dark mode toggle
- Override reason: Demo for customer, will refactor next sprint
- Lines added: +75
- New total: 5,639 lines
```

---

## Architecture Check Script

### What It Checks

**1. File Size Limits** (God Class Detection)
- Green: < 1,000 lines
- Yellow: 1,000-1,499 lines (warning)
- Red: >= 1,500 lines (BLOCK)

**2. SQL in UI Classes**
- Searches: `src/ui/**/*.cpp` for SQL queries
- Pattern: `QSqlQuery`, `db.exec`, `query.exec`
- Action: BLOCK, recommend Repository class

**3. Hardcoded Hex Colors**
- Pattern: `= "#RRGGBB"` (excluding ThemeManager.cpp)
- Action: BLOCK, recommend ThemeManager

**4. setParent(nullptr) Calls**
- Pattern: `setParent(nullptr)`
- Action: BLOCK, recommend deleteLater() or hide()

### Running Manually

```bash
# Check architecture health
bash scripts/check_architecture.sh

# View violations
cat .claude/VIOLATIONS.md

# View metrics history
cat .claude/METRICS.md
```

### Example Output

```
🔍 TR4QT Architecture Health Check
==================================

📏 Check 1: File Size Limits
   Thresholds: Yellow=1000, Red=1500

   ❌ VIOLATION: src/ui/MainWindow.cpp
      Lines: 5564 (limit: 1500)
      Overage: 4064 lines (270.9% over)

📊 Check 2: SQL Queries in UI Classes

   ❌ VIOLATION: SQL queries found in UI classes

   src/ui/MainWindow.cpp:3104: QSqlQuery query = db.execute(
   [... 10 violations shown ...]

🎨 Check 3: Hardcoded Hex Colors

   ✅ PASS: No hardcoded colors (using ThemeManager)

🪟 Check 4: setParent(nullptr) Calls

   ✅ PASS: No setParent(nullptr) calls

==================================
📊 Summary
==================================

   MainWindow: 5564 lines
   God classes: 1
   Violations: 2
   Warnings: 0

❌ FAILED: 2 architecture violations found

See: .claude/VIOLATIONS.md for details
See: .claude/CHECKPOINTS.md for resolution steps
```

---

## Checkpoint Details

### CHECKPOINT 1: Task Start (MANDATORY)

**Trigger**: Before implementing ANY feature

**What it checks**:
- File size limits (god class detection)
- Target file status (GREEN/YELLOW/RED)
- Existing violations in log

**Actions**:
- RED: Block task, output refusal, wait for user
- YELLOW: Warn, propose new service instead
- GREEN: Proceed

**See**: `.claude/CHECKPOINTS.md` section 1

---

### CHECKPOINT 2: Before Commit (MANDATORY)

**Trigger**: Before `git commit`

**What it checks**:
- All tests pass
- Architecture violations (file size, SQL, colors, etc.)
- Forbidden patterns

**Actions**:
- Tests fail: Block commit
- Violations found: Block commit
- All pass: Proceed

**See**: `.claude/CHECKPOINTS.md` section 2

---

### CHECKPOINT 3: After Extraction (MANDATORY)

**Trigger**: After extracting service from MainWindow

**What it checks**:
- MainWindow LOC reduced by expected amount
- Extracted service has tests (>80% coverage)
- All tests pass (no regressions)

**Actions**:
- Metrics updated in `.claude/METRICS.md`
- Commit with LOC reduction in message
- Update extraction progress

**See**: `.claude/CHECKPOINTS.md` section 3

---

### CHECKPOINT 4: Weekly Review (RECOMMENDED)

**Trigger**: Start of week

**What it generates**:
- Architecture health report
- Trend analysis (vs last week)
- Recommendations

**See**: `.claude/CHECKPOINTS.md` section 4

---

## Refusal Templates

**Location**: `.claude/REFUSAL_TEMPLATES.md`

**Templates available**:

1. **God Class Limit Exceeded** - File >= 1,500 lines
2. **File in Yellow Zone** - File >= 1,000 lines (warning)
3. **No Tests for New Feature** - Feature without tests
4. **SQL in UI Class** - Database code in UI
5. **Business Logic Loop in Event Handler** - Logic in UI handler
6. **Refactoring Without Tests** - No tests to prove equivalence

**Usage**: Claude copies exact text, fills in variables, outputs to user.

**Example variables**:
- `{file_path}` - Path to violating file
- `{current_lines}` - Current line count
- `{percentage}` - Percent over limit
- `{user_task_description}` - What user requested

---

## Violations Log

**Location**: `.claude/VIOLATIONS.md`

**Format**:
```markdown
## Violation: 2026-01-12 10:35:29
- Type: God Class (File Size)
- File: src/ui/MainWindow.cpp
- Lines: 5564
- Limit: 1500
- Overage: 4064 lines (270.9% over)
- Status: ❌ RED
```

**Automatic logging**: `scripts/check_architecture.sh` appends violations

**Manual logging**: User overrides recorded with reason

**Review**: Check periodically to see violation trends

---

## Metrics Tracking

**Location**: `.claude/METRICS.md`

**Tracks**:
- MainWindow LOC over time
- God class count
- Test coverage
- Architecture health score

**Format**:
```
| Date | MainWindow LOC | God Classes | Test Coverage | Notes |
|------|----------------|-------------|---------------|-------|
| 2026-01-12 | 5,564 | 1 | ~65% | Baseline |
```

**Automatic updates**: `scripts/check_architecture.sh` appends metrics

**Weekly reviews**: Add manual review using template

**Goal tracking**: Target MainWindow < 2,500 lines (55% reduction)

---

## Integration with Git

### Pre-Commit Hook (Future)

**Not yet implemented**, but recommended:

```bash
#!/bin/bash
# .git/hooks/pre-commit

# Run architecture check
bash scripts/check_architecture.sh

if [ $? -ne 0 ]; then
    echo "❌ Commit blocked by architecture violations"
    echo "Fix violations or use: git commit --no-verify"
    exit 1
fi
```

**Benefits**:
- Prevents commits with violations
- Catches issues before CI
- Can be bypassed with `--no-verify` if needed

---

## Success Metrics

**Goal**: Reduce MainWindow from 5,564 → 2,500 lines (55% reduction)

**Milestones**:
- [ ] Week 1: Extract QSOLoggingService (-306 lines) → 5,258 lines
- [ ] Week 2: Extract QSOPersistenceService (-140 lines) → 5,118 lines
- [ ] Week 3: Extract ExchangeMemoryService (-50 lines) → 5,068 lines
- [ ] Week 4-8: Continue extractions → 2,500 lines (target)

**Weekly tracking**: `.claude/METRICS.md`

**Automation**: `scripts/check_architecture.sh` updates metrics

---

## Lessons Learned (for Future Projects)

### What Went Wrong with Original CLAUDE.md

1. **Written for humans, not AI**
   - Used subjective language ("consider", "should")
   - No explicit triggers
   - Assumed proactive monitoring

2. **No enforcement mechanism**
   - All guidelines, no constraints
   - Task completion prioritized over architecture
   - No penalty for violations

3. **No feedback loop**
   - Violations invisible until manual review
   - AI never received signal that rules were broken
   - No persistent state across tasks

### How Checkpoint System Fixes This

1. **Algorithmic rules**
   - IF/THEN logic
   - Explicit thresholds (1,500 lines = BLOCK)
   - No subjective judgment

2. **Hard constraints**
   - Task FAILS if violated
   - Requires explicit user override
   - All overrides logged

3. **Automated feedback**
   - Scripts check and log violations
   - AI reads logs at task start
   - Violations visible immediately

4. **Externalized state**
   - `.claude/VIOLATIONS.md` persists across tasks
   - `.claude/METRICS.md` tracks trends
   - AI doesn't need memory

### Applying to Other Projects

**Template structure**:
```
project/
├── .claude/
│   ├── CHECKPOINTS.md         # Workflow checkpoints
│   ├── REFUSAL_TEMPLATES.md   # Block messages
│   ├── VIOLATIONS.md          # Violation log
│   └── METRICS.md             # Health tracking
├── scripts/
│   └── check_[constraint].sh  # Automated checks
└── CLAUDE.md                  # AI guidance (algorithmic)
```

**Key principles**:
- Algorithmic rules (IF/THEN), not prose
- Explicit triggers (task start, pre-commit)
- Hard constraints (task fails if violated)
- Automated checks (scripts write logs)
- Externalized state (logs persist)

---

## Next Steps

1. **Test the system**: Try adding a feature to MainWindow
   - Claude should refuse and output template
   - Verify violations logged to `.claude/VIOLATIONS.md`

2. **Start extraction**: Follow `docs/EXTRACTION_ARCHITECTURE.md`
   - Phase 1: Extract QSOLoggingService
   - Write tests FIRST
   - Extract while keeping tests green

3. **Monitor metrics**: Check `.claude/METRICS.md` weekly
   - Track MainWindow LOC reduction
   - Verify no new violations
   - Adjust extraction plan as needed

4. **Refine checkpoints**: Add new checks as patterns emerge
   - Update `.claude/CHECKPOINTS.md`
   - Add templates to `.claude/REFUSAL_TEMPLATES.md`
   - Update `scripts/check_architecture.sh`

---

## FAQ

**Q: Can I bypass checkpoints?**
A: Yes, with `OVERRIDE: reason`. All overrides logged to `.claude/VIOLATIONS.md`.

**Q: What if I need to add code to MainWindow urgently?**
A: Use override, but document why. Plan refactoring for next sprint.

**Q: Will checkpoints slow down development?**
A: Initially, yes (learning curve). Long-term, no (prevents god classes that take weeks to refactor).

**Q: Can I disable checkpoints?**
A: Technically yes (don't run `check_architecture.sh`), but defeats the purpose. Better to adjust thresholds if needed.

**Q: What if the script reports false positives?**
A: Update `scripts/check_architecture.sh` to exclude valid patterns. Document exclusions.

**Q: How do I add new checkpoints?**
A: 1) Add to `.claude/CHECKPOINTS.md`, 2) Create template in `.claude/REFUSAL_TEMPLATES.md`, 3) Update `scripts/check_architecture.sh`.

---

## Summary

**Problem**: God class accumulated to 5,564 lines despite having prevention rules.

**Root Cause**: Rules were advisory, not enforceable. No triggers, no feedback.

**Solution**: Checkpoint system with algorithmic rules, explicit triggers, automated checks.

**Result**: AI agents now BLOCKED from adding to oversized files until extraction complete.

**Benefits**:
- ✅ Prevents god class recurrence
- ✅ Forces extraction before features
- ✅ Tracks violations and metrics
- ✅ Applicable to future projects

**Files to consult**:
- `.claude/CHECKPOINTS.md` - What to check and when
- `.claude/REFUSAL_TEMPLATES.md` - How to refuse tasks
- `.claude/VIOLATIONS.md` - Violation history
- `.claude/METRICS.md` - Architecture health
- `docs/EXTRACTION_ARCHITECTURE.md` - How to extract services

**Start here**: Read `.claude/CHECKPOINTS.md`, then run `bash scripts/check_architecture.sh`.
