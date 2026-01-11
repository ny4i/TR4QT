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
