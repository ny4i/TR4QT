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

#ifndef CQWWSSBCONTEST_H
#define CQWWSSBCONTEST_H

#include "CQWWBase.h"
#include "ContestRegistry.h"

namespace TR4QT {

/**
 * CQ World Wide DX Contest - SSB
 * Date: Last full weekend of October
 * Mode: SSB/Phone only (USB/LSB)
 * Bands: 160m-10m (all HF bands)
 *
 * SSB gets lower QSO points than CW (2/3)
 */
class CQWWSSBContest : public CQWWBase {
public:
    // ===== Contest Identifiers =====
    static constexpr int WA7BNM_ID = 4;
    static constexpr const char* CABRILLO_NAME = "CQ-WW-SSB";
    static constexpr const char* ADIF_CONTEST_ID = "CQ-WW-SSB";

    explicit CQWWSSBContest(const StationInfo& myStation)
        : CQWWBase(myStation) {}

    ~CQWWSSBContest() override = default;

    // ===== Factory Methods =====
    static ContestMetadata getMetadata();
    static ContestBase* create(ModeType mode, const StationInfo& myStation);

    // ===== Contest Identity =====
    QString getContestId() const override { return "CQWW_SSB"; }
    QString getContestName() const override { return "CQ World Wide DX Contest - SSB"; }
    ModeType getContestMode() const override { return ModeType::USB; }
    QString getADIFContestId() const override { return ADIF_CONTEST_ID; }
    int getWA7BNMContestId() const override { return WA7BNM_ID; }

    // ===== Mode Validation =====
    bool isValidMode(ModeType mode, QString& errorMsg) const override {
        if (mode != ModeType::USB && mode != ModeType::LSB) {
            errorMsg = "CQ WW SSB only allows Phone modes (USB/LSB)";
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
        return 0.67;  // SSB gets 2/3 of CW points (2 for different continent vs 3 for CW)
    }
};

} // namespace TR4QT

#endif // CQWWSSBCONTEST_H
