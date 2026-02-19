/**
 * Unit tests for QSOQueryService
 *
 * Tests stateless query operations on QSO collections:
 * worked callsigns, worked bands, time window counting, rate calculation.
 */

#include <QTest>
#include "../src/services/QSOQueryService.h"

using namespace TR4QT;

class TestQSOQueryService : public QObject {
    Q_OBJECT

private:
    QSO makeQSO(const QString& callsign, BandType band, ModeType mode,
                 const QDateTime& timestamp = QDateTime()) {
        QSO qso;
        qso.callsign = callsign;
        qso.band = band;
        qso.mode = mode;
        qso.timestamp = timestamp.isValid() ? timestamp : QDateTime::currentDateTimeUtc();
        return qso;
    }

private slots:
    // --- getWorkedCallsigns ---

    void testGetWorkedCallsigns_Empty();
    void testGetWorkedCallsigns_Unique();
    void testGetWorkedCallsigns_Duplicates();
    void testGetWorkedCallsigns_CaseNormalized();

    // --- getWorkedBandsForCallsign ---

    void testGetWorkedBands_Empty();
    void testGetWorkedBands_EmptyCallsign();
    void testGetWorkedBands_CaseInsensitive();
    void testGetWorkedBands_MultipleBands();
    void testGetWorkedBands_DuplicateBand();

    // --- countQSOsInTimeWindow ---

    void testCountTimeWindow_AllInside();
    void testCountTimeWindow_SomeOutside();
    void testCountTimeWindow_BoundaryInclusive();
    void testCountTimeWindow_Empty();

    // --- calculateRate ---

    void testCalculateRate_LessThan2QSOs();
    void testCalculateRate_Normal();
    void testCalculateRate_LookbackLargerThanList();
    void testCalculateRate_SameTimestamp();
};

// --- getWorkedCallsigns ---

void TestQSOQueryService::testGetWorkedCallsigns_Empty()
{
    QSOQueryService svc;
    QSet<QString> result = svc.getWorkedCallsigns(QList<QSO>());
    QVERIFY(result.isEmpty());
}

void TestQSOQueryService::testGetWorkedCallsigns_Unique()
{
    QSOQueryService svc;
    QList<QSO> qsos = {
        makeQSO("W1AW", BandType::Band20M, ModeType::CW),
        makeQSO("DL1ABC", BandType::Band20M, ModeType::CW),
        makeQSO("JA1ABC", BandType::Band20M, ModeType::CW),
    };

    QSet<QString> result = svc.getWorkedCallsigns(qsos);
    QCOMPARE(result.size(), 3);
    QVERIFY(result.contains("W1AW"));
    QVERIFY(result.contains("DL1ABC"));
    QVERIFY(result.contains("JA1ABC"));
}

void TestQSOQueryService::testGetWorkedCallsigns_Duplicates()
{
    QSOQueryService svc;
    QList<QSO> qsos = {
        makeQSO("W1AW", BandType::Band20M, ModeType::CW),
        makeQSO("W1AW", BandType::Band15M, ModeType::CW),
    };

    QSet<QString> result = svc.getWorkedCallsigns(qsos);
    QCOMPARE(result.size(), 1);
}

void TestQSOQueryService::testGetWorkedCallsigns_CaseNormalized()
{
    QSOQueryService svc;
    QList<QSO> qsos = {
        makeQSO("w1aw", BandType::Band20M, ModeType::CW),
    };

    QSet<QString> result = svc.getWorkedCallsigns(qsos);
    QVERIFY(result.contains("W1AW"));  // Normalized to upper
}

// --- getWorkedBandsForCallsign ---

void TestQSOQueryService::testGetWorkedBands_Empty()
{
    QSOQueryService svc;
    QList<BandType> result = svc.getWorkedBandsForCallsign(QList<QSO>(), "W1AW");
    QVERIFY(result.isEmpty());
}

void TestQSOQueryService::testGetWorkedBands_EmptyCallsign()
{
    QSOQueryService svc;
    QList<QSO> qsos = {makeQSO("W1AW", BandType::Band20M, ModeType::CW)};

    QList<BandType> result = svc.getWorkedBandsForCallsign(qsos, "");
    QVERIFY(result.isEmpty());
}

void TestQSOQueryService::testGetWorkedBands_CaseInsensitive()
{
    QSOQueryService svc;
    QList<QSO> qsos = {makeQSO("W1AW", BandType::Band20M, ModeType::CW)};

    QList<BandType> result = svc.getWorkedBandsForCallsign(qsos, "w1aw");
    QCOMPARE(result.size(), 1);
    QVERIFY(result.contains(BandType::Band20M));
}

void TestQSOQueryService::testGetWorkedBands_MultipleBands()
{
    QSOQueryService svc;
    QList<QSO> qsos = {
        makeQSO("W1AW", BandType::Band20M, ModeType::CW),
        makeQSO("W1AW", BandType::Band15M, ModeType::CW),
        makeQSO("W1AW", BandType::Band10M, ModeType::CW),
    };

    QList<BandType> result = svc.getWorkedBandsForCallsign(qsos, "W1AW");
    QCOMPARE(result.size(), 3);
}

