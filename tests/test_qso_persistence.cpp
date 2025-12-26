#include <QtTest>
#include "../src/models/QSO.h"
#include "../src/data/QSORepository.h"
#include "../src/data/Database.h"
#include <QTemporaryDir>

using namespace TR4QT;

/**
 * Test QSO persistence - verify all fields are saved and loaded correctly
 */
class TestQSOPersistence : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void init();
    void cleanup();

    // Test cases
    void testAllFieldsPersisted();
    void testOperatorFieldPersisted();
    void testMultipliersPersisted();
    void testEmptyFieldsPersisted();

private:
    QTemporaryDir* m_tempDir{nullptr};
    QString m_dbPath;
    int m_contestId{-1};

    QSO createTestQSO();
    void compareQSOs(const QSO& original, const QSO& loaded);
};

void TestQSOPersistence::initTestCase() {
    // Create temporary directory for test database
    m_tempDir = new QTemporaryDir();
    QVERIFY(m_tempDir->isValid());
    m_dbPath = m_tempDir->path() + "/test_persistence.db";
}

void TestQSOPersistence::cleanupTestCase() {
    delete m_tempDir;
}

void TestQSOPersistence::init() {
    // Create fresh database for each test
    Database& db = Database::instance();
    QVERIFY(db.open(m_dbPath));

    // Create a test contest
    QSqlQuery query(db.connection());
    query.prepare(R"(
        INSERT INTO contests (name, mode, start_time)
        VALUES (?, ?, ?)
    )");
    query.addBindValue("Test Contest");
    query.addBindValue("CW");
    query.addBindValue(QDateTime::currentSecsSinceEpoch());
    QVERIFY(query.exec());
    m_contestId = query.lastInsertId().toInt();

    // Leave database open for the test
}

void TestQSOPersistence::cleanup() {
    // Close database and remove test database file
    Database::instance().close();
    QFile::remove(m_dbPath);
}

QSO TestQSOPersistence::createTestQSO() {
    QSO qso;

    // Basic fields
    qso.timestamp = QDateTime::fromString("2025-12-25T20:30:00Z", Qt::ISODate);
    qso.callsign = "W1AW";
    qso.frequency = 14000000;  // 14 MHz
    qso.mode = ModeType::CW;
    qso.band = BandType::Band20M;

    // Exchange
    qso.rstSent = "599";
    qso.rstReceived = "579";
    qso.exchangeSent = "599 001";
    qso.exchangeReceived = "579 14";

    // DXCC/Geographic
    qso.dxccEntity = "United States";
    qso.dxccPrefix = "K";
    qso.cqZone = 5;
    qso.ituZone = 8;
    qso.continent = "NA";
    qso.state = "CT";
    qso.county = "Hartford";
    qso.arrlSection = "CT";

    // Scoring
    qso.qsoPoints = 3;
    qso.isDupe = false;
    qso.isMultiplier = true;
    qso.multipliers << "K" << "5";

    // Metadata
    qso.serialNumber = 1;
    qso.operatorCall = "NY4I";
    qso.deleted = false;
    qso.notes = "Test QSO notes";

    return qso;
}

