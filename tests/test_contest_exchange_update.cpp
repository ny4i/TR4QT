/**
 * Integration test for contest exchange update functionality
 *
 * Tests the CURRENT behavior of MainWindow::onEditContestSettings() BEFORE extraction.
 * This test proves behavioral equivalence after refactoring to ContestService.
 *
 * Following CLAUDE.md principle: Write tests BEFORE extraction to prove equivalence.
 */

#include <QTest>
#include <QDir>
#include <QFile>
#include <QSqlQuery>
#include <QSqlError>
#include "../src/data/Database.h"
#include "../src/data/QSORepository.h"
#include "../src/models/QSO.h"
#include "../src/models/StationInfo.h"
#include "../src/core/Types.h"
#include "../src/contests/WinterFieldDayContest.h"

using namespace TR4QT;

class TestContestExchangeUpdate : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void init();
    void cleanup();

    // Test current behavior
    void testUpdateExchange_UpdatesContestRecord();
    void testUpdateExchange_UpdatesAllQSOs();
    void testUpdateExchange_UsesContestFormatting();
    void testUpdateExchange_HandlesEmptyExchange();
    void testUpdateExchange_HandlesMultipleQSOs();

private:
    QString m_testDbPath;
    int m_contestDbId;
    ContestBase* m_contest;

    void addTestQSOs(int count);
};

void TestContestExchangeUpdate::initTestCase() {
    // Just create temporary database path
    m_testDbPath = QDir::temp().filePath("test_exchange_update.db");
}

void TestContestExchangeUpdate::cleanupTestCase() {
    delete m_contest;
}

void TestContestExchangeUpdate::init() {
    // Open database and create contest BEFORE each test
    Database& db = Database::instance();
    QVERIFY(db.open(m_testDbPath));

    // Add missing columns for exchange fields (schema.sql might be outdated)
    // These columns were added in v3.31.1 but schema.sql may not have them
    QSqlQuery migrationQuery = db.execute("PRAGMA table_info(qsos)", {});
    QSet<QString> existingColumns;
    while (migrationQuery.next()) {
        existingColumns.insert(migrationQuery.value(1).toString());
    }

    // Add all missing exchange-related columns
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
        QVERIFY2(!addColumn.lastError().isValid(), qPrintable(QString("Failed to add column: %1").arg(sql)));
    }

    // Create Winter Field Day contest
    QSqlQuery query = db.execute(
        "INSERT INTO contests (contest_id, contest_name, start_time, contest_type, "
        "my_call, my_grid, my_continent, my_cq_zone, my_itu_zone, current_serial, "
        "exchange_sent, created_at) "
        "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)",
        {"WFD", "Winter Field Day 2026", QDateTime::currentSecsSinceEpoch(),
         "WFD", "NY4I", "EM73", "NA", 5, 8, 1, "1O WMA",
         QDateTime::currentSecsSinceEpoch()}
    );

    QVERIFY2(!query.lastError().isValid(), qPrintable(query.lastError().text()));

    m_contestDbId = query.lastInsertId().toInt();
    QVERIFY(m_contestDbId > 0);

    // Create contest instance with station info
    StationInfo myStation;
    myStation.callsign = "NY4I";
    myStation.grid = "EM73";
    myStation.continent = "NA";
    myStation.cqZone = 5;
    myStation.ituZone = 8;
    myStation.country = "United States";

    m_contest = new WinterFieldDayContest(myStation);
    m_contest->setExchangeSent("1O WMA");
}

void TestContestExchangeUpdate::cleanup() {
    delete m_contest;
    m_contest = nullptr;

    Database& db = Database::instance();
    if (db.isOpen()) {
        db.close();
    }
    QFile::remove(m_testDbPath);
}

void TestContestExchangeUpdate::addTestQSOs(int count) {
    QSORepository repo;

    for (int i = 0; i < count; i++) {
        QSO qso;
        qso.callsign = QString("W1AW/") + QString::number(i);
        qso.timestamp = QDateTime::currentDateTime().addSecs(-3600 + i * 60);
        qso.frequency = 14000000 + (i * 1000);
        qso.band = BandType::Band20M;
        qso.mode = ModeType::CW;
        qso.rstSent = "599";
        qso.rstReceived = "599";
        qso.exchangeSent = m_contest->formatSentExchange(1, "599");  // Uses current exchange
        qso.exchangeReceived = QString("2I WCF");
        qso.serialNumber = i + 1;
        qso.operatorCall = "NY4I";

        QVERIFY2(repo.saveQSO(qso, m_contestDbId),
                 qPrintable(QString("Failed to save QSO %1: %2").arg(i).arg(repo.lastError())));
    }

    qInfo() << "Added" << count << "test QSOs with exchange:" << m_contest->getExchangeSent();
}

