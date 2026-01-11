#include "CQWWCWContest.h"
#include "ContestMetadata.h"

namespace TR4QT {

ContestMetadata CQWWCWContest::getMetadata() {
    ContestMetadata meta;
    meta.id = "CQWW_CW";
    meta.displayName = "CQ World Wide DX Contest - CW";
    meta.shortName = "CQ WW CW";
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

    meta.schedule = "Last full weekend of November";
    meta.floatingDates = {
        FloatingDate(11, "Last full weekend")  // November
    };

    meta.website = "https://www.cqww.com/";
    meta.description = "Work as many countries and CQ zones as possible. Exchange: RST + CQ Zone.";

    return meta;
}

ContestBase* CQWWCWContest::create(ModeType mode, const StationInfo& myStation) {
    Q_UNUSED(mode);  // Mode is always CW for this contest
    return new CQWWCWContest(myStation);
}

} // namespace TR4QT

// Register contest
REGISTER_CONTEST(TR4QT::CQWWCWContest, "CQWW_CW");
