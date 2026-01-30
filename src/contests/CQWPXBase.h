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

#ifndef CQWPXBASE_H
#define CQWPXBASE_H

#include "ContestBase.h"

namespace TR4QT {

/**
 * CQ WPX Contest Base Class (Worked All Prefixes)
 *
 * Shared logic for all CQ WPX contests (CW, SSB/Phone)
 *
 * Exchange: RST + Serial Number (auto-increment)
 * Multipliers:
 *   - Callsign prefixes (all-band, counted once per prefix)
 *   - Prefix is callsign up to and including first digit
 *     Examples: W1AW → W1, DL1ABC → DL1, JA1234XYZ → JA1
 * Scoring:
 *   - QSO Points vary by band, continent, and mode
 *   - Different continent: higher points
 *   - Same continent, different country: medium points
 *   - Same country: 1 point
 *   - 160m, 80m, 40m (low bands): higher points than high bands
 *   - CW gets more points than SSB
 * Total Score: QSO points × Total prefixes
 *
 * Contest website: https://www.cqwpx.com/
 */
class CQWPXBase : public ContestBase {
protected:
    explicit CQWPXBase(const StationInfo& myStation)
        : ContestBase(myStation) {}

public:
    ~CQWPXBase() override = default;

    // ===== Exchange Configuration (shared across all modes) =====
    QList<ExchangeField> getReceivedExchangeFields() const override;
    QList<ExchangeField> getSentExchangeFields() const override;
    QList<TableColumn> getTableColumns() const override;
    QString formatSentExchange(int serialNumber, const QString& rst = "599") const override;
    bool validateReceivedExchange(const QString& exchange, QString& errorMsg) const override;
    void parseReceivedExchange(const QString& exchange, QSO& qso) const override;

    // ===== Multipliers (shared - Prefixes all-band) =====
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
    bool usesSerialNumbers() const override { return true; }

    DuplicateCheckingRule getDuplicateCheckingRule() const override {
        return DuplicateCheckingRule::PerBandMode;  // Can work same call on different bands
    }

    // ===== Prefix Extraction (shared utility) =====
    /**
     * Extract WPX prefix from callsign
     * Prefix is everything up to and including the first digit
     * Examples:
     *   W1AW → W1
     *   DL1ABC → DL1
     *   JA1234XYZ → JA1
     *   VP9/G3XYZ → VP9 (use prefix before /)
     */
    static QString extractPrefix(const QString& callsign);

protected:
    /**
     * Get mode-specific point multiplier
     * CW gets higher points than SSB
     * Subclasses override to provide their multiplier
     */
    virtual double getModePointMultiplier() const = 0;
};

} // namespace TR4QT

#endif // CQWPXBASE_H
