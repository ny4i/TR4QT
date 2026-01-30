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

#ifndef NAQPCONTEST_H
#define NAQPCONTEST_H

#include "ContestBase.h"

namespace TR4QT {

struct ContestMetadata;

/**
 * North American QSO Party (NAQP)
 *
 * Exchange: Name + State/Province/DXCC
 * Modes: CW and SSB (separate contests)
 * Multipliers:
 *   - States/Provinces (per band)
 * Scoring:
 *   - 1 point per QSO
 *   - Final score = Total QSOs × Sum of multipliers across all bands
 * Special Rules:
 *   - Maximum 100W power limit
 *   - 12 hour contest (operate 10 of 12 hours)
 *   - 10 minute rule: must stay on band for 10 minutes before changing
 *
 * Contest website: https://ncjweb.com/naqp/
 */
class NAQPContest : public ContestBase {
public:
    NAQPContest(ModeType mode, const StationInfo& myStation);
    ~NAQPContest() override = default;

    // ===== Contest Identifiers =====
    // WA7BNM IDs - different for each mode
    static constexpr int WA7BNM_ID_CW = 181;
    static constexpr int WA7BNM_ID_SSB = 182;
    static constexpr int WA7BNM_ID_RTTY = 183;

    static inline const QString CABRILLO_NAME_CW = "NAQP-CW";
    static inline const QString CABRILLO_NAME_SSB = "NAQP-SSB";
    static inline const QString CABRILLO_NAME_RTTY = "NAQP-RTTY";

    static inline const QString ADIF_CONTEST_ID_CW = "NAQP-CW";
    static inline const QString ADIF_CONTEST_ID_SSB = "NAQP-SSB";
    static inline const QString ADIF_CONTEST_ID_RTTY = "NAQP-RTTY";

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

    // ===== Band Restrictions =====
    QList<BandType> getAllowedBands() const override;  // RTTY excludes 160m

    QMap<QString, QString> getCabrilloHeaders() const override;

private:
    ModeType m_mode;  // CW, SSB, or RTTY
};

} // namespace TR4QT

#endif // NAQPCONTEST_H
