#include "CQWPXCWContest.h"
#include "ContestMetadata.h"

namespace TR4QT {

ContestMetadata CQWPXCWContest::getMetadata() {
    ContestMetadata meta;
    meta.id = "CQWPX_CW";
    meta.displayName = "CQ WPX Contest - CW";
    meta.shortName = "CQ WPX CW";
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

    meta.schedule = "Last full weekend of May";
    meta.floatingDates = {
        FloatingDate(5, "Last full weekend")  // May
    };

    meta.website = "https://www.cqwpx.com/";
    meta.description = "Work as many callsign prefixes as possible. Exchange: RST + Serial Number.";

    return meta;
}

ContestBase* CQWPXCWContest::create(ModeType mode, const StationInfo& myStation) {
    Q_UNUSED(mode);  // Mode is always CW for this contest
    return new CQWPXCWContest(myStation);
}

} // namespace TR4QT

// Register contest
REGISTER_CONTEST(TR4QT::CQWPXCWContest, "CQWPX_CW");
