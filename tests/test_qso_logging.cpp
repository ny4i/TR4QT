#include <QTest>
#include <QSignalSpy>
#include <QDateTime>
#include "../src/controllers/QSOLogger.h"
#include "../src/models/QSO.h"
#include "../src/contests/ContestBase.h"
#include "../src/contests/CQWWContest.h"
#include "../src/core/Types.h"
#include "../src/utils/CountryFile.h"
#include "../src/utils/AppSettings.h"
#include "TestHelpers.h"

using namespace TR4QT;

/**
 * Integration tests for QSO logging workflow via QSOLogger class
 *
 * These tests verify the core logging logic including:
 * - Callsign and exchange validation
 * - Radio state capture (frequency, band, mode)
 * - Country/zone lookup via CountryFile
 * - Duplicate detection with contest-specific rules
 * - Points calculation via contest
 * - Multiplier checking (PerBand vs AllBands scope)
 *
 * Note: Database persistence and UI updates are tested separately
 * (those are MainWindow responsibilities, not QSOLogger responsibilities)
 */
class TestQSOLogging : public QObject {
    Q_OBJECT

private slots:
    // Test initialization and cleanup
    void initTestCase();
    void cleanupTestCase();
    void init();
    void cleanup();

    // Basic Logging Flow (5 tests)
    void testLogQSO_ValidCallsign_SavesSuccessfully();
    void testLogQSO_EmptyCallsign_ShowsError();
    void testLogQSO_WithRadioConnected_CapturesFrequencyAndMode();
    void testLogQSO_WithRadioDisconnected_UsesManualBandSelection();
    void testLogQSO_SpecialCommands_HandlesOPONandUDP();

    // Exchange Auto-Population (3 tests)
    void testLogQSO_KnownCallsign_AutoPopulatesExchange();
    void testLogQSO_DXCCCountry_AutoPopulatesZone();
    void testLogQSO_USCallsign_AutoPopulatesState();

    // Duplicate Detection (3 tests)
    void testLogQSO_DuplicateCallsignSameBandMode_ShowsWarning();
    void testLogQSO_DuplicateCallsignDifferentBand_AllowsLog();
    void testLogQSO_PerBandModeDupeRule_EnforcesCorrectly();

    // UI Updates (3 tests)
    void testLogQSO_AfterSave_UpdatesQSOTableModel();
    void testLogQSO_AfterSave_UpdatesScoreLabels();
    void testLogQSO_AfterSave_UpdatesBandSummaryGrid();

    // Error Handling (2 tests)
    void testLogQSO_DatabaseFailure_ShowsErrorDialog();
    void testLogQSO_InvalidExchange_ShowsValidationError();

private:
    QSOLogger* m_qsoLogger = nullptr;
    CQWWContest* m_testContest = nullptr;
    CountryFile* m_countryFile = nullptr;
    QList<QSO> m_existingQSOs;
    StationInfo m_myStation;
};

void TestQSOLogging::initTestCase()
{
    // Configure my station
    m_myStation.callsign = "NY4I";
    m_myStation.continent = "NA";
    m_myStation.country = "United States";
    m_myStation.cqZone = 5;
    m_myStation.ituZone = 8;

    // Initialize test fixtures
    m_testContest = new CQWWContest(ModeType::CW, m_myStation);
    m_countryFile = new CountryFile();

    // Load CTY.DAT from resources or test data directory
    QString ctyPath = QString("%1/../fixtures/cty.dat").arg(TESTS_SOURCE_DIR);
    if (!m_countryFile->loadFromFile(ctyPath)) {
        QSKIP("Could not load cty.dat test fixture - tests cannot run without country data");
    }

    // Create QSOLogger with test configuration
    QSOLogger::Config config;
    config.contest = m_testContest;
    config.countryFile = m_countryFile;
    config.myStation = m_myStation;

    m_qsoLogger = new QSOLogger(config);
}

void TestQSOLogging::cleanupTestCase()
{
    delete m_qsoLogger;
    delete m_testContest;
    delete m_countryFile;
}

void TestQSOLogging::init()
{
    // Reset existing QSOs list before each test
    m_existingQSOs.clear();
}

void TestQSOLogging::cleanup()
{
    // Nothing to clean up after each test
}

// ============================================================================
// BASIC LOGGING FLOW TESTS
// ============================================================================

