#ifndef TEST_HELPERS_H
#define TEST_HELPERS_H

#include <QString>
#include <QDateTime>
#include <QList>
#include "../src/models/QSO.h"
#include "../src/models/StationInfo.h"
#include "../src/core/Types.h"
#include "../src/radio/RadioInterface.h"

using namespace TR4QT;

/**
 * Test helpers for creating QSO test data and assertions
 *
 * Provides utility functions to:
 * - Create valid/invalid QSO objects with realistic data
 * - Create test station info and radio states
 * - Compare QSOs for equality
 * - Format test failure reports
 */
namespace TestHelpers {

// ============================================================================
// QSO CREATION HELPERS
// ============================================================================

/**
 * Creates a valid QSO with minimal required fields
 *
 * @param callsign Station callsign (default: "W1AW")
 * @param band Band (default: B20M)
 * @param mode Mode (default: CW)
 * @return Valid QSO object ready for testing
 */
inline QSO createValidQSO(
    const QString& callsign = "W1AW",
    BandType band = BandType::Band20M,
    ModeType mode = ModeType::CW)
{
    QSO qso;
    qso.callsign = callsign;
    qso.band = band;
    qso.mode = mode;
    qso.timestamp = QDateTime::currentDateTimeUtc();
    qso.rstSent = "599";
    qso.rstReceived = "599";
    qso.frequency = 14025000;  // 14.025 MHz (20M CW)
    return qso;
}

/**
 * Creates a QSO with invalid/missing required fields
 *
 * @return Invalid QSO object for error testing
 */
inline QSO createInvalidQSO()
{
    QSO qso;
    // Callsign intentionally empty (required field)
    qso.band = BandType::None;
    qso.mode = ModeType::None;
    // No timestamp set
    return qso;
}

/**
 * Creates a complete QSO with all fields populated
 *
 * @param callsign Station callsign
 * @param frequency Frequency in Hz
 * @param band Band
 * @param mode Mode
 * @param rstSent RST sent
 * @param rstReceived RST received
 * @param exchangeSent Sent exchange
 * @param exchangeReceived Received exchange
 * @param operatorCallsign Operator callsign
 * @return Fully populated QSO object
 */
inline QSO createCompleteQSO(
    const QString& callsign,
    freq_t frequency,
    BandType band,
    ModeType mode,
    const QString& rstSent,
    const QString& rstReceived,
    const QString& exchangeSent,
    const QString& exchangeReceived,
    const QString& operatorCallsign = "")
{
    QSO qso;
    qso.callsign = callsign;
    qso.frequency = frequency;
    qso.band = band;
    qso.mode = mode;
    qso.rstSent = rstSent;
    qso.rstReceived = rstReceived;
    qso.exchangeSent = exchangeSent;
    qso.exchangeReceived = exchangeReceived;
    qso.timestamp = QDateTime::currentDateTimeUtc();

    if (!operatorCallsign.isEmpty()) {
        qso.operatorCall = operatorCallsign;
    }

    return qso;
}

/**
 * Creates a QSO for a specific contest
 *
 * @param callsign Station callsign
 * @param band Band
 * @param mode Mode
 * @param contestId Contest identifier (e.g., "CQWW", "CQWPX")
 * @return QSO configured for contest
 */
inline QSO createContestQSO(
    const QString& callsign,
    BandType band,
    ModeType mode,
    const QString& contestId)
{
    QSO qso = createValidQSO(callsign, band, mode);

    // Set contest-specific exchanges
    if (contestId == "CQWW") {
        qso.exchangeSent = "599 005";      // RST + CQ Zone
        qso.exchangeReceived = "599 005";
    } else if (contestId == "CQWPX") {
        qso.exchangeSent = "599 001";      // RST + Serial
        qso.exchangeReceived = "599 123";
    } else if (contestId == "WFD") {
        qso.exchangeSent = "1O CT";        // Class + Section
        qso.exchangeReceived = "2O MA";
    }

    return qso;
}

// ============================================================================
// STATION INFO HELPERS
// ============================================================================

/**
 * Creates test station information
 *
 * @param callsign Station callsign (default: "NY4I")
 * @param gridSquare Maidenhead grid (default: "EM95")
 * @return StationInfo object for testing
 */
inline StationInfo createTestStation(
    const QString& callsign = "NY4I",
    const QString& continent = "NA")
{
    StationInfo station;
    station.callsign = callsign;
    station.continent = continent;
    station.cqZone = 5;
    station.ituZone = 8;
    return station;
}

// ============================================================================
// RADIO STATE HELPERS
// ============================================================================

/**
 * Creates test radio state
 *
 * @param frequency Frequency in Hz (default: 14.025 MHz)
 * @param mode Mode (default: CW)
 * @param band Band (default: Band20M, auto-calculated from frequency)
 * @return RadioState object for testing
 */
inline RadioState createTestRadioState(
    freq_t frequency = 14025000,
    ModeType mode = ModeType::CW,
    BandType band = BandType::Band20M)
{
    RadioState state;
    state.frequencyA = frequency;
    state.modeA = mode;
    state.bandA = band;
    return state;
}

/**
 * Creates radio state for disconnected radio
 *
 * @return RadioState with None values (simulating no radio)
 */
inline RadioState createDisconnectedRadioState()
{
    RadioState state;
    state.frequencyA = 0;
    state.modeA = ModeType::None;
    state.bandA = BandType::None;
    return state;
}

// ============================================================================
// COMPARISON HELPERS
// ============================================================================

/**
 * Compares two QSOs for equality (ignoring ID and timestamp precision)
 *
 * @param a First QSO
 * @param b Second QSO
 * @return true if QSOs match on all important fields
 */
inline bool compareQSOs(const QSO& a, const QSO& b)
{
    return a.callsign == b.callsign &&
           a.band == b.band &&
           a.mode == b.mode &&
           a.frequency == b.frequency &&
           a.rstSent == b.rstSent &&
           a.rstReceived == b.rstReceived &&
           a.exchangeSent == b.exchangeSent &&
           a.exchangeReceived == b.exchangeReceived;
    // Note: ID and timestamp intentionally not compared (auto-generated)
}

/**
 * Formats differences between two QSOs for test failure messages
 *
 * @param expected Expected QSO
 * @param actual Actual QSO
 * @return Human-readable difference report
 */
inline QString formatQSODifferences(const QSO& expected, const QSO& actual)
{
    QStringList diffs;

    if (expected.callsign != actual.callsign) {
        diffs << QString("Callsign: expected '%1', got '%2'")
                 .arg(expected.callsign, actual.callsign);
    }
    if (expected.band != actual.band) {
        diffs << QString("Band: expected %1, got %2")
                 .arg(static_cast<int>(expected.band))
                 .arg(static_cast<int>(actual.band));
    }
    if (expected.mode != actual.mode) {
        diffs << QString("Mode: expected %1, got %2")
                 .arg(static_cast<int>(expected.mode))
                 .arg(static_cast<int>(actual.mode));
    }
    if (expected.frequency != actual.frequency) {
        diffs << QString("Frequency: expected %1 Hz, got %2 Hz")
                 .arg(expected.frequency)
                 .arg(actual.frequency);
    }
    if (expected.exchangeReceived != actual.exchangeReceived) {
        diffs << QString("Exchange: expected '%1', got '%2'")
                 .arg(expected.exchangeReceived, actual.exchangeReceived);
    }

    return diffs.isEmpty() ? "No differences found" : diffs.join("\n");
}

// ============================================================================
// TEST REPORT HELPERS
// ============================================================================

/**
 * Formats a list of errors as a test report
 *
 * @param errors List of error messages
 * @return Formatted report string
 */
inline QString formatTestReport(const QList<QString>& errors)
{
    if (errors.isEmpty()) {
        return "All checks passed";
    }

    QString report = QString("%1 error(s) found:\n").arg(errors.size());
    for (int i = 0; i < errors.size(); ++i) {
        report += QString("  %1. %2\n").arg(i + 1).arg(errors[i]);
    }
    return report;
}

/**
 * Creates a temporary test database path
 *
 * @param testName Name of test (used in filename)
 * @return Path to temporary database file
 */
inline QString createTempDatabasePath(const QString& testName)
{
    return QString("/tmp/tr4qt_test_%1_%2.db")
           .arg(testName)
           .arg(QDateTime::currentMSecsSinceEpoch());
}

} // namespace TestHelpers

#endif // TEST_HELPERS_H
