/**
 * Unit tests for ScoreCalculationService
 *
 * Critical priority: validates scoring logic including per-band vs all-bands
 * multiplier tracking, the bug fixed in v3.40.31.
 */

#include <QTest>
#include "../src/services/ScoreCalculationService.h"
#include "../src/contests/CQWWContest.h"
#include "../src/contests/CQWPXContest.h"
#include "../src/contests/CQWPXCWContest.h"
#include "../src/contests/CQWWCWContest.h"
#include "../src/models/StationInfo.h"

using namespace TR4QT;

class TestScoreCalculationService : public QObject {
    Q_OBJECT

private:
    StationInfo makeUSStation() {
        StationInfo station;
        station.callsign = "W1AW";
        station.country = "United States";
        station.continent = "NA";
        station.cqZone = 5;
        station.ituZone = 8;
        return station;
    }

    QSO makeQSO(const QString& callsign, BandType band, ModeType mode,
                 int cqZone, const QString& continent, const QString& country,
                 int qsoPoints = 0, bool isMultiplier = false,
                 const QStringList& multipliers = QStringList()) {
        QSO qso;
        qso.callsign = callsign;
        qso.band = band;
        qso.mode = mode;
        qso.cqZone = cqZone;
        qso.continent = continent;
        qso.dxccEntity = country;
        qso.dxccPrefix = callsign.left(2);
        qso.qsoPoints = qsoPoints;
        qso.isMultiplier = isMultiplier;
        qso.multipliers = multipliers;
        qso.timestamp = QDateTime::currentDateTimeUtc();
        return qso;
    }

private slots:
    // --- Basic tests ---

    void testEmptyQSOList();
    void testNullContest();
    void testSkipNoBandQSOs();

    // --- CQ WW: PerBand multipliers (Zone + Country) ---

    void testCQWW_PerBandZoneMultipliers();
    void testCQWW_SameZoneDifferentBands_CountsSeparately();
    void testCQWW_SameZoneSameBand_CountsOnce();

    // --- CQ WPX: AllBands multiplier (Prefix) ---

    void testCQWPX_AllBandsPrefix_CountsOnce();
    void testCQWPX_DifferentPrefixes_CountSeparately();

    // --- QSO Points aggregation ---

    void testBandPointsAccumulation();
    void testTotalQSOPoints();

    // --- Score formula ---

    void testCQWW_FinalScoreIsPointsTimesMultipliers();

    // --- Standard bands ---

    void testGetStandardBands();
};

void TestScoreCalculationService::testEmptyQSOList()
{
    ScoreCalculationService svc;
    auto station = makeUSStation();
    CQWWCWContest contest(station);

    ScoreResult result = svc.calculateScore(QList<QSO>(), &contest);

    QCOMPARE(result.totalQSOs, 0);
    QCOMPARE(result.totalQSOPoints, 0);
    QCOMPARE(result.totalMultipliers, 0);
    QCOMPARE(result.finalScore, 0);
}

void TestScoreCalculationService::testNullContest()
{
    ScoreCalculationService svc;

    QSO qso = makeQSO("DL1ABC", BandType::Band20M, ModeType::CW, 14, "EU", "Germany", 3);
    QList<QSO> qsos = {qso};

    ScoreResult result = svc.calculateScore(qsos, nullptr);

    QCOMPARE(result.totalQSOs, 1);
    QCOMPARE(result.totalQSOPoints, 3);
    // With null contest, finalScore = totalQSOPoints (no multiplier formula)
    QCOMPARE(result.finalScore, 3);
}

void TestScoreCalculationService::testSkipNoBandQSOs()
{
    ScoreCalculationService svc;
    auto station = makeUSStation();
    CQWWCWContest contest(station);

    QSO qso = makeQSO("DL1ABC", BandType::None, ModeType::CW, 14, "EU", "Germany", 3);
    QList<QSO> qsos = {qso};

    ScoreResult result = svc.calculateScore(qsos, &contest);

    QCOMPARE(result.totalQSOs, 0);  // Skipped
}

