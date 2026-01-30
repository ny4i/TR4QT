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

#ifndef ARRLSSCWCONTEST_H
#define ARRLSSCWCONTEST_H

#include "ARRLSSBase.h"
#include "ContestRegistry.h"

namespace TR4QT {

/**
 * ARRL Sweepstakes Contest - CW
 * Date: 1st full weekend of November
 * Mode: CW only
 * Bands: 160m-10m (all HF bands)
 */
class ARRLSSCWContest : public ARRLSSBase {
public:
    // ===== Contest Identifiers =====
    static constexpr int WA7BNM_ID = 1;
    static constexpr const char* CABRILLO_NAME = "ARRL-SS-CW";
    static constexpr const char* ADIF_CONTEST_ID = "ARRL-SS-CW";

    explicit ARRLSSCWContest(const StationInfo& myStation)
        : ARRLSSBase(myStation) {}

    ~ARRLSSCWContest() override = default;

    // ===== Factory Methods =====
    static ContestMetadata getMetadata();
    static ContestBase* create(ModeType mode, const StationInfo& myStation);

    // ===== Contest Identity =====
    QString getContestId() const override { return "ARRL_SS_CW"; }
    QString getContestName() const override { return "ARRL Sweepstakes - CW"; }
    ModeType getContestMode() const override { return ModeType::CW; }
    QString getADIFContestId() const override { return ADIF_CONTEST_ID; }
    int getWA7BNMContestId() const override { return WA7BNM_ID; }

    // ===== Mode Validation =====
    bool isValidMode(ModeType mode, QString& errorMsg) const override {
        if (mode != ModeType::CW && mode != ModeType::CWR) {
            errorMsg = "ARRL Sweepstakes CW only allows CW mode";
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

#endif // ARRLSSCWCONTEST_H
