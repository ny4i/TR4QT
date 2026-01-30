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

#ifndef ARRLDXCWCONTEST_H
#define ARRLDXCWCONTEST_H

#include "ARRLDXBase.h"
#include "ContestRegistry.h"

namespace TR4QT {

/**
 * ARRL International DX Contest - CW
 * Date: 3rd weekend of February (Saturday-Sunday, 48 hours)
 * Mode: CW only
 * Bands: 160m-10m (all HF bands)
 */
class ARRLDXCWContest : public ARRLDXBase {
public:
    // ===== Contest Identifiers =====
    static constexpr int WA7BNM_ID = 8;
    static constexpr const char* CABRILLO_NAME = "ARRL-DX-CW";
    static constexpr const char* ADIF_CONTEST_ID = "ARRL-DX-CW";

    explicit ARRLDXCWContest(const StationInfo& myStation)
        : ARRLDXBase(myStation) {}

    ~ARRLDXCWContest() override = default;

    // ===== Factory Methods =====
    static ContestMetadata getMetadata();
    static ContestBase* create(ModeType mode, const StationInfo& myStation);

    // ===== Contest Identity =====
    QString getContestId() const override { return "ARRL_DX_CW"; }
    QString getContestName() const override { return "ARRL International DX Contest - CW"; }
    ModeType getContestMode() const override { return ModeType::CW; }
    QString getADIFContestId() const override { return ADIF_CONTEST_ID; }
    int getWA7BNMContestId() const override { return WA7BNM_ID; }

    // ===== Mode Validation =====
    bool isValidMode(ModeType mode, QString& errorMsg) const override {
        if (mode != ModeType::CW && mode != ModeType::CWR) {
            errorMsg = "ARRL DX CW only allows CW mode";
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

#endif // ARRLDXCWCONTEST_H
