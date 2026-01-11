# CLAUDE.md Gaps Analysis - onEditContestSettings() Post-Mortem

## What Went Wrong

**Feature Added:** Edit Contest Settings (v3.37.0-3.37.1)

**Implementation Location:** `MainWindow::onEditContestSettings()` - 73 lines

**Violations:**
1. ✗ SQL queries in MainWindow (`db.execute("UPDATE contests SET ..."`)
2. ✗ Business logic loop in event handler (iterating over QSOs)
3. ✗ Direct database updates in UI layer
4. ✗ Added code to MainWindow (already 5,564 lines, 3.7X over limit)
5. ✗ No service layer extraction
6. ✗ Mixed UI, business logic, and data access

## What CLAUDE.md Was Missing

### Global CLAUDE.md (~/.claude/CLAUDE.md)

**Before (Gaps):**
- ✓ Abstract principle: "Business logic separated from UI"
- ✗ **NO concrete forbidden patterns**
- ✗ **NO decision tree for where code belongs**
- ✗ **NO enforcement rules for UI classes**
- ✗ **NO size limit enforcement (only mentioned in push back)**
- ✗ **NO examples of what NOT to do**

**After (Added):**

1. **Section: "UI Class Forbidden Patterns"** (lines 159-203)
   - ❌ SQL Queries in UI
   - ❌ Business Logic Loops in UI
   - ❌ Validation Rules in UI
   - ❌ Complex Event Handlers (>20 lines)
   - ✅ What IS allowed in UI

2. **Section: "Where Code Belongs"** (lines 205-232)
   - Decision tree for every new method
   - Example of correct delegation pattern
   - Clear Repository vs Service vs UI distinction

3. **Section: "IMMEDIATELY REJECT these patterns"** (lines 277-311)
   - Concrete dialogue examples for rejecting violations
   - Specific guidance for each forbidden pattern
   - **REFUSE to implement** directive for severe violations

### Project CLAUDE.md (TR4QT/CLAUDE.md)

**Before (Gaps):**
- ✓ Build process documentation
- ✓ Deployment guides
- ✓ Patterns to avoid (magic numbers, DialogHelper)
- ✗ **NO architecture guidance specific to TR4QT**
- ✗ **NO MainWindow size warnings**
- ✗ **NO service layer documentation**
- ✗ **NO "where to put new features" guidance**

**After (Added):**

1. **Section: "🚨 ARCHITECTURE: MainWindow is 5,564 lines"** (lines 73-122)
   - Current MainWindow size violation (3.7X over limit)
   - Pre-feature checklist (4 critical questions)
   - Reference to ARCHITECTURE_RULES.md
   - Example of WRONG vs CORRECT approach
   - Side-by-side code comparison

### New File: ARCHITECTURE_RULES.md

**Created to document TR4QT-specific architecture rules:**

1. **Section: "MainWindow Forbidden Patterns"**
   - 5 forbidden patterns with code examples
   - Why each is forbidden
   - Where each belongs instead

2. **Section: "ALLOWED in MainWindow"**
   - 5 allowed patterns
   - UI construction, delegation, signal/slot, UI state

3. **Section: "TR4QT Architecture Layers"**
   - ASCII diagram of 5 layers
   - Responsibilities of each layer
   - Current state (Service layer missing)

4. **Section: "Where New Features Go"**
   - Decision tree for "Edit Contest Exchange"
   - Repository → Service → MainWindow pattern
   - Example code for each layer

5. **Section: "Pre-Feature Checklist"**
   - 6 questions to answer before implementing
   - Each violation points to correct approach

6. **Section: "Current Extraction Priorities"**
   - 5 services that must be created
   - Line counts to extract for each
   - Target: MainWindow under 2,000 lines

7. **Section: "Enforcement"**
   - Pre-commit hook checks
   - Code review checklist
   - Specific patterns to block

## What Would Have Prevented the Mistake

**If CLAUDE.md had these rules BEFORE v3.37.0:**

### Step 1: User Request
```
User: "that UX is confusing. I can only edit the exchange when i select create new."
```

### Step 2: Claude's Analysis (WITH new rules)

**Check CLAUDE.md checklist:**
1. ❌ Will this add SQL to MainWindow? → **YES** (db.execute)
2. ❌ Will this add business logic to MainWindow? → **YES** (loop over QSOs)
3. ❌ Will handler exceed 20 lines? → **YES** (73 lines)
4. ❌ Does MainWindow exceed 1,500 lines? → **YES** (5,564 lines)

**Result:** All 4 checks FAIL → Trigger STOP

### Step 3: Claude's Response (WITH new rules)

