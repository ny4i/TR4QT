# Claude Development Rules for TR4QT

**CRITICAL**: This file contains MANDATORY checkpoints and HARD CONSTRAINTS for TR4QT development.

**For Claude (AI Agent)**: You are a stateless rule-following system. These rules use explicit triggers and algorithmic logic, not subjective judgment.

---

## 🤖 CHECKPOINT SYSTEM (MANDATORY)

**All architectural rules are enforced via checkpoints.**

### Quick Reference

| Checkpoint | When | Command |
|------------|------|---------|
| Task Start | Before ANY code change | Read `.claude/CHECKPOINTS.md` |
| Before Commit | Before `git commit` | `bash scripts/check_architecture.sh` |
| After Extraction | After service extraction | Run tests + update metrics |
| Weekly Review | Start of week | Generate health report |

**Full checkpoint details**: See `.claude/CHECKPOINTS.md`

---

## 🚦 MANDATORY: Task Start Protocol

**TRIGGER**: Beginning of EVERY task

**ALGORITHM**:
```
1. Read user request
2. Identify target files that will be modified
3. FOR EACH file:
     lines = execute("wc -l " + file)
     IF lines >= 1500:
       EXECUTE refusal_protocol(file, lines)
       WAIT FOR user_override OR user_starts_extraction
       RETURN WITHOUT IMPLEMENTING
     ENDIF
   ENDFOR
4. IF all files GREEN: PROCEED with implementation
```

**EXECUTE THIS CHECK**:
```bash
bash scripts/check_architecture.sh
```

**IF violations found:**
- READ: `.claude/REFUSAL_TEMPLATES.md`
- OUTPUT: Appropriate refusal template
- WAIT: User decision (extract services or override)
- DO NOT: Proceed with feature implementation

**Files to consult:**
- `.claude/CHECKPOINTS.md` - Mandatory checks at each workflow stage
- `.claude/REFUSAL_TEMPLATES.md` - Exact text for refusing tasks
- `docs/EXTRACTION_ARCHITECTURE.md` - Extraction plan when blocked

---

## 🤖 CRITICAL: Checkpoint-Based Architecture Enforcement

**IMPORTANT**: TR4QT now uses an algorithmic checkpoint system to enforce architecture rules.

### How This Works

1. **At task start**: Read `.claude/CHECKPOINTS.md` and execute CHECKPOINT 1
2. **Before any code change**: Run `bash scripts/check_architecture.sh`
3. **If violations found**: Output refusal template from `.claude/REFUSAL_TEMPLATES.md`
4. **If clear**: Proceed with implementation

### Quick Reference

```bash
# Before starting any task
bash scripts/check_architecture.sh

# If violations found:
# 1. Read .claude/VIOLATIONS.md
# 2. Read .claude/REFUSAL_TEMPLATES.md
# 3. Output appropriate refusal template
# 4. Wait for user decision (extract, override, cancel)
```

**See:**
- `.claude/CHECKPOINTS.md` - Mandatory checks at each workflow point
- `.claude/REFUSAL_TEMPLATES.md` - Exact text for refusing tasks
- `docs/EXTRACTION_ARCHITECTURE.md` - Service extraction plan

---

## 🤖 MANDATORY WORKFLOW FOR CLAUDE

### STEP 1: Task Start Checkpoint (EVERY TASK)

**BEFORE implementing ANY feature:**

```bash
# Execute architecture check
bash scripts/check_architecture.sh
```

**ALGORITHM**:
```
IF check exits with code 1:
  - Read .claude/VIOLATIONS.md
  - Identify violation type
  - Output refusal template from .claude/REFUSAL_TEMPLATES.md
  - WAIT FOR user decision (extract services OR override)
  - DO NOT implement feature
ELSE:
  - PROCEED with implementation
ENDIF
```

**This check is MANDATORY. No exceptions.**

---

## 🔴 HARD STOP: Checkpoint System

**NEW WORKFLOW** (effective 2026-01-12):

All architectural rules are now enforced via **checkpoint system**.

### Required Reading at Task Start

**BEFORE implementing ANY feature, Claude MUST:**

1. **Read**: `.claude/CHECKPOINTS.md` (mandatory checks)
2. **Execute**: CHECKPOINT 1 (Task Start)
3. **Check**: File size limits, forbidden patterns
4. **IF violations found**: Output refusal (see `.claude/REFUSAL_TEMPLATES.md`)
5. **IF clear**: Proceed with implementation

### Checkpoint Files