void TestScoreCalculationService::testCQWW_PerBandZoneMultipliers()
{
    ScoreCalculationService svc;
    auto station = makeUSStation();
    CQWWCWContest contest(station);

    // Zone 14 on 20m, Zone 25 on 20m — two different zones on same band = 2 zone mults
    QSO qso1 = makeQSO("DL1ABC", BandType::Band20M, ModeType::CW, 14, "EU", "Germany", 3, true);
    QSO qso2 = makeQSO("JA1ABC", BandType::Band20M, ModeType::CW, 25, "AS", "Japan", 3, true);
    QList<QSO> qsos = {qso1, qso2};

    ScoreResult result = svc.calculateScore(qsos, &contest);

    QCOMPARE(result.totalQSOs, 2);
    // Zone multipliers should count two zones on 20m
    QVERIFY(result.multiplierCounts.contains(MultiplierType::CQZone));
    QCOMPARE(result.multiplierCounts[MultiplierType::CQZone], 2);
}

void TestScoreCalculationService::testCQWW_SameZoneDifferentBands_CountsSeparately()
{
    ScoreCalculationService svc;
    auto station = makeUSStation();
    CQWWCWContest contest(station);

    // Zone 14 on 20m AND zone 14 on 15m — PerBand, so counts as 2 zone multipliers
    QSO qso1 = makeQSO("DL1ABC", BandType::Band20M, ModeType::CW, 14, "EU", "Germany", 3, true);
    QSO qso2 = makeQSO("DL2XYZ", BandType::Band15M, ModeType::CW, 14, "EU", "Germany", 3, true);
    QList<QSO> qsos = {qso1, qso2};

    ScoreResult result = svc.calculateScore(qsos, &contest);

    // PerBand zone: zone 14 on 20m + zone 14 on 15m = 2
    QCOMPARE(result.multiplierCounts[MultiplierType::CQZone], 2);
}

void TestScoreCalculationService::testCQWW_SameZoneSameBand_CountsOnce()
{
    ScoreCalculationService svc;
    auto station = makeUSStation();
    CQWWCWContest contest(station);

    // Zone 14 on 20m twice — should count as 1 zone multiplier
    QSO qso1 = makeQSO("DL1ABC", BandType::Band20M, ModeType::CW, 14, "EU", "Germany", 3, true);
    QSO qso2 = makeQSO("DL2XYZ", BandType::Band20M, ModeType::CW, 14, "EU", "Germany", 3, false);
    QList<QSO> qsos = {qso1, qso2};

    ScoreResult result = svc.calculateScore(qsos, &contest);

    // Same zone, same band = 1 zone multiplier
    QCOMPARE(result.multiplierCounts[MultiplierType::CQZone], 1);
}

void TestScoreCalculationService::testCQWPX_AllBandsPrefix_CountsOnce()
{
    ScoreCalculationService svc;
    auto station = makeUSStation();
    CQWPXCWContest contest(station);

    // Same prefix "DL" on 20m and 15m — AllBands, so counts as 1 prefix multiplier
    QSO qso1 = makeQSO("DL1ABC", BandType::Band20M, ModeType::CW, 14, "EU", "Germany", 3, true);
    qso1.dxccPrefix = "DL1";
    QSO qso2 = makeQSO("DL1XYZ", BandType::Band15M, ModeType::CW, 14, "EU", "Germany", 3, false);
    qso2.dxccPrefix = "DL1";
    QList<QSO> qsos = {qso1, qso2};

    ScoreResult result = svc.calculateScore(qsos, &contest);

    // AllBands prefix: DL1 across two bands = 1 prefix mult
    QVERIFY(result.multiplierCounts.contains(MultiplierType::Prefix));
    QCOMPARE(result.multiplierCounts[MultiplierType::Prefix], 1);
}