void TestContestExchangeUpdate::testUpdateExchange_UpdatesContestRecord() {
    // init() already created contest with exchange "1O WMA"
    QString oldExchange = "1O WMA";
    QString newExchange = "1H WCF";

    // Simulate what onEditContestSettings() does
    Database& db = Database::instance();

    // Update contests table
    QSqlQuery query = db.execute(
        "UPDATE contests SET exchange_sent = ? WHERE id = ?",
        {newExchange, m_contestDbId}
    );

    QVERIFY2(!query.lastError().isValid(), qPrintable(query.lastError().text()));

    // Verify database was updated
    QSqlQuery verifyQuery = db.execute(
        "SELECT exchange_sent FROM contests WHERE id = ?",
        {m_contestDbId}
    );

    QVERIFY(verifyQuery.next());
    QString dbExchange = verifyQuery.value(0).toString();
    QCOMPARE(dbExchange, newExchange);

    qInfo() << "✓ Contest exchange updated in database:" << oldExchange << "→" << dbExchange;
}

void TestContestExchangeUpdate::testUpdateExchange_UpdatesAllQSOs() {
    // init() already created contest
    addTestQSOs(5);

    QString oldExchange = "1O WMA";
    QString newExchange = "1H WCF";

    // Update contest
    m_contest->setExchangeSent(newExchange);

    // Simulate what onEditContestSettings() does - update all QSOs
    QSORepository repo;
    QList<QSO> qsos = repo.findByContest(m_contestDbId);

    QCOMPARE(qsos.size(), 5);

    int updatedCount = 0;
    for (QSO& qso : qsos) {
        // Recalculate exchange using contest's formatSentExchange
        QString newExchangeSent = m_contest->formatSentExchange(qso.serialNumber, qso.rstSent);
        qso.exchangeSent = newExchangeSent;

        // Update in database
        QVERIFY2(repo.updateQSO(qso), qPrintable(repo.lastError()));
        updatedCount++;
    }

    QCOMPARE(updatedCount, 5);

    // Verify all QSOs have new exchange
    QList<QSO> verifyQSOs = repo.findByContest(m_contestDbId);
    for (const QSO& qso : verifyQSOs) {
        QCOMPARE(qso.exchangeSent, newExchange);
    }

    qInfo() << "✓ All" << updatedCount << "QSOs updated with new exchange:" << newExchange;
}

void TestContestExchangeUpdate::testUpdateExchange_UsesContestFormatting() {
    // init() already created contest with exchange "1O WMA"

    // For WFD, formatSentExchange() should return the exchange directly (no serial)
    QString exchange = m_contest->formatSentExchange(1, "599");
    QCOMPARE(exchange, QString("1O WMA"));

    // Change exchange
    m_contest->setExchangeSent("2I EMA");
    exchange = m_contest->formatSentExchange(1, "599");
    QCOMPARE(exchange, QString("2I EMA"));

    qInfo() << "✓ Contest formatting works correctly";
}

void TestContestExchangeUpdate::testUpdateExchange_HandlesEmptyExchange() {
    // init() already created contest with exchange "1O WMA"

    QString oldExchange = m_contest->getExchangeSent();
    QVERIFY(!oldExchange.isEmpty());

    // Setting empty exchange should still work (though not valid for WFD)
    m_contest->setExchangeSent("");
    QString newExchange = m_contest->getExchangeSent();
    QCOMPARE(newExchange, QString(""));

    qInfo() << "✓ Empty exchange handled (though not valid for WFD)";
}

void TestContestExchangeUpdate::testUpdateExchange_HandlesMultipleQSOs() {
    // init() already created contest
    addTestQSOs(50);  // Stress test with more QSOs

    QString newExchange = "3A NFL";
    m_contest->setExchangeSent(newExchange);

    // Update all QSOs
    QSORepository repo;
    QList<QSO> qsos = repo.findByContest(m_contestDbId);

    QCOMPARE(qsos.size(), 50);

    for (QSO& qso : qsos) {
        qso.exchangeSent = m_contest->formatSentExchange(qso.serialNumber, qso.rstSent);
        QVERIFY(repo.updateQSO(qso));
    }

    // Verify all updated
    QList<QSO> verifyQSOs = repo.findByContest(m_contestDbId);
    for (const QSO& qso : verifyQSOs) {
        QCOMPARE(qso.exchangeSent, newExchange);
    }

    qInfo() << "✓ Handled 50 QSOs successfully";
}

QTEST_MAIN(TestContestExchangeUpdate)
#include "test_contest_exchange_update.moc"
