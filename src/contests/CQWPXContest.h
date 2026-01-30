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

#ifndef CQWPXCONTEST_H
#define CQWPXCONTEST_H

#include "ContestBase.h"

namespace TR4QT {

struct ContestMetadata;

/**
 * CQ WPX Contest (Worked All Prefixes)
 *
 * Exchange: RST + Serial Number (auto-increment)
 * Modes: CW and SSB (separate contests)
 * Multipliers:
 *   - Callsign prefixes (all-band, counted once per prefix)
 *   - Prefix is callsign up to and including first digit
 *     Examples: W1AW → W1, DL1ABC → DL1, JA1234XYZ → JA1
 * Scoring:
 *   - QSO Points vary by band and continent:
 *     - Same continent: 1 point
 *     - Different continent: 3 points (CW), 2 points (SSB)
 *     - 160m and 10m: Double points
 * Total Score: QSO points × Total prefixes
 *
 * Contest website: https://www.cqwpx.com/
 */
class CQWPXContest : public ContestBase {
public:
    CQWPXContest(ModeType mode, const StationInfo& myStation);
    ~CQWPXContest() override = default;

    // ===== Contest Identifiers =====
    // WA7BNM Contest Calendar IDs
    static constexpr int WA7BNM_ID_CW = 7;
    static constexpr int WA7BNM_ID_SSB = 8;

    // Cabrillo contest names
    static inline const QString CABRILLO_NAME_CW = "CQ-WPX-CW";
    static inline const QString CABRILLO_NAME_SSB = "CQ-WPX-SSB";

    // ADIF Contest-ID values
    static inline const QString ADIF_CONTEST_ID_CW = "CQ-WPX-CW";
    static inline const QString ADIF_CONTEST_ID_SSB = "CQ-WPX-SSB";

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
    bool usesSerialNumbers() const override { return true; }

    DuplicateCheckingRule getDuplicateCheckingRule() const override {
        return DuplicateCheckingRule::PerBandMode;  // Can work same call on different bands
    }

    // ===== Band Restrictions =====
    QList<BandType> getAllowedBands() const override;  // RTTY excludes 160m

    QMap<QString, QString> getCabrilloHeaders() const override;

    // ===== Prefix Extraction =====
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

private:
    ModeType m_mode;  // CW or SSB
};

} // namespace TR4QT

#endif // CQWPXCONTEST_H