void TestQSOQueryService::testGetWorkedBands_DuplicateBand()
{
    QSOQueryService svc;
    QList<QSO> qsos = {
        makeQSO("W1AW", BandType::Band20M, ModeType::CW),
        makeQSO("W1AW", BandType::Band20M, ModeType::USB),  // Same band, different mode
    };

    QList<BandType> result = svc.getWorkedBandsForCallsign(qsos, "W1AW");
    QCOMPARE(result.size(), 1);  // Band20M counted once
}

// --- countQSOsInTimeWindow ---

void TestQSOQueryService::testCountTimeWindow_AllInside()
{
    QSOQueryService svc;
    QDateTime base = QDateTime(QDate(2026, 1, 1), QTime(12, 0), Qt::UTC);

    QList<QSO> qsos = {
        makeQSO("W1AW", BandType::Band20M, ModeType::CW, base.addSecs(60)),
        makeQSO("DL1ABC", BandType::Band20M, ModeType::CW, base.addSecs(120)),
    };

    int count = svc.countQSOsInTimeWindow(qsos, base, base.addSecs(300));
    QCOMPARE(count, 2);
}

void TestQSOQueryService::testCountTimeWindow_SomeOutside()
{
    QSOQueryService svc;
    QDateTime base = QDateTime(QDate(2026, 1, 1), QTime(12, 0), Qt::UTC);

    QList<QSO> qsos = {
        makeQSO("W1AW", BandType::Band20M, ModeType::CW, base.addSecs(60)),
        makeQSO("DL1ABC", BandType::Band20M, ModeType::CW, base.addSecs(600)),  // Outside
    };

    int count = svc.countQSOsInTimeWindow(qsos, base, base.addSecs(300));
    QCOMPARE(count, 1);
}

void TestQSOQueryService::testCountTimeWindow_BoundaryInclusive()
{
    QSOQueryService svc;
    QDateTime start = QDateTime(QDate(2026, 1, 1), QTime(12, 0), Qt::UTC);
    QDateTime end = QDateTime(QDate(2026, 1, 1), QTime(13, 0), Qt::UTC);

    QList<QSO> qsos = {
        makeQSO("W1AW", BandType::Band20M, ModeType::CW, start),   // Exactly at start
        makeQSO("DL1ABC", BandType::Band20M, ModeType::CW, end),   // Exactly at end
    };

    int count = svc.countQSOsInTimeWindow(qsos, start, end);
    QCOMPARE(count, 2);  // Both boundaries inclusive
}

void TestQSOQueryService::testCountTimeWindow_Empty()
{
    QSOQueryService svc;
    QDateTime base = QDateTime(QDate(2026, 1, 1), QTime(12, 0), Qt::UTC);

    int count = svc.countQSOsInTimeWindow(QList<QSO>(), base, base.addSecs(3600));
    QCOMPARE(count, 0);
}

// --- calculateRate ---

void TestQSOQueryService::testCalculateRate_LessThan2QSOs()
{
    QSOQueryService svc;

    QCOMPARE(svc.calculateRate(QList<QSO>()), 0);

    QList<QSO> oneQSO = {makeQSO("W1AW", BandType::Band20M, ModeType::CW)};
    QCOMPARE(svc.calculateRate(oneQSO), 0);
}

void TestQSOQueryService::testCalculateRate_Normal()
{
    QSOQueryService svc;
    QDateTime base = QDateTime(QDate(2026, 1, 1), QTime(12, 0), Qt::UTC);

    // 10 QSOs over 30 minutes = ~18 QSOs/hour
    // Formula: (lookback - 1) * 3600 / periodSecs
    // With default lookback=10, all 10 used: (10-1) * 3600 / 1800 = 18
    QList<QSO> qsos;
    for (int i = 0; i < 10; i++) {
        qsos.append(makeQSO(QString("W%1AW").arg(i),
                             BandType::Band20M, ModeType::CW,
                             base.addSecs(i * 200)));  // Every 200 seconds
    }

    int rate = svc.calculateRate(qsos, 10);
    // (10-1) * 3600 / (9 * 200) = 9 * 3600 / 1800 = 18
    QCOMPARE(rate, 18);
}

void TestQSOQueryService::testCalculateRate_LookbackLargerThanList()
{
    QSOQueryService svc;
    QDateTime base = QDateTime(QDate(2026, 1, 1), QTime(12, 0), Qt::UTC);

    // Only 3 QSOs but lookback=10 — should use all 3
    QList<QSO> qsos = {
        makeQSO("W1AW", BandType::Band20M, ModeType::CW, base),
        makeQSO("W2AW", BandType::Band20M, ModeType::CW, base.addSecs(600)),
        makeQSO("W3AW", BandType::Band20M, ModeType::CW, base.addSecs(1200)),
    };

    int rate = svc.calculateRate(qsos, 10);
    // (3-1) * 3600 / 1200 = 6
    QCOMPARE(rate, 6);
}

void TestQSOQueryService::testCalculateRate_SameTimestamp()
{
    QSOQueryService svc;
    QDateTime base = QDateTime(QDate(2026, 1, 1), QTime(12, 0), Qt::UTC);

    // All QSOs at same time — periodSecs = 0, should return 0
    QList<QSO> qsos = {
        makeQSO("W1AW", BandType::Band20M, ModeType::CW, base),
        makeQSO("W2AW", BandType::Band20M, ModeType::CW, base),
    };

    int rate = svc.calculateRate(qsos, 10);
    QCOMPARE(rate, 0);
}

QTEST_MAIN(TestQSOQueryService)
#include "test_qso_query_service.moc"
