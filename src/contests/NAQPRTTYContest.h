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

#ifndef NAQPRTTYCONTEST_H
#define NAQPRTTYCONTEST_H

#include "NAQPBase.h"
#include "ContestRegistry.h"

namespace TR4QT {

/**
 * NAQP RTTY Contest
 * Dates: 3rd Saturday of May and October
 * Mode: RTTY only
 * Bands: 80m-10m (excludes 160m - standard for RTTY contests)
 */
class NAQPRTTYContest : public NAQPBase {
public:
    // ===== Contest Identifiers =====
    static constexpr int WA7BNM_ID_MAY = 183;  // May RTTY
    static constexpr int WA7BNM_ID_OCT = 186;  // October RTTY
    static constexpr const char* CABRILLO_NAME = "NAQP-RTTY";
    static constexpr const char* ADIF_CONTEST_ID = "NAQP-RTTY";

    explicit NAQPRTTYContest(const StationInfo& myStation)
        : NAQPBase(myStation) {}

    ~NAQPRTTYContest() override = default;

    // ===== Factory Methods =====
    static ContestMetadata getMetadata();
    static ContestBase* create(ModeType mode, const StationInfo& myStation);

    // ===== Contest Identity =====
    QString getContestId() const override { return "NAQP_RTTY"; }
    QString getContestName() const override { return "North American QSO Party - RTTY"; }
    ModeType getContestMode() const override { return ModeType::RTTY; }
    QString getADIFContestId() const override { return ADIF_CONTEST_ID; }
    int getWA7BNMContestId() const override { return WA7BNM_ID_MAY; }  // Use May ID as default

    // ===== Bands (RTTY excludes 160m) =====
    QList<BandType> getAllowedBands() const override {
        return {BandType::Band80M, BandType::Band40M, BandType::Band20M,
                BandType::Band15M, BandType::Band10M};
    }

    // ===== Mode Validation =====
    bool isValidMode(ModeType mode, QString& errorMsg) const override {
        if (mode != ModeType::RTTY && mode != ModeType::RTTYR) {
            errorMsg = "NAQP-RTTY only allows RTTY modes";
            return false;
        }
        return true;
    }

    // ===== Cabrillo =====
    QMap<QString, QString> getCabrilloHeaders() const override {
        QMap<QString, QString> headers;
        headers["CONTEST"] = CABRILLO_NAME;
        return headers;
    }
};

} // namespace TR4QT

#endif // NAQPRTTYCONTEST_H