void TestQSOPersistence::compareQSOs(const QSO& original, const QSO& loaded) {
    // Basic fields
    QVERIFY(loaded.id >= 0);  // Should have database ID
    QCOMPARE(loaded.timestamp, original.timestamp);
    QCOMPARE(loaded.callsign, original.callsign);
    QCOMPARE(loaded.frequency, original.frequency);
    QCOMPARE(loaded.mode, original.mode);
    QCOMPARE(loaded.band, original.band);

    // Exchange
    QCOMPARE(loaded.rstSent, original.rstSent);
    QCOMPARE(loaded.rstReceived, original.rstReceived);
    QCOMPARE(loaded.exchangeSent, original.exchangeSent);
    QCOMPARE(loaded.exchangeReceived, original.exchangeReceived);

    // DXCC/Geographic
    QCOMPARE(loaded.dxccEntity, original.dxccEntity);
    QCOMPARE(loaded.dxccPrefix, original.dxccPrefix);
    QCOMPARE(loaded.cqZone, original.cqZone);
    QCOMPARE(loaded.ituZone, original.ituZone);
    QCOMPARE(loaded.continent, original.continent);
    QCOMPARE(loaded.state, original.state);
    QCOMPARE(loaded.county, original.county);
    QCOMPARE(loaded.arrlSection, original.arrlSection);

    // Scoring
    QCOMPARE(loaded.qsoPoints, original.qsoPoints);
    QCOMPARE(loaded.isDupe, original.isDupe);
    QCOMPARE(loaded.isMultiplier, original.isMultiplier);
    QCOMPARE(loaded.multipliers, original.multipliers);

    // Metadata
    QCOMPARE(loaded.serialNumber, original.serialNumber);
    QCOMPARE(loaded.operatorCall, original.operatorCall);  // This was the bug!
    QCOMPARE(loaded.deleted, original.deleted);
    QCOMPARE(loaded.notes, original.notes);
}

void TestQSOPersistence::testAllFieldsPersisted() {
    // Create and save QSO
    QSO original = createTestQSO();

    QSORepository repo;
    QVERIFY(repo.saveQSO(original, m_contestId));
    QVERIFY(original.id >= 0);  // Should have ID after save

    // Load QSO back
    QSO loaded = repo.findById(original.id);

    // Compare all fields
    compareQSOs(original, loaded);
}

void TestQSOPersistence::testOperatorFieldPersisted() {
    // Specific test for operator field (the bug we just fixed)
    QSO qso;
    qso.timestamp = QDateTime::currentDateTimeUtc();
    qso.callsign = "K1XYZ";
    qso.frequency = 14000000;
    qso.mode = ModeType::CW;
    qso.band = BandType::Band20M;
    qso.operatorCall = "NY4I";  // This field wasn't being loaded!

    QSORepository repo;
    QVERIFY(repo.saveQSO(qso, m_contestId));

    // Load back and verify operator
    QSO loaded = repo.findById(qso.id);
    QCOMPARE(loaded.operatorCall, QString("NY4I"));
}

void TestQSOPersistence::testMultipliersPersisted() {
    // Test multipliers array persistence
    QSO qso = createTestQSO();
    qso.multipliers.clear();
    qso.multipliers << "W" << "5" << "NA";

    QSORepository repo;
    QVERIFY(repo.saveQSO(qso, m_contestId));

    QSO loaded = repo.findById(qso.id);
    QCOMPARE(loaded.multipliers.size(), 3);
    QVERIFY(loaded.multipliers.contains("W"));
    QVERIFY(loaded.multipliers.contains("5"));
    QVERIFY(loaded.multipliers.contains("NA"));
}

void TestQSOPersistence::testEmptyFieldsPersisted() {
    // Test that empty/null fields are handled correctly
    QSO qso;
    qso.timestamp = QDateTime::currentDateTimeUtc();
    qso.callsign = "AA1ZZZ";
    qso.frequency = 7000000;
    qso.mode = ModeType::CW;
    qso.band = BandType::Band40M;
    // Leave most other fields empty

    QSORepository repo;
    QVERIFY(repo.saveQSO(qso, m_contestId));

    QSO loaded = repo.findById(qso.id);
    QCOMPARE(loaded.callsign, QString("AA1ZZZ"));
    QCOMPARE(loaded.operatorCall, QString(""));  // Should be empty, not null
    QCOMPARE(loaded.notes, QString(""));
    QCOMPARE(loaded.state, QString(""));
    QCOMPARE(loaded.serialNumber, 0);
    QCOMPARE(loaded.qsoPoints, 0);
}

QTEST_MAIN(TestQSOPersistence)
#include "test_qso_persistence.moc"
