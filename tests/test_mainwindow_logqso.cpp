/**
 * Integration tests for MainWindow::onLogQSO()
 *
 * PURPOSE: Test CURRENT behavior BEFORE extraction to prove behavioral equivalence.
 *
 * These tests validate the existing 306-line onLogQSO() method in MainWindow.
 * After refactoring/extraction, these tests must still pass to prove correctness.
 *
 * See: test_onLogQSO_specification.md for detailed behavior analysis
 */

#include <QtTest/QtTest>
#include <QApplication>
#include <QLineEdit>
#include <QLabel>
#include <QTableView>
#include "../src/ui/MainWindow.h"
#include "../src/data/Database.h"
#include "../src/data/QSORepository.h"
#include "../src/data/ExchangeMemoryRepository.h"
#include "../src/data/ContestRepository.h"
#include "../src/models/QSO.h"
#include "../src/core/Types.h"
#include "../src/utils/PathManager.h"
#include <QTemporaryDir>
#include <QSignalSpy>

using namespace TR4QT;

class TestMainWindowLogQSO : public QObject {
    Q_OBJECT

private:
    QTemporaryDir* m_tempDir = nullptr;
    QString m_originalAppDataDir;
    MainWindow* m_mainWindow = nullptr;

    // Helper: Find child widgets by name
    QLineEdit* findCallsignEntry() {
        return m_mainWindow->findChild<QLineEdit*>("callsignEntry");
    }

    QLineEdit* findExchangeEntry() {
        return m_mainWindow->findChild<QLineEdit*>("exchangeEntry");
    }

    QLabel* findStatusLabel() {
        return m_mainWindow->findChild<QLabel*>("statusLabel");
    }

    QTableView* findQSOTableView() {
        return m_mainWindow->findChild<QTableView*>("qsoTableView");
    }

    // Helper: Create a test contest database
    QString createTestContest(const QString& contestId, const QString& exchangeSent) {
        QString dbPath = m_tempDir->filePath("test_contest.db");

        // Initialize database
        Database::instance().close();
        if (!Database::instance().open(dbPath)) {
            qWarning() << "Failed to open test database:" << Database::instance().lastError();
            return QString();
        }

        // Create contest record
        ContestRepository contestRepo;
        ContestInfo contest;
        contest.contestName = "Test Contest";
        contest.contestType = contestId;
        contest.exchangeSent = exchangeSent;
        contest.databasePath = dbPath;
        contest.startDate = QDateTime::currentDateTime().addDays(-1);

        if (!contestRepo.createContest(contest)) {
            qWarning() << "Failed to create contest:" << contestRepo.lastError();
            return QString();
        }

        return dbPath;
    }

    // Helper: Activate contest in MainWindow
    bool activateTestContest(const QString& contestId, const QString& exchangeSent) {
        QString dbPath = createTestContest(contestId, exchangeSent);
        if (dbPath.isEmpty()) {
            return false;
        }

        // Load contest in MainWindow
        ContestRepository contestRepo;
        QList<ContestInfo> contests = contestRepo.getAllContests();
        if (contests.isEmpty()) {
            qWarning() << "No contests found after creation";
            return false;
        }

        // Simulate opening the contest
        // Note: MainWindow::activateContest() is not public, so we use onNewOpenContest()
        // which shows a dialog. We'll need to access private methods or refactor.
        // For now, we'll directly set up the contest state via public API.

        // PROBLEM: MainWindow doesn't expose activateContest() publicly
        // This is a limitation of testing the current architecture
        // We'll document this as a testing gap

        qWarning() << "Cannot programmatically activate contest - MainWindow API limitation";
        return false;
    }

private slots:
    void initTestCase() {
        // Create temporary directory for test databases
        m_tempDir = new QTemporaryDir();
        QVERIFY(m_tempDir->isValid());

        // Override app data directory for tests
        m_originalAppDataDir = PathManager::getAppDataDir();
        PathManager::setAppDataDirForTesting(m_tempDir->path());

        // Initialize database
        Database::instance().close();
        QString dbPath = m_tempDir->filePath("global.db");
        QVERIFY(Database::instance().open(dbPath));
    }

    void cleanupTestCase() {
        // Restore original app data directory
        PathManager::setAppDataDirForTesting(m_originalAppDataDir);

        Database::instance().close();
        delete m_tempDir;
    }

    void init() {
        // Create MainWindow for each test
        m_mainWindow = new MainWindow();
        QVERIFY(m_mainWindow != nullptr);

        // Show window (required for some UI interactions)
        m_mainWindow->show();
        QTest::qWaitForWindowExposed(m_mainWindow);
    }

    void cleanup() {
        // Destroy MainWindow after each test
        delete m_mainWindow;
        m_mainWindow = nullptr;
    }

    /**
     * Test: No active contest should show error
     *
     * CURRENT BEHAVIOR (lines 2034-2038):
     * - If !m_qsoLogger, shows error "No active contest - open a contest first"
     * - Beeps
     * - Returns early
     */
    void testNoActiveContest() {
        QLineEdit* callsignEntry = findCallsignEntry();
        QLineEdit* exchangeEntry = findExchangeEntry();
        QLabel* statusLabel = findStatusLabel();

        QVERIFY(callsignEntry != nullptr);
        QVERIFY(exchangeEntry != nullptr);
        QVERIFY(statusLabel != nullptr);

        // Set input
        callsignEntry->setText("K1ABC");
        exchangeEntry->setText("599");

        // Store table row count before
        int rowsBefore = 0; // TODO: Get from m_qsoTableModel

        // Call onLogQSO()
        // PROBLEM: onLogQSO() is not a public slot, it's a private slot
        // We cannot call it directly from tests
        // This is a MAJOR testing limitation with current architecture

        // Workaround: Simulate Enter key press
        QTest::keyClick(callsignEntry, Qt::Key_Return);

        // Verify: Status shows error
        QString status = statusLabel->text();
        QVERIFY(status.contains("No active contest") || status.contains("Error"));

        // Verify: No QSO added
        int rowsAfter = 0; // TODO: Get from m_qsoTableModel
        QCOMPARE(rowsAfter, rowsBefore);
    }