- **`.claude/CHECKPOINTS.md`** - Mandatory checks (task start, pre-commit, post-extraction)
- **`.claude/REFUSAL_TEMPLATES.md`** - Exact text for blocking violations
- **`.claude/VIOLATIONS.md`** - Log of rule violations and overrides
- **`.claude/METRICS.md`** - Architecture health tracking

### Automated Enforcement

**Run before every task:**
```bash
bash scripts/check_architecture.sh
```

**If violations found:**
1. Read `.claude/VIOLATIONS.md` for details
2. Output appropriate refusal template from `.claude/REFUSAL_TEMPLATES.md`
3. Wait for user decision (extraction vs. override)
4. DO NOT proceed with feature until resolved

**If all checks pass:**
- Proceed with feature implementation

---

## 🤖 AI AGENT WORKFLOW (MANDATORY)

**Claude MUST follow this algorithmic workflow for EVERY task:**

### PHASE 1: Task Start Checkpoint

```
EXECUTE:
  1. Read user request
  2. Identify target files to modify
  3. FOR EACH file:
       bash wc -l <file>
       IF lines >= 1500 THEN
           output(refusal_template_god_class)
           WAIT FOR user_override OR user_starts_extraction
           STOP
       ELIF lines >= 1000 THEN
           output_warning()
           propose_new_service_instead()
       ENDIF
  4. IF all files < 1500: PROCEED with implementation

CHECKPOINTS:
  - See: .claude/CHECKPOINTS.md (MANDATORY reading)
  - Run: bash scripts/check_architecture.sh
  - Check: .claude/VIOLATIONS.md for existing violations
```

**CRITICAL**: Read `.claude/CHECKPOINTS.md` at the START of EVERY task.

---

## 🤖 ALGORITHMIC ENFORCEMENT RULES

**These rules are HARD CONSTRAINTS - not suggestions.**

###CHECKPOINT: Task Start (MANDATORY)

**EXECUTE BEFORE implementing ANY feature:**

```
ALGORITHM:
  1. Parse user request for target files
  2. FOR EACH file IN target_files:
       EXECUTE: wc -l <file>
       IF lines >= 1500:
         OUTPUT: .claude/REFUSAL_TEMPLATES.md#Template1
         LOG: Violation to .claude/VIOLATIONS.md
         WAIT: User override OR extraction plan approval
         RETURN: Do not implement feature
       ELIF lines >= 1000:
         OUTPUT: Warning (file approaching limit)
         PROPOSE: Implement in new service class instead
       ENDIF
  3. PROCEED with implementation only if all files GREEN

EXECUTION:
  bash scripts/check_architecture.sh
  IF exit_code != 0:
    READ .claude/VIOLATIONS.md
    OUTPUT violations to user
    WAIT FOR user_response
  ENDIF
```

**Example output:**
```
📊 File Health Check: src/ui/MainWindow.cpp
├─ Current: 5,564 lines
├─ Threshold: 1,500 (STOP limit)
├─ Status: 🔴 RED (3.7X over limit)
└─ Action: 🚫 BLOCKED

I cannot add features to MainWindow until we extract services.

See: .claude/REFUSAL_TEMPLATES.md (Template 1)
See: docs/EXTRACTION_ARCHITECTURE.md (Extraction plan)

Options:
A) Extract services first (recommended)
B) Type 'OVERRIDE: {reason}' to add technical debt
```

**This is NOT optional. This is a HARD CONSTRAINT.**

---

## CHECKPOINT 2: Before Commit

**TRIGGER**: Before running `git commit`

**EXECUTION**:
```bash
# Run tests
cd build && ctest --output-on-failure

# Check architecture
bash scripts/check_architecture.sh

# Verify no violations
cat .claude/VIOLATIONS.md
```

**IF violations found**: BLOCK commit, fix violations first

**IF tests fail**: BLOCK commit, fix tests first

**Details**: See `.claude/CHECKPOINTS.md` section 2

---

## CHECKPOINT 3: After Extraction

**TRIGGER**: After extracting service from MainWindow

**EXECUTION**:
```bash
# Verify LOC reduction
wc -l src/ui/MainWindow.cpp

# Verify tests exist
ls tests/test_<service_name>.cpp

# Run all tests
cd build && ctest --output-on-failure
```

**SUCCESS CRITERIA**:
- MainWindow LOC reduced by expected amount
- Extracted service has >80% test coverage
- All tests pass
- No regressions

**Details**: See `.claude/CHECKPOINTS.md` section 3

---

## Quick Reference: Forbidden Patterns

