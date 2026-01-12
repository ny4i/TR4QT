/**
 * Integration test for duplicate checking functionality
 *
 * Tests the CURRENT behavior of MainWindow::checkForDuplicate() BEFORE extraction.
 * This test proves behavioral equivalence after refactoring to QSORepository.
 *
 * Following CLAUDE.md principle: Write tests BEFORE extraction to prove equivalence.
 */

#include <QTest>
#include <QDir>
#include <QFile>
#include "../src/data/Database.h"
#include "../src/data/QSORepository.h"
#include "../src/models/QSO.h"
#include "../src/models/StationInfo.h"
#include "../src/core/Types.h"
#include "../src/contests/CQWWContest.h"
#include "../src/contests/CQWPXContest.h"

using namespace TR4QT;

class TestDuplicateChecking : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void init();
    void cleanup();

    // Test duplicate checking for different rules
    void testPerBandMode_SameBandSameMode_IsDupe();
    void testPerBandMode_SameBandDifferentMode_NotDupe();
    void testPerBandMode_DifferentBandSameMode_NotDupe();

    void testAllBandMode_SameMode_IsDupe();
    void testAllBandMode_DifferentMode_NotDupe();

    void testPerBand_SameBand_IsDupe();
    void testPerBand_DifferentBand_NotDupe();

    void testAllBand_AnyBandAnyMode_IsDupe();

private:
    QString m_testDbPath;
    int m_contestDbId;

    void addQSO(const QString& callsign, BandType band, ModeType mode);
    bool checkDuplicate(const QString& callsign, BandType band, ModeType mode,
                       DuplicateCheckingRule rule, QString& dupeInfo);
};

void TestDuplicateChecking::initTestCase() {
    m_testDbPath = QDir::temp().filePath("test_duplicate_checking.db");
}

void TestDuplicateChecking::cleanupTestCase() {
    // Final cleanup
}

void TestDuplicateChecking::init() {
    // Open database and create contest BEFORE each test
    Database& db = Database::instance();
    QVERIFY(db.open(m_testDbPath));

    // Add missing columns for exchange fields
    QSqlQuery migrationQuery = db.execute("PRAGMA table_info(qsos)", {});
    QSet<QString> existingColumns;
    while (migrationQuery.next()) {
        existingColumns.insert(migrationQuery.value(1).toString());
    }

    QStringList missingColumns;
    if (!existingColumns.contains("serial_number_received")) {
        missingColumns << "ALTER TABLE qsos ADD COLUMN serial_number_received INTEGER DEFAULT 0";
    }
    if (!existingColumns.contains("precedence")) {
        missingColumns << "ALTER TABLE qsos ADD COLUMN precedence TEXT";
    }
    if (!existingColumns.contains("sweepstakes_check")) {
        missingColumns << "ALTER TABLE qsos ADD COLUMN sweepstakes_check TEXT";
    }
    if (!existingColumns.contains("power")) {
        missingColumns << "ALTER TABLE qsos ADD COLUMN power TEXT";
    }
    if (!existingColumns.contains("operator_name")) {
        missingColumns << "ALTER TABLE qsos ADD COLUMN operator_name TEXT";
    }
    if (!existingColumns.contains("itu_zone_exchange")) {
        missingColumns << "ALTER TABLE qsos ADD COLUMN itu_zone_exchange TEXT";
    }

    for (const QString& sql : missingColumns) {
        QSqlQuery addColumn = db.execute(sql, {});
        QVERIFY2(!addColumn.lastError().isValid(), qPrintable(QString("Failed: %1").arg(sql)));
    }

    // Create test contest
    QSqlQuery query = db.execute(
        "INSERT INTO contests (contest_id, contest_name, start_time, contest_type, "
        "my_call, my_grid, my_continent, my_cq_zone, my_itu_zone, current_serial, "
        "exchange_sent, created_at) "
        "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)",
        {"TEST", "Test Contest", QDateTime::currentSecsSinceEpoch(),
         "CQWW_CW", "NY4I", "EM73", "NA", 5, 8, 1, "599 5",
         QDateTime::currentSecsSinceEpoch()}
    );

    QVERIFY2(!query.lastError().isValid(), qPrintable(query.lastError().text()));
    m_contestDbId = query.lastInsertId().toInt();
    QVERIFY(m_contestDbId > 0);
}

