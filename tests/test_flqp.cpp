#include <QtTest>
#include "../src/contests/FloridaQSOPartyContest.h"
#include "../src/models/StationInfo.h"
#include "../src/models/QSO.h"
#include "../src/core/Types.h"

using namespace TR4QT;

class TestFloridaQSOParty : public QObject {
    Q_OBJECT

private:
    StationInfo createFloridaStation() {
        StationInfo station;
        station.callsign = "W4ABC";
        station.state = "FL";
        station.county = "PAL";  // Palm Beach County
        station.continent = "NA";
        station.cqZone = 5;
        station.power = 100;  // High power
        return station;
    }

    StationInfo createNonFloridaStation() {
        StationInfo station;
        station.callsign = "W1AW";
        station.state = "CT";
        station.continent = "NA";
        station.cqZone = 5;
        station.power = 50;  // Low power (×2)
        return station;
    }

    StationInfo createQRPStation() {
        StationInfo station;
        station.callsign = "K3LR";
        station.state = "PA";
        station.continent = "NA";
        station.cqZone = 5;
        station.power = 5;  // QRP (×3)
        return station;
    }

private slots:
    //
    // Contest Identity
    //
    void testGetContestId() {
        FloridaQSOPartyContest contest(ModeType::None, createFloridaStation());
        QCOMPARE(contest.getContestId(), QString("FL_QP"));
    }

    void testGetContestName() {
        FloridaQSOPartyContest contest(ModeType::None, createFloridaStation());
        QCOMPARE(contest.getContestName(), QString("Florida QSO Party"));
    }

    //
    // Exchange Fields - Florida Station (In-State)
    //
    void testExchangeFields_FloridaStation_Received() {
        FloridaQSOPartyContest contest(ModeType::CW, createFloridaStation());
        QList<ExchangeField> fields = contest.getReceivedExchangeFields();

        QCOMPARE(fields.size(), 2);
        QCOMPARE(fields[0].name, QString("RST"));
        QCOMPARE(fields[1].name, QString("State"));  // FL station expects State from others
        QCOMPARE(fields[1].hint, QString("State/Prov/DX"));
    }

    void testExchangeFields_FloridaStation_Sent() {
        StationInfo station = createFloridaStation();
        FloridaQSOPartyContest contest(ModeType::CW, station);
        QList<ExchangeField> fields = contest.getSentExchangeFields();

        QCOMPARE(fields.size(), 2);
        QCOMPARE(fields[0].name, QString("RST"));
        QCOMPARE(fields[1].name, QString("County"));  // FL station sends County
        QCOMPARE(fields[1].hint, QString("PAL"));  // Auto-filled from station
        QVERIFY(fields[1].autoFill);
    }

    //
    // Exchange Fields - Non-Florida Station (Out-of-State)
    //
    void testExchangeFields_NonFloridaStation_Received() {
        FloridaQSOPartyContest contest(ModeType::CW, createNonFloridaStation());
        QList<ExchangeField> fields = contest.getReceivedExchangeFields();

        QCOMPARE(fields.size(), 2);
        QCOMPARE(fields[0].name, QString("RST"));
        QCOMPARE(fields[1].name, QString("County"));  // Non-FL expects County from FL stations
        QCOMPARE(fields[1].hint, QString("FL County"));
    }

    void testExchangeFields_NonFloridaStation_Sent() {
        StationInfo station = createNonFloridaStation();
        FloridaQSOPartyContest contest(ModeType::CW, station);
        QList<ExchangeField> fields = contest.getSentExchangeFields();

        QCOMPARE(fields.size(), 2);
        QCOMPARE(fields[0].name, QString("RST"));
        QCOMPARE(fields[1].name, QString("State"));  // Non-FL sends State
        QCOMPARE(fields[1].hint, QString("CT"));  // Auto-filled from station
        QVERIFY(fields[1].autoFill);
    }