```
Claude: "STOP. I cannot implement this in MainWindow because:

1. MainWindow is 5,564 lines (3.7X over the 1,500 line STOP limit)
2. This feature requires:
   - SQL queries (db.execute)
   - Business logic (looping over QSOs)
   - Data updates (exchange_sent field)

Per ARCHITECTURE_RULES.md, this belongs in:
- ContestService (business logic)
- ContestRepository (database access)
- MainWindow (UI delegation only)

We need to create ContestService first. Would you like me to:
1. Create ContestService with updateExchange() method
2. Extract existing contest logic from MainWindow
3. Then implement the Edit Exchange feature properly

This will prevent MainWindow from growing further."
```

### Step 4: Correct Implementation (hypothetical)

**What SHOULD have been created:**

```cpp
// 1. ContestRepository.h (new file)
class ContestRepository {
public:
    bool updateExchange(int contestId, const QString& exchange);
    QString getExchange(int contestId);
};

// 2. ContestService.h (new file)
class ContestService {
public:
    Result<void> updateContestExchange(
        int contestId,
        const QString& newExchange,
        QSOTableModel* model,
        ContestBase* activeContest
    );
private:
    ContestRepository* m_repository;
    QSORepository* m_qsoRepository;
};

// 3. ContestService.cpp
Result<void> ContestService::updateContestExchange(...) {
    // Validation
    if (newExchange.isEmpty()) {
        return Result<void>::error("Exchange cannot be empty");
    }

    // Update contest in database
    if (!m_repository->updateExchange(contestId, newExchange)) {
        return Result<void>::error("Database update failed");
    }

    // Update active contest instance
    activeContest->setExchangeSent(newExchange);

    // Update all QSOs
    for (int row = 0; row < model->count(); ++row) {
        QSO qso = model->getQSO(row);
        QString newExchangeSent = activeContest->formatSentExchange(
            qso.serialNumber, qso.rstSent
        );
        qso.exchangeSent = newExchangeSent;

        if (!m_qsoRepository->update(qso)) {
            return Result<void>::error("Failed to update QSO");
        }

        model->updateQSO(row, qso);
    }

    return Result<void>::success();
}

// 4. MainWindow.cpp (minimal delegation)
void MainWindow::onEditContestSettings() {
    auto dialog = new EditContestDialog(m_activeContest, this);
    if (dialog->exec() == QDialog::Accepted) {
        auto result = m_contestService->updateContestExchange(
            m_currentContestDbId,
            dialog->getExchange(),
            m_qsoTableModel,
            m_activeContest
        );

        if (!result.success) {
            DialogHelper::critical(this, "Error", result.error);
        } else {
            m_statusLabel->setText("Exchange updated successfully");
        }
    }
}
```

**Lines added to MainWindow:** ~15 (vs 73 actual)
**Services created:** ContestService, ContestRepository
**Testability:** Service logic testable without UI

## Summary: Gaps That Allowed the Mistake

### 1. No Concrete Enforcement Rules

**Gap:** CLAUDE.md said "separate business logic from UI" but didn't define what that means in practice

**Fix:** Added explicit forbidden patterns with code examples

### 2. No Pre-Feature Checklist

**Gap:** No structured questions to ask before implementing

**Fix:** Added 4-question checklist that triggers STOP

### 3. No Size Limit Enforcement

**Gap:** MainWindow size mentioned in "push back" but not enforced proactively

**Fix:** Added size check as first question in checklist

### 4. No TR4QT Architecture Documentation

**Gap:** No guidance on TR4QT's specific layers and where code belongs

**Fix:** Created ARCHITECTURE_RULES.md with layers, examples, and decision tree

### 5. No Rejection Examples

**Gap:** "Push back" was vague - no concrete dialogue showing how to refuse

**Fix:** Added specific rejection examples for each violation

### 6. No Project-Specific Warnings

**Gap:** CLAUDE.md didn't warn about MainWindow's current state (5,564 lines)

**Fix:** Added prominent warning at top of TR4QT/CLAUDE.md CRITICAL REMINDERS

## Effectiveness Test

**Question:** Would the updated CLAUDE.md prevent this mistake in the future?

**Answer:** YES, because:

1. ✓ MainWindow size violation is now FIRST item in CRITICAL REMINDERS
2. ✓ 4-question checklist catches ALL violations in onEditContestSettings()
3. ✓ Forbidden patterns section explicitly shows SQL + loops are not allowed
4. ✓ "IMMEDIATELY REJECT" section gives concrete refusal dialogue
5. ✓ ARCHITECTURE_RULES.md provides the correct alternative approach
6. ✓ Decision tree clearly maps "database update" → Repository → Service

**The mistake was NOT the idea (edit contest exchange is valid), but WHERE it was implemented (MainWindow instead of Service).**

The updated CLAUDE.md would catch this at analysis time and redirect to proper architecture.
