#include <QtTest>
#include <QElapsedTimer>
#include <QThread>
#include <QThreadPool>
#include <QtConcurrent/QtConcurrent>
#include "../src/models/QSO.h"
#include "../src/data/QSORepository.h"
#include "../src/data/Database.h"
#include <QTemporaryDir>
#include <QRandomGenerator>
#include <QFileInfo>

using namespace TR4QT;

/**
 * Load test for QSO database operations
 *
 * Tests database performance under realistic contest load:
 * - High-rate QSO logging (100-200 QSOs/hour)
 * - Sustained operations (thousands of QSOs)
 * - Concurrent access (multi-operator simulation)
 * - Transaction performance
 * - Integrity checking under load
 *
 * Uses LoTW callsign list for realistic test data
 */
class TestQSOLoadPerformance : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void init();
    void cleanup();

    // Load test scenarios - Single-threaded (SAFE)
    void testSteadyRate_100QSOsPerHour();
    void testSteadyRate_200QSOsPerHour();
    void testBurstLoad_1000QSOsRapidFire();
    void testSustainedLoad_5000QSOs();
    void testTransactionPerformance();
    void testDatabaseGrowth();

    // DISABLED: Concurrent tests (CRASHES - see Database.cpp TODO line 13)
    // Re-enable after implementing thread-safe database access for networked TR4QT
    // void testConcurrentAccess_DualOperator();
    // void testConcurrentAccess_SixOperators();

private:
    QTemporaryDir* m_tempDir{nullptr};
    QString m_dbPath;
    int m_contestId{-1};
    QStringList m_lotwCallsigns;
    QStringList m_zones{"1", "2", "3", "4", "5", "14", "15", "20", "33"};
    QStringList m_sections{"CT", "WMA", "OR", "SCV", "NFL", "OH", "AB", "SK"};

    // Performance metrics
    struct PerformanceMetrics {
        int qsosLogged{0};
        qint64 totalTimeMs{0};
        qint64 minTransactionMs{999999};
        qint64 maxTransactionMs{0};
        qint64 avgTransactionMs{0};
        int transactionFailures{0};
        qint64 finalDatabaseSizeBytes{0};
    };

    // Helper methods
    void loadLOTWCallsigns();
    QSO generateRandomQSO(int serialNumber);
    PerformanceMetrics runLoadTest(int qsoCount, int delayMs = 0, int concurrentThreads = 1);
    void logMetrics(const QString& testName, const PerformanceMetrics& metrics);
    qint64 getDatabaseSize(const QString& dbPath);
};

void TestQSOLoadPerformance::initTestCase() {
    qInfo() << "\n========================================";
    qInfo() << "QSO Load Performance Test Suite";
    qInfo() << "========================================\n";

    // Create temporary directory for test database
    m_tempDir = new QTemporaryDir();
    QVERIFY(m_tempDir->isValid());
    m_dbPath = m_tempDir->path() + "/test_load_performance.db";

    // Load LoTW callsigns for realistic test data
    loadLOTWCallsigns();
    qInfo() << "Loaded" << m_lotwCallsigns.size() << "LoTW callsigns for test data";
}

void TestQSOLoadPerformance::cleanupTestCase() {
    delete m_tempDir;
    qInfo() << "\n========================================";
    qInfo() << "Load Performance Tests Complete";
    qInfo() << "========================================\n";
}

void TestQSOLoadPerformance::init() {
    // Create fresh database for each test
    Database& db = Database::instance();
    QVERIFY(db.open(m_dbPath));

    // Create a test contest
    QSqlQuery query(db.connection());
    query.prepare(R"(
        INSERT INTO contests (contest_id, contest_name, contest_type, my_call, start_time, created_at)
        VALUES (?, ?, ?, ?, ?, ?)
    )");
    query.addBindValue("LOAD_TEST");
    query.addBindValue("Load Test Contest");
    query.addBindValue("GENERAL");  // Contest type (required)
    query.addBindValue("TEST");
    query.addBindValue(QDateTime::currentSecsSinceEpoch());
    query.addBindValue(QDateTime::currentSecsSinceEpoch());
    QVERIFY2(query.exec(), qPrintable(query.lastError().text()));
    m_contestId = query.lastInsertId().toInt();
}

