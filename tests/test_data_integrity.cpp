#include <QTest>
#include "../src/controllers/DataIntegrityManager.h"
#include "../src/data/Database.h"
#include "../src/data/QSORepository.h"
#include "../src/contests/CQWWContest.h"
#include "../src/utils/CountryFile.h"
#include "TestHelpers.h"

using namespace TR4QT;

/**
 * Tests for DataIntegrityManager
 *
 * Verifies:
 * - Quick integrity check (count comparison)
 * - Full integrity check (detailed report)
 * - Contest rescoring (points, multipliers, duplicates)
 */
class TestDataIntegrity : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void init();
    void cleanup();

    // Quick integrity check tests
    void testQuickIntegrityCheck_CountsMatch_ReturnsTrue();
    void testQuickIntegrityCheck_CountMismatch_ReturnsFalse();

    // Full integrity check tests
    void testFullIntegrityCheck_AllValid_ReturnsReport();

    // Rescore tests
    void testRescoreContest_RecalculatesPoints();
    void testRescoreContest_MarksDuplicates();

private:
    DataIntegrityManager* m_manager = nullptr;
    CountryFile* m_countryFile = nullptr;
    CQWWContest* m_contest = nullptr;
    QString m_dbPath;
    int m_contestId = -1;
};

void TestDataIntegrity::initTestCase()
{
    // Create temporary database
    m_dbPath = TestHelpers::createTempDatabasePath("data_integrity");

    // Load country file
    m_countryFile = new CountryFile();
    QString ctyPath = QString("%1/../fixtures/cty.dat").arg(TESTS_SOURCE_DIR);
    QVERIFY2(m_countryFile->loadFromFile(ctyPath), "Could not load cty.dat");

    // Create contest
    StationInfo myStation = TestHelpers::createTestStation();
    m_contest = new CQWWContest(ModeType::CW, myStation);
}

void TestDataIntegrity::cleanupTestCase()
{
    delete m_manager;
    delete m_contest;
    delete m_countryFile;

    // Clean up database
    Database::instance().close();
    QFile::remove(m_dbPath);
}

void TestDataIntegrity::init()
{
    // Open database and create contest
    Database& db = Database::instance();
    QVERIFY(db.open(m_dbPath));

    // Create contest record
    QSqlQuery query(db.connection());
    query.prepare(R"(
        INSERT INTO contests (contest_id, contest_name, my_call, start_time, created_at, contest_type)
        VALUES (?, ?, ?, ?, ?, ?)
    )");
    query.addBindValue("TEST_CONTEST");
    query.addBindValue("Test Contest");
    query.addBindValue("NY4I");
    query.addBindValue(QDateTime::currentSecsSinceEpoch());
    query.addBindValue(QDateTime::currentSecsSinceEpoch());
    query.addBindValue("CQWW_CW");
    QVERIFY2(query.exec(), qPrintable(query.lastError().text()));
    m_contestId = query.lastInsertId().toInt();

    // Create DataIntegrityManager
    DataIntegrityManager::Config config;
    config.countryFile = m_countryFile;
    config.currentContestDbId = m_contestId;
    m_manager = new DataIntegrityManager(config);
}

void TestDataIntegrity::cleanup()
{
    delete m_manager;
    m_manager = nullptr;

    Database::instance().close();
    QFile::remove(m_dbPath);
}

void TestDataIntegrity::testQuickIntegrityCheck_CountsMatch_ReturnsTrue()
{
    // Given: 3 QSOs in database
    QSORepository repo;
    for (int i = 0; i < 3; i++) {
        QSO qso = TestHelpers::createValidQSO(QString("W1AW%1").arg(i));
        QVERIFY(repo.saveQSO(qso, m_contestId));
    }

    // When: Running quick integrity check with matching count
    DataIntegrityManager::QuickCheckResult result = m_manager->quickIntegrityCheck(3);

    // Then: Check passes
    QVERIFY(result.passed);
}

void TestDataIntegrity::testQuickIntegrityCheck_CountMismatch_ReturnsFalse()
{
    // Given: 2 QSOs in database
    QSORepository repo;
    for (int i = 0; i < 2; i++) {
        QSO qso = TestHelpers::createValidQSO(QString("W1AW%1").arg(i));
        QVERIFY(repo.saveQSO(qso, m_contestId));
    }

    // When: Running quick integrity check with mismatched count (memory says 5)
    DataIntegrityManager::QuickCheckResult result = m_manager->quickIntegrityCheck(5);

    // Then: Check fails
    QVERIFY(!result.passed);
}

void TestDataIntegrity::testFullIntegrityCheck_AllValid_ReturnsReport()
{
    // Given: 2 valid QSOs in database and memory
    QList<QSO> memoryQSOs;
    QSORepository repo;
    for (int i = 0; i < 2; i++) {
        QSO qso = TestHelpers::createValidQSO(QString("W1AW%1").arg(i));
        QVERIFY(repo.saveQSO(qso, m_contestId));
        memoryQSOs.append(qso);
    }

    // When: Running full integrity check
    QString report = m_manager->fullIntegrityCheck(memoryQSOs, false);

    // Then: Report generated with checks passed
    QVERIFY(report.contains("QSO count matches"));
    QVERIFY(report.contains("All memory QSOs exist in database"));
    QVERIFY(report.contains("No orphaned QSOs in database"));
}

void TestDataIntegrity::testRescoreContest_RecalculatesPoints()
{
    // Given: QSO with incorrect points, saved to database
    QSO qso = TestHelpers::createValidQSO("G3XYZ", BandType::Band20M, ModeType::CW);
    qso.qsoPoints = 999;  // Wrong!
    qso.dxccEntity = "United Kingdom";
    qso.continent = "EU";

    // Save to database first (required for rescore to update)
    QSORepository repo;
    QVERIFY2(repo.saveQSO(qso, m_contestId), "Failed to save QSO to database");
    QVERIFY2(qso.id > 0, "QSO should have valid ID after save");

    QList<QSO> qsos;
    qsos.append(qso);

    StationInfo myStation = TestHelpers::createTestStation("NY4I", "NA");
    myStation.country = "United States";

    // When: Rescoring
    RescoreStats stats = m_manager->rescoreContestSilent(qsos, m_contest, myStation);

    // Then: Points recalculated (US->UK on CW = 3 points)
    QCOMPARE(qsos[0].qsoPoints, 3);
    QCOMPARE(stats.qsosUpdated, 1);
}

void TestDataIntegrity::testRescoreContest_MarksDuplicates()
{
    // Given: Two identical QSOs (same call, band, mode)
    QSO qso1 = TestHelpers::createValidQSO("W1AW", BandType::Band20M, ModeType::CW);
    QSO qso2 = TestHelpers::createValidQSO("W1AW", BandType::Band20M, ModeType::CW);

    QList<QSO> qsos;
    qsos.append(qso1);
    qsos.append(qso2);

    StationInfo myStation = TestHelpers::createTestStation();

    // When: Rescoring
    RescoreStats stats = m_manager->rescoreContestSilent(qsos, m_contest, myStation);

    // Then: Second QSO marked as dupe with 0 points
    QVERIFY(!qsos[0].isDupe);  // First is not dupe
    QVERIFY(qsos[1].isDupe);   // Second is dupe
    QCOMPARE(qsos[1].qsoPoints, 0);  // Dupes get 0 points
    QCOMPARE(stats.dupesFound, 1);
}

QTEST_MAIN(TestDataIntegrity)
#include "test_data_integrity.moc"