    //
    // Exchange Parsing - Florida Station
    //
    void testParseExchange_FloridaStation_ValidState() {
        FloridaQSOPartyContest contest(ModeType::CW, createFloridaStation());
        QSO qso;

        contest.parseReceivedExchange("599 CT", qso);

        QCOMPARE(qso.rstReceived, QString("599"));
        QCOMPARE(qso.state, QString("CT"));
        QCOMPARE(qso.county, QString(""));  // No county for non-FL
    }

    void testParseExchange_FloridaStation_OrderAgnostic() {
        FloridaQSOPartyContest contest(ModeType::CW, createFloridaStation());
        QSO qso;

        // Reversed order: state first, then RST
        contest.parseReceivedExchange("PA 599", qso);

        QCOMPARE(qso.rstReceived, QString("599"));
        QCOMPARE(qso.state, QString("PA"));
    }

    void testParseExchange_FloridaStation_CanadianProvince() {
        FloridaQSOPartyContest contest(ModeType::CW, createFloridaStation());
        QSO qso;

        contest.parseReceivedExchange("599 ON", qso);

        QCOMPARE(qso.rstReceived, QString("599"));
        QCOMPARE(qso.state, QString("ON"));
    }

    void testParseExchange_FloridaStation_DX() {
        FloridaQSOPartyContest contest(ModeType::CW, createFloridaStation());
        QSO qso;

        contest.parseReceivedExchange("599 JA", qso);

        QCOMPARE(qso.rstReceived, QString("599"));
        QCOMPARE(qso.state, QString("JA"));  // DX stored in state field
    }

    //
    // Exchange Parsing - Non-Florida Station
    //
    void testParseExchange_NonFloridaStation_ValidCounty() {
        FloridaQSOPartyContest contest(ModeType::CW, createNonFloridaStation());
        QSO qso;

        contest.parseReceivedExchange("599 PAL", qso);

        QCOMPARE(qso.rstReceived, QString("599"));
        QCOMPARE(qso.county, QString("PAL"));
        QCOMPARE(qso.state, QString("FL"));  // County implies Florida
    }

    void testParseExchange_NonFloridaStation_OrderAgnostic() {
        FloridaQSOPartyContest contest(ModeType::CW, createNonFloridaStation());
        QSO qso;

        // Reversed order: county first
        contest.parseReceivedExchange("DUV 599", qso);

        QCOMPARE(qso.rstReceived, QString("599"));
        QCOMPARE(qso.county, QString("DUV"));
        QCOMPARE(qso.state, QString("FL"));
    }

    //
    // Exchange Validation - Florida Station
    //
    void testValidateExchange_FloridaStation_ValidState() {
        FloridaQSOPartyContest contest(ModeType::CW, createFloridaStation());

        QString errorMsg;
        QVERIFY(contest.validateReceivedExchange("599 NY", errorMsg));
        QVERIFY(errorMsg.isEmpty());
    }

    void testValidateExchange_FloridaStation_InvalidState() {
        FloridaQSOPartyContest contest(ModeType::CW, createFloridaStation());

        QString errorMsg;
        // Test exchange that's too short (1 character)
        QVERIFY(!contest.validateReceivedExchange("599 X", errorMsg));
        QVERIFY(errorMsg.contains("Invalid State/Province/DX"));

        errorMsg.clear();
        // Test exchange that's too long (5+ characters)
        QVERIFY(!contest.validateReceivedExchange("599 TOOLONG", errorMsg));
        QVERIFY(errorMsg.contains("Invalid State/Province/DX"));
    }

    //
    // Exchange Validation - Non-Florida Station
    //
    void testValidateExchange_NonFloridaStation_ValidCounty() {
        FloridaQSOPartyContest contest(ModeType::CW, createNonFloridaStation());

        QString errorMsg;
        QVERIFY(contest.validateReceivedExchange("599 PAL", errorMsg));
        QVERIFY(errorMsg.isEmpty());
    }