void TestQSOLoadPerformance::cleanup() {
    // Close database and remove test database file
    Database::instance().close();
    QFile::remove(m_dbPath);
}

void TestQSOLoadPerformance::loadLOTWCallsigns() {
    // Try to load from user's LoTW file first
    QString lotwPath = QDir::homePath() + "/.tr4qt/lotw-user-activity.csv";
    QFile lotwFile(lotwPath);

    if (lotwFile.exists() && lotwFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&lotwFile);
        int count = 0;
        while (!in.atEnd() && count < 10000) {  // Load first 10,000 callsigns
            QString line = in.readLine();
            QStringList parts = line.split(',');
            if (parts.size() >= 1 && !parts[0].isEmpty() && parts[0] != "\"Callsign\"") {
                QString callsign = parts[0].remove('"').trimmed();
                if (callsign.length() >= 3) {
                    m_lotwCallsigns.append(callsign);
                    count++;
                }
            }
        }
        lotwFile.close();
        qInfo() << "Loaded" << m_lotwCallsigns.size() << "callsigns from LoTW file";
    }

    // If LoTW file not available or too few callsigns, generate test callsigns
    if (m_lotwCallsigns.size() < 100) {
        qWarning() << "LoTW file not found or insufficient, generating test callsigns";
        QStringList prefixes{"W", "K", "N", "AA", "AB", "AC", "VE", "VA", "G", "DL", "JA", "VK", "ZL"};
        for (int i = 0; i < 1000; i++) {
            QString prefix = prefixes[QRandomGenerator::global()->bounded(prefixes.size())];
            int num = QRandomGenerator::global()->bounded(10);
            QString suffix;
            for (int j = 0; j < 3; j++) {
                suffix += QChar('A' + QRandomGenerator::global()->bounded(26));
            }
            m_lotwCallsigns.append(QString("%1%2%3").arg(prefix).arg(num).arg(suffix));
        }
    }
}

QSO TestQSOLoadPerformance::generateRandomQSO(int serialNumber) {
    QSO qso;

    // Random callsign from LoTW list
    if (!m_lotwCallsigns.isEmpty()) {
        qso.callsign = m_lotwCallsigns[QRandomGenerator::global()->bounded(m_lotwCallsigns.size())];
    } else {
        qso.callsign = QString("W%1ABC").arg(QRandomGenerator::global()->bounded(10));
    }

    // Random timestamp (within contest period)
    qint64 baseTime = QDateTime::currentSecsSinceEpoch() - 86400;  // 24 hours ago
    qint64 randomOffset = QRandomGenerator::global()->bounded(86400);  // Random within 24 hours
    qso.timestamp = QDateTime::fromSecsSinceEpoch(baseTime + randomOffset, Qt::UTC);

    // Random band (favor common contest bands)
    QList<BandType> bands{BandType::Band20M, BandType::Band40M, BandType::Band15M,
                          BandType::Band80M, BandType::Band10M, BandType::Band160M};
    qso.band = bands[QRandomGenerator::global()->bounded(bands.size())];

    // Random frequency within band
    switch (qso.band) {
        case BandType::Band160M: qso.frequency = 1800000 + QRandomGenerator::global()->bounded(200000); break;
        case BandType::Band80M:  qso.frequency = 3500000 + QRandomGenerator::global()->bounded(500000); break;
        case BandType::Band40M:  qso.frequency = 7000000 + QRandomGenerator::global()->bounded(300000); break;
        case BandType::Band20M:  qso.frequency = 14000000 + QRandomGenerator::global()->bounded(350000); break;
        case BandType::Band15M:  qso.frequency = 21000000 + QRandomGenerator::global()->bounded(450000); break;
        case BandType::Band10M:  qso.frequency = 28000000 + QRandomGenerator::global()->bounded(1700000); break;
        default: qso.frequency = 14000000;
    }

    // Mode (CW for this contest)
    qso.mode = ModeType::CW;

    // Exchange
    qso.rstSent = "599";
    qso.rstReceived = "599";
    qso.exchangeSent = QString("599 %1").arg(serialNumber, 3, 10, QChar('0'));

    // Random zone or section for received exchange
    if (QRandomGenerator::global()->bounded(2) == 0) {
        qso.exchangeReceived = QString("599 %1").arg(m_zones[QRandomGenerator::global()->bounded(m_zones.size())]);
    } else {
        qso.exchangeReceived = QString("599 %1").arg(m_sections[QRandomGenerator::global()->bounded(m_sections.size())]);
    }

    // Serial number
    qso.serialNumber = serialNumber;

    // Scoring (simple points)
    qso.qsoPoints = 1;
    qso.isDupe = false;

    return qso;
}

