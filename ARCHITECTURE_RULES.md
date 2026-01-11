# TR4QT Architecture Rules

**CRITICAL: These rules MUST be followed when adding new features to TR4QT.**

## 🚨 MainWindow Forbidden Patterns

MainWindow is at **5,564 lines** (3.7X over the 1,500 line STOP limit).

**NEVER add these to MainWindow:**

### ❌ FORBIDDEN in MainWindow

1. **SQL Queries**
   ```cpp
   // ❌ NEVER DO THIS
   Database& db = Database::instance();
   db.execute("UPDATE contests SET ...", ...);
   ```
   **Why:** Database access belongs in Repository layer
   **Where:** Create/use Repository or Service class

2. **Business Logic**
   ```cpp
   // ❌ NEVER DO THIS
   for (int row = 0; row < count; ++row) {
       QSO qso = model->getQSO(row);
       qso.exchangeSent = formatExchange(...);
       db.execute("UPDATE qsos SET ...", ...);
   }
   ```
   **Why:** Data iteration and updates are business logic
   **Where:** Create Service class (e.g., ContestService, QSOService)

3. **Complex Event Handlers (>20 lines)**
   ```cpp
   // ❌ NEVER DO THIS
   void MainWindow::onSomeAction() {
       // 70+ lines of logic
   }
   ```
   **Why:** Event handlers should delegate, not implement
   **Where:** Extract to Service method, call from handler

4. **Direct Model Updates**
   ```cpp
   // ❌ NEVER DO THIS
   for (int i = 0; i < model->count(); ++i) {
       model->updateQSO(i, modifiedQSO);
   }
   ```
   **Why:** Bulk updates belong in Service layer
   **Where:** Service method that updates DB + refreshes model

5. **Validation Rules**
   ```cpp
   // ❌ NEVER DO THIS
   if (exchange.isEmpty() || !isValidFormat(exchange)) {
       showError(...);
       return;
   }
   ```
   **Why:** Validation is business logic
   **Where:** Service or Domain class validates, UI displays result

### ✅ ALLOWED in MainWindow

1. **UI Construction**
   ```cpp
   m_callsignEntry = new QLineEdit(this);
   layout->addWidget(m_callsignEntry);
   ```

2. **Simple Delegation**
   ```cpp
   void MainWindow::onEditContestSettings() {
       m_contestService->showEditDialog(m_currentContestDbId);
   }
   ```

3. **Signal/Slot Connections**
   ```cpp
   connect(button, &QPushButton::clicked, this, &MainWindow::onButtonClicked);
   ```

4. **UI State Updates**
   ```cpp
   m_statusLabel->setText("Processing...");
   m_progressBar->setValue(50);
   ```

5. **Dialog Display**
   ```cpp
   auto dialog = new SomeDialog(this);
   if (dialog->exec() == QDialog::Accepted) {
       m_service->handleResult(dialog->getData());
   }
   ```

## 📐 TR4QT Architecture Layers

```
┌─────────────────────────────────────────┐
│  UI Layer (MainWindow, Dialogs)        │  ← ONLY user interaction
│  - Display data                         │
│  - Capture input                        │
│  - Delegate to services                 │
└─────────────────────────────────────────┘
              ↓ delegates to
┌─────────────────────────────────────────┐
│  Service Layer (NEW - mostly missing)   │  ← Business workflows
│  - ContestService                       │
│  - QSOService                           │
│  - RadioService                         │
│  - IntegrityService                     │
└─────────────────────────────────────────┘
              ↓ uses
┌─────────────────────────────────────────┐
│  Manager Layer (exists)                 │  ← Coordination
│  - ContestManager                       │
│  - DataIntegrityManager                 │
│  - QSOLogger                            │
└─────────────────────────────────────────┘
              ↓ uses
┌─────────────────────────────────────────┐
│  Domain Layer (Contest classes)         │  ← Business rules
│  - ContestBase                          │
│  - CQWWContest                          │
│  - WinterFieldDayContest                │
└─────────────────────────────────────────┘
              ↓ uses
┌─────────────────────────────────────────┐
│  Repository Layer (needs extraction)    │  ← Data access
│  - QSORepository                        │
│  - ContestRepository (missing)          │
│  - Database                             │
└─────────────────────────────────────────┘
```

