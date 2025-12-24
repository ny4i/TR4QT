#include <QTest>
#include <QDateTime>
#include "../src/logging/LogFormatter.h"
#include "../src/logging/LogLevel.h"

using namespace TR4QT;

/**
 * Unit tests for LogFormatter
 * Tests TR4W-compatible log message formatting
 */
class TestLogFormatter : public QObject {
    Q_OBJECT

private slots:
    // Test formatTimestamp()
    void testFormatTimestamp_Basic();
    void testFormatTimestamp_MillisecondPadding();
    void testFormatTimestamp_ZeroMilliseconds();
    void testFormatTimestamp_VariousDates();

    // Test formatThreadId()
    void testFormatThreadId_Format();
    void testFormatThreadId_DifferentValues();

    // Test format() - complete message formatting
    void testFormat_Complete();
    void testFormat_AllLogLevels();
    void testFormat_EmptyMessage();
    void testFormat_EmptyCategory();
    void testFormat_LongMessage();
    void testFormat_SpecialCharacters();
    void testFormat_Newline();
};

// formatTimestamp() tests

void TestLogFormatter::testFormatTimestamp_Basic() {
    // Test basic timestamp formatting: "24 Dec 2025 14:23:45.123"
    QDateTime dt(QDate(2025, 12, 24), QTime(14, 23, 45, 123), Qt::UTC);
    QString result = LogFormatter::formatTimestamp(dt);

    QCOMPARE(result, QString("24 Dec 2025 14:23:45.123"));
}

void TestLogFormatter::testFormatTimestamp_MillisecondPadding() {
    // Test that milliseconds are zero-padded to 3 digits
    QDateTime dt1(QDate(2025, 1, 1), QTime(12, 0, 0, 5), Qt::UTC);
    QString result1 = LogFormatter::formatTimestamp(dt1);
    QCOMPARE(result1, QString("01 Jan 2025 12:00:00.005"));

    QDateTime dt2(QDate(2025, 1, 1), QTime(12, 0, 0, 50), Qt::UTC);
    QString result2 = LogFormatter::formatTimestamp(dt2);
    QCOMPARE(result2, QString("01 Jan 2025 12:00:00.050"));

    QDateTime dt3(QDate(2025, 1, 1), QTime(12, 0, 0, 500), Qt::UTC);
    QString result3 = LogFormatter::formatTimestamp(dt3);
    QCOMPARE(result3, QString("01 Jan 2025 12:00:00.500"));
}

void TestLogFormatter::testFormatTimestamp_ZeroMilliseconds() {
    // Test zero milliseconds: "00:00:00.000"
    QDateTime dt(QDate(2025, 1, 1), QTime(0, 0, 0, 0), Qt::UTC);
    QString result = LogFormatter::formatTimestamp(dt);

    QCOMPARE(result, QString("01 Jan 2025 00:00:00.000"));
}

void TestLogFormatter::testFormatTimestamp_VariousDates() {
    // Test different dates to verify month formatting
    QDateTime jan(QDate(2025, 1, 15), QTime(10, 30, 45, 100), Qt::UTC);
    QVERIFY(LogFormatter::formatTimestamp(jan).startsWith("15 Jan 2025"));

    QDateTime dec(QDate(2025, 12, 31), QTime(23, 59, 59, 999), Qt::UTC);
    QVERIFY(LogFormatter::formatTimestamp(dec).startsWith("31 Dec 2025"));

    QDateTime jul(QDate(2025, 7, 4), QTime(12, 0, 0, 0), Qt::UTC);
    QVERIFY(LogFormatter::formatTimestamp(jul).startsWith("04 Jul 2025"));
}

// formatThreadId() tests

void TestLogFormatter::testFormatThreadId_Format() {
    // Test that thread ID is formatted with brackets: "[12345]"
    Qt::HANDLE tid = reinterpret_cast<Qt::HANDLE>(12345);
    QString result = LogFormatter::formatThreadId(tid);

    QCOMPARE(result, QString("[12345]"));
}

void TestLogFormatter::testFormatThreadId_DifferentValues() {
    // Test various thread ID values
    Qt::HANDLE tid1 = reinterpret_cast<Qt::HANDLE>(1);
    QCOMPARE(LogFormatter::formatThreadId(tid1), QString("[1]"));

    Qt::HANDLE tid2 = reinterpret_cast<Qt::HANDLE>(999999);
    QCOMPARE(LogFormatter::formatThreadId(tid2), QString("[999999]"));

    Qt::HANDLE tid3 = reinterpret_cast<Qt::HANDLE>(8775016576ULL);
    QCOMPARE(LogFormatter::formatThreadId(tid3), QString("[8775016576]"));
}

// format() - complete message formatting tests