These patterns are BLOCKED by checkpoints:

| Pattern | Violation | Checkpoint |
|---------|-----------|------------|
| File > 1,500 lines | God class | CHECKPOINT 1 |
| SQL in `src/ui/` | SQL in UI | CHECKPOINT 2 |
| `= "#RRGGBB"` | Hardcoded color | CHECKPOINT 2 |
| `setParent(nullptr)` | Creates top-level window | CHECKPOINT 2 |
| New feature without test | No test coverage | Manual check |

---

## When Checkpoint Fails

**Claude's response template**:
```
❌ CHECKPOINT FAILED: [checkpoint_name]

[Copy exact text from .claude/REFUSAL_TEMPLATES.md]

Options:
A) Fix violation (recommended)
B) Type 'OVERRIDE: <reason>'
```

**User override format**: `OVERRIDE: <reason>`

**Logging**: All overrides logged to `.claude/VIOLATIONS.md`

---

## Project-Specific Information

(Rest of CLAUDE.md continues with project-specific details: version management, build process, testing, etc.)

---

## Always Bump Version Before Building

**CRITICAL**: Update version in 4 places before building:

1. `/src/core/Constants.h` - `APP_VERSION`
2. `/installer/tr4qt.nsi` - `APPVERSION`
3. `/src/CMakeLists.txt` - `MACOSX_BUNDLE_*_VERSION`
4. `/resources/tr4qt.rc` - `VER_FILEVERSION` (4 places)

---

## Adding New Qt Modules

When adding Qt modules to `CMakeLists.txt`:

1. Add to `/CMakeLists.txt`: `find_package(Qt6 ... NewModule)`
2. Add to `/src/CMakeLists.txt`: `Qt6::NewModule`
3. **CRITICAL**: Add to `.github/workflows/build.yml`: `modules: '... qtnewmodule'`

**Why**: Windows CI only installs explicitly listed modules. macOS has all modules.

---

## Check for Existing Data Before Loading

**Rule**: ALWAYS check if data already exists before creating new copy.

**Bad**:
```cpp
// Creates duplicate CountryFile
CountryFile countryFile;
countryFile.loadFromFile("cty.dat");
```

**Good**:
```cpp
// Receives pointer to existing data
ADIFImportDialog(CountryFile* countryFile, QWidget* parent);
```

**Why**: Single Source of Truth, no sync issues, memory efficient.

---

## ALWAYS Use DialogHelper for User Dialogs

**Rule**: ALL dialogs must go through DialogHelper (automatic logging).

**Bad**:
```cpp
QMessageBox::critical(this, "Error", "Failed");
```

**Good**:
```cpp
DialogHelper::critical(this, "Error", "Failed");
```

**Why**: All dialogs logged, text selectable, consistent UX.

---

## ALWAYS Avoid Magic Numbers

**Rule**: Use named constants, never literal numbers.

**Bad**:
```cpp
m_label->setMaximumHeight(45);
QString formatted = QString("%1").arg(callsign, -12);
```

**Good**:
```cpp
const int LABEL_HEIGHT = m_label->fontMetrics().height() + 10;
m_label->setMaximumHeight(LABEL_HEIGHT);

const int CALLSIGN_FIELD_WIDTH = 12;
QString formatted = QString("%1").arg(callsign, -CALLSIGN_FIELD_WIDTH);
```

---

## ALWAYS Use ThemeManager for Colors

**Rule**: No hardcoded hex colors. Use `ThemeManager::instance().color(ColorRole::...)`.

**Bad**:
```cpp
m_label->setStyleSheet("QLabel { color: #006600; }");
```

**Good**:
```cpp
QString color = ThemeManager::instance().colorName(ColorRole::LotwUserText);
m_label->setStyleSheet(QString("QLabel { color: %1; }").arg(color));
```

**Enforcement**: Pre-commit hook blocks hardcoded colors.

---

## NEVER Use setParent(nullptr)

**Rule**: `setParent(nullptr)` creates unwanted top-level windows.

**Use instead**:
- `widget->deleteLater()` - Delete widget
- `widget->hide()` - Hide widget
- Remove from layout - Layout handles parent

**Enforcement**: Pre-commit hook blocks `setParent(nullptr)`.

---

## Version Management

**Update 4 files** when releasing:
1. Constants.h
2. tr4qt.nsi
3. CMakeLists.txt
4. tr4qt.rc

---

## Dependency Version Requirements

**See**: `VERSION_REQUIREMENTS.md`

