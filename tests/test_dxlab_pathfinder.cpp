/**
 * Unit tests for DXLabPathfinder
 *
 * Tests the callsign parsing logic used to extract callsigns from
 * DXLab's ADIF-like DDE command format:
 *   "002getqslinfo<callsign:N>CALL"
 *
 * Note: DDE server/client functionality is not tested here as it
 * requires a running Windows DDE environment with SpotCollector.
 */

#include <QtTest/QtTest>
#include "../src/services/DXLabPathfinder.h"

class TestDXLabPathfinder : public QObject {
    Q_OBJECT

private slots:
    void testParseCallsign_basic() {
        QCOMPARE(DXLabPathfinder::parseCallsign("002getqslinfo<callsign:4>AK7G"),
                 QStringLiteral("AK7G"));
    }

    void testParseCallsign_longerCall() {
        QCOMPARE(DXLabPathfinder::parseCallsign("002getqslinfo<callsign:5>OK2LA"),
                 QStringLiteral("OK2LA"));
    }

    void testParseCallsign_prefixedCall() {
        QCOMPARE(DXLabPathfinder::parseCallsign("002getqslinfo<callsign:6>3D2USU"),
                 QStringLiteral("3D2USU"));
    }

    void testParseCallsign_shortCall() {
        QCOMPARE(DXLabPathfinder::parseCallsign("002getqslinfo<callsign:4>P44W"),
                 QStringLiteral("P44W"));
    }

    void testParseCallsign_caseInsensitiveTag() {
        // The <callsign:N> tag should be matched case-insensitively
        QCOMPARE(DXLabPathfinder::parseCallsign("002getqslinfo<CALLSIGN:4>AK7G"),
                 QStringLiteral("AK7G"));
    }

    void testParseCallsign_emptyCommand() {
        QVERIFY(DXLabPathfinder::parseCallsign("").isEmpty());
    }

    void testParseCallsign_noCallsignField() {
        QVERIFY(DXLabPathfinder::parseCallsign("002getqslinfo").isEmpty());
    }

    void testParseCallsign_malformedTag() {
        QVERIFY(DXLabPathfinder::parseCallsign("002getqslinfo<callsign:>AK7G").isEmpty());
    }

    void testParseCallsign_lengthTrimsExtraData() {
        // Length says 4 but value is longer — should truncate to 4
        QCOMPARE(DXLabPathfinder::parseCallsign("002getqslinfo<callsign:4>AK7Gextra"),
                 QStringLiteral("AK7G"));
    }

    void testParseCallsign_trailingSpaces() {
        // Length includes padding spaces — trimmed result
        QCOMPARE(DXLabPathfinder::parseCallsign("002getqslinfo<callsign:6>AK7G  "),
                 QStringLiteral("AK7G"));
    }
};

QTEST_MAIN(TestDXLabPathfinder)
#include "test_dxlab_pathfinder.moc"
