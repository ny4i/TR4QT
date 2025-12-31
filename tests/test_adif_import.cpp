#include <QtTest>
#include "../src/importers/ADIFImporter.h"
#include "../src/models/QSO.h"

using namespace TR4QT;

class TestADIFImport : public QObject {
    Q_OBJECT

private slots:
    void testParseN1MMFile() {
        ADIFImporter importer;
        QList<QSO> qsos;

        // Test with the N1MM file from test fixtures
        QString filePath = QString(TESTS_SOURCE_DIR) + "/fixtures/2025-09-28_CQWW_RTTY_W4TA_Run2.adi";
        bool success = importer.importFile(filePath, qsos);

        // Should succeed
        QVERIFY2(success, qPrintable(importer.lastError()));

        // Should have imported QSOs
        QVERIFY(importer.importedCount() > 0);
        qDebug() << "Imported" << importer.importedCount() << "QSOs";
        qDebug() << "Failed" << importer.failedCount() << "QSOs";

        if (importer.failedCount() > 0) {
            qDebug() << "Warnings:";
            for (const QString& warning : importer.warnings()) {
                qDebug() << "  " << warning;
            }
        }

        // Verify first QSO (NS4X from file line 6)
        QVERIFY(qsos.size() > 0);
        const QSO& firstQSO = qsos[0];

        QCOMPARE(firstQSO.callsign, QString("NS4X"));
        QCOMPARE(firstQSO.band, BandType::Band20M);
        QCOMPARE(firstQSO.mode, ModeType::RTTY);
        QCOMPARE(firstQSO.rstSent, QString("599"));
        QCOMPARE(firstQSO.rstReceived, QString("599"));
        QCOMPARE(firstQSO.serialNumber, 1);
        QCOMPARE(firstQSO.cqZone, 4);
        QCOMPARE(firstQSO.continent, QString("NA"));
        QCOMPARE(firstQSO.state, QString("ON"));
        QVERIFY(firstQSO.frequency > 0);

        // Verify N1MM custom fields
        QCOMPARE(firstQSO.guid, QString("31d91675021f4dcea03a8b37b21b3ca6"));
        QCOMPARE(firstQSO.isRunQSO, false);  // N1MM Run/S&P indicator
        QCOMPARE(firstQSO.qsoPoints, 1);

        // Verify date/time parsing (2025-09-27 00:24:48 UTC)
        QDateTime expected = QDateTime(QDate(2025, 9, 27), QTime(0, 24, 48), QTimeZone::UTC);
        QCOMPARE(firstQSO.timestamp, expected);
    }

    void testParseADIFString() {
        ADIFImporter importer;
        QList<QSO> qsos;

        QString adifContent = R"(
ADIF Export Test
<EOH>
<CALL:4>W1AW <QSO_DATE:8>20250101 <TIME_ON:6>120000 <BAND:3>20M <MODE:2>CW <RST_SENT:3>599 <RST_RCVD:3>599 <CQZ:1>5 <STATE:2>CT <CONT:2>NA <EOR>
<CALL:4>DL1A <QSO_DATE:8>20250101 <TIME_ON:6>120100 <BAND:3>40M <MODE:3>USB <RST_SENT:2>59 <RST_RCVD:2>59 <CQZ:2>14 <CONT:2>EU <EOR>
)";

        bool success = importer.importFromString(adifContent, qsos);

        QVERIFY2(success, qPrintable(importer.lastError()));
        QCOMPARE(importer.importedCount(), 2);
        QCOMPARE(qsos.size(), 2);

        // Verify first QSO
        QCOMPARE(qsos[0].callsign, QString("W1AW"));
        QCOMPARE(qsos[0].band, BandType::Band20M);
        QCOMPARE(qsos[0].mode, ModeType::CW);
        QCOMPARE(qsos[0].state, QString("CT"));
        QCOMPARE(qsos[0].continent, QString("NA"));

        // Verify second QSO
        QCOMPARE(qsos[1].callsign, QString("DL1A"));
        QCOMPARE(qsos[1].band, BandType::Band40M);
        QCOMPARE(qsos[1].mode, ModeType::USB);
        QCOMPARE(qsos[1].continent, QString("EU"));
    }

    void testMissingRequiredFields() {
        ADIFImporter importer;
        QList<QSO> qsos;

        // Missing CALL
        QString adifContent = R"(
<EOH>
<QSO_DATE:8>20250101 <TIME_ON:6>120000 <BAND:3>20M <MODE:2>CW <EOR>
)";

        bool success = importer.importFromString(adifContent, qsos);
        QVERIFY(!success);
        QCOMPARE(importer.importedCount(), 0);

        // Missing QSO_DATE
        adifContent = R"(
<EOH>
<CALL:4>W1AW <TIME_ON:6>120000 <BAND:3>20M <MODE:2>CW <EOR>
)";

        success = importer.importFromString(adifContent, qsos);
        QVERIFY(!success);

        // Missing BAND
        adifContent = R"(
<EOH>
<CALL:4>W1AW <QSO_DATE:8>20250101 <TIME_ON:6>120000 <MODE:2>CW <EOR>
)";

        success = importer.importFromString(adifContent, qsos);
        QVERIFY(!success);
    }

    void testUnknownFieldsIgnored() {
        ADIFImporter importer;
        QList<QSO> qsos;

        // Include many unknown fields - should be silently ignored
        QString adifContent = R"(
<EOH>
<CALL:4>W1AW <QSO_DATE:8>20250101 <TIME_ON:6>120000 <BAND:3>20M <MODE:2>CW
<UNKNOWN_FIELD:10>SomeValue <APP_CUSTOM_THING:5>12345 <WEIRD:3>XYZ <EOR>
)";

        bool success = importer.importFromString(adifContent, qsos);
        QVERIFY2(success, qPrintable(importer.lastError()));
        QCOMPARE(importer.importedCount(), 1);
        QCOMPARE(qsos[0].callsign, QString("W1AW"));
    }
};

QTEST_MAIN(TestADIFImport)
#include "test_adif_import.moc"