**Critical**: CI and local dev MUST use same Qt/Hamlib versions.

---

## Build Process

```bash
# Standard build
cmake --build build

# Clean rebuild
rm -rf build && cmake -B build && cmake --build build

# Run (kill existing first)
pkill -9 tr4qt
./build/src/tr4qt.app/Contents/MacOS/tr4qt
```

---

## Testing

```bash
cd build && ctest --output-on-failure
```

---

## Git Workflow

**Commit format**:
```
Brief description - vX.Y.Z

Detailed explanation.

Changes:
- Bullet list

🤖 Generated with [Claude Code](https://claude.com/claude-code)

Co-Authored-By: Claude Sonnet 4.5 <noreply@anthropic.com>
```

---

## Pre-Release Checklist

**Before creating release tag**:

1. [ ] Verify Qt modules match CI (check CMakeLists.txt vs .github/workflows/build.yml)
2. [ ] Verify last CI build passed (`gh run list --limit 3`)
3. [ ] Version bumped in all 4 files
4. [ ] Build and test locally (`cmake --build build && ctest`)
5. [ ] No uncommitted changes (`git status`)
6. [ ] Create tag (`git tag vX.Y.Z`)
7. [ ] Monitor CI (`gh run watch`)

---

## Common Patterns

### Radio State
`m_currentState` (RadioState): frequencyA, bandA, modeA

### Country Lookup
```cpp
CountryData countryData = m_countryFile.lookup(callsign);
```

### Exchange Prediction
```cpp
QString prediction = InitialExchangeManager::instance().predictExchange(...);
```

### Database Operations
```cpp
if (m_hasActiveContest) {
    QSORepository repo;
    if (!repo.saveQSO(qso, m_currentContestDbId)) {
        LOG_WARN("Tag", QString("Failed: %1").arg(repo.lastError()));
    }
}
```

---

## Architecture Notes

### Contest System
- Base: `ContestBase` (abstract)
- Implementations: `CQWWContest`, `CQWPXContest`, `WinterFieldDayContest`
- Registry: `REGISTER_CONTEST(ClassName, "ID")`
- Factory: `ContestRegistry::instance().createContest("ID")`

### Exchange Fields
```cpp
QList<ExchangeField> getReceivedExchangeFields() const override;
QList<ExchangeField> getSentExchangeFields() const override;
```

### Duplicate Checking
```cpp
DuplicateCheckingRule getDuplicateCheckingRule() const override {
    return DuplicateCheckingRule::PerBandMode;
}
```

Options: PerBandMode, AllBandMode, PerBand, AllBand

---

## Key Files

### Core
- `/src/core/Constants.h` - VERSION
- `/src/core/Types.h` - Enums, types
- `/src/models/QSO.h` - QSO structure

### UI
- `/src/ui/MainWindow.cpp` - Main logic (⚠️ 5,564 lines - extraction needed)
- `/src/ui/models/QSOTableModel.cpp` - Log display

### Contests
- `/src/contests/ContestBase.h` - Interface
- `/src/contests/CQWWContest.cpp` - CQ WW
- `/src/contests/CQWPXContest.cpp` - CQ WPX

### Exchange System
- `/src/exchanges/InitialExchangeManager.cpp` - Prediction
- `/src/data/ExchangeMemoryRepository.cpp` - Memory

### Country Data
- `/src/utils/CountryFile.cpp` - CTY.DAT parser
- `~/.tr4qt/cty.dat` - Country data

---

## Recent Changes

See git log for recent changes.

---

## Known Issues / TODOs

**Platform-Specific**:
- [ ] Linux: Implement "Email Logs to Support" feature

**UI Enhancements**:
- [ ] DialogHelper: Add Cmd-C keyboard copy support (deferred - right-click works)

**Architecture** (tracked in .claude/VIOLATIONS.md):
- [ ] MainWindow: Extract services (5,564 lines → target 2,500 lines)
- [ ] See: docs/EXTRACTION_ARCHITECTURE.md

---

## Deployment Guides

**Detailed docs**:
- macOS: `docs/macos-deployment.md`
- Windows: `docs/windows-deployment.md`

**Key principle**: Never trust automatic tools (windeployqt, macdeployqt). Use explicit file copying.

---

## Platform-Specific Notes

### macOS
- Qt::ALT = Option, Qt::CTRL = Command
- Paths: `~/Library/Application Support/TR4QT/`

### Windows
- Paths: `%LOCALAPPDATA%\TR4QT\`
- Avoid "interface" as parameter name (COM conflict)
