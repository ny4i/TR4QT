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

#ifndef ARRLDXCONTEST_H
#define ARRLDXCONTEST_H

#include "ContestBase.h"

namespace TR4QT {

struct ContestMetadata;

/**
 * ARRL International DX Contest
 *
 * Exchange:
 *   - W/VE stations send: RST + State/Province
 *   - DX stations send: RST + Power
 * Modes: CW and SSB (separate contests)
 * Multipliers:
 *   - DXCC Countries (per band) for W/VE stations
 *   - US States + Canadian Provinces (per band) for DX stations
 * Scoring:
 *   - 3 points per QSO
 *   - W/VE multiply by countries worked
 *   - DX multiply by states/provinces worked
 * Special Rules:
 *   - W/VE stations may ONLY work DX stations
 *   - DX stations may ONLY work W/VE stations
 *   - Hawaii (KH6), Alaska (KL7), St. Paul Is. (CY9), Sable Is. (CY0) count as DX
 *
 * Contest website: https://contests.arrl.org/
 */
class ARRLDXContest : public ContestBase {
public:
    ARRLDXContest(ModeType mode, const StationInfo& myStation);
    ~ARRLDXContest() override = default;

    // ===== Contest Identifiers =====
    static constexpr int WA7BNM_ID_CW = 8;
    static constexpr int WA7BNM_ID_SSB = 9;

    static inline const QString CABRILLO_NAME_CW = "ARRL-DX-CW";
    static inline const QString CABRILLO_NAME_SSB = "ARRL-DX-SSB";

    static inline const QString ADIF_CONTEST_ID_CW = "ARRL-DX-CW";
    static inline const QString ADIF_CONTEST_ID_SSB = "ARRL-DX-SSB";

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
        return DuplicateCheckingRule::PerBandMode;
    }

    bool isValidQSO(
        const QSO& qso,
        const StationInfo& myStation,
        QString& errorMsg) const override;

    QMap<QString, QString> getCabrilloHeaders() const override;

private:
    ModeType m_mode;  // CW or SSB

    static bool isWVEStation(const StationInfo& station);
    static bool isValidPower(const QString& power);
};

} // namespace TR4QT

#endif // ARRLDXCONTEST_H
