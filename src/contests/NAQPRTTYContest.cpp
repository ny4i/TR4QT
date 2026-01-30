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

#include "NAQPRTTYContest.h"
#include "ContestMetadata.h"

namespace TR4QT {

ContestMetadata NAQPRTTYContest::getMetadata() {
    ContestMetadata meta;
    meta.id = "NAQP_RTTY";
    meta.displayName = "North American QSO Party - RTTY";
    meta.shortName = "NAQP-RTTY";
    meta.supportedModes = {ModeType::RTTY, ModeType::RTTYR};
    meta.hasSeparateContests = false;  // This IS the specific RTTY contest

    meta.wa7bnmIdCW = 0;
    meta.wa7bnmIdSSB = 0;
    meta.wa7bnmIdMixed = WA7BNM_ID_MAY;

    meta.cabrilloNameCW = "";
    meta.cabrilloNameSSB = "";
    meta.cabrilloNameMixed = CABRILLO_NAME;

    meta.adifContestIdCW = "";
    meta.adifContestIdSSB = "";
    meta.adifContestIdMixed = ADIF_CONTEST_ID;

    meta.schedule = "4th Saturday of February, 3rd Saturday of May, 3rd Saturday of October";
    meta.floatingDates = {
        FloatingDate(2, "4th Saturday"),   // February
        FloatingDate(5, "3rd Saturday"),   // May
        FloatingDate(10, "3rd Saturday")   // October
    };

    meta.website = "https://ncjweb.com/naqp/";
    meta.description = "12-hour low power RTTY sprint. Exchange: Name + State/Province. Max 100W. No 160m.";

    return meta;
}

ContestBase* NAQPRTTYContest::create(ModeType mode, const StationInfo& myStation) {
    Q_UNUSED(mode);  // Mode is always RTTY for this contest
    return new NAQPRTTYContest(myStation);
}

} // namespace TR4QT

// Register contest
REGISTER_CONTEST(TR4QT::NAQPRTTYContest, "NAQP_RTTY");