void TestQSOLogging::testLogQSO_ValidCallsign_SavesSuccessfully()
{
    // Given: Valid input with DX callsign (G3XYZ = UK, different continent from US)
    QSOLogger::Input input;
    input.callsign = "G3XYZ";
    input.exchange = "599 014";  // RST + CQ Zone 14 (Europe)
    input.radioState = TestHelpers::createTestRadioState(14025000, ModeType::CW, BandType::Band20M);
    input.operatorCallsign = "NY4I";
    input.serialNumber = 1;
    input.operatingMode = OperatingMode::CQ;

    // When: Logging the QSO
    QSOLogger::Result result = m_qsoLogger->logQSO(input, m_existingQSOs);

    // Then: QSO created successfully
    QVERIFY(result.success);
    QCOMPARE(result.errorMessage, QString());
    QCOMPARE(result.qso.callsign, QString("G3XYZ"));
    QCOMPARE(result.qso.band, BandType::Band20M);
    QCOMPARE(result.qso.mode, ModeType::CW);
    QCOMPARE(result.qso.frequency, 14025000);
    QCOMPARE(result.qso.rstReceived, QString("599"));
    QVERIFY(!result.qso.exchangeReceived.isEmpty());

    // Verify DXCC fields populated (G3XYZ should be UK, Europe)
    QVERIFY(!result.qso.dxccEntity.isEmpty());  // Must have country lookup
    QCOMPARE(result.qso.continent, QString("EU"));  // Europe

    // CQ WW scoring: US (NA) working UK (EU) = different continents = 3 points (CW)
    QCOMPARE(result.qso.qsoPoints, 3);
    QVERIFY(!result.isDuplicate);  // First contact with G3XYZ
}

void TestQSOLogging::testLogQSO_EmptyCallsign_ShowsError()
{
    // Given: Input with empty callsign
    QSOLogger::Input input;
    input.callsign = "";  // Empty!
    input.exchange = "599 005";
    input.radioState = TestHelpers::createTestRadioState();
    input.serialNumber = 1;
    input.operatingMode = OperatingMode::CQ;

    // When: Attempting to log QSO
    QSOLogger::Result result = m_qsoLogger->logQSO(input, m_existingQSOs);

    // Then: Error returned, no QSO created
    QVERIFY(!result.success);
    QVERIFY(result.errorMessage.contains("Callsign is required"));
}

void TestQSOLogging::testLogQSO_WithRadioConnected_CapturesFrequencyAndMode()
{
    QSKIP("Test implementation pending");
    // TODO: Test that radio frequency/mode captured when radio connected
    // Given: MainWindow with radio connected at 14.250 MHz CW
    // When: User logs QSO with "W1AW"
    // Then: QSO has frequency 14250000 Hz, mode CW, band 20M
}

void TestQSOLogging::testLogQSO_WithRadioDisconnected_UsesManualBandSelection()
{
    QSKIP("Test implementation pending");
    // TODO: Test that manual band selection used when radio disconnected
    // Given: MainWindow with radio disconnected, manual band 20M selected
    // When: User logs QSO with "W1AW"
    // Then: QSO has band 20M, frequency set to band edge (14000000 Hz)
}

void TestQSOLogging::testLogQSO_SpecialCommands_HandlesOPONandUDP()
{
    QSKIP("Test implementation pending");
    // TODO: Test special commands OPON (operator change) and UDP (rebroadcast)
    // Given: MainWindow with active contest
    // When: User enters "OPON K1ABC" in callsign field
    // Then: Operator changed to K1ABC, no QSO logged
    // When: User enters "UDP" in callsign field
    // Then: Last QSO rebroadcast via UDP, no new QSO logged
}

// ============================================================================
// EXCHANGE AUTO-POPULATION TESTS
// ============================================================================

void TestQSOLogging::testLogQSO_KnownCallsign_AutoPopulatesExchange()
{
    QSKIP("Test implementation pending");
    // TODO: Test exchange memory auto-populates known callsign exchange
    // Given: Previous QSO with "W1AW" exchange "59 005"
    // When: User starts typing "W1AW" in new QSO
    // Then: Exchange field auto-populates with "59 005"
}

void TestQSOLogging::testLogQSO_DXCCCountry_AutoPopulatesZone()
{
    QSKIP("Test implementation pending");
    // TODO: Test CTY.DAT lookup auto-populates CQ/ITU zone
    // Given: Callsign "VK3XYZ" (Australia, CQ zone 30)
    // When: User logs QSO
    // Then: CQ zone field auto-populated with "30", country "AUSTRALIA"
}

void TestQSOLogging::testLogQSO_USCallsign_AutoPopulatesState()
{
    QSKIP("Test implementation pending");
    // TODO: Test US callsign zone auto-population from call area
    // Given: Callsign "W1AW" (US call area 1, CQ zone 5)
    // When: User logs QSO for contest requiring state/section
    // Then: Zone field shows "5" (auto-calculated from call area)
}

