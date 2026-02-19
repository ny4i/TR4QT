/**
 * Unit tests for StatusNotifier
 *
 * Tests formatMessage and styleForEvent static methods.
 * These are pure functions — no singleton or signal testing needed.
 */

#include <QTest>
#include "../src/services/StatusNotifier.h"

using namespace TR4QT;

class TestStatusNotifier : public QObject {
    Q_OBJECT

private slots:
    // --- formatMessage: data-less events ---

    void testFormatMessage_Ready();
    void testFormatMessage_RadioDisconnected();
    void testFormatMessage_CWAborted();
    void testFormatMessage_LogCleared();
    void testFormatMessage_SearchCancelled();
    void testFormatMessage_PreferencesSaved();

    // --- formatMessage: events with data ---

    void testFormatMessage_Error_WithData();
    void testFormatMessage_Error_NoData();
    void testFormatMessage_RadioConnected_WithModel();
    void testFormatMessage_CWSpeedChanged();
    void testFormatMessage_QSOLogged_WithCallsign();
    void testFormatMessage_QSOLogged_NoData();
    void testFormatMessage_ContestCreated();
    void testFormatMessage_PointsRecalculated();
    void testFormatMessage_UDPBroadcastStarted();
    void testFormatMessage_UDPBroadcastComplete();
    void testFormatMessage_OperatorChanged();
    void testFormatMessage_BandManuallySet();

    // --- formatMessage: failure events with data ---

    void testFormatMessage_CTYUpdateFailed();
    void testFormatMessage_LOTWDownloadFailed();

    // --- styleForEvent: Error style ---

    void testStyleForEvent_ErrorEvents();

    // --- styleForEvent: Warning style ---

    void testStyleForEvent_WarningEvents();

    // --- styleForEvent: Success style ---

    void testStyleForEvent_SuccessEvents();

    // --- styleForEvent: Highlight style ---

    void testStyleForEvent_HighlightEvents();

    // --- styleForEvent: Normal style (default) ---

    void testStyleForEvent_NormalEvents();
};

// --- formatMessage: data-less events ---

void TestStatusNotifier::testFormatMessage_Ready()
{
    QCOMPARE(StatusNotifier::formatMessage(StatusEvent::Ready), QString("Ready"));
}

void TestStatusNotifier::testFormatMessage_RadioDisconnected()
{
    QCOMPARE(StatusNotifier::formatMessage(StatusEvent::RadioDisconnected), QString("Radio disconnected"));
}

void TestStatusNotifier::testFormatMessage_CWAborted()
{
    QCOMPARE(StatusNotifier::formatMessage(StatusEvent::CWAborted), QString("CW transmission aborted"));
}

void TestStatusNotifier::testFormatMessage_LogCleared()
{
    QCOMPARE(StatusNotifier::formatMessage(StatusEvent::LogCleared), QString("Log cleared"));
}

void TestStatusNotifier::testFormatMessage_SearchCancelled()
{
    QString msg = StatusNotifier::formatMessage(StatusEvent::SearchCancelled);
    QVERIFY(msg.contains("Search cancelled"));
}

void TestStatusNotifier::testFormatMessage_PreferencesSaved()
{
    QCOMPARE(StatusNotifier::formatMessage(StatusEvent::PreferencesSaved), QString("Preferences saved"));
}

// --- formatMessage: events with data ---

void TestStatusNotifier::testFormatMessage_Error_WithData()
{
    QString msg = StatusNotifier::formatMessage(StatusEvent::Error, QVariant("disk full"));
    QVERIFY(msg.contains("Error"));
    QVERIFY(msg.contains("disk full"));
}

void TestStatusNotifier::testFormatMessage_Error_NoData()
{
    QString msg = StatusNotifier::formatMessage(StatusEvent::Error);
    QCOMPARE(msg, QString("Error"));
}

void TestStatusNotifier::testFormatMessage_RadioConnected_WithModel()
{
    QString msg = StatusNotifier::formatMessage(StatusEvent::RadioConnected, QVariant("K4"));
    QVERIFY(msg.contains("K4"));
    QVERIFY(msg.contains("connected"));
}

void TestStatusNotifier::testFormatMessage_CWSpeedChanged()
{
    QString msg = StatusNotifier::formatMessage(StatusEvent::CWSpeedChanged, QVariant(28));
    QVERIFY(msg.contains("28"));
    QVERIFY(msg.contains("WPM"));
}

void TestStatusNotifier::testFormatMessage_QSOLogged_WithCallsign()
{
    QString msg = StatusNotifier::formatMessage(StatusEvent::QSOLogged, QVariant("DL1ABC"));
    QVERIFY(msg.contains("DL1ABC"));
}

void TestStatusNotifier::testFormatMessage_QSOLogged_NoData()
{
    QString msg = StatusNotifier::formatMessage(StatusEvent::QSOLogged);
    QVERIFY(msg.contains("QSO logged"));
}

void TestStatusNotifier::testFormatMessage_ContestCreated()
{
    QString msg = StatusNotifier::formatMessage(StatusEvent::ContestCreated, QVariant("CQ WW CW"));
    QVERIFY(msg.contains("CQ WW CW"));
}

