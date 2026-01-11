#include "CQWPXSSBContest.h"
#include "ContestMetadata.h"

namespace TR4QT {

ContestMetadata CQWPXSSBContest::getMetadata() {
    ContestMetadata meta;
    meta.id = "CQWPX_SSB";
    meta.displayName = "CQ WPX Contest - SSB";
    meta.shortName = "CQ WPX SSB";
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

    meta.schedule = "4th Saturday of March";
    meta.floatingDates = {
        FloatingDate(3, "4th Saturday")  // March
    };

    meta.website = "https://www.cqwpx.com/";
    meta.description = "Work as many callsign prefixes as possible. Exchange: RST + Serial Number.";

    return meta;
}

ContestBase* CQWPXSSBContest::create(ModeType mode, const StationInfo& myStation) {
    Q_UNUSED(mode);  // Mode is always SSB for this contest
    return new CQWPXSSBContest(myStation);
}

} // namespace TR4QT

// Register contest
REGISTER_CONTEST(TR4QT::CQWPXSSBContest, "CQWPX_SSB");
