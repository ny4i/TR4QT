# Refusal Templates for TR4QT

**PURPOSE**: Exact text Claude should output when blocking tasks due to architecture violations.

**USAGE**: Copy template, replace {variables}, output to user.

---

## Template 1: God Class Limit Exceeded

**TRIGGER**: File >= 1,500 lines

**TEMPLATE**:
```
❌ TASK BLOCKED: Architecture Violation

**File:** {file_path}
**Current size:** {current_lines} lines
**Limit:** 1,500 lines
**Overage:** {current_lines - 1500} lines ({percentage}% over limit)

**Requested task:** {user_task_description}
**Estimated addition:** +{estimated_lines} lines
**Result if added:** {current_lines + estimated_lines} lines

**Why this is blocked:**
{file_name} is a god class ({multiplier}X over limit). Adding more code:
- Increases technical debt exponentially
- Makes the codebase harder to maintain and test
- Violates Single Responsibility Principle
- Blocks other developers from working on this file

**Required action:**
Extract services to bring {file_name} under 1,500 lines BEFORE adding new features.

**Extraction plan:**
See: docs/EXTRACTION_ARCHITECTURE.md

Recommended extractions:
1. {service_1_name}: {description} (-{lines_1} lines)
2. {service_2_name}: {description} (-{lines_2} lines)
3. {service_3_name}: {description} (-{lines_3} lines)

**Estimated effort:** {time_estimate}

**Your options:**
A) Proceed with extraction (recommended) - Type 'yes' to start
B) Override and add technical debt - Type 'OVERRIDE: {reason}'

**Note:** Overrides are logged to .claude/VIOLATIONS.md and create technical debt
that becomes more expensive to fix over time.

What would you like to do?
```

**EXAMPLE**:
```
❌ TASK BLOCKED: Architecture Violation

**File:** src/ui/MainWindow.cpp
**Current size:** 5,564 lines
**Limit:** 1,500 lines
**Overage:** 4,064 lines (271% over limit)

**Requested task:** Add dark mode toggle to Settings menu
**Estimated addition:** +75 lines
**Result if added:** 5,639 lines

**Why this is blocked:**
MainWindow.cpp is a god class (3.7X over limit). Adding more code:
- Increases technical debt exponentially
- Makes the codebase harder to maintain and test
- Violates Single Responsibility Principle
- Blocks other developers from working on this file

**Required action:**
Extract services to bring MainWindow under 1,500 lines BEFORE adding new features.

**Extraction plan:**
See: docs/EXTRACTION_ARCHITECTURE.md

Recommended extractions:
1. QSOLoggingService: Extract onLogQSO() workflow (-306 lines)
2. QSOPersistenceService: Extract database save/retry logic (-140 lines)
3. ExchangeMemoryService: Extract exchange prediction (-50 lines)

**Estimated effort:** 2-3 days

**Your options:**
A) Proceed with extraction (recommended) - Type 'yes' to start
B) Override and add technical debt - Type 'OVERRIDE: I understand the risk'

What would you like to do?
```

---

## Template 2: File in Yellow Zone (Warning)

**TRIGGER**: File >= 1,000 lines and < 1,500 lines

**TEMPLATE**:
```
⚠️  ARCHITECTURE WARNING: Approaching Limit

**File:** {file_path}
**Current size:** {current_lines} lines
**Yellow threshold:** 1,000 lines
**Red threshold:** 1,500 lines
**Headroom:** {1500 - current_lines} lines remaining

**Requested task:** {user_task_description}
**Estimated addition:** +{estimated_lines} lines

**Recommendation:**
Instead of adding to {file_name}, implement this feature in a NEW service class.

**Benefits:**
✅ Keeps {file_name} under 1,500 line limit
✅ Better separation of concerns
✅ Easier to test (can inject mocks)
✅ Prevents future god class refactoring

**Proposed approach:**
1. Create {service_name} class
2. Implement feature in service (-0 lines to {file_name})
3. {file_name} delegates to service (+~10 lines for delegation)

**Your options:**
A) Implement in new service (recommended) - Type 'yes'
B) Add to existing file (uses {headroom} lines of headroom) - Type 'add to existing'

What would you like to do?
```

