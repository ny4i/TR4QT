#include "ARRLSSSSBContest.h"
#include "ContestMetadata.h"

namespace TR4QT {

ContestMetadata ARRLSSSSBContest::getMetadata() {
    ContestMetadata meta;
    meta.id = "ARRL_SS_SSB";
    meta.displayName = "ARRL Sweepstakes - SSB";
    meta.shortName = "SS SSB";
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

    meta.schedule = "3rd full weekend of November";
    meta.floatingDates = {
        FloatingDate(11, "3rd full weekend")  // November
    };

    meta.website = "http://www.arrl.org/sweepstakes";
    meta.description = "Exchange: Serial# + Precedence + Check + ARRL Section. All 80 sections are multipliers.";

    return meta;
}

ContestBase* ARRLSSSSBContest::create(ModeType mode, const StationInfo& myStation) {
    Q_UNUSED(mode);  // Mode is always SSB for this contest
    return new ARRLSSSSBContest(myStation);
}

} // namespace TR4QT

// Register contest
REGISTER_CONTEST(TR4QT::ARRLSSSSBContest, "ARRL_SS_SSB");
