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

#ifndef FLORIDAQSOPARTYCONTEST_H
#define FLORIDAQSOPARTYCONTEST_H

#include "QSOPartyContestBase.h"

namespace TR4QT {

struct ContestMetadata;

/**
 * Florida QSO Party
 *
 * Exchange (State-Dependent):
 *   - Florida stations send: RST + County abbreviation (67 counties)
 *   - Non-Florida stations send: RST + State/Province/DX
 *
 * Modes: Phone and CW only (NO digital modes)
 * Bands: 40M, 20M, 15M, 10M only (NO 160M, 80M, WARC, or VHF)
 *
 * Multipliers (State-Dependent):
 *   - Florida stations: US States + Canadian Provinces + DXCC Countries
 *   - Non-Florida stations: Florida counties (67 total)
 *
 * Scoring:
 *   - Phone: 1 point per QSO
 *   - CW: 2 points per QSO
 *   - Formula: QSO Points × Multipliers × Power Multiplier
 *   - Power Multipliers: QRP(≤5W)=×3, Low(>5-<100W)=×2, High(≥100W)=×1
 *
 * Contest Period: Last full weekend of April
 *   - Saturday 16:00Z to Sunday 02:00Z (10 hours)
 *   - Sunday 12:00Z to 22:00Z (10 hours)
 *
 * Contest website: https://floridaqsoparty.org/
 */
class FloridaQSOPartyContest : public QSOPartyContestBase {
public:
    FloridaQSOPartyContest(ModeType mode, const StationInfo& myStation);
    ~FloridaQSOPartyContest() override = default;

    // ===== Contest Identifiers =====
    // WA7BNM Contest Calendar ID
    static constexpr int WA7BNM_ID_MIXED = 282;

    // Cabrillo contest name
    static inline const QString CABRILLO_NAME_MIXED = "FL-QSO-PARTY";

    // ADIF Contest-ID value
    static inline const QString ADIF_CONTEST_ID_MIXED = "FL-QSO-PARTY";

    // ===== Factory Methods =====
    static ContestMetadata getMetadata();
    static ContestBase* create(ModeType mode, const StationInfo& myStation);

    // ===== Contest Identity =====
    QString getContestId() const override;
    QString getContestName() const override;
    ModeType getContestMode() const override { return m_mode; }
    QString getADIFContestId() const override;
    int getWA7BNMContestId() const override { return WA7BNM_ID_MIXED; }

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
        return DuplicateCheckingRule::PerBandMode;
    }

    // ===== Band and Mode Restrictions =====
    QList<BandType> getAllowedBands() const override;
    bool isValidMode(ModeType mode, QString& errorMsg) const override;

    QMap<QString, QString> getCabrilloHeaders() const override;

protected:
    // ===== QSO Party Base Class Overrides =====

    /**
     * Determine if the operator is in Florida (in-state station)
     * Used for state-dependent exchange and multiplier rules
     */
    bool isInState() const override;

    /**
     * Get the QSO Party state abbreviation
     */
    QString getQSOPartyState() const override { return "FL"; }

private:
    ModeType m_mode;

    /**
     * Validate Florida county abbreviation (67 counties)
     */
    bool isValidFloridaCounty(const QString& county) const;

    /**
     * Validate US state or Canadian province abbreviation
     * 50 US states + DC + 13 Canadian provinces/territories
     */
    bool isValidStateOrProvince(const QString& state) const;

    /**
     * Get county full name from abbreviation (for display)
     */
    QString getCountyName(const QString& abbrev) const;

    /**
     * Calculate power multiplier based on station power
     * QRP (≤5W) = ×3
     * Low (>5W to <100W) = ×2
     * High (≥100W) = ×1
     */
    int getPowerMultiplier() const;
};

} // namespace TR4QT

#endif // FLORIDAQSOPARTYCONTEST_H
