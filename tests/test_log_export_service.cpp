/**
 * Unit tests for LogExportService
 *
 * Note: LogExportService requires user interaction (dialogs, file system)
 * so full testing is limited. These tests verify the result structures
 * and edge cases that don't require dialogs.
 *
 * Full testing of this service is done via manual integration tests.
 */

#include <QtTest/QtTest>
#include "../src/services/LogExportService.h"

using namespace TR4QT;

class TestLogExportService : public QObject {
    Q_OBJECT

private:
    LogExportService m_service;

private slots:
    /**
     * Test: LogExportResult default initialization
     */
    void testResultDefaults() {
        LogExportResult result;

        QVERIFY(!result.success);
        QVERIFY(!result.cancelled);
        QVERIFY(result.zipFilePath.isEmpty());
        QVERIFY(result.errorMessage.isEmpty());
    }

    /**
     * Test: LogExportResult cancelled state
     */
    void testResultCancelledState() {
        LogExportResult result;
        result.cancelled = true;

        QVERIFY(!result.success);
        QVERIFY(result.cancelled);
        QVERIFY(result.zipFilePath.isEmpty());
    }

    /**
     * Test: LogExportResult success state
     */
    void testResultSuccessState() {
        LogExportResult result;
        result.success = true;
        result.zipFilePath = "/path/to/logs.zip";

        QVERIFY(result.success);
        QVERIFY(!result.cancelled);
        QCOMPARE(result.zipFilePath, QString("/path/to/logs.zip"));
    }

    /**
     * Test: LogExportResult error state
     */
    void testResultErrorState() {
        LogExportResult result;
        result.success = false;
        result.errorMessage = "Failed to create zip";

        QVERIFY(!result.success);
        QVERIFY(!result.cancelled);
        QCOMPARE(result.errorMessage, QString("Failed to create zip"));
    }

    /**
     * Test: Service can be constructed
     */
    void testServiceConstruction() {
        LogExportService service;
        // Just verify construction doesn't throw
        QVERIFY(true);
    }

    /**
     * Note: exportLogsForSupport requires user interaction and cannot be
     * effectively unit tested. It is tested via manual integration tests.
     *
     * The method does the following:
     * 1. Collects logs from Logger singleton
     * 2. Builds system info string
     * 3. Shows preview dialog (user interaction)
     * 4. Creates zip file on Desktop
     * 5. Shows success dialog with options (user interaction)
     * 6. Opens file manager and/or email client
     */
};

QTEST_GUILESS_MAIN(TestLogExportService)
#include "test_log_export_service.moc"