## 🎯 Where New Features Go

**Decision tree for adding new features:**

### Example: "Edit Contest Exchange"

**Question 1: Does it involve database updates?**
- **YES** → Need Repository method

**Question 2: Does it involve business logic?**
- **YES** → Need Service method

**Question 3: Does it need UI?**
- **YES** → Create Dialog, call from MainWindow

**Result:**
```cpp
// 1. Repository (data access)
class ContestRepository {
    bool updateExchange(int contestId, const QString& exchange);
};

// 2. Service (business logic)
class ContestService {
    Result<void> updateContestExchange(int contestId, const QString& newExchange) {
        // Validate exchange
        // Update database via repository
        // Update active contest instance
        // Update affected QSOs via QSOService
        // Refresh UI models
    }
};

// 3. MainWindow (UI orchestration ONLY)
void MainWindow::onEditContestSettings() {
    auto dialog = new EditContestDialog(m_activeContest, this);
    if (dialog->exec() == QDialog::Accepted) {
        auto result = m_contestService->updateContestExchange(
            m_currentContestDbId,
            dialog->getExchange()
        );
        if (!result.success) {
            showError(result.error);
        }
    }
}
```

## 🔍 Pre-Feature Checklist

**BEFORE implementing ANY new feature, answer these questions:**

1. **Does MainWindow exceed 1,500 lines?**
   - If YES: **STOP**. Extract services first.

2. **Will this add SQL to MainWindow?**
   - If YES: Create/use Repository class instead

3. **Will this add a loop over QSOs/data in MainWindow?**
   - If YES: Create/use Service class instead

4. **Will the event handler exceed 20 lines?**
   - If YES: Extract to Service method

5. **Does this duplicate logic from another feature?**
   - If YES: Extract common logic to Service

6. **Can this be tested without a UI?**
   - If NO: You mixed business logic with UI (bad)
   - If YES: Good - logic is in Service

## 🏗️ Current Extraction Priorities

**MainWindow is 5,564 lines. Target: 2,000 lines.**

**Services that MUST be created:**

1. **ContestService** (~800 lines to extract)
   - `activateContest()`
   - `updateExchange()`
   - `reopenLastContest()`
   - `updateExchangeFields()`

2. **QSOService** (~600 lines to extract)
   - `logQSO()`
   - `rescoreAllQSOs()`
   - `updateQSOExchanges()`
   - `checkDuplicate()`

3. **RadioService** (~400 lines to extract)
   - `connect()/disconnect()`
   - `handleStateUpdate()`
   - `autoReconnect()`

4. **WindowService** (~300 lines to extract)
   - `showWindow(WindowType)`
   - `raiseAllWindows()`
   - `updateWindowMenu()`

5. **IntegrityService** (~200 lines to extract)
   - `quickCheck()`
   - `fullCheck()`
   - `handleMismatch()`

## 🚫 Enforcement

**Pre-commit hook should check:**
- [ ] No `db.execute()` in MainWindow.cpp
- [ ] No `for (int i = 0; i < model->count()` in MainWindow.cpp
- [ ] MainWindow event handlers <30 lines each
- [ ] No new business logic methods in MainWindow

**Code review should verify:**
- [ ] New features use Service layer
- [ ] MainWindow only orchestrates
- [ ] Business logic is testable without UI
- [ ] No SQL outside Repository classes

## 📚 References

**Bad Example (what NOT to do):**
- `MainWindow::onEditContestSettings()` - 73 lines, SQL, loops, business logic
- **Violation:** Should be in ContestService

**Good Example (what TO do):**
- `MainWindow::onShowDXCluster()` - 3 lines, pure delegation
- **Correct:** UI orchestration only
