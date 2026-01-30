/*
    TR4QT - An Amateur Radio Contesting Logger inspired by TR4W and TRLog.
    Copyright (C) 2026 Thomas M. Schaefer NY4I ny4i@ny4i.com

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#ifndef ARRLFIELDDAYCONTEST_H
#define ARRLFIELDDAYCONTEST_H

#include "ContestBase.h"

namespace TR4QT {

struct ContestMetadata;

/**
 * ARRL Field Day Contest
 *
 * Exchange: Class + Section
 *   - Class: Number of transmitters + Category (e.g., "1A", "3B", "5F")
 *     * Number: 1-32 (transmitters operating simultaneously)
 *     * Category: A (portable, 3+ people), B (portable, 1-2 people),
 *                 C (mobile), D (home, battery), E (home, commercial power),
 *                 F (EOC)
 *   - Section: ARRL or RAC section (e.g., "EMA", "CT", "GTA")
 *   - DX sends: Class + "DX" (e.g., "2A DX")
 *
 * Modes: Mixed (CW, Digital, Phone)
 *
 * Multipliers: None (uses power multiplier and bonus points instead)
 *
 * Scoring:
 *   - CW/Digital: 2 points per QSO
 *   - Phone: 1 point per QSO
 *   - Final Score: (QSO points × Power multiplier) + Bonus points
 *
 * Power Multipliers:
 *   - 5: QRP (≤5W) on alternative power
 *   - 2: Low power (≤100W) on battery OR QRP on generator
 *   - 1: Standard power (≤500W for A/B/C, ≤100W for D/E/F)
 *
 * Bonus Points (examples):
 *   - 100% Emergency Power: 100 pts per transmitter (max 2000)
 *   - Media publicity: 100 pts
 *   - Public location: 100 pts
 *   - Information booth: 100 pts
 *   - Many others (see official rules)
 *
 * Note: Field Day doesn't use traditional multipliers. The power multiplier
 * applies to ALL QSO points, and bonus points are added afterward.
 *
 * Contest website: https://www.arrl.org/field-day
 */
class ARRLFieldDayContest : public ContestBase {
public:
    ARRLFieldDayContest(const StationInfo& myStation);
    ~ARRLFieldDayContest() override = default;

    // ===== Contest Identifiers =====
    static constexpr int WA7BNM_ID = 60;  // ARRL Field Day
    static inline const QString CABRILLO_NAME = "ARRL-FIELD-DAY";
    static inline const QString ADIF_CONTEST_ID = "ARRL-FIELD-DAY";

    // ===== Factory Methods =====
    static ContestMetadata getMetadata();
    static ContestBase* create(ModeType mode, const StationInfo& myStation);

    // ===== Contest Identity =====
    QString getContestId() const override;
    QString getContestName() const override;
    ModeType getContestMode() const override { return ModeType::None; }  // Mixed
    QString getADIFContestId() const override;
    int getWA7BNMContestId() const override { return WA7BNM_ID; }

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

    // Field Day doesn't use multipliers in traditional sense
    bool usesMultipliers() const override { return false; }

    // Field Day tracks QSOs separately by mode group (Phone/CW/Digital)
    bool usesModeGroupBreakdown() const override { return true; }

    // ===== Special Rules =====
    bool usesSerialNumbers() const override { return false; }

    DuplicateCheckingRule getDuplicateCheckingRule() const override {
        return DuplicateCheckingRule::PerBandMode;
    }

    // ===== Band Restrictions =====
    QList<BandType> getAllowedBands() const override;  // HF + VHF/UHF (if enabled)

    QMap<QString, QString> getCabrilloHeaders() const override;

    // ===== Field Day-Specific Methods =====
    /**
     * Validate class format (e.g., "1A", "3B", "5F")
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

#endif // ARRLFIELDDAYCONTEST_H