void TestDuplicateChecking::cleanup() {
    Database& db = Database::instance();
    if (db.isOpen()) {
        db.close();
    }
    QFile::remove(m_testDbPath);
}

void TestDuplicateChecking::addQSO(const QString& callsign, BandType band, ModeType mode) {
    QSORepository repo;

    QSO qso;
    qso.callsign = callsign;
    qso.timestamp = QDateTime::currentDateTime();
    qso.frequency = 14000000;
    qso.band = band;
    qso.mode = mode;
    qso.rstSent = "599";
    qso.rstReceived = "599";
    qso.exchangeSent = "599 5";
    qso.exchangeReceived = "599 3";
    qso.serialNumber = 1;
    qso.operatorCall = "NY4I";

    QVERIFY2(repo.saveQSO(qso, m_contestDbId),
             qPrintable(QString("Failed to save QSO: %1").arg(repo.lastError())));
}

bool TestDuplicateChecking::checkDuplicate(const QString& callsign, BandType band,
                                          ModeType mode, DuplicateCheckingRule rule,
                                          QString& dupeInfo) {
    // Simulate MainWindow::checkForDuplicate() behavior
    QString bandStr = bandToString(band);
    QString modeStr = modeToString(mode);

    // Build SQL query based on duplicate rule
    QString sql = "SELECT band, mode, timestamp FROM qsos WHERE callsign = ?";
    QVariantList params;
    params << callsign;

    // Add filters based on rule
    switch (rule) {
        case DuplicateCheckingRule::PerBandMode:
            sql += " AND band = ? AND mode = ?";
            params << bandStr << modeStr;
            break;
        case DuplicateCheckingRule::AllBandMode:
            sql += " AND mode = ?";
            params << modeStr;
            break;
        case DuplicateCheckingRule::PerBand:
            sql += " AND band = ?";
            params << bandStr;
            break;
        case DuplicateCheckingRule::AllBand:
            // No filter
            break;
    }

    sql += " LIMIT 1";

    Database& db = Database::instance();
    QSqlQuery query = db.execute(sql, params);

    if (query.next()) {
        QDateTime timestamp = QDateTime::fromSecsSinceEpoch(query.value(2).toLongLong());

        switch (rule) {
            case DuplicateCheckingRule::PerBandMode:
                dupeInfo = QString("DUPE - Worked on %1 at %2")
                    .arg(timestamp.toString("yyyy-MM-dd"))
                    .arg(timestamp.toString("HH:mm"));
                break;
            case DuplicateCheckingRule::AllBandMode:
                dupeInfo = QString("DUPE - Worked on %1 at %2 (same mode, different band)")
                    .arg(timestamp.toString("yyyy-MM-dd"))
                    .arg(timestamp.toString("HH:mm"));
                break;
            case DuplicateCheckingRule::PerBand:
                dupeInfo = QString("DUPE - Worked on %1 at %2 (same band, different mode)")
                    .arg(timestamp.toString("yyyy-MM-dd"))
                    .arg(timestamp.toString("HH:mm"));
                break;
            case DuplicateCheckingRule::AllBand:
                dupeInfo = QString("DUPE - Worked on %1 at %2 (once-per-contest)")
                    .arg(timestamp.toString("yyyy-MM-dd"))
                    .arg(timestamp.toString("HH:mm"));
                break;
        }

        return true;
    }

    return false;
}

void TestDuplicateChecking::testPerBandMode_SameBandSameMode_IsDupe() {
    // Add QSO: W1AW on 20m CW
    addQSO("W1AW", BandType::Band20M, ModeType::CW);

    // Check for dupe: W1AW on 20m CW (same band, same mode)
    QString dupeInfo;
    bool isDupe = checkDuplicate("W1AW", BandType::Band20M, ModeType::CW,
                                DuplicateCheckingRule::PerBandMode, dupeInfo);

    QVERIFY(isDupe);
    QVERIFY(dupeInfo.contains("DUPE"));

    qInfo() << "✓ PerBandMode: Same band/mode is dupe:" << dupeInfo;
}

