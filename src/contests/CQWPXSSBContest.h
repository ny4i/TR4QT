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

#ifndef CQWPXSSBCONTEST_H
#define CQWPXSSBCONTEST_H

#include "CQWPXBase.h"
#include "ContestRegistry.h"

namespace TR4QT {

/**
 * CQ WPX SSB Contest
 * Date: Last full weekend of March
 * Mode: SSB/Phone only (USB/LSB)
 * Bands: 160m-10m (all HF bands)
 *
 * SSB gets lower QSO points than CW (approximately 2/3)
 */
class CQWPXSSBContest : public CQWPXBase {
public:
    // ===== Contest Identifiers =====
    static constexpr int WA7BNM_ID = 8;
    static constexpr const char* CABRILLO_NAME = "CQ-WPX-SSB";
    static constexpr const char* ADIF_CONTEST_ID = "CQ-WPX-SSB";

    explicit CQWPXSSBContest(const StationInfo& myStation)
        : CQWPXBase(myStation) {}

    ~CQWPXSSBContest() override = default;

    // ===== Factory Methods =====
    static ContestMetadata getMetadata();
    static ContestBase* create(ModeType mode, const StationInfo& myStation);

    // ===== Contest Identity =====
    QString getContestId() const override { return "CQWPX_SSB"; }
    QString getContestName() const override { return "CQ WPX Contest - SSB"; }
    ModeType getContestMode() const override { return ModeType::USB; }
    QString getADIFContestId() const override { return ADIF_CONTEST_ID; }
    int getWA7BNMContestId() const override { return WA7BNM_ID; }

    // ===== Bands (all HF bands including 160m) =====
    QList<BandType> getAllowedBands() const override {
        return {BandType::Band160M, BandType::Band80M, BandType::Band40M,
                BandType::Band20M, BandType::Band15M, BandType::Band10M};
    }

    // ===== Mode Validation =====
    bool isValidMode(ModeType mode, QString& errorMsg) const override {
        if (mode != ModeType::USB && mode != ModeType::LSB) {
            errorMsg = "CQ WPX SSB only allows Phone modes (USB/LSB)";
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
        return 0.67;  // SSB gets 2/3 of CW points (rounded down)
    }
};

} // namespace TR4QT

#endif // CQWPXSSBCONTEST_H