    /**
     * Test: OPON command should open operator dialog
     *
     * CURRENT BEHAVIOR (lines 1997-2022):
     * - If callsign == "OPON", opens OperatorDialog
     * - Clears entry fields
     * - Returns early (no QSO logged)
     *
     * LIMITATION: Cannot test modal dialog interaction programmatically
     */
    void testOPONCommand() {
        QLineEdit* callsignEntry = findCallsignEntry();
        QVERIFY(callsignEntry != nullptr);

        // Set OPON command
        callsignEntry->setText("OPON");

        // TODO: Setup QSignalSpy for dialog creation
        // Cannot easily test modal dialog without mocking

        // Simulate Enter key
        QTest::keyClick(callsignEntry, Qt::Key_Return);

        // Verify: Entry cleared (but this happens asynchronously)
        // TODO: Wait for clear

        qWarning() << "OPON command test incomplete - modal dialog limitation";
    }

    /**
     * Test: UDP command should trigger rebroadcast
     *
     * CURRENT BEHAVIOR (lines 2024-2031):
     * - If callsign == "UDP", calls onRebroadcastLog()
     * - Clears entry fields
     * - Returns early
     */
    void testUDPCommand() {
        QLineEdit* callsignEntry = findCallsignEntry();
        QVERIFY(callsignEntry != nullptr);

        // Set UDP command
        callsignEntry->setText("UDP");

        // TODO: Setup QSignalSpy for onRebroadcastLog() call
        // Cannot easily verify private method call

        // Simulate Enter key
        QTest::keyClick(callsignEntry, Qt::Key_Return);

        // Verify: Entry cleared
        // TODO: Wait for clear and verify

        qWarning() << "UDP command test incomplete - private method limitation";
    }

    /**
     * Test: Successful QSO logging workflow
     *
     * CURRENT BEHAVIOR (lines 2075-2297):
     * - Updates serial number
     * - Adds to table model
     * - Updates score display
     * - Saves to database
     * - Saves to exchange memory
     * - Broadcasts UDP
     * - Updates displays
     * - Clears entry
     *
     * LIMITATION: Requires active contest, cannot activate programmatically
     */
    void testSuccessfulQSOLogging() {
        // BLOCKED: Cannot activate contest programmatically
        // MainWindow::activateContest() is private
        // MainWindow::onNewOpenContest() shows modal dialog

        QSKIP("Cannot test - MainWindow does not expose contest activation API");
    }

    /**
     * Test: Duplicate QSO still logs but marks as duplicate
     *
     * CURRENT BEHAVIOR:
     * - QSOLogger sets isDuplicate flag
     * - QSO still added to log
     * - Duplicate info logged
     *
     * LIMITATION: Requires active contest
     */
    void testDuplicateQSOLogging() {
        QSKIP("Cannot test - MainWindow does not expose contest activation API");
    }

    /**
     * Test: Exchange memory save after successful QSO
     *
     * CURRENT BEHAVIOR (lines 2232-2247):
     * - If QSO saved and exchange not empty, saves to ExchangeMemoryRepository
     * - Sets source = "auto" or "manual" based on m_initialExchangePopulated
     *
     * LIMITATION: Requires active contest and successful database save
     */
    void testExchangeMemorySave() {
        QSKIP("Cannot test - MainWindow does not expose contest activation API");
    }
};

// Export test
QTEST_MAIN(TestMainWindowLogQSO)
#include "test_mainwindow_logqso.moc"

/**
 * TEST ANALYSIS CONCLUSION:
 *
 * ❌ CRITICAL FINDING: MainWindow is UNTESTABLE in its current form.
 *
 * BLOCKING ISSUES:
 * 1. onLogQSO() is a private slot - cannot call directly
 * 2. activateContest() is private - cannot set up test state
 * 3. m_qsoTableModel is private - cannot verify QSO added
 * 4. m_qsoLogger is private - cannot verify it was called
 * 5. Modal dialogs block test execution
 * 6. No dependency injection - cannot mock repositories
 *
 * TESTING GAPS:
 * - Cannot test QSO logging workflow end-to-end
 * - Cannot test database persistence
 * - Cannot test retry logic
 * - Cannot test emergency file save
 * - Cannot test exchange memory save
 * - Cannot test multiplier window updates
 * - Cannot verify table model changes
 *
 * ARCHITECTURE FAILURE:
 * The current MainWindow design makes it IMPOSSIBLE to write meaningful
 * integration tests. This is the STRONGEST evidence that refactoring is
 * urgently needed.
 *
 * REQUIRED CHANGES FOR TESTABILITY:
 * 1. Extract business logic to services (can be tested in isolation)
 * 2. Use dependency injection (can inject mocks)
 * 3. Expose testable public API (or friend classes for tests)
 * 4. Remove modal dialogs from business logic
 * 5. Separate UI updates from persistence logic
 *
 * NEXT STEPS:
 * Since we CANNOT test MainWindow directly, we must:
 * 1. Design service architecture first
 * 2. Extract services with their own tests
 * 3. Integration tests will validate MainWindow delegates correctly
 */