    void testValidateExchange_NonFloridaStation_InvalidCounty() {
        FloridaQSOPartyContest contest(ModeType::CW, createNonFloridaStation());

        QString errorMsg;
        QVERIFY(!contest.validateReceivedExchange("599 XXX", errorMsg));
        QVERIFY(errorMsg.contains("Invalid Florida county"));
    }

    void testValidateExchange_NonFloridaStation_AllCounties() {
        FloridaQSOPartyContest contest(ModeType::CW, createNonFloridaStation());

        // Test all 67 Florida counties
        QStringList counties = {
            "ALC", "BAK", "BAY", "BRA", "BRE", "BRO", "CAH", "CHA", "CIT", "CLA",
            "CLR", "CLM", "DAD", "DES", "DIX", "DUV", "ESC", "FLG", "FRA", "GAD",
            "GIL", "GLA", "GUL", "HAM", "HAR", "HEN", "HER", "HIG", "HIL", "HOL",
            "IDR", "JAC", "JEF", "LAF", "LAK", "LEE", "LEO", "LEV", "LIB", "MAD",
            "MTE", "MAO", "MRT", "MON", "NAS", "OKA", "OKE", "ORA", "OSC", "PAL",
            "PAS", "PIN", "POL", "PUT", "SAN", "SAR", "SEM", "STJ", "STL", "SUM",
            "SUW", "TAY", "UNI", "VOL", "WAK", "WAL", "WAG"
        };

        for (const QString& county : counties) {
            QString exchange = QString("599 %1").arg(county);
            QString errorMsg;
            QVERIFY2(contest.validateReceivedExchange(exchange, errorMsg),
                     qPrintable(QString("County %1 should be valid").arg(county)));
        }
    }

    //
    // Multiplier Types - Florida Station
    //
    void testMultiplierTypes_FloridaStation() {
        FloridaQSOPartyContest contest(ModeType::None, createFloridaStation());
        QList<MultiplierDefinition> mults = contest.getMultiplierTypes();

        QCOMPARE(mults.size(), 2);
        QCOMPARE(mults[0].type, MultiplierType::State);
        QCOMPARE(mults[0].scope, MultiplierScope::AllBands);
        QCOMPARE(mults[1].type, MultiplierType::Country);
        QCOMPARE(mults[1].scope, MultiplierScope::AllBands);
    }

    void testMultiplierValue_FloridaStation_State() {
        FloridaQSOPartyContest contest(ModeType::None, createFloridaStation());
        QSO qso;
        qso.state = "PA";

        QString mult = contest.getMultiplierValue(qso, MultiplierType::State, QStringList());
        QCOMPARE(mult, QString("PA"));
    }

    void testMultiplierValue_FloridaStation_Country() {
        FloridaQSOPartyContest contest(ModeType::None, createFloridaStation());
        QSO qso;
        qso.dxccPrefix = "JA";

        QString mult = contest.getMultiplierValue(qso, MultiplierType::Country, QStringList());
        QCOMPARE(mult, QString("JA"));
    }

    void testMultiplierValue_FloridaStation_AlreadyWorked() {
        FloridaQSOPartyContest contest(ModeType::None, createFloridaStation());
        QSO qso;
        qso.state = "PA";

        // PA already worked
        QString mult = contest.getMultiplierValue(qso, MultiplierType::State, QStringList{"PA"});
        QVERIFY(mult.isEmpty());  // Not a new multiplier
    }

    //
    // Multiplier Types - Non-Florida Station
    //
    void testMultiplierTypes_NonFloridaStation() {
        FloridaQSOPartyContest contest(ModeType::None, createNonFloridaStation());
        QList<MultiplierDefinition> mults = contest.getMultiplierTypes();

        QCOMPARE(mults.size(), 1);
        QCOMPARE(mults[0].type, MultiplierType::County);
        QCOMPARE(mults[0].scope, MultiplierScope::AllBands);
    }