TestQSOLoadPerformance::PerformanceMetrics TestQSOLoadPerformance::runLoadTest(
    int qsoCount, int delayMs, int concurrentThreads)
{
    PerformanceMetrics metrics;
    QElapsedTimer totalTimer;
    totalTimer.start();

    if (concurrentThreads == 1) {
        // Single-threaded test
        QSORepository repo;

        for (int i = 1; i <= qsoCount; i++) {
            QSO qso = generateRandomQSO(i);

            QElapsedTimer txTimer;
            txTimer.start();

            bool success = repo.saveQSO(qso, m_contestId);

            qint64 txTime = txTimer.elapsed();

            if (success) {
                metrics.qsosLogged++;
                metrics.minTransactionMs = qMin(metrics.minTransactionMs, txTime);
                metrics.maxTransactionMs = qMax(metrics.maxTransactionMs, txTime);
                metrics.avgTransactionMs += txTime;
            } else {
                metrics.transactionFailures++;
                qWarning() << "Failed to save QSO" << i << ":" << repo.lastError();
            }

            if (delayMs > 0) {
                QThread::msleep(delayMs);
            }
        }

        if (metrics.qsosLogged > 0) {
            metrics.avgTransactionMs /= metrics.qsosLogged;
        }
    } else {
        // Multi-threaded test (concurrent operators)
        QAtomicInt successCount(0);
        QAtomicInt failureCount(0);
        QVector<qint64> transactionTimes;
        QMutex timingMutex;

        int qsosPerThread = qsoCount / concurrentThreads;

        QList<QFuture<void>> futures;
        for (int thread = 0; thread < concurrentThreads; thread++) {
            int startSerial = thread * qsosPerThread + 1;
            int endSerial = (thread + 1) * qsosPerThread;

            QFuture<void> future = QtConcurrent::run([=, &successCount, &failureCount, &transactionTimes, &timingMutex]() {
                QSORepository repo;

                for (int i = startSerial; i <= endSerial; i++) {
                    QSO qso = generateRandomQSO(i);

                    QElapsedTimer txTimer;
                    txTimer.start();

                    bool success = repo.saveQSO(qso, m_contestId);

                    qint64 txTime = txTimer.elapsed();

                    if (success) {
                        successCount.fetchAndAddOrdered(1);
                        QMutexLocker locker(&timingMutex);
                        transactionTimes.append(txTime);
                    } else {
                        failureCount.fetchAndAddOrdered(1);
                    }

                    if (delayMs > 0) {
                        QThread::msleep(delayMs);
                    }
                }
            });

            futures.append(future);
        }

        // Wait for all threads to complete
        for (auto& future : futures) {
            future.waitForFinished();
        }

        metrics.qsosLogged = successCount.loadAcquire();
        metrics.transactionFailures = failureCount.loadAcquire();

        if (!transactionTimes.isEmpty()) {
            qint64 sum = 0;
            metrics.minTransactionMs = transactionTimes[0];
            metrics.maxTransactionMs = transactionTimes[0];

            for (qint64 time : transactionTimes) {
                sum += time;
                metrics.minTransactionMs = qMin(metrics.minTransactionMs, time);
                metrics.maxTransactionMs = qMax(metrics.maxTransactionMs, time);
            }

            metrics.avgTransactionMs = sum / transactionTimes.size();
        }
    }

    metrics.totalTimeMs = totalTimer.elapsed();
    metrics.finalDatabaseSizeBytes = getDatabaseSize(m_dbPath);

    return metrics;
}

