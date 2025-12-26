#include <QtTest>
#include <QElapsedTimer>
#include "../src/models/QSO.h"
#include "../src/core/Types.h"

using namespace TR4QT;

/**
 * Performance test for score calculation
 * Simulates updateScoreDisplay() logic to measure time with varying QSO counts
 */
class TestScorePerformance : public QObject {
    Q_OBJECT

private slots:
    void benchmarkScoreCalculation_data();
    void benchmarkScoreCalculation();

private:
    QSO createRandomQSO(int index);
    void calculateScores(const QList<QSO>& qsos);
};

QSO TestScorePerformance::createRandomQSO(int index) {
    QSO qso;

    // Rotate through bands
    static QList<BandType> bands = {
        BandType::Band160M, BandType::Band80M, BandType::Band40M,
        BandType::Band20M, BandType::Band15M, BandType::Band10M
    };
    qso.band = bands[index % bands.size()];

    // Random callsign
    qso.callsign = QString("W%1ABC").arg(index % 10);

    // Random DXCC prefix
    static QStringList prefixes = {"W", "K", "VE", "G", "DL", "JA", "VK", "ZL", "PY", "LU"};
    qso.dxccPrefix = prefixes[index % prefixes.size()];

    // Random CQ zone
    qso.cqZone = (index % 40) + 1;  // Zones 1-40

    // Random points (1-3 points typical)
    qso.qsoPoints = (index % 3) + 1;

    qso.timestamp = QDateTime::currentDateTimeUtc();
    qso.mode = ModeType::CW;
    qso.frequency = 14000000;

    return qso;
}

void TestScorePerformance::calculateScores(const QList<QSO>& qsos) {
    // Simulate updateScoreDisplay() logic
    QMap<BandType, int> qsosPerBand;
    QMap<BandType, int> pointsPerBand;
    QMap<BandType, QSet<QString>> multsPerBand;
    QMap<BandType, QSet<int>> zonesPerBand;

    int totalQSOs = 0;
    int totalPoints = 0;

    for (const QSO& qso : qsos) {
        if (qso.band == BandType::None) {
            continue;
        }

        qsosPerBand[qso.band]++;
        totalQSOs++;

        pointsPerBand[qso.band] += qso.qsoPoints;
        totalPoints += qso.qsoPoints;

        if (!qso.dxccPrefix.isEmpty()) {
            multsPerBand[qso.band].insert(qso.dxccPrefix);
        }

        if (qso.cqZone > 0) {
            zonesPerBand[qso.band].insert(qso.cqZone);
        }
    }

    // Prevent optimization away
    Q_UNUSED(totalQSOs);
    Q_UNUSED(totalPoints);
}

void TestScorePerformance::benchmarkScoreCalculation_data() {
    QTest::addColumn<int>("qsoCount");

    // Test with realistic QSO counts
    QTest::newRow("100 QSOs") << 100;
    QTest::newRow("500 QSOs") << 500;
    QTest::newRow("1000 QSOs") << 1000;
    QTest::newRow("2000 QSOs") << 2000;
    QTest::newRow("5000 QSOs") << 5000;     // Large multi-op contest
    QTest::newRow("10000 QSOs") << 10000;   // Extreme case
}

void TestScorePerformance::benchmarkScoreCalculation() {
    QFETCH(int, qsoCount);

    // Generate test QSOs
    QList<QSO> qsos;
    qsos.reserve(qsoCount);
    for (int i = 0; i < qsoCount; ++i) {
        qsos.append(createRandomQSO(i));
    }

    // Measure time for single calculation
    QElapsedTimer timer;
    timer.start();
    calculateScores(qsos);
    qint64 elapsed = timer.nsecsElapsed();

    double milliseconds = elapsed / 1000000.0;

    qInfo() << QString("%1 QSOs: %2 ms")
        .arg(qsoCount, 5)
        .arg(milliseconds, 0, 'f', 3);

    // Benchmark using Qt's QBENCHMARK
    QBENCHMARK {
        calculateScores(qsos);
    }
}

QTEST_MAIN(TestScorePerformance)
#include "test_score_performance.moc"
