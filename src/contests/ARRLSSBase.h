#ifndef ARRLSSBASE_H
#define ARRLSSBASE_H

#include "ContestBase.h"

namespace TR4QT {

/**
 * ARRL Sweepstakes Contest Base Class
 *
 * Shared logic for all ARRL SS contests (CW, SSB)
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
 * Multipliers:
 *   - ARRL/RAC Sections (80 total, all-band)
 * Scoring:
 *   - 2 points per QSO
 *   - Score = QSO Points × Sections Worked
 *
 * Contest website: http://www.arrl.org/sweepstakes
 */
class ARRLSSBase : public ContestBase {
protected:
    explicit ARRLSSBase(const StationInfo& myStation)
        : ContestBase(myStation) {}

public:
    ~ARRLSSBase() override = default;

    // ===== Exchange Configuration (shared) =====
    QList<ExchangeField> getReceivedExchangeFields() const override;
    QList<ExchangeField> getSentExchangeFields() const override;
    QList<TableColumn> getTableColumns() const override;
    QList<ContestConfigField> getConfigFields() const override;
    QString formatSentExchange(int serialNumber, const QString& rst = "599") const override;
    bool validateReceivedExchange(const QString& exchange, QString& errorMsg) const override;
    void parseReceivedExchange(const QString& exchange, QSO& qso) const override;

    // ===== Serial Numbers =====
    bool usesSerialNumbers() const override { return true; }

    // ===== Scoring (shared) =====
    int calculateQSOPoints(
        const QSO& qso,
        const StationInfo& myStation) const override;

    int calculateTotalScore(
        int totalQSOPoints,
        const QMap<MultiplierType, int>& multiplierCounts) const override;

    // ===== Multipliers (shared) =====
    QList<MultiplierDefinition> getMultiplierTypes() const override;
    QString getMultiplierValue(
        const QSO& qso,
        MultiplierType multType,
        const QStringList& alreadyWorkedValues) const override;

    // ===== Special Rules =====
    DuplicateCheckingRule getDuplicateCheckingRule() const override {
        return DuplicateCheckingRule::AllBandMode;  // One contact per call per contest
    }

    // ===== Bands (all HF bands including 160m) =====
    QList<BandType> getAllowedBands() const override {
        return {BandType::Band160M, BandType::Band80M, BandType::Band40M,
                BandType::Band20M, BandType::Band15M, BandType::Band10M};
    }

protected:
    /**
     * Validate precedence character (Q/A/B/M/U/S)
     */
    static bool isValidPrecedence(QChar prec);

    /**
     * Validate check (2 digits: 00-99)
     */
    static bool isValidCheck(const QString& check);
};

} // namespace TR4QT

#endif // ARRLSSBASE_H
