#include "ARRLDXPhoneContest.h"
#include "ContestMetadata.h"

namespace TR4QT {

ContestMetadata ARRLDXPhoneContest::getMetadata() {
    ContestMetadata meta;
    meta.id = "ARRL_DX_SSB";
    meta.displayName = "ARRL International DX Contest - Phone";
    meta.shortName = "ARRL DX Phone";
    meta.supportedModes = {ModeType::USB, ModeType::LSB};
    meta.hasSeparateContests = false;  // This IS the specific Phone contest

    meta.wa7bnmIdCW = 0;
    meta.wa7bnmIdSSB = WA7BNM_ID;
    meta.wa7bnmIdMixed = 0;

    meta.cabrilloNameCW = "";
    meta.cabrilloNameSSB = CABRILLO_NAME;
    meta.cabrilloNameMixed = "";

    meta.adifContestIdCW = "";
    meta.adifContestIdSSB = ADIF_CONTEST_ID;
    meta.adifContestIdMixed = "";

    meta.schedule = "1st weekend of March";
    meta.floatingDates = {
        FloatingDate(3, "1st Saturday")  // March
    };

    meta.website = "https://contests.arrl.org/";
    meta.description = "W/VE stations work DX only. W/VE send RST+State, DX sends RST+Power.";

    return meta;
}

ContestBase* ARRLDXPhoneContest::create(ModeType mode, const StationInfo& myStation) {
    Q_UNUSED(mode);  // Mode is always Phone for this contest
    return new ARRLDXPhoneContest(myStation);
}

} // namespace TR4QT

// Register contest
REGISTER_CONTEST(TR4QT::ARRLDXPhoneContest, "ARRL_DX_SSB");