void TestStatusNotifier::testFormatMessage_PointsRecalculated()
{
    QString msg = StatusNotifier::formatMessage(StatusEvent::PointsRecalculated, QVariant(150));
    QVERIFY(msg.contains("150"));
    QVERIFY(msg.contains("QSOs"));
}

void TestStatusNotifier::testFormatMessage_UDPBroadcastStarted()
{
    QString msg = StatusNotifier::formatMessage(StatusEvent::UDPBroadcastStarted, QVariant(500));
    QVERIFY(msg.contains("500"));
}

void TestStatusNotifier::testFormatMessage_UDPBroadcastComplete()
{
    QString msg = StatusNotifier::formatMessage(StatusEvent::UDPBroadcastComplete, QVariant(500));
    QVERIFY(msg.contains("500"));
    QVERIFY(msg.contains("complete"));
}

void TestStatusNotifier::testFormatMessage_OperatorChanged()
{
    QString msg = StatusNotifier::formatMessage(StatusEvent::OperatorChanged, QVariant("NY4I"));
    QVERIFY(msg.contains("NY4I"));
}

void TestStatusNotifier::testFormatMessage_BandManuallySet()
{
    QString msg = StatusNotifier::formatMessage(StatusEvent::BandManuallySet, QVariant("20M"));
    QVERIFY(msg.contains("20M"));
}

// --- formatMessage: failure events with data ---

void TestStatusNotifier::testFormatMessage_CTYUpdateFailed()
{
    QString msg = StatusNotifier::formatMessage(StatusEvent::CTYUpdateFailed, QVariant("timeout"));
    QVERIFY(msg.contains("timeout"));
    QVERIFY(msg.contains("failed"));
}

void TestStatusNotifier::testFormatMessage_LOTWDownloadFailed()
{
    QString msg = StatusNotifier::formatMessage(StatusEvent::LOTWDownloadFailed, QVariant("auth error"));
    QVERIFY(msg.contains("auth error"));
}

// --- styleForEvent: Error ---

void TestStatusNotifier::testStyleForEvent_ErrorEvents()
{
    QCOMPARE(StatusNotifier::styleForEvent(StatusEvent::Error), StatusStyle::Error);
    QCOMPARE(StatusNotifier::styleForEvent(StatusEvent::QSOError), StatusStyle::Error);
    QCOMPARE(StatusNotifier::styleForEvent(StatusEvent::FrequencyError), StatusStyle::Error);
    QCOMPARE(StatusNotifier::styleForEvent(StatusEvent::NoActiveContest), StatusStyle::Error);
    QCOMPARE(StatusNotifier::styleForEvent(StatusEvent::CTYUpdateFailed), StatusStyle::Error);
    QCOMPARE(StatusNotifier::styleForEvent(StatusEvent::WebServerFailed), StatusStyle::Error);
    QCOMPARE(StatusNotifier::styleForEvent(StatusEvent::UDPBroadcastDisabled), StatusStyle::Error);
}

// --- styleForEvent: Warning ---

void TestStatusNotifier::testStyleForEvent_WarningEvents()
{
    QCOMPARE(StatusNotifier::styleForEvent(StatusEvent::Warning), StatusStyle::Warning);
    QCOMPARE(StatusNotifier::styleForEvent(StatusEvent::QSODuplicate), StatusStyle::Warning);
    QCOMPARE(StatusNotifier::styleForEvent(StatusEvent::CWAutoSendOff), StatusStyle::Warning);
}

// --- styleForEvent: Success ---

void TestStatusNotifier::testStyleForEvent_SuccessEvents()
{
    QCOMPARE(StatusNotifier::styleForEvent(StatusEvent::QSOLogged), StatusStyle::Success);
    QCOMPARE(StatusNotifier::styleForEvent(StatusEvent::ContestCreated), StatusStyle::Success);
    QCOMPARE(StatusNotifier::styleForEvent(StatusEvent::CTYUpdateComplete), StatusStyle::Success);
    QCOMPARE(StatusNotifier::styleForEvent(StatusEvent::ExportComplete), StatusStyle::Success);
    QCOMPARE(StatusNotifier::styleForEvent(StatusEvent::ImportComplete), StatusStyle::Success);
}

// --- styleForEvent: Highlight ---

void TestStatusNotifier::testStyleForEvent_HighlightEvents()
{
    QCOMPARE(StatusNotifier::styleForEvent(StatusEvent::RadioConnecting), StatusStyle::Highlight);
    QCOMPARE(StatusNotifier::styleForEvent(StatusEvent::RescoreStarted), StatusStyle::Highlight);
    QCOMPARE(StatusNotifier::styleForEvent(StatusEvent::UDPBroadcastStarted), StatusStyle::Highlight);
}

// --- styleForEvent: Normal (default) ---

void TestStatusNotifier::testStyleForEvent_NormalEvents()
{
    QCOMPARE(StatusNotifier::styleForEvent(StatusEvent::Ready), StatusStyle::Normal);
    QCOMPARE(StatusNotifier::styleForEvent(StatusEvent::RadioDisconnected), StatusStyle::Normal);
    QCOMPARE(StatusNotifier::styleForEvent(StatusEvent::PreferencesSaved), StatusStyle::Normal);
    QCOMPARE(StatusNotifier::styleForEvent(StatusEvent::LogCleared), StatusStyle::Normal);
}

QTEST_MAIN(TestStatusNotifier)
#include "test_status_notifier.moc"
