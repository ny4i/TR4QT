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

#ifndef ARRLDXBASE_H
#define ARRLDXBASE_H

#include "ContestBase.h"

namespace TR4QT {

/**
 * ARRL International DX Contest Base Class
 *
 * Shared logic for all ARRL DX contests (CW, Phone/SSB)
 *
 * Exchange:
 *   - W/VE stations send: RST + State/Province
 *   - DX stations send: RST + Power
 * Multipliers:
 *   - DXCC Countries (per band) for W/VE stations
 *   - US States + Canadian Provinces (per band) for DX stations
 * Scoring:
 *   - 3 points per QSO (both modes)
 *   - W/VE multiply by countries worked
 *   - DX multiply by states/provinces worked
 * Special Rules:
 *   - W/VE stations may ONLY work DX stations
 *   - DX stations may ONLY work W/VE stations
 *   - Hawaii (KH6), Alaska (KL7), St. Paul Is. (CY9), Sable Is. (CY0) count as DX
 *
 * Contest website: https://contests.arrl.org/
 */
class ARRLDXBase : public ContestBase {
protected:
    explicit ARRLDXBase(const StationInfo& myStation)
        : ContestBase(myStation) {}

public:
    ~ARRLDXBase() override = default;

    // ===== Exchange Configuration (shared, varies by station) =====
    QList<ExchangeField> getReceivedExchangeFields() const override;
    QList<ExchangeField> getSentExchangeFields() const override;
    QList<TableColumn> getTableColumns() const override;
    QString formatSentExchange(int serialNumber, const QString& rst = "599") const override;
    bool validateReceivedExchange(const QString& exchange, QString& errorMsg) const override;
    void parseReceivedExchange(const QString& exchange, QSO& qso) const override;

    // ===== Multipliers (different for W/VE vs DX) =====
    QList<MultiplierDefinition> getMultiplierTypes() const override;
    QString getMultiplierValue(
        const QSO& qso,
        MultiplierType multType,
        const QStringList& alreadyWorkedValues) const override;

    // ===== Scoring (same for both modes) =====
    int calculateQSOPoints(
        const QSO& qso,
        const StationInfo& myStation) const override;

    int calculateTotalScore(
        int totalQSOPoints,
        const QMap<MultiplierType, int>& multiplierCounts) const override;

    // ===== Special Rules (shared) =====
    bool usesSerialNumbers() const override { return false; }

    DuplicateCheckingRule getDuplicateCheckingRule() const override {
        return DuplicateCheckingRule::PerBandMode;
    }

    bool isValidQSO(
        const QSO& qso,
        const StationInfo& myStation,
        QString& errorMsg) const override;

    // ===== Bands (all HF bands including 160m) =====
    QList<BandType> getAllowedBands() const override {
        return {BandType::Band160M, BandType::Band80M, BandType::Band40M,
                BandType::Band20M, BandType::Band15M, BandType::Band10M};
    }

protected:
    /**
     * Check if station is W/VE
     * W/VE stations have different exchange and scoring than DX
     */
    static bool isWVEStation(const StationInfo& station);

    /**
     * Validate power format (e.g., "100", "1K", "5K")
     */
    static bool isValidPower(const QString& power);
};

} // namespace TR4QT

#endif // ARRLDXBASE_H
