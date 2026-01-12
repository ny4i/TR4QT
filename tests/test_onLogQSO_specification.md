# onLogQSO() Integration Test Specification

## Purpose
Document the CURRENT behavior of MainWindow::onLogQSO() before extraction.
These tests prove behavioral equivalence after refactoring.

## Method Location
`src/ui/MainWindow.cpp:1992-2298` (306 lines)

## Responsibilities Analysis

### 1. Command Handling (lines 1996-2031)
**OPON Command:**
- Input: Callsign field = "OPON"
- Behavior:
  - Opens OperatorDialog
  - Pre-populates with current operator from AppSettings
  - If accepted: Updates AppSettings, updates m_operatorLabel, logs change
  - If cancelled: Shows cancelled message
  - Clears entry fields, focuses callsign
  - Returns early (no QSO logged)

**UDP Command:**
- Input: Callsign field = "UDP"
- Behavior:
  - Calls onRebroadcastLog()
  - Clears entry fields, focuses callsign
  - Returns early (no QSO logged)

### 2. Validation (lines 2033-2073)
**Prerequisites:**
- m_qsoLogger must exist (checks for active contest)
- If null: Shows error "No active contest - open a contest first", beeps, returns

**QSOLogger Validation:**
- Builds QSOLogger::Input with:
  - callsign, exchange, radioState, operatorCallsign, serialNumber, operatingMode
- Collects existingQSOs from m_qsoTableModel
- Calls m_qsoLogger->logQSO()
- If validation fails:
  - Shows error in red/bold status label
  - Beeps
  - Sets focus to appropriate field (callsign/exchange/none)
  - Returns early

### 3. Success Path - UI Updates (lines 2075-2115)
- Updates m_nextSerialNumber from result
- Extracts QSO, isDuplicate, dupeInfo, multiplierValues from result
- Logs duplicate info if present
- Adds QSO to m_qsoTableModel
- Calls updateScoreDisplay()
- Updates multiplier window (if exists):
  - Gets primary multiplier type from contest
  - Gets multiplier value for QSO
  - Calls setMultiplierWorked()
- Scrolls table view to bottom

### 4. Success Path - Database Persistence (lines 2118-2258)
**If m_hasActiveContest:**

**Save Loop (retry up to 3 times):**
- Creates QSORepository
- Calls repo.saveQSO(qso, m_currentContestDbId)
- If success:
  - Logs with database ID
  - Updates table model with ID (for Edit QSO support)
  - Saves to exchange memory (if exchange not empty)
  - Checks auto-backup via BackupManager
  - Breaks loop

**If save fails:**
- Shows modal dialog with 3 options:
  1. **Retry Database**: Increments retry counter, loops
  2. **Save to Emergency File**:
     - Writes ADIF to emergency_log.adi
     - Creates file with header if new
     - Appends QSO record
     - Shows success dialog with file path
     - Breaks loop (considered saved)
  3. **Stop Contesting**:
     - Shows critical warning
     - Breaks loop
     - QSO only in memory

**Exchange Memory Save:**
- If QSO saved to database and exchangeReceived not empty:
  - Creates ExchangeMemoryEntry with callsign, exchange, contest, mode, timestamp, source
  - Saves via ExchangeMemoryRepository
  - Logs warning if save fails

**Auto-Backup Check:**
- Gets current QSO count from repository
- Calls BackupManager::instance().autoBackupIfNeeded()

### 5. Success Path - Post-Save Actions (lines 2261-2297)
- **UDP Broadcast**: Calls m_udpBroadcastManager->onQSOLogged()
- **Last QSO Time**: Updates m_lastQSOTime
- **Status Label**: Shows "Logged: CALL on BAND MODE"
- **Integrity Check**: Every 50 QSOs, calls quickIntegrityCheck()
- **Auto-CW Send**: If CW mode, radio connected, auto-send enabled, sends QSL message
- **Clear Entry**: Calls onClearEntry()
- **Update Displays**: Calls updateScoreDisplay() and updateTimeDisplay()

## Test Coverage Plan