void TestLogFormatter::testFormat_Complete() {
    // Test complete TR4W format: "DD MMM YYYY HH:MM:SS.mmm elapsed [thread] level category - message\n"
    QDateTime dt(QDate(2025, 12, 24), QTime(18, 14, 2, 968), Qt::UTC);
    qint64 elapsedMs = 2650;
    Qt::HANDLE threadId = reinterpret_cast<Qt::HANDLE>(10252);
    LogLevel level = LogLevel::Info;
    QString category = "TR4WDebugLog";
    QString message = "DecimalSeparator = .";

    QString result = LogFormatter::format(dt, elapsedMs, threadId, level, category, message);

    // Expected: "24 Dec 2025 18:14:02.968 2650 [10252] info TR4WDebugLog - DecimalSeparator = .\n"
    QString expected = "24 Dec 2025 18:14:02.968 2650 [10252] info TR4WDebugLog - DecimalSeparator = .\n";
    QCOMPARE(result, expected);
}

void TestLogFormatter::testFormat_AllLogLevels() {
    // Test formatting with all log levels
    QDateTime dt(QDate(2025, 1, 1), QTime(12, 0, 0, 0), Qt::UTC);
    Qt::HANDLE tid = reinterpret_cast<Qt::HANDLE>(1);

    QString trace = LogFormatter::format(dt, 100, tid, LogLevel::Trace, "Test", "msg");
    QVERIFY(trace.contains("trace"));

    QString debug = LogFormatter::format(dt, 100, tid, LogLevel::Debug, "Test", "msg");
    QVERIFY(debug.contains("debug"));

    QString info = LogFormatter::format(dt, 100, tid, LogLevel::Info, "Test", "msg");
    QVERIFY(info.contains("info"));

    QString warn = LogFormatter::format(dt, 100, tid, LogLevel::Warn, "Test", "msg");
    QVERIFY(warn.contains("warn"));

    QString error = LogFormatter::format(dt, 100, tid, LogLevel::Error, "Test", "msg");
    QVERIFY(error.contains("error"));

    QString fatal = LogFormatter::format(dt, 100, tid, LogLevel::Fatal, "Test", "msg");
    QVERIFY(fatal.contains("fatal"));
}

void TestLogFormatter::testFormat_EmptyMessage() {
    // Test with empty message
    QDateTime dt(QDate(2025, 1, 1), QTime(12, 0, 0, 0), Qt::UTC);
    Qt::HANDLE tid = reinterpret_cast<Qt::HANDLE>(1);

    QString result = LogFormatter::format(dt, 100, tid, LogLevel::Info, "Test", "");

    // Should still have format, just empty message
    QVERIFY(result.contains("info"));
    QVERIFY(result.contains("Test"));
    QVERIFY(result.endsWith(" - \n"));
}

void TestLogFormatter::testFormat_EmptyCategory() {
    // Test with empty category
    QDateTime dt(QDate(2025, 1, 1), QTime(12, 0, 0, 0), Qt::UTC);
    Qt::HANDLE tid = reinterpret_cast<Qt::HANDLE>(1);

    QString result = LogFormatter::format(dt, 100, tid, LogLevel::Info, "", "Test message");

    // Should still format correctly with empty category
    QVERIFY(result.contains("info"));
    QVERIFY(result.contains("Test message"));
    QVERIFY(result.contains(" - Test message\n"));
}

void TestLogFormatter::testFormat_LongMessage() {
    // Test with long message (should not truncate)
    QDateTime dt(QDate(2025, 1, 1), QTime(12, 0, 0, 0), Qt::UTC);
    Qt::HANDLE tid = reinterpret_cast<Qt::HANDLE>(1);
    QString longMsg = QString("A").repeated(1000);

    QString result = LogFormatter::format(dt, 100, tid, LogLevel::Info, "Test", longMsg);

    // Should contain the full message
    QVERIFY(result.contains(longMsg));
    QVERIFY(result.length() > 1000);
}

void TestLogFormatter::testFormat_SpecialCharacters() {
    // Test with special characters in message
    QDateTime dt(QDate(2025, 1, 1), QTime(12, 0, 0, 0), Qt::UTC);
    Qt::HANDLE tid = reinterpret_cast<Qt::HANDLE>(1);
    QString msg = "Test with \"quotes\" and 'apostrophes' and <tags>";

    QString result = LogFormatter::format(dt, 100, tid, LogLevel::Info, "Test", msg);

    // Should preserve special characters
    QVERIFY(result.contains("\"quotes\""));
    QVERIFY(result.contains("'apostrophes'"));
    QVERIFY(result.contains("<tags>"));
}

void TestLogFormatter::testFormat_Newline() {
    // Test that format always ends with newline
    QDateTime dt(QDate(2025, 1, 1), QTime(12, 0, 0, 0), Qt::UTC);
    Qt::HANDLE tid = reinterpret_cast<Qt::HANDLE>(1);

    QString result = LogFormatter::format(dt, 100, tid, LogLevel::Info, "Test", "Message");

    QVERIFY(result.endsWith("\n"));
}

QTEST_MAIN(TestLogFormatter)
#include "test_logformatter.moc"
