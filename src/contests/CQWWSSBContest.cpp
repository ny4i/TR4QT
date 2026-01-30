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

#include "CQWWSSBContest.h"
#include "ContestMetadata.h"

namespace TR4QT {

ContestMetadata CQWWSSBContest::getMetadata() {
    ContestMetadata meta;
    meta.id = "CQWW_SSB";
    meta.displayName = "CQ World Wide DX Contest - SSB";
    meta.shortName = "CQ WW SSB";
    meta.supportedModes = {ModeType::USB, ModeType::LSB};
    meta.hasSeparateContests = false;  // This IS the specific SSB contest

    meta.wa7bnmIdCW = 0;
    meta.wa7bnmIdSSB = WA7BNM_ID;
    meta.wa7bnmIdMixed = 0;

    meta.cabrilloNameCW = "";
    meta.cabrilloNameSSB = CABRILLO_NAME;
    meta.cabrilloNameMixed = "";

    meta.adifContestIdCW = "";
    meta.adifContestIdSSB = ADIF_CONTEST_ID;
    meta.adifContestIdMixed = "";

    meta.schedule = "4th full weekend of October";
    meta.floatingDates = {
        FloatingDate(10, "4th full weekend")  // October
    };

    meta.website = "https://www.cqww.com/";
    meta.description = "Work as many countries and CQ zones as possible. Exchange: RST + CQ Zone.";

    return meta;
}

ContestBase* CQWWSSBContest::create(ModeType mode, const StationInfo& myStation) {
    Q_UNUSED(mode);  // Mode is always SSB for this contest
    return new CQWWSSBContest(myStation);
}

} // namespace TR4QT

// Register contest
REGISTER_CONTEST(TR4QT::CQWWSSBContest, "CQWW_SSB");