### Test 1: OPON Command
- Setup: Active contest, callsign field = "OPON"
- Verify: OperatorDialog shown, entry cleared, no QSO logged

### Test 2: UDP Command
- Setup: Active contest, callsign field = "UDP"
- Verify: onRebroadcastLog() called, entry cleared, no QSO logged

### Test 3: No Active Contest
- Setup: No contest, callsign = "K1ABC", exchange = "599"
- Verify: Error shown, beep, no QSO logged

### Test 4: Validation Failure - Bad Callsign
- Setup: Active contest, callsign = "", exchange = "599"
- Verify: QSOLogger returns error, status shows error in red, focus on callsign

### Test 5: Validation Failure - Bad Exchange
- Setup: Active contest, callsign = "K1ABC", exchange = ""
- Verify: QSOLogger returns error, status shows error in red, focus on exchange

### Test 6: Successful QSO Logging
- Setup: Active contest, valid callsign/exchange, radio state
- Verify:
  - QSO added to table model
  - Serial number incremented
  - Score display updated
  - Multiplier window updated
  - Table scrolled to bottom
  - QSO saved to database
  - Exchange memory saved
  - UDP broadcast sent
  - Status shows success
  - Entry fields cleared
  - Last QSO time updated

### Test 7: Duplicate QSO Logging
- Setup: Same callsign/band/mode already in log
- Verify:
  - QSO still logged (duplicate allowed)
  - isDuplicate flag set
  - Duplicate info logged
  - All normal success behaviors occur

### Test 8: Database Save Failure
- Setup: Repository.saveQSO() returns false
- Verify: Modal dialog shown with retry options
- **Note**: Cannot fully test modal dialog interactions in automated test
- **Manual Test Required**: Retry, Emergency File, Stop Contesting buttons

### Test 9: Exchange Memory Save
- Setup: Successful QSO with exchange "599 CA"
- Verify:
  - ExchangeMemoryEntry saved with correct data
  - Source = "auto" if m_initialExchangePopulated, else "manual"

### Test 10: Auto-Backup Trigger
- Setup: 50th QSO logged (or backup interval reached)
- Verify: BackupManager::autoBackupIfNeeded() called with correct params

### Test 11: Integrity Check Trigger
- Setup: 50th QSO logged
- Verify: quickIntegrityCheck() called, counter reset

### Test 12: Auto-CW Send
- Setup: CW mode, radio connected, auto-send enabled
- Verify: sendCWMessage() called with QSL message after logging

## Test Limitations

**Cannot be fully tested without GUI:**
1. Modal dialogs (OPON, database retry)
2. Focus changes (callsign/exchange fields)
3. Table view scrolling
4. Status label styling (red/bold)

**Workarounds:**
- Use QSignalSpy for dialog signals
- Mock QWidget methods where possible
- Document manual test cases for GUI interactions

## Architecture Violations Found

This method violates EVERY rule in ARCHITECTURE_RULES.md:

1. ❌ **SQL in UI**: Line 2254 `repo.getQSOCount(m_currentContestDbId)`
2. ❌ **Business Logic Loop in UI**: Lines 2124-2221 (retry loop)
3. ❌ **File I/O in Event Handler**: Lines 2164-2195 (emergency ADIF writing)
4. ❌ **Event Handler >20 Lines**: 306 lines (15.3X over limit)
5. ❌ **Modal Dialogs in Business Logic**: Lines 2138-2220
6. ❌ **Multiple Responsibilities**: Command parsing, validation, UI updates, persistence, broadcasting, integrity checks, CW sending

## Extraction Targets

After tests pass, extract:
1. **Command handling** → CommandDispatcher service
2. **QSO validation** → QSOLogger (already exists, keep using it)
3. **QSO persistence** → QSOPersistenceService (database save, retry, emergency file)
4. **Exchange memory** → ExchangeMemoryService (already has repository)
5. **Post-logging actions** → QSOLoggingCoordinator (UDP, integrity, auto-CW, updates)

MainWindow should orchestrate, not implement.
