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

#ifndef CQWWBASE_H
#define CQWWBASE_H

#include "ContestBase.h"

namespace TR4QT {

/**
 * CQ World Wide DX Contest Base Class
 *
 * Shared logic for all CQ WW contests (CW, SSB/Phone)
 *
 * Exchange: RST + CQ Zone (1-40)
 * Multipliers:
 *   - DXCC Countries (per band)
 *   - CQ Zones (per band)
 * Scoring:
 *   - Same continent, different country: 1 point (both modes)
 *   - Different continent: 3 points (CW), 2 points (SSB)
 *   - Special rule for W/VE stations working each other: 2 points
 * Total Score: QSO points × (Countries + Zones)
 *
 * Contest website: https://www.cqww.com/
 */
class CQWWBase : public ContestBase {
protected:
    explicit CQWWBase(const StationInfo& myStation)
        : ContestBase(myStation) {}

public:
    ~CQWWBase() override = default;

    // ===== Exchange Configuration (shared across all modes) =====
    QList<ExchangeField> getReceivedExchangeFields() const override;
    QList<ExchangeField> getSentExchangeFields() const override;
    QList<TableColumn> getTableColumns() const override;
    QString formatSentExchange(int serialNumber, const QString& rst = "599") const override;
    bool validateReceivedExchange(const QString& exchange, QString& errorMsg) const override;
    void parseReceivedExchange(const QString& exchange, QSO& qso) const override;

    // ===== Multipliers (shared - Countries and Zones per band) =====
    QList<MultiplierDefinition> getMultiplierTypes() const override;
    QString getMultiplierValue(
        const QSO& qso,
        MultiplierType multType,
        const QStringList& alreadyWorkedValues) const override;

    // ===== Scoring (mode-specific points, but shared formula) =====
    int calculateQSOPoints(
        const QSO& qso,
        const StationInfo& myStation) const override;

    int calculateTotalScore(
        int totalQSOPoints,
        const QMap<MultiplierType, int>& multiplierCounts) const override;

    // ===== Special Rules (shared) =====
    bool usesSerialNumbers() const override { return false; }

    DuplicateCheckingRule getDuplicateCheckingRule() const override {
        return DuplicateCheckingRule::PerBandMode;  // Can work same call on different bands
    }

    // ===== Bands (all HF bands including 160m) =====
    QList<BandType> getAllowedBands() const override {
        return {BandType::Band160M, BandType::Band80M, BandType::Band40M,
                BandType::Band20M, BandType::Band15M, BandType::Band10M};
    }

protected:
    /**
     * Get mode-specific point multiplier
     * CW gets higher points than SSB
     * Subclasses override to provide their multiplier
     */
    virtual double getModePointMultiplier() const = 0;
};

} // namespace TR4QT

#endif // CQWWBASE_H