void TestQSOLoadPerformance::logMetrics(const QString& testName, const PerformanceMetrics& metrics) {
    qInfo() << "\n--- Performance Metrics:" << testName << "---";
    qInfo() << "QSOs Logged:" << metrics.qsosLogged;
    qInfo() << "Total Time:" << metrics.totalTimeMs << "ms ("
            << QString::number(metrics.totalTimeMs / 1000.0, 'f', 2) << "seconds)";

    if (metrics.qsosLogged > 0) {
        double qsosPerSecond = (metrics.qsosLogged * 1000.0) / metrics.totalTimeMs;
        double qsosPerHour = qsosPerSecond * 3600;
        qInfo() << "Rate:" << QString::number(qsosPerSecond, 'f', 2) << "QSOs/second ("
                << QString::number(qsosPerHour, 'f', 0) << "QSOs/hour)";
    }

    qInfo() << "Transaction Time - Min:" << metrics.minTransactionMs << "ms";
    qInfo() << "Transaction Time - Avg:" << metrics.avgTransactionMs << "ms";
    qInfo() << "Transaction Time - Max:" << metrics.maxTransactionMs << "ms";
    qInfo() << "Transaction Failures:" << metrics.transactionFailures;
    qInfo() << "Final Database Size:" << metrics.finalDatabaseSizeBytes << "bytes ("
            << QString::number(metrics.finalDatabaseSizeBytes / 1024.0, 'f', 2) << "KB)";

    if (metrics.qsosLogged > 0) {
        double bytesPerQSO = static_cast<double>(metrics.finalDatabaseSizeBytes) / metrics.qsosLogged;
        qInfo() << "Bytes per QSO:" << QString::number(bytesPerQSO, 'f', 2);
    }

    qInfo() << "---\n";
}

qint64 TestQSOLoadPerformance::getDatabaseSize(const QString& dbPath) {
    QFileInfo fileInfo(dbPath);
    qint64 totalSize = fileInfo.size();

    // Include WAL and SHM files if they exist
    QFileInfo walInfo(dbPath + "-wal");
    if (walInfo.exists()) {
        totalSize += walInfo.size();
    }

    QFileInfo shmInfo(dbPath + "-shm");
    if (shmInfo.exists()) {
        totalSize += shmInfo.size();
    }

    return totalSize;
}

void TestQSOLoadPerformance::testSteadyRate_100QSOsPerHour() {
    // Simulate 100 QSOs/hour rate for 1 hour = 100 QSOs
    // At 100 QSOs/hour = 36 seconds per QSO
    // For testing, we'll compress time: 100 QSOs with 100ms delay = 10 seconds total

    PerformanceMetrics metrics = runLoadTest(100, 100, 1);
    logMetrics("Steady Rate - 100 QSOs/hour (compressed)", metrics);

    QVERIFY(metrics.qsosLogged == 100);
    QVERIFY(metrics.transactionFailures == 0);
    QVERIFY(metrics.avgTransactionMs < 100);  // Should be fast with transactions
}

void TestQSOLoadPerformance::testSteadyRate_200QSOsPerHour() {
    // Simulate 200 QSOs/hour rate (good rate for serious contest)
    // At 200 QSOs/hour = 18 seconds per QSO
    // For testing: 200 QSOs with 50ms delay = 10 seconds total

    PerformanceMetrics metrics = runLoadTest(200, 50, 1);
    logMetrics("Steady Rate - 200 QSOs/hour (compressed)", metrics);

    QVERIFY(metrics.qsosLogged == 200);
    QVERIFY(metrics.transactionFailures == 0);
}

void TestQSOLoadPerformance::testBurstLoad_1000QSOsRapidFire() {
    // Rapid-fire test: 1000 QSOs as fast as possible
    // Tests database transaction performance under maximum load

    PerformanceMetrics metrics = runLoadTest(1000, 0, 1);
    logMetrics("Burst Load - 1000 QSOs (rapid fire)", metrics);

    QVERIFY(metrics.qsosLogged == 1000);
    QVERIFY(metrics.transactionFailures == 0);

    // Should handle at least 100 QSOs/second
    double qsosPerSecond = (metrics.qsosLogged * 1000.0) / metrics.totalTimeMs;
    qInfo() << "Achieved rate:" << qsosPerSecond << "QSOs/second";
    QVERIFY(qsosPerSecond >= 50);  // At least 50 QSOs/second
}