    void testMultiplierValue_NonFloridaStation_County() {
        FloridaQSOPartyContest contest(ModeType::None, createNonFloridaStation());
        QSO qso;
        qso.county = "PAL";

        QString mult = contest.getMultiplierValue(qso, MultiplierType::County, QStringList());
        QCOMPARE(mult, QString("PAL"));
    }

    void testMultiplierValue_NonFloridaStation_InvalidCounty() {
        FloridaQSOPartyContest contest(ModeType::None, createNonFloridaStation());
        QSO qso;
        qso.county = "XXX";  // Invalid

        QString mult = contest.getMultiplierValue(qso, MultiplierType::County, QStringList());
        QVERIFY(mult.isEmpty());  // Invalid county doesn't count
    }

    //
    // Scoring - QSO Points
    //
    void testCalculateQSOPoints_CW() {
        FloridaQSOPartyContest contest(ModeType::None, createFloridaStation());
        QSO qso;
        qso.mode = ModeType::CW;

        int points = contest.calculateQSOPoints(qso, createFloridaStation());
        QCOMPARE(points, 2);
    }

    void testCalculateQSOPoints_Phone() {
        FloridaQSOPartyContest contest(ModeType::None, createFloridaStation());
        QSO qso;
        qso.mode = ModeType::USB;

        int points = contest.calculateQSOPoints(qso, createFloridaStation());
        QCOMPARE(points, 1);
    }

    //
    // Scoring - Power Multipliers
    //
    void testPowerMultiplier_QRP() {
        FloridaQSOPartyContest contest(ModeType::None, createQRPStation());

        QMap<MultiplierType, int> mults;
        mults[MultiplierType::County] = 10;

        int score = contest.calculateTotalScore(100, mults);  // 100 points × 10 mults × 3 (QRP)
        QCOMPARE(score, 3000);
    }

    void testPowerMultiplier_Low() {
        FloridaQSOPartyContest contest(ModeType::None, createNonFloridaStation());

        QMap<MultiplierType, int> mults;
        mults[MultiplierType::County] = 10;

        int score = contest.calculateTotalScore(100, mults);  // 100 points × 10 mults × 2 (Low)
        QCOMPARE(score, 2000);
    }

    void testPowerMultiplier_High() {
        FloridaQSOPartyContest contest(ModeType::None, createFloridaStation());

        QMap<MultiplierType, int> mults;
        mults[MultiplierType::State] = 20;
        mults[MultiplierType::Country] = 10;

        int score = contest.calculateTotalScore(100, mults);  // 100 points × 30 mults × 1 (High)
        QCOMPARE(score, 3000);
    }

    //
    // Band Restrictions
    //
    void testAllowedBands() {
        FloridaQSOPartyContest contest(ModeType::None, createFloridaStation());
        QList<BandType> bands = contest.getAllowedBands();

        QCOMPARE(bands.size(), 4);
        QVERIFY(bands.contains(BandType::Band40M));
        QVERIFY(bands.contains(BandType::Band20M));
        QVERIFY(bands.contains(BandType::Band15M));
        QVERIFY(bands.contains(BandType::Band10M));

        // Should NOT contain other bands
        QVERIFY(!bands.contains(BandType::Band160M));
        QVERIFY(!bands.contains(BandType::Band80M));
        QVERIFY(!bands.contains(BandType::Band30M));
    }

    //
    // Mode Restrictions
    //
    void testIsValidMode_CW() {
        FloridaQSOPartyContest contest(ModeType::None, createFloridaStation());
        QString errorMsg;

        QVERIFY(contest.isValidMode(ModeType::CW, errorMsg));
        QVERIFY(errorMsg.isEmpty());
    }

    void testIsValidMode_Phone() {
        FloridaQSOPartyContest contest(ModeType::None, createFloridaStation());
        QString errorMsg;

        QVERIFY(contest.isValidMode(ModeType::USB, errorMsg));
        QVERIFY(contest.isValidMode(ModeType::LSB, errorMsg));
        QVERIFY(contest.isValidMode(ModeType::FM, errorMsg));
    }

