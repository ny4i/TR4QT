/**
 * Unit tests for FrequencyInputService
 *
 * Tests frequency parsing from callsign field input:
 * - Decimal MHz format (14.200)
 * - Absolute kHz format (14210)
 * - Offset from band edge (300)
 * - Non-frequency input (callsigns)
 * - Edge cases and error handling
 */

#include <QtTest/QtTest>
#include "../src/services/FrequencyInputService.h"

using namespace TR4QT;

class TestFrequencyInputService : public QObject {
    Q_OBJECT

private:
    FrequencyInputService m_service;

private slots:
    /**
     * Test: Decimal MHz format (14.200 -> 14200 kHz)
     */
    void testDecimalMHzFormat() {
        auto result = m_service.parseFrequencyInput("14.200", BandType::Band20M);

        QVERIFY(result.isFrequency);
        QCOMPARE(result.frequencyHz, static_cast<freq_t>(14200000));  // 14.200 MHz in Hz
        QVERIFY(result.errorMessage.isEmpty());
        QVERIFY(!result.statusMessage.isEmpty());
    }

    /**
     * Test: Various decimal MHz values
     */
    void testDecimalMHzVariants() {
        // 7.040 MHz (40m CW)
        auto result1 = m_service.parseFrequencyInput("7.040", BandType::Band40M);
        QVERIFY(result1.isFrequency);
        QCOMPARE(result1.frequencyHz, static_cast<freq_t>(7040000));

        // 21.350 MHz (15m SSB)
        auto result2 = m_service.parseFrequencyInput("21.350", BandType::Band15M);
        QVERIFY(result2.isFrequency);
        QCOMPARE(result2.frequencyHz, static_cast<freq_t>(21350000));

        // 3.573 MHz (80m FT8)
        auto result3 = m_service.parseFrequencyInput("3.573", BandType::Band80M);
        QVERIFY(result3.isFrequency);
        QCOMPARE(result3.frequencyHz, static_cast<freq_t>(3573000));
    }

    /**
     * Test: Absolute kHz format (14210 -> 14210 kHz)
     */
    void testAbsoluteKHzFormat() {
        auto result = m_service.parseFrequencyInput("14210", BandType::Band20M);

        QVERIFY(result.isFrequency);
        QCOMPARE(result.frequencyHz, static_cast<freq_t>(14210000));  // 14210 kHz in Hz
        QVERIFY(result.errorMessage.isEmpty());
    }

    /**
     * Test: Various absolute kHz values
     */
    void testAbsoluteKHzVariants() {
        // 7040 kHz
        auto result1 = m_service.parseFrequencyInput("7040", BandType::Band40M);
        QVERIFY(result1.isFrequency);
        QCOMPARE(result1.frequencyHz, static_cast<freq_t>(7040000));

        // 21350 kHz
        auto result2 = m_service.parseFrequencyInput("21350", BandType::Band15M);
        QVERIFY(result2.isFrequency);
        QCOMPARE(result2.frequencyHz, static_cast<freq_t>(21350000));

        // 28500 kHz (10m)
        auto result3 = m_service.parseFrequencyInput("28500", BandType::Band10M);
        QVERIFY(result3.isFrequency);
        QCOMPARE(result3.frequencyHz, static_cast<freq_t>(28500000));
    }

    /**
     * Test: Offset from band edge (300 on 20m -> 14000 + 300 = 14300 kHz)
     */
    void testOffsetFromBandEdge() {
        // 300 on 20m -> 14000 + 300 = 14300 kHz
        auto result = m_service.parseFrequencyInput("300", BandType::Band20M);

        QVERIFY(result.isFrequency);
        QCOMPARE(result.frequencyHz, static_cast<freq_t>(14300000));  // 14300 kHz in Hz
        QVERIFY(result.errorMessage.isEmpty());
    }

    /**
     * Test: Offset on various bands
     */
    void testOffsetOnVariousBands() {
        // 40 on 40m -> 7000 + 40 = 7040 kHz
        auto result1 = m_service.parseFrequencyInput("40", BandType::Band40M);
        QVERIFY(result1.isFrequency);
        QCOMPARE(result1.frequencyHz, static_cast<freq_t>(7040000));

        // 350 on 15m -> 21000 + 350 = 21350 kHz
        auto result2 = m_service.parseFrequencyInput("350", BandType::Band15M);
        QVERIFY(result2.isFrequency);
        QCOMPARE(result2.frequencyHz, static_cast<freq_t>(21350000));

        // 200 on 80m -> 3500 + 200 = 3700 kHz
        auto result3 = m_service.parseFrequencyInput("200", BandType::Band80M);
        QVERIFY(result3.isFrequency);
        QCOMPARE(result3.frequencyHz, static_cast<freq_t>(3700000));

        // 100 on 160m -> 1800 + 100 = 1900 kHz
        auto result4 = m_service.parseFrequencyInput("100", BandType::Band160M);
        QVERIFY(result4.isFrequency);
        QCOMPARE(result4.frequencyHz, static_cast<freq_t>(1900000));
    }

