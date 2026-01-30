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

#ifndef CQWPXCWCONTEST_H
#define CQWPXCWCONTEST_H

#include "CQWPXBase.h"
#include "ContestRegistry.h"

namespace TR4QT {

/**
 * CQ WPX CW Contest
 * Date: Last full weekend of May
 * Mode: CW only
 * Bands: 160m-10m (all HF bands)
 *
 * CW gets higher QSO points than SSB
 */
class CQWPXCWContest : public CQWPXBase {
public:
    // ===== Contest Identifiers =====
    static constexpr int WA7BNM_ID = 7;
    static constexpr const char* CABRILLO_NAME = "CQ-WPX-CW";
    static constexpr const char* ADIF_CONTEST_ID = "CQ-WPX-CW";

    explicit CQWPXCWContest(const StationInfo& myStation)
        : CQWPXBase(myStation) {}

    ~CQWPXCWContest() override = default;

    // ===== Factory Methods =====
    static ContestMetadata getMetadata();
    static ContestBase* create(ModeType mode, const StationInfo& myStation);

    // ===== Contest Identity =====
    QString getContestId() const override { return "CQWPX_CW"; }
    QString getContestName() const override { return "CQ WPX Contest - CW"; }
    ModeType getContestMode() const override { return ModeType::CW; }
    QString getADIFContestId() const override { return ADIF_CONTEST_ID; }
    int getWA7BNMContestId() const override { return WA7BNM_ID; }

    // ===== Bands (all HF bands including 160m) =====
    QList<BandType> getAllowedBands() const override {
        return {BandType::Band160M, BandType::Band80M, BandType::Band40M,
                BandType::Band20M, BandType::Band15M, BandType::Band10M};
    }

    // ===== Mode Validation =====
    bool isValidMode(ModeType mode, QString& errorMsg) const override {
        if (mode != ModeType::CW && mode != ModeType::CWR) {
            errorMsg = "CQ WPX CW only allows CW mode";
            return false;
        }
        return true;
    }

    // ===== Cabrillo =====
    QMap<QString, QString> getCabrilloHeaders() const override {
        QMap<QString, QString> headers = ContestBase::getCabrilloHeaders();
        headers["CONTEST"] = CABRILLO_NAME;
        return headers;
    }

protected:
    // ===== Mode Point Multiplier =====
    double getModePointMultiplier() const override {
        return 1.0;  // CW gets full points
    }
};

} // namespace TR4QT

#endif // CQWPXCWCONTEST_H