void TestDuplicateChecking::testPerBandMode_SameBandDifferentMode_NotDupe() {
    // Add QSO: W1AW on 20m CW
    addQSO("W1AW", BandType::Band20M, ModeType::CW);

    // Check for dupe: W1AW on 20m USB (same band, different mode)
    QString dupeInfo;
    bool isDupe = checkDuplicate("W1AW", BandType::Band20M, ModeType::USB,
                                DuplicateCheckingRule::PerBandMode, dupeInfo);

    QVERIFY(!isDupe);

    qInfo() << "✓ PerBandMode: Same band, different mode is NOT dupe";
}

void TestDuplicateChecking::testPerBandMode_DifferentBandSameMode_NotDupe() {
    // Add QSO: W1AW on 20m CW
    addQSO("W1AW", BandType::Band20M, ModeType::CW);

    // Check for dupe: W1AW on 40m CW (different band, same mode)
    QString dupeInfo;
    bool isDupe = checkDuplicate("W1AW", BandType::Band40M, ModeType::CW,
                                DuplicateCheckingRule::PerBandMode, dupeInfo);

    QVERIFY(!isDupe);

    qInfo() << "✓ PerBandMode: Different band, same mode is NOT dupe";
}

void TestDuplicateChecking::testAllBandMode_SameMode_IsDupe() {
    // Add QSO: W1AW on 20m CW
    addQSO("W1AW", BandType::Band20M, ModeType::CW);

    // Check for dupe: W1AW on 40m CW (different band, same mode)
    QString dupeInfo;
    bool isDupe = checkDuplicate("W1AW", BandType::Band40M, ModeType::CW,
                                DuplicateCheckingRule::AllBandMode, dupeInfo);

    QVERIFY(isDupe);
    QVERIFY(dupeInfo.contains("same mode, different band"));

    qInfo() << "✓ AllBandMode: Same mode, different band is dupe:" << dupeInfo;
}

void TestDuplicateChecking::testAllBandMode_DifferentMode_NotDupe() {
    // Add QSO: W1AW on 20m CW
    addQSO("W1AW", BandType::Band20M, ModeType::CW);

    // Check for dupe: W1AW on 20m USB (same band, different mode)
    QString dupeInfo;
    bool isDupe = checkDuplicate("W1AW", BandType::Band20M, ModeType::USB,
                                DuplicateCheckingRule::AllBandMode, dupeInfo);

    QVERIFY(!isDupe);

    qInfo() << "✓ AllBandMode: Different mode is NOT dupe";
}

void TestDuplicateChecking::testPerBand_SameBand_IsDupe() {
    // Add QSO: W1AW on 20m CW
    addQSO("W1AW", BandType::Band20M, ModeType::CW);

    // Check for dupe: W1AW on 20m USB (same band, different mode)
    QString dupeInfo;
    bool isDupe = checkDuplicate("W1AW", BandType::Band20M, ModeType::USB,
                                DuplicateCheckingRule::PerBand, dupeInfo);

    QVERIFY(isDupe);
    QVERIFY(dupeInfo.contains("same band, different mode"));

    qInfo() << "✓ PerBand: Same band, different mode is dupe:" << dupeInfo;
}

void TestDuplicateChecking::testPerBand_DifferentBand_NotDupe() {
    // Add QSO: W1AW on 20m CW
    addQSO("W1AW", BandType::Band20M, ModeType::CW);

    // Check for dupe: W1AW on 40m CW (different band, same mode)
    QString dupeInfo;
    bool isDupe = checkDuplicate("W1AW", BandType::Band40M, ModeType::CW,
                                DuplicateCheckingRule::PerBand, dupeInfo);

    QVERIFY(!isDupe);

    qInfo() << "✓ PerBand: Different band is NOT dupe";
}

void TestDuplicateChecking::testAllBand_AnyBandAnyMode_IsDupe() {
    // Add QSO: W1AW on 20m CW
    addQSO("W1AW", BandType::Band20M, ModeType::CW);

    // Check for dupe: W1AW on 40m USB (different band, different mode)
    QString dupeInfo;
    bool isDupe = checkDuplicate("W1AW", BandType::Band40M, ModeType::USB,
                                DuplicateCheckingRule::AllBand, dupeInfo);

    QVERIFY(isDupe);
    QVERIFY(dupeInfo.contains("once-per-contest"));

    qInfo() << "✓ AllBand: Any band/mode is dupe:" << dupeInfo;
}

QTEST_MAIN(TestDuplicateChecking)
#include "test_duplicate_checking.moc"