    /**
     * Test: Callsigns are NOT recognized as frequencies
     */
    void testCallsignNotFrequency() {
        QStringList callsigns = {"K1ABC", "W3XYZ", "VE3ABC", "G4ABC", "JA1ABC"};

        for (const QString& callsign : callsigns) {
            auto result = m_service.parseFrequencyInput(callsign, BandType::Band20M);

            QVERIFY2(!result.isFrequency,
                     qPrintable(QString("Callsign %1 incorrectly parsed as frequency").arg(callsign)));
            QCOMPARE(result.frequencyHz, static_cast<freq_t>(0));
        }
    }

    /**
     * Test: Empty input returns non-frequency result
     */
    void testEmptyInput() {
        auto result = m_service.parseFrequencyInput("", BandType::Band20M);

        QVERIFY(!result.isFrequency);
        QCOMPARE(result.frequencyHz, static_cast<freq_t>(0));
        QVERIFY(result.errorMessage.isEmpty());
    }

    /**
     * Test: Threshold between offset and absolute (999 vs 1000)
     */
    void testOffsetThreshold() {
        // 999 should be treated as offset (< 1000)
        auto result1 = m_service.parseFrequencyInput("999", BandType::Band20M);
        QVERIFY(result1.isFrequency);
        QCOMPARE(result1.frequencyHz, static_cast<freq_t>(14999000));  // 14000 + 999 = 14999 kHz

        // 1000 should be treated as absolute kHz (>= 1000)
        auto result2 = m_service.parseFrequencyInput("1000", BandType::Band20M);
        QVERIFY(result2.isFrequency);
        QCOMPARE(result2.frequencyHz, static_cast<freq_t>(1000000));  // 1000 kHz absolute
    }

    /**
     * Test: Band edge frequencies are correct
     */
    void testBandEdges() {
        // Test offset of 0 returns band edge (but 0 is not > 0, so won't be frequency)
        // Instead test offset of 1 to verify band edge

        struct BandTest {
            BandType band;
            freq_t expectedEdgeHz;  // band edge + 1 kHz
        };

        QList<BandTest> tests = {
            {BandType::Band160M, 1801000},    // 1800 + 1 = 1801 kHz
            {BandType::Band80M,  3501000},    // 3500 + 1 = 3501 kHz
            {BandType::Band40M,  7001000},    // 7000 + 1 = 7001 kHz
            {BandType::Band30M,  10101000},   // 10100 + 1 = 10101 kHz
            {BandType::Band20M,  14001000},   // 14000 + 1 = 14001 kHz
            {BandType::Band17M,  18069000},   // 18068 + 1 = 18069 kHz
            {BandType::Band15M,  21001000},   // 21000 + 1 = 21001 kHz
            {BandType::Band12M,  24891000},   // 24890 + 1 = 24891 kHz
            {BandType::Band10M,  28001000},   // 28000 + 1 = 28001 kHz
            {BandType::Band6M,   50001000},   // 50000 + 1 = 50001 kHz
            {BandType::Band2M,   144001000},  // 144000 + 1 = 144001 kHz
        };

        for (const auto& test : tests) {
            auto result = m_service.parseFrequencyInput("1", test.band);
            QVERIFY2(result.isFrequency,
                     qPrintable(QString("Band %1 offset 1 not parsed as frequency").arg(static_cast<int>(test.band))));
            QCOMPARE(result.frequencyHz, test.expectedEdgeHz);
        }
    }

    /**
     * Test: Invalid band returns error
     */
    void testInvalidBand() {
        auto result = m_service.parseFrequencyInput("100", BandType::None);

        QVERIFY(!result.isFrequency);
        QVERIFY(!result.errorMessage.isEmpty());
    }

    /**
     * Test: Zero input is not a frequency
     */
    void testZeroInput() {
        auto result = m_service.parseFrequencyInput("0", BandType::Band20M);

        QVERIFY(!result.isFrequency);
        QCOMPARE(result.frequencyHz, static_cast<freq_t>(0));
    }

    /**
     * Test: Negative decimal is not a frequency
     */
    void testNegativeDecimal() {
        auto result = m_service.parseFrequencyInput("-14.200", BandType::Band20M);

        QVERIFY(!result.isFrequency);
    }

    /**
     * Test: Status message is populated on success
     */
    void testStatusMessage() {
        auto result = m_service.parseFrequencyInput("14.200", BandType::Band20M);

        QVERIFY(result.isFrequency);
        QVERIFY(result.statusMessage.contains("14200"));
    }

    /**
     * Test: Mixed alphanumeric is not a frequency
     */
    void testMixedAlphanumeric() {
        QStringList inputs = {"14A00", "1420X", "14.2A0", "K1"};

        for (const QString& input : inputs) {
            auto result = m_service.parseFrequencyInput(input, BandType::Band20M);
            QVERIFY2(!result.isFrequency,
                     qPrintable(QString("Mixed input '%1' incorrectly parsed as frequency").arg(input)));
        }
    }
};

QTEST_GUILESS_MAIN(TestFrequencyInputService)
#include "test_frequency_input_service.moc"
