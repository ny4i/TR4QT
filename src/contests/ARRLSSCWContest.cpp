#include "ARRLSSCWContest.h"
#include "ContestMetadata.h"

namespace TR4QT {

ContestMetadata ARRLSSCWContest::getMetadata() {
    ContestMetadata meta;
    meta.id = "ARRL_SS_CW";
    meta.displayName = "ARRL Sweepstakes - CW";
    meta.shortName = "SS CW";
    meta.supportedModes = {ModeType::CW, ModeType::CWR};
    meta.hasSeparateContests = false;  // This IS the specific CW contest

    meta.wa7bnmIdCW = WA7BNM_ID;
    meta.wa7bnmIdSSB = 0;
    meta.wa7bnmIdMixed = 0;

    meta.cabrilloNameCW = CABRILLO_NAME;
    meta.cabrilloNameSSB = "";
    meta.cabrilloNameMixed = "";

    meta.adifContestIdCW = ADIF_CONTEST_ID;
    meta.adifContestIdSSB = "";
    meta.adifContestIdMixed = "";

    meta.schedule = "1st full weekend of November";
    meta.floatingDates = {
        FloatingDate(11, "1st full weekend")  // November
    };

    meta.website = "http://www.arrl.org/sweepstakes";
    meta.description = "Exchange: Serial# + Precedence + Check + ARRL Section. All 80 sections are multipliers.";

    return meta;
}

ContestBase* ARRLSSCWContest::create(ModeType mode, const StationInfo& myStation) {
    Q_UNUSED(mode);  // Mode is always CW for this contest
    return new ARRLSSCWContest(myStation);
}

} // namespace TR4QT

// Register contest
REGISTER_CONTEST(TR4QT::ARRLSSCWContest, "ARRL_SS_CW");