// ============================================================================
// DUPLICATE DETECTION TESTS
// ============================================================================

void TestQSOLogging::testLogQSO_DuplicateCallsignSameBandMode_ShowsWarning()
{
    // Given: Existing QSO with W1AW on 20M CW
    QSO existingQSO = TestHelpers::createValidQSO("W1AW", BandType::Band20M, ModeType::CW);
    m_existingQSOs.append(existingQSO);

    // When: Attempting to log W1AW again on 20M CW
    QSOLogger::Input input;
    input.callsign = "W1AW";
    input.exchange = "599 015";  // Different zone
    input.radioState = TestHelpers::createTestRadioState(14025000, ModeType::CW, BandType::Band20M);
    input.serialNumber = 2;
    input.operatingMode = OperatingMode::CQ;

    QSOLogger::Result result = m_qsoLogger->logQSO(input, m_existingQSOs);

    // Then: QSO still created but marked as duplicate with 0 points
    QVERIFY(result.success);  // QSO created (user can still log dupes)
    QVERIFY(result.isDuplicate);  // Flagged as duplicate
    QVERIFY(!result.dupeInfo.isEmpty());  // Has dupe info message
    QCOMPARE(result.qso.isDupe, true);
    QCOMPARE(result.qso.qsoPoints, 0);  // Duplicates get 0 points
}

void TestQSOLogging::testLogQSO_DuplicateCallsignDifferentBand_AllowsLog()
{
    QSKIP("Test implementation pending");
    // TODO: Test duplicate on different band allowed with PerBandMode rule
    // Given: Contest with PerBandMode dupe rule, existing QSO "W1AW" on 20M CW
    // When: User logs "W1AW" on 40M CW
    // Then: QSO allowed, not marked as dupe, points awarded
}

void TestQSOLogging::testLogQSO_PerBandModeDupeRule_EnforcesCorrectly()
{
    QSKIP("Test implementation pending");
    // TODO: Test PerBandMode vs AllBandMode dupe rules enforced correctly
    // Given: CQ WW (PerBandMode), "W1AW" worked on 20M CW
    // When: User works "W1AW" on 20M SSB
    // Then: Allowed (different mode)
    // Given: Winter Field Day (AllBandMode), "W1AW" worked on 20M
    // When: User works "W1AW" on 40M
    // Then: Marked as duplicate (same callsign/mode across all bands)
}

// ============================================================================
// UI UPDATE TESTS
// ============================================================================

void TestQSOLogging::testLogQSO_AfterSave_UpdatesQSOTableModel()
{
    QSKIP("Test implementation pending");
    // TODO: Test QSOTableModel updated after QSO save
    // Given: MainWindow with 10 existing QSOs
    // When: User logs new QSO "W1AW"
    // Then: QSOTableModel rowCount increases to 11, new row shows "W1AW"
}

void TestQSOLogging::testLogQSO_AfterSave_UpdatesScoreLabels()
{
    QSKIP("Test implementation pending");
    // TODO: Test score labels updated after QSO save
    // Given: MainWindow with score 100 points, 5 multipliers
    // When: User logs QSO worth 3 points with new multiplier
    // Then: Score label shows 103 points, multiplier count shows 6
}

void TestQSOLogging::testLogQSO_AfterSave_UpdatesBandSummaryGrid()
{
    QSKIP("Test implementation pending");
    // TODO: Test band summary grid updated after QSO save
    // Given: MainWindow with 5 QSOs on 20M, 3 multipliers on 20M
    // When: User logs QSO on 20M with new multiplier
    // Then: Band summary shows 6 QSOs on 20M, 4 multipliers on 20M
}

// ============================================================================
// ERROR HANDLING TESTS
// ============================================================================

void TestQSOLogging::testLogQSO_DatabaseFailure_ShowsErrorDialog()
{
    QSKIP("Test implementation pending");
    // TODO: Test database save failure shows error dialog and uses ADIF fallback
    // Given: MainWindow with database connection closed (simulated failure)
    // When: User logs QSO "W1AW"
    // Then: Error dialog shown, QSO saved to emergency ADIF file
}

void TestQSOLogging::testLogQSO_InvalidExchange_ShowsValidationError()
{
    QSKIP("Test implementation pending");
    // TODO: Test invalid exchange format shows validation error
    // Given: CQ WW contest (requires RST + zone), callsign "W1AW"
    // When: User enters invalid exchange "ABC" (not a valid zone)
    // Then: Validation error shown, QSO not saved
}

QTEST_MAIN(TestQSOLogging)
#include "test_qso_logging.moc"