void TestScoreCalculationService::testCQWPX_DifferentPrefixes_CountSeparately()
{
    ScoreCalculationService svc;
    auto station = makeUSStation();
    CQWPXCWContest contest(station);

    QSO qso1 = makeQSO("DL1ABC", BandType::Band20M, ModeType::CW, 14, "EU", "Germany", 3, true);
    qso1.dxccPrefix = "DL1";
    QSO qso2 = makeQSO("G3XYZ", BandType::Band20M, ModeType::CW, 14, "EU", "England", 3, true);
    qso2.dxccPrefix = "G3";
    QList<QSO> qsos = {qso1, qso2};

    ScoreResult result = svc.calculateScore(qsos, &contest);

    // Two different prefixes = 2 prefix mults
    QVERIFY(result.multiplierCounts.contains(MultiplierType::Prefix));
    QCOMPARE(result.multiplierCounts[MultiplierType::Prefix], 2);
}

void TestScoreCalculationService::testBandPointsAccumulation()
{
    ScoreCalculationService svc;
    auto station = makeUSStation();
    CQWWCWContest contest(station);

    QSO qso1 = makeQSO("DL1ABC", BandType::Band20M, ModeType::CW, 14, "EU", "Germany", 3);
    QSO qso2 = makeQSO("JA1ABC", BandType::Band20M, ModeType::CW, 25, "AS", "Japan", 3);
    QSO qso3 = makeQSO("VK2ABC", BandType::Band15M, ModeType::CW, 30, "OC", "Australia", 3);
    QList<QSO> qsos = {qso1, qso2, qso3};

    ScoreResult result = svc.calculateScore(qsos, &contest);

    QCOMPARE(result.bandStats[BandType::Band20M].points, 6);  // 3 + 3
    QCOMPARE(result.bandStats[BandType::Band15M].points, 3);
    QCOMPARE(result.bandStats[BandType::Band20M].qsoCount, 2);
    QCOMPARE(result.bandStats[BandType::Band15M].qsoCount, 1);
}

void TestScoreCalculationService::testTotalQSOPoints()
{
    ScoreCalculationService svc;
    auto station = makeUSStation();
    CQWWCWContest contest(station);

    QSO qso1 = makeQSO("DL1ABC", BandType::Band20M, ModeType::CW, 14, "EU", "Germany", 3);
    QSO qso2 = makeQSO("JA1ABC", BandType::Band15M, ModeType::CW, 25, "AS", "Japan", 3);
    QList<QSO> qsos = {qso1, qso2};

    ScoreResult result = svc.calculateScore(qsos, &contest);

    QCOMPARE(result.totalQSOPoints, 6);
}

void TestScoreCalculationService::testCQWW_FinalScoreIsPointsTimesMultipliers()
{
    ScoreCalculationService svc;
    auto station = makeUSStation();
    CQWWCWContest contest(station);

    // 2 QSOs: both zone 14 on 20m, EU countries (1 zone mult + 2 country mults)
    QSO qso1 = makeQSO("DL1ABC", BandType::Band20M, ModeType::CW, 14, "EU", "Germany", 3, true);
    QSO qso2 = makeQSO("G3ABC", BandType::Band20M, ModeType::CW, 14, "EU", "England", 3, true);
    QList<QSO> qsos = {qso1, qso2};

    ScoreResult result = svc.calculateScore(qsos, &contest);

    // finalScore = totalQSOPoints × totalMultipliers (CQ WW formula)
    QCOMPARE(result.finalScore, result.totalQSOPoints * result.totalMultipliers);
}

void TestScoreCalculationService::testGetStandardBands()
{
    QList<BandType> bands = ScoreCalculationService::getStandardBands();

    QCOMPARE(bands.size(), 6);
    QVERIFY(bands.contains(BandType::Band160M));
    QVERIFY(bands.contains(BandType::Band80M));
    QVERIFY(bands.contains(BandType::Band40M));
    QVERIFY(bands.contains(BandType::Band20M));
    QVERIFY(bands.contains(BandType::Band15M));
    QVERIFY(bands.contains(BandType::Band10M));
}

QTEST_MAIN(TestScoreCalculationService)
#include "test_score_calculation_service.moc"