---

## Template 3: No Tests for New Feature

**TRIGGER**: User requests feature without tests

**TEMPLATE**:
```
🧪 TEST REQUIREMENT: Feature Blocked Without Tests

**Requested feature:** {feature_description}
**Target file:** {file_path}
**Test file:** {test_file_path} (DOES NOT EXIST)

**Why this is blocked:**
CLAUDE.md requires all new features have accompanying tests (TDD approach).

**Required:** Write tests BEFORE implementing feature.

**Test approach:**
1. Create {test_file_path}
2. Write test for expected behavior:
   - {test_case_1_description}
   - {test_case_2_description}
   - {test_case_3_description}
3. Tests should FAIL initially (feature not implemented)
4. Implement feature to make tests pass
5. Refactor while keeping tests green

**Benefits:**
✅ Tests prove the feature works correctly
✅ Tests prevent regressions
✅ Tests document expected behavior
✅ Enables safe refactoring later

**Your options:**
A) Write tests first (TDD approach) - Type 'yes'
B) Override and skip tests - Type 'OVERRIDE_TEST: {reason}'

**Note:** Features without tests = technical debt = harder to refactor later.

What would you like to do?
```

---

## Template 4: SQL in UI Class

**TRIGGER**: User requests adding database code to UI class

**TEMPLATE**:
```
🚨 ARCHITECTURE VIOLATION: SQL in UI Class

**Requested:** Add database code to {ui_class_name}
**Pattern:** {code_pattern}

**Why this is FORBIDDEN:**
UI classes MUST NOT contain SQL queries. This violates separation of concerns.

**Correct architecture:**
1. **Repository class:** Handles database access (SQL queries)
2. **Service class:** Handles business logic (calls repository)
3. **UI class:** Handles display/input (calls service)

**Example:**
```cpp
// ❌ WRONG: SQL in UI
void MainWindow::onSave() {
    db.exec("UPDATE qsos SET ...");
}

// ✅ RIGHT: Repository + Service + UI
void MainWindow::onSave() {
    auto result = m_service->saveData(getData());
    if (!result.success) showError(result.error);
}

class DataService {
    Result saveData(const Data& data) {
        return m_repository->save(data);
    }
private:
    DataRepository* m_repository;
};

class DataRepository {
    bool save(const Data& data) {
        QSqlQuery query;
        query.prepare("UPDATE qsos SET ...");
        return query.exec();
    }
};
```

**Required action:**
1. Create (or use existing) Repository class for database access
2. Create (or use existing) Service class for business logic
3. {ui_class_name} calls service method (delegation only)

**Your options:**
A) Create proper architecture (recommended) - Type 'yes'
B) Override (NOT recommended) - Type 'OVERRIDE_SQL: {reason}'

**Warning:** SQL in UI classes makes code untestable and unmaintainable.

What would you like to do?
```

---

## Template 5: Business Logic Loop in Event Handler

**TRIGGER**: Event handler will contain loop with business logic

**TEMPLATE**:
```
🚨 ARCHITECTURE VIOLATION: Business Logic in Event Handler

**Event handler:** {handler_name}
**Problem:** Contains business logic loop (iterating data, applying rules)

**Why this is FORBIDDEN:**
Event handlers should DELEGATE to services, not implement logic.

**Rule:** Event handlers limited to ~20 lines:
- Capture input
- Call service method
- Handle result (update UI)

**Example:**
```cpp
// ❌ WRONG: Business logic in event handler
void MainWindow::onUpdateAll() {
    for (int i = 0; i < data.count(); ++i) {
        // 50 lines of business logic
        applyBusinessRules(data[i]);
        validateData(data[i]);
        updateDatabase(data[i]);
    }
}

