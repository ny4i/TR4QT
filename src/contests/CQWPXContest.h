#ifndef CQWPXCONTEST_H
#define CQWPXCONTEST_H

#include "ContestBase.h"

namespace TR4QT {

struct ContestMetadata;

/**
 * CQ WPX Contest (Worked All Prefixes)
 *
 * Exchange: RST + Serial Number (auto-increment)
 * Modes: CW and SSB (separate contests)
 * Multipliers:
 *   - Callsign prefixes (all-band, counted once per prefix)
 *   - Prefix is callsign up to and including first digit
 *     Examples: W1AW → W1, DL1ABC → DL1, JA1234XYZ → JA1
 * Scoring:
 *   - QSO Points vary by band and continent:
 *     - Same continent: 1 point
 *     - Different continent: 3 points (CW), 2 points (SSB)
 *     - 160m and 10m: Double points
 * Total Score: QSO points × Total prefixes
 *
 * Contest website: https://www.cqwpx.com/
 */
class CQWPXContest : public ContestBase {
public:
    explicit CQWPXContest(ModeType mode);
    ~CQWPXContest() override = default;

    // ===== Contest Identifiers =====
    // WA7BNM Contest Calendar IDs
    static constexpr int WA7BNM_ID_CW = 7;
    static constexpr int WA7BNM_ID_SSB = 8;

    // Cabrillo contest names
    static inline const QString CABRILLO_NAME_CW = "CQ-WPX-CW";
    static inline const QString CABRILLO_NAME_SSB = "CQ-WPX-SSB";

    // ADIF Contest-ID values
    static inline const QString ADIF_CONTEST_ID_CW = "CQ-WPX-CW";
    static inline const QString ADIF_CONTEST_ID_SSB = "CQ-WPX-SSB";

    // ===== Factory Methods =====
    static ContestMetadata getMetadata();
    static ContestBase* create(ModeType mode);

    // ===== Contest Identity =====
    QString getContestId() const override;
    QString getContestName() const override;
    ModeType getContestMode() const override { return m_mode; }

    // ===== Exchange Configuration =====
    QList<ExchangeField> getReceivedExchangeFields() const override;
    QList<ExchangeField> getSentExchangeFields() const override;
    QString formatSentExchange(int serialNumber, const QString& rst = "599") const override;
    bool validateReceivedExchange(const QString& exchange, QString& errorMsg) const override;
    QMap<QString, QString> parseReceivedExchange(const QString& exchange) const override;

    // ===== Scoring =====
    int calculateQSOPoints(
        const QSO& qso,
        const StationInfo& myStation) const override;

    int calculateTotalScore(
        int totalQSOPoints,
        const QMap<MultiplierType, int>& multiplierCounts) const override;

    // ===== Multipliers =====
    QList<MultiplierDefinition> getMultiplierTypes() const override;

    QString getMultiplierValue(
        const QSO& qso,
        MultiplierType multType,
        const QStringList& alreadyWorkedValues) const override;

    // ===== Special Rules =====
    bool usesSerialNumbers() const override { return true; }

    DuplicateCheckingRule getDuplicateCheckingRule() const override {
        return DuplicateCheckingRule::PerBandMode;  // Can work same call on different bands
    }

    QMap<QString, QString> getCabrilloHeaders() const override;

    // ===== Prefix Extraction =====
    /**
     * Extract WPX prefix from callsign
     * Prefix is everything up to and including the first digit
     * Examples:
     *   W1AW → W1
     *   DL1ABC → DL1
     *   JA1234XYZ → JA1
     *   VP9/G3XYZ → VP9 (use prefix before /)
     */
    static QString extractPrefix(const QString& callsign);

private:
    ModeType m_mode;  // CW or SSB
};

} // namespace TR4QT

#endif // CQWPXCONTEST_H
