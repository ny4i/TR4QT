#ifndef ARRLRTTYROUNDUPCONTEST_H
#define ARRLRTTYROUNDUPCONTEST_H

#include "ContestBase.h"

namespace TR4QT {

struct ContestMetadata;

/**
 * ARRL RTTY Roundup
 *
 * Exchange:
 *   - W/VE stations: RST + State/Province
 *   - DX stations: RST + Serial Number
 * Mode: RTTY (digital modes)
 * Multipliers:
 *   - DXCC Entities (excluding US/Canada)
 *   - US States
 *   - Canadian Provinces
 *   - Multipliers count once overall (not per-band)
 * Scoring:
 *   - 1 point per QSO
 *   - Final score = QSO points × Total multipliers
 * Bands: 80, 40, 20, 15, 10 meters
 * Duration: 1800 UTC Saturday to 2359 UTC Sunday (30 hours, operate 24)
 *
 * Contest website: https://contests.arrl.org/rttyru/
 */
class ARRLRTTYRoundupContest : public ContestBase {
public:
    ARRLRTTYRoundupContest(const StationInfo& myStation);
    ~ARRLRTTYRoundupContest() override = default;

    // ===== Contest Identifiers =====
    static constexpr int WA7BNM_ID = 179;
    static inline const QString CABRILLO_NAME = "ARRL-RTTY";
    static inline const QString ADIF_CONTEST_ID = "ARRL-RTTY";

    // ===== Factory Methods =====
    static ContestMetadata getMetadata();
    static ContestBase* create(ModeType mode, const StationInfo& myStation);

    // ===== Contest Identity =====
    QString getContestId() const override;
    QString getContestName() const override;
    ModeType getContestMode() const override { return ModeType::RTTY; }
    QString getADIFContestId() const override;

    // ===== Exchange Configuration =====
    QList<ExchangeField> getReceivedExchangeFields() const override;
    QList<ExchangeField> getSentExchangeFields() const override;
    QList<TableColumn> getTableColumns() const override;
    QString formatSentExchange(int serialNumber, const QString& rst = "599") const override;
    bool validateReceivedExchange(const QString& exchange, QString& errorMsg) const override;
    void parseReceivedExchange(const QString& exchange, QSO& qso) const override;

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
    bool usesSerialNumbers() const override { return true; }  // DX stations send serial

    DuplicateCheckingRule getDuplicateCheckingRule() const override {
        return DuplicateCheckingRule::PerBandMode;  // Can work same call on different bands
    }

    // ===== Band Restrictions =====
    QList<BandType> getAllowedBands() const override;  // RTTY: excludes 160m

    QMap<QString, QString> getCabrilloHeaders() const override;

private:
    static bool isValidState(const QString& state);
    static bool isValidProvince(const QString& province);
};

} // namespace TR4QT

#endif // ARRLRTTYROUNDUPCONTEST_H
