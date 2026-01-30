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

#include "NAQPSSBContest.h"
#include "ContestMetadata.h"

namespace TR4QT {

ContestMetadata NAQPSSBContest::getMetadata() {
    ContestMetadata meta;
    meta.id = "NAQP_SSB";
    meta.displayName = "North American QSO Party - SSB";
    meta.shortName = "NAQP-SSB";
    meta.supportedModes = {ModeType::USB, ModeType::LSB};
    meta.hasSeparateContests = false;  // This IS the specific SSB contest

    meta.wa7bnmIdCW = 0;
    meta.wa7bnmIdSSB = WA7BNM_ID_JAN;
    meta.wa7bnmIdMixed = WA7BNM_ID_AUG;

    meta.cabrilloNameCW = "";
    meta.cabrilloNameSSB = CABRILLO_NAME;
    meta.cabrilloNameMixed = CABRILLO_NAME;

    meta.adifContestIdCW = "";
    meta.adifContestIdSSB = ADIF_CONTEST_ID;
    meta.adifContestIdMixed = ADIF_CONTEST_ID;

    meta.schedule = "3rd Saturday of January and August";
    meta.floatingDates = {
        FloatingDate(1, "3rd Saturday"),  // January
        FloatingDate(8, "3rd Saturday")   // August
    };

    meta.website = "https://ncjweb.com/naqp/";
    meta.description = "12-hour low power SSB sprint. Exchange: Name + State/Province. Max 100W.";

    return meta;
}

ContestBase* NAQPSSBContest::create(ModeType mode, const StationInfo& myStation) {
    Q_UNUSED(mode);  // Mode is always SSB for this contest
    return new NAQPSSBContest(myStation);
}

} // namespace TR4QT

// Register contest
REGISTER_CONTEST(TR4QT::NAQPSSBContest, "NAQP_SSB");