    void testIsValidMode_DigitalNotAllowed() {
        FloridaQSOPartyContest contest(ModeType::None, createFloridaStation());
        QString errorMsg;

        QVERIFY(!contest.isValidMode(ModeType::RTTY, errorMsg));
        QVERIFY(errorMsg.contains("Digital modes are not allowed"));

        errorMsg.clear();
        QVERIFY(!contest.isValidMode(ModeType::FT8, errorMsg));
        QVERIFY(errorMsg.contains("Digital modes are not allowed"));
    }

    //
    // Table Columns
    //
    void testTableColumns_FloridaStation() {
        FloridaQSOPartyContest contest(ModeType::None, createFloridaStation());
        QList<TableColumn> cols = contest.getTableColumns();

        QCOMPARE(cols.size(), 1);
        QCOMPARE(cols[0].fieldName, QString("State"));
    }

    void testTableColumns_NonFloridaStation() {
        FloridaQSOPartyContest contest(ModeType::None, createNonFloridaStation());
        QList<TableColumn> cols = contest.getTableColumns();

        QCOMPARE(cols.size(), 1);
        QCOMPARE(cols[0].fieldName, QString("County"));
    }

    //
    // Station Validation
    //
    void testValidateStationInfo_ValidFloridaStation() {
        FloridaQSOPartyContest contest(ModeType::None, createFloridaStation());
        QStringList issues = contest.validateStationInfo();

        // Should have no critical errors, maybe info about power
        for (const QString& issue : issues) {
            QVERIFY2(!issue.startsWith("ERROR"), qPrintable(issue));
        }
    }

    void testValidateStationInfo_ValidNonFloridaStation() {
        FloridaQSOPartyContest contest(ModeType::None, createNonFloridaStation());
        QStringList issues = contest.validateStationInfo();

        // Should have no critical errors
        for (const QString& issue : issues) {
            QVERIFY2(!issue.startsWith("ERROR"), qPrintable(issue));
        }
    }

    void testValidateStationInfo_MissingCallsign() {
        StationInfo station = createFloridaStation();
        station.callsign = "";  // Missing callsign

        FloridaQSOPartyContest contest(ModeType::None, station);
        QStringList issues = contest.validateStationInfo();

        // Should have error about missing callsign
        bool foundCallsignError = false;
        for (const QString& issue : issues) {
            if (issue.contains("ERROR") && issue.contains("callsign")) {
                foundCallsignError = true;
                break;
            }
        }
        QVERIFY2(foundCallsignError, "Should have error about missing callsign");
    }

    void testValidateStationInfo_MissingState() {
        StationInfo station = createFloridaStation();
        station.state = "";  // Missing state

        FloridaQSOPartyContest contest(ModeType::None, station);
        QStringList issues = contest.validateStationInfo();

        // Should have error about missing state
        bool foundStateError = false;
        for (const QString& issue : issues) {
            if (issue.contains("ERROR") && issue.contains("state")) {
                foundStateError = true;
                break;
            }
        }
        QVERIFY2(foundStateError, "Should have error about missing state");
    }

    void testValidateStationInfo_FloridaStationMissingCounty() {
        StationInfo station = createFloridaStation();
        station.county = "";  // Missing county

        FloridaQSOPartyContest contest(ModeType::None, station);
        QStringList issues = contest.validateStationInfo();

        // Should have warning about missing county (not error)
        bool foundCountyWarning = false;
        for (const QString& issue : issues) {
            if (issue.contains("WARNING") && issue.contains("county")) {
                foundCountyWarning = true;
                break;
            }
        }
        QVERIFY2(foundCountyWarning, "Should have warning about missing county for FL station");
    }
};

QTEST_MAIN(TestFloridaQSOParty)
#include "test_flqp.moc"