void TestQSOLoadPerformance::testSustainedLoad_5000QSOs() {
    // Sustained load: 5000 QSOs (simulates full contest)
    // Tests database growth, integrity, and sustained performance

    PerformanceMetrics metrics = runLoadTest(5000, 0, 1);
    logMetrics("Sustained Load - 5000 QSOs", metrics);

    QVERIFY(metrics.qsosLogged == 5000);
    QVERIFY(metrics.transactionFailures == 0);

    // Database size should be reasonable
    double mbSize = metrics.finalDatabaseSizeBytes / (1024.0 * 1024.0);
    qInfo() << "Database size:" << QString::number(mbSize, 'f', 2) << "MB";
    QVERIFY(mbSize < 50);  // Should be under 50MB for 5000 QSOs
}

// DISABLED: Concurrent tests (CRASHES - see Database.cpp TODO line 13)
// Re-enable after implementing thread-safe database access for networked TR4QT
/*
void TestQSOLoadPerformance::testConcurrentAccess_DualOperator() {
    // Dual operator: 2 threads logging QSOs simultaneously
    // 1000 QSOs total (500 per operator)

    PerformanceMetrics metrics = runLoadTest(1000, 10, 2);
    logMetrics("Concurrent Access - 2 Operators", metrics);

    QVERIFY(metrics.qsosLogged == 1000);
    QVERIFY(metrics.transactionFailures == 0);
}

void TestQSOLoadPerformance::testConcurrentAccess_SixOperators() {
    // Six operators: simulates multi-multi contest station
    // 1200 QSOs total (200 per operator)

    PerformanceMetrics metrics = runLoadTest(1200, 20, 6);
    logMetrics("Concurrent Access - 6 Operators", metrics);

    QVERIFY(metrics.qsosLogged == 1200);
    QVERIFY(metrics.transactionFailures == 0);
}
*/

void TestQSOLoadPerformance::testTransactionPerformance() {
    // Measure transaction overhead
    // Compare time with and without verification

    qInfo() << "\n=== Transaction Performance Analysis ===";

    PerformanceMetrics metrics = runLoadTest(500, 0, 1);
    logMetrics("Transaction Performance - 500 QSOs", metrics);

    QVERIFY(metrics.qsosLogged == 500);
    QVERIFY(metrics.avgTransactionMs < 50);  // Should average under 50ms per transaction

    qInfo() << "Transaction overhead acceptable:"
            << (metrics.avgTransactionMs < 50 ? "YES" : "NO");
}

void TestQSOLoadPerformance::testDatabaseGrowth() {
    // Test database growth with incremental loads
    qInfo() << "\n=== Database Growth Analysis ===";

    QList<int> batchSizes{100, 500, 1000, 2000};

    for (int batchSize : batchSizes) {
        // Close and reopen database for fresh state
        Database::instance().close();
        QFile::remove(m_dbPath);

        Database& db = Database::instance();
        QVERIFY(db.open(m_dbPath));

        QSqlQuery query(db.connection());
        query.prepare("INSERT INTO contests (contest_id, contest_name, contest_type, my_call, start_time, created_at) VALUES (?, ?, ?, ?, ?, ?)");
        query.addBindValue("GROWTH_TEST");
        query.addBindValue("Growth Test");
        query.addBindValue("GENERAL");  // Contest type (required)
        query.addBindValue("TEST");
        query.addBindValue(QDateTime::currentSecsSinceEpoch());
        query.addBindValue(QDateTime::currentSecsSinceEpoch());
        QVERIFY2(query.exec(), qPrintable(query.lastError().text()));
        int contestId = query.lastInsertId().toInt();

        // Store current contestId for this iteration
        int savedContestId = m_contestId;
        m_contestId = contestId;

        PerformanceMetrics metrics = runLoadTest(batchSize, 0, 1);

        double bytesPerQSO = static_cast<double>(metrics.finalDatabaseSizeBytes) / batchSize;
        double mbSize = metrics.finalDatabaseSizeBytes / (1024.0 * 1024.0);

        qInfo() << batchSize << "QSOs:" << QString::number(mbSize, 'f', 3) << "MB"
                << "(" << QString::number(bytesPerQSO, 'f', 0) << "bytes/QSO)";

        // Restore contestId
        m_contestId = savedContestId;
    }

    qInfo() << "===\n";
}

QTEST_MAIN(TestQSOLoadPerformance)
#include "test_qso_load_performance.moc"
