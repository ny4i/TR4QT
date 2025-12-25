#ifndef WINTERFIELDDAYCONTEST_H
#define WINTERFIELDDAYCONTEST_H

#include "ContestBase.h"

namespace TR4QT {

struct ContestMetadata;

/**
 * Winter Field Day Contest
 *
 * Exchange: Class + Section
 *   - Class: Operator category (e.g., "1O", "2O", "3O", "2I", "Home")
 *   - Section: ARRL or RAC section (e.g., "WMA", "VA", "ONE", "GTA")
 * Modes: Mixed (CW, Digital, Phone)
 * Multipliers:
 *   - ARRL/RAC Sections (all-band)
 * Scoring:
 *   - CW/Digital: 2 points per QSO
 *   - Phone: 1 point per QSO
 *   - Bonus points (selected via checkboxes on Cabrillo submission):
 *     * 100% emergency power
 *     * Outdoor setup
 *     * Not-at-home location
 *     * Media publicity
 *     * Information booth
 *     * etc.
 * Total Score: QSO points + Bonus points (multipliers not multiplied)
 *
 * Contest website: https://www.winterfieldday.com/
 */
class WinterFieldDayContest : public ContestBase {
public:
    WinterFieldDayContest();
    ~WinterFieldDayContest() override = default;

    // ===== Contest Identifiers =====
    // WA7BNM Contest Calendar ID
    static constexpr int WA7BNM_ID = 421;

    // Cabrillo contest name
    static inline const QString CABRILLO_NAME = "WINTER-FIELD-DAY";

    // ADIF Contest-ID value
    static inline const QString ADIF_CONTEST_ID = "WINTER-FIELD-DAY";

    // ===== Factory Methods =====
    static ContestMetadata getMetadata();
    static ContestBase* create(ModeType mode);

    // ===== Contest Identity =====
    QString getContestId() const override;
    QString getContestName() const override;
    ModeType getContestMode() const override { return ModeType::None; }  // Mixed - all modes allowed

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
    bool usesSerialNumbers() const override { return false; }

    DuplicateCheckingRule getDuplicateCheckingRule() const override {
        return DuplicateCheckingRule::PerBandMode;  // Can work same call on different bands
    }

    QMap<QString, QString> getCabrilloHeaders() const override;

    // ===== WFD-Specific Methods =====
    /**
     * Validate class format (e.g., "1O", "2O", "3O", "2I", "Home")
     */
    static bool isValidClass(const QString& classStr);

    /**
     * Validate section (ARRL or RAC section abbreviation)
     */
    static bool isValidSection(const QString& section);

    /**
     * Get list of valid ARRL/RAC sections
     */
    static QStringList getValidSections();
};

} // namespace TR4QT

#endif // WINTERFIELDDAYCONTEST_H
