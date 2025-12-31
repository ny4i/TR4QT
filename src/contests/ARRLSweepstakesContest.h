#ifndef ARRLSWEEPSTAKESCONTEST_H
#define ARRLSWEEPSTAKESCONTEST_H

#include "ContestBase.h"

namespace TR4QT {

struct ContestMetadata;

/**
 * ARRL Sweepstakes Contest (CW and SSB)
 *
 * Exchange: Serial# + Precedence + Check + Section
 *   - Serial: Sequential number starting from 1
 *   - Precedence: Q/A/B/M/U/S (operator category)
 *     Q = QRP (<5W output)
 *     A = Single Operator - all power levels
 *     B = Single Operator - school club
 *     M = Multi-Operator
 *     U = Unlimited
 *     S = School club
 *   - Check: Last 2 digits of year first licensed, or
 *            year born if licensed before 1984
 *   - Section: ARRL section (80 total: US + Canadian + DX)
 *
 * Modes: Separate contests for CW and SSB
 * Multipliers:
 *   - ARRL/RAC Sections (80 total)
 * Scoring:
 *   - 2 points per QSO
 *   - Score = QSO Points × Sections Worked
 *
 * Contest dates: First full weekend of November (CW and SSB)
 * Contest website: http://www.arrl.org/sweepstakes
 *
 * WA7BNM Contest Calendar IDs:
 *   - SS CW: 1
 *   - SS SSB: 2
 */
class ARRLSweepstakesContest : public ContestBase {
public:
    ARRLSweepstakesContest(ModeType mode, const StationInfo& myStation);
    ~ARRLSweepstakesContest() override = default;

    // ===== Contest Identifiers =====
    // WA7BNM Contest Calendar IDs
    static constexpr int WA7BNM_ID_CW = 1;
    static constexpr int WA7BNM_ID_SSB = 2;

    // Cabrillo contest names
    static inline const QString CABRILLO_NAME_CW = "ARRL-SS-CW";
    static inline const QString CABRILLO_NAME_SSB = "ARRL-SS-SSB";

    // ADIF Contest-ID values
    static inline const QString ADIF_CONTEST_ID_CW = "ARRL-SS-CW";
    static inline const QString ADIF_CONTEST_ID_SSB = "ARRL-SS-SSB";

    // ===== Factory Methods =====
    static ContestMetadata getMetadata();
    static ContestBase* create(ModeType mode, const StationInfo& myStation);

    // ===== Contest Identity =====
    QString getContestId() const override;
    QString getContestName() const override;
    ModeType getContestMode() const override { return m_mode; }
    QString getADIFContestId() const override;

    // ===== Exchange Configuration =====
    QList<ExchangeField> getReceivedExchangeFields() const override;
    QList<ExchangeField> getSentExchangeFields() const override;
    QList<TableColumn> getTableColumns() const override;  // 4 columns for SS exchange
    QString formatSentExchange(int serialNumber, const QString& rst = "599") const override;
    bool validateReceivedExchange(const QString& exchange, QString& errorMsg) const override;
    void parseReceivedExchange(const QString& exchange, QSO& qso) const override;

    // ===== Serial Numbers =====
    bool usesSerialNumbers() const override { return true; }

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

    // ===== Duplicate Checking =====
    DuplicateCheckingRule getDuplicateCheckingRule() const override {
        return DuplicateCheckingRule::PerBandMode;
    }

    // ===== Exchange Field Validation Helpers =====
    // Made public for use by SmartExchangeParser
    bool isValidPrecedence(const QString& precedence) const;
    bool isValidCheck(const QString& check) const;
    bool isValidSection(const QString& section) const;

private:
    ModeType m_mode;  // CW or SSB
};

} // namespace TR4QT

#endif // ARRLSWEEPSTAKESCONTEST_H
