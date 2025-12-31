#ifndef CQWWCONTEST_H
#define CQWWCONTEST_H

#include "ContestBase.h"

namespace TR4QT {

struct ContestMetadata;

/**
 * CQ World Wide DX Contest
 *
 * Exchange: RST + CQ Zone (1-40)
 * Modes: CW and SSB (separate contests)
 * Multipliers:
 *   - DXCC Countries (per band)
 *   - CQ Zones (per band)
 * Scoring:
 *   - Same continent, different country: 1 point (CW), 1 point (SSB)
 *   - Different continent: 3 points (CW), 2 points (SSB)
 *   - Special rule for W/VE stations working each other: 2 points
 * Total Score: QSO points × (Countries + Zones)
 *
 * Contest website: https://www.cqww.com/
 */
class CQWWContest : public ContestBase {
public:
    CQWWContest(ModeType mode, const StationInfo& myStation);
    ~CQWWContest() override = default;

    // ===== Contest Identifiers =====
    // WA7BNM Contest Calendar IDs
    static constexpr int WA7BNM_ID_CW = 3;
    static constexpr int WA7BNM_ID_SSB = 4;

    // Cabrillo contest names
    static inline const QString CABRILLO_NAME_CW = "CQ-WW-CW";
    static inline const QString CABRILLO_NAME_SSB = "CQ-WW-SSB";

    // ADIF Contest-ID values
    static inline const QString ADIF_CONTEST_ID_CW = "CQ-WW-CW";
    static inline const QString ADIF_CONTEST_ID_SSB = "CQ-WW-SSB";

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
    bool usesSerialNumbers() const override { return false; }

    DuplicateCheckingRule getDuplicateCheckingRule() const override {
        return DuplicateCheckingRule::PerBandMode;  // Can work same call on different bands
    }

    QMap<QString, QString> getCabrilloHeaders() const override;

private:
    ModeType m_mode;  // CW or SSB
};

} // namespace TR4QT

#endif // CQWWCONTEST_H