// ✅ RIGHT: Delegate to service
void MainWindow::onUpdateAll() {
    auto result = m_service->updateAllData();
    if (!result.success) {
        showError(result.error);
    } else {
        showSuccess(result.message);
        refreshDisplay();
    }
}

// Service implements business logic
class DataService {
    Result updateAllData() {
        for (const Data& item : m_repository->getAll()) {
            if (!validate(item)) return Error("Invalid");
            if (!process(item)) return Error("Process failed");
        }
        return Success("All data updated");
    }
};
```

**Required action:**
1. Create {service_name} service class
2. Move loop + business logic to service method
3. {handler_name} calls service (delegation only, ~10 lines)

**Your options:**
A) Extract to service (recommended) - Type 'yes'
B) Override (NOT recommended) - Type 'OVERRIDE: {reason}'

What would you like to do?
```

---

## Template 6: Refactoring Without Tests

**TRIGGER**: User requests refactoring code that has no tests

**TEMPLATE**:
```
🧪 REFACTORING BLOCKED: No Tests to Prove Equivalence

**Requested refactoring:** {refactoring_description}
**Target:** {file_path}::{method_name}
**Test coverage:** 0% (NO TESTS EXIST)

**Why this is blocked:**
Refactoring without tests is "extract and hope" - no proof of behavioral equivalence.

**CLAUDE.md requirement:**
1. Write tests against CURRENT behavior
2. Verify tests pass with existing code
3. Refactor while keeping tests green
4. Tests prove no regressions introduced

**Example:**
```cpp
// Step 1: Write test for CURRENT behavior
TEST(QSOLogger, ValidatesCallsign) {
    // Test what the code does NOW (before refactoring)
    QSOLogger logger;
    auto result = logger.log("", "599");
    EXPECT_FALSE(result.success);
    EXPECT_EQ(result.error, "Callsign required");
}

// Step 2: Run test - should PASS with existing code

// Step 3: Refactor (extract method, rename, etc.)

// Step 4: Run test again - should STILL PASS
//         If it passes, refactoring preserved behavior ✅
//         If it fails, refactoring changed behavior ❌
```

**Required action:**
1. Write tests for {method_name} BEFORE refactoring
2. Verify tests pass with current code
3. Then refactor while keeping tests green

**Your options:**
A) Write tests first (recommended) - Type 'yes'
B) Override (risky) - Type 'OVERRIDE_TEST: {reason}'

**Warning:** Refactoring without tests = high risk of breaking existing functionality.

What would you like to do?
```

---

## Usage Instructions for Claude

**When checkpoint fails:**

1. **Identify violation type** (god class, SQL in UI, no tests, etc.)
2. **Select appropriate template** from this file
3. **Fill in {variables}** with actual values
4. **Output complete template** to user
5. **Wait for user response** (proceed with fix, or override)
6. **Log to violations.md** if user overrides

**Do NOT:**
- ❌ Proceed without user confirmation
- ❌ Silently ignore violations
- ❌ Skip templates (output exact text)
- ❌ Suggest workarounds that bypass checks

**Example workflow:**
```
1. User: "Add feature X to MainWindow"
2. Claude: Runs CHECKPOINT 1 (task start)
3. Check: wc -l src/ui/MainWindow.cpp → 5,564 lines
4. Status: RED (> 1,500 limit)
5. Action: Output Template 1 (God Class Limit Exceeded)
6. Wait: User types "yes" (start extraction) or "OVERRIDE: reason"
7. If yes: Start extraction workflow
8. If override: Log to .claude/VIOLATIONS.md, proceed with warning
```

---

## Adding New Templates

When you discover a new blockable pattern:

1. Add checkpoint to `.claude/CHECKPOINTS.md`
2. Create refusal template in this file
3. Update `scripts/check_architecture.sh` to detect pattern
4. Test by trying to violate rule (should be blocked)

**Template format:**
```markdown
## Template N: {Violation Name}

**TRIGGER**: {When this template is used}

**TEMPLATE**:
```
{Exact text with {variables}}
```

**EXAMPLE**:
```
{Filled-in example}
```
```
