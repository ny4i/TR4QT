#include <QtTest>
#include "../src/importers/ADIFImporter.h"
#include "../src/models/QSO.h"
#include "../src/core/Types.h"

using namespace TR4QT;

class TestADIFMappingDemo : public QObject {
    Q_OBJECT

private slots:
    void showFirstRecordMapping() {
        ADIFImporter importer;
        QList<QSO> qsos;

        // Import the N1MM file from test fixtures
        QString filePath = QString(TESTS_SOURCE_DIR) + "/fixtures/2025-09-28_CQWW_RTTY_W4TA_Run2.adi";
        bool success = importer.importFile(filePath, qsos);

        QVERIFY(success);
        QVERIFY(qsos.size() > 0);

        // Get first QSO
        const QSO& qso = qsos[0];

        // Print the mapping
        qDebug() << "\n========================================";
        qDebug() << "FIRST RECORD ADIF → QSO MAPPING";
        qDebug() << "========================================\n";

        qDebug() << "ADIF Field                    → QSO Field                  = Value";
        qDebug() << "--------------------------------------------------------------------------------";

        qDebug().noquote() << QString("CALL:4 = \"NS4X\"              → qso.callsign            = \"%1\"").arg(qso.callsign);
        qDebug().noquote() << QString("QSO_DATE:8 = \"20250927\"      → qso.timestamp (date)    = %1").arg(qso.timestamp.toString("yyyy-MM-dd"));
        qDebug().noquote() << QString("TIME_ON:6 = \"002448\"         → qso.timestamp (time)    = %1 UTC").arg(qso.timestamp.toString("HH:mm:ss"));
        qDebug().noquote() << QString("                                 qso.timestamp (full)    = %1").arg(qso.timestamp.toString(Qt::ISODate));
        qDebug() << "";

        qDebug().noquote() << QString("FREQ:8 = \"14.11099\"          → qso.frequency           = %1 Hz").arg(qso.frequency);
        qDebug().noquote() << QString("BAND:3 = \"20M\"               → qso.band                = %1 (enum %2)").arg(bandToString(qso.band)).arg((int)qso.band);
        qDebug().noquote() << QString("MODE:4 = \"RTTY\"              → qso.mode                = %1 (enum %2)").arg(modeToString(qso.mode)).arg((int)qso.mode);
        qDebug().noquote() << QString("SUBMODE (not present)          → qso.submode             = \"%1\"").arg(qso.submode);
        qDebug() << "";

        qDebug().noquote() << QString("RST_SENT:3 = \"599\"           → qso.rstSent             = \"%1\"").arg(qso.rstSent);
        qDebug().noquote() << QString("RST_RCVD:3 = \"599\"           → qso.rstReceived         = \"%1\"").arg(qso.rstReceived);
        qDebug().noquote() << QString("STX:1 = \"1\"                  → qso.serialNumber        = %1").arg(qso.serialNumber);
        qDebug().noquote() << QString("                                 qso.exchangeSent         = \"%1\"").arg(qso.exchangeSent);
        qDebug().noquote() << QString("CQZ:1 = \"4\"                  → qso.exchangeReceived    = \"%1\"").arg(qso.exchangeReceived);
        qDebug() << "";

        qDebug().noquote() << QString("STATE:2 = \"ON\"               → qso.state               = \"%1\"").arg(qso.state);
        qDebug().noquote() << QString("CQZ:1 = \"4\"                  → qso.cqZone              = %1").arg(qso.cqZone);
        qDebug().noquote() << QString("PFX:3 = \"NS4\"                → qso.dxccPrefix          = \"%1\"").arg(qso.dxccPrefix);
        qDebug() << "";

        qDebug().noquote() << QString("APP_N1MM_ID:32 = \"31d9...\"   → qso.guid                = \"%1\"").arg(qso.guid);
        qDebug().noquote() << QString("APP_N1MM_CONTINENT:2 = \"NA\"  → qso.continent           = \"%1\"").arg(qso.continent);
        qDebug().noquote() << QString("APP_N1MM_ISRUNQSO:1 = \"0\"    → qso.isRunQSO            = %1").arg(qso.isRunQSO ? "true" : "false");
        qDebug().noquote() << QString("APP_N1MM_POINTS:1 = \"1\"      → qso.qsoPoints           = %1").arg(qso.qsoPoints);
        qDebug().noquote() << QString("OPERATOR:6 = \"AA1ZZZ\"        → qso.operatorCall        = \"%1\"").arg(qso.operatorCall);
        qDebug() << "";

        qDebug() << "Fields NOT in ADIF (set by importer):";
        qDebug().noquote() << QString("                                 qso.id                   = %1 (not saved yet)").arg(qso.id);
        qDebug().noquote() << QString("                                 qso.isDupe               = %1").arg(qso.isDupe ? "true" : "false");
        qDebug().noquote() << QString("                                 qso.isMultiplier         = %1").arg(qso.isMultiplier ? "true" : "false");
        qDebug().noquote() << QString("                                 qso.deleted              = %1").arg(qso.deleted ? "true" : "false");
        qDebug() << "";

        qDebug() << "Fields NOT populated from this ADIF file:";
        qDebug().noquote() << QString("                                 qso.dxccEntity           = \"%1\" (would come from COUNTRY field)").arg(qso.dxccEntity);
        qDebug().noquote() << QString("                                 qso.dxccEntityCode       = %1 (would come from DXCC field)").arg(qso.dxccEntityCode);
        qDebug().noquote() << QString("                                 qso.ituZone              = %1 (would come from ITUZ field)").arg(qso.ituZone);
        qDebug().noquote() << QString("                                 qso.county               = \"%1\" (would come from CNTY field)").arg(qso.county);
        qDebug().noquote() << QString("                                 qso.arrlSection          = \"%1\" (would come from ARRL_SECT field)").arg(qso.arrlSection);
        qDebug().noquote() << QString("                                 qso.gridSquare           = \"%1\" (would come from GRIDSQUARE field)").arg(qso.gridSquare);
        qDebug().noquote() << QString("                                 qso.iotaReference        = \"%1\" (would come from IOTA field)").arg(qso.iotaReference);
        qDebug().noquote() << QString("                                 qso.contestClass         = \"%1\" (not in N1MM ADIF)").arg(qso.contestClass);

        qDebug() << "\n========================================\n";
    }
};

QTEST_MAIN(TestADIFMappingDemo)
#include "test_adif_mapping_demo.moc"
