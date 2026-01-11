#include "ARRLDXCWContest.h"
#include "ContestMetadata.h"

namespace TR4QT {

ContestMetadata ARRLDXCWContest::getMetadata() {
    ContestMetadata meta;
    meta.id = "ARRL_DX_CW";
    meta.displayName = "ARRL International DX Contest - CW";
    meta.shortName = "ARRL DX CW";
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

    meta.schedule = "3rd weekend of February";
    meta.floatingDates = {
        FloatingDate(2, "3rd Saturday")  // February
    };

    meta.website = "https://contests.arrl.org/";
    meta.description = "W/VE stations work DX only. W/VE send RST+State, DX sends RST+Power.";

    return meta;
}

ContestBase* ARRLDXCWContest::create(ModeType mode, const StationInfo& myStation) {
    Q_UNUSED(mode);  // Mode is always CW for this contest
    return new ARRLDXCWContest(myStation);
}

} // namespace TR4QT

// Register contest
REGISTER_CONTEST(TR4QT::ARRLDXCWContest, "ARRL_DX_CW");
