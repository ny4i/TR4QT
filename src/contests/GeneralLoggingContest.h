#ifndef GENERALLOGGINGCONTEST_H
#define GENERALLOGGINGCONTEST_H

#include "ContestBase.h"

namespace TR4QT {

struct ContestMetadata;

/**
 * General Logging "Contest"
 *
 * A simple mode for logging QSOs without contest scoring.
 * - No point calculation (all QSOs = 0 points)
 * - No multipliers
 * - Optional RS/RST exchange field
 * - Free-form comments field for any additional exchange information
 * - Supports all modes (CW, Phone, Digital)
 * - Duplicate checking per band/mode (same as most contests)
 *
 * This is useful for casual operating, nets, ragchewing, or events
 * that don't have formal scoring rules.
 */
class GeneralLoggingContest : public ContestBase {
public:
    explicit GeneralLoggingContest(ModeType mode, const StationInfo& myStation);
    ~GeneralLoggingContest() override = default;

    // ===== Factory Methods =====
    static ContestMetadata getMetadata();
    static ContestBase* create(ModeType mode, const StationInfo& myStation);

    // ===== Contest Identity =====
    QString getContestId() const override { return "GENERAL"; }
    QString getContestName() const override { return "General Logging"; }
    ModeType getContestMode() const override { return ModeType::None; }  // Supports all modes
    QString getADIFContestId() const override { return ""; }  // No official ADIF Contest-ID

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
        const StationInfo& myStation) const override {
        Q_UNUSED(qso);
        Q_UNUSED(myStation);
        return 0;  // No points for general logging
    }

    int calculateTotalScore(
        int totalQSOPoints,
        const QMap<MultiplierType, int>& multiplierCounts) const override {
        Q_UNUSED(totalQSOPoints);
        Q_UNUSED(multiplierCounts);
        return 0;  // No scoring
    }

    // ===== Multipliers =====
    QList<MultiplierDefinition> getMultiplierTypes() const override {
        return {};  // No multipliers
    }

    QString getMultiplierValue(
        const QSO& qso,
        MultiplierType multType,
        const QStringList& alreadyWorkedValues) const override {
        Q_UNUSED(qso);
        Q_UNUSED(multType);
        Q_UNUSED(alreadyWorkedValues);
        return "";  // No multipliers
    }

    // ===== Special Rules =====
    bool usesSerialNumbers() const override { return false; }

    bool requiresExchange() const override { return false; }  // Exchange is optional in General Logging

    DuplicateCheckingRule getDuplicateCheckingRule() const override {
        return DuplicateCheckingRule::PerBandMode;
    }

    QMap<QString, QString> getCabrilloHeaders() const override {
        QMap<QString, QString> headers;
        headers["CONTEST"] = "GENERAL";
        return headers;
    }

private:
    ModeType m_mode;
};

} // namespace TR4QT

#endif // GENERALLOGGINGCONTEST_H
