#ifndef IARUHFCONTEST_H
#define IARUHFCONTEST_H

#include "ContestBase.h"

namespace TR4QT {

struct ContestMetadata;

/**
 * IARU HF World Championship Contest
 *
 * Exchange: RST + ITU Zone (or HQ/AC/R1/R2/R3 for special stations)
 *   - Regular stations: RST + ITU Zone (1-90)
 *   - IARU HQ stations: RST + "HQ"
 *   - IARU officials: RST + "AC", "R1", "R2", or "R3"
 *
 * Modes: Mixed (CW/SSB)
 *
 * QSO Points (distance-based):
 *   - 1 point: Same ITU zone
 *   - 3 points: Same continent, different ITU zone
 *   - 5 points: Different continent, different ITU zone
 *
 * Multipliers (per band):
 *   - ITU zones worked (1-90)
 *   - IARU HQ stations worked
 *   - IARU officials (AC, R1, R2, R3) - max 4 per band
 *
 * Scoring: Total QSO points × (Total multipliers across all bands)
 *
 * Note: HQ stations and officials count as multipliers but NOT as zones
 *
 * Contest website: https://www.arrl.org/iaru-hf-world-championship
 */
class IARUHFContest : public ContestBase {
public:
    IARUHFContest(ModeType mode, const StationInfo& myStation);
    ~IARUHFContest() override = default;

    // ===== Contest Identifiers =====
    static constexpr int WA7BNM_ID_CW = 131;    // IARU HF Championship (CW)
    static constexpr int WA7BNM_ID_SSB = 130;   // IARU HF Championship (SSB)
    static constexpr int WA7BNM_ID_MIXED = 0;   // No mixed mode

    static inline const QString CABRILLO_NAME_CW = "IARU-HF";
    static inline const QString CABRILLO_NAME_SSB = "IARU-HF";
    static inline const QString ADIF_CONTEST_ID_CW = "IARU-HF";
    static inline const QString ADIF_CONTEST_ID_SSB = "IARU-HF";

    // ===== Factory Methods =====
    static ContestMetadata getMetadata();
    static ContestBase* create(ModeType mode, const StationInfo& myStation);

    // ===== Contest Identity =====
    QString getContestId() const override;
    QString getContestName() const override;
    ModeType getContestMode() const override { return m_mode; }
    QString getADIFContestId() const override;
    int getWA7BNMContestId() const override {
        // Return mode-specific ID (CW default if no mode or mixed)
        if (m_mode == ModeType::USB || m_mode == ModeType::LSB) return WA7BNM_ID_SSB;
        return WA7BNM_ID_CW;  // Default for CW and other modes
    }

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

    // IARU HF is mixed mode (Phone/CW/Digital)
    bool usesModeGroupBreakdown() const override { return true; }

    DuplicateCheckingRule getDuplicateCheckingRule() const override {
        // Per rules: "A station may be worked once per band per mode"
        return DuplicateCheckingRule::PerBandMode;
    }

    QMap<QString, QString> getCabrilloHeaders() const override;

    // ===== IARU-Specific Methods =====
    /**
     * Check if exchange is a special station (HQ, AC, R1, R2, R3)
     */
    static bool isSpecialStation(const QString& exchange);

    /**
     * Check if exchange is a valid ITU zone (1-90)
     */
    static bool isValidITUZone(const QString& zone);

private:
    ModeType m_mode;
};

} // namespace TR4QT

#endif // IARUHFCONTEST_H
