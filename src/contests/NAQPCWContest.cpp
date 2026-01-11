#include "NAQPCWContest.h"
#include "ContestMetadata.h"

namespace TR4QT {

ContestMetadata NAQPCWContest::getMetadata() {
    ContestMetadata meta;
    meta.id = "NAQP_CW";
    meta.displayName = "North American QSO Party - CW";
    meta.shortName = "NAQP-CW";
    meta.supportedModes = {ModeType::CW};
    meta.hasSeparateContests = false;  // This IS the specific CW contest

    meta.wa7bnmIdCW = WA7BNM_ID_JAN;
    meta.wa7bnmIdSSB = 0;
    meta.wa7bnmIdMixed = WA7BNM_ID_AUG;

    meta.cabrilloNameCW = CABRILLO_NAME;
    meta.cabrilloNameSSB = "";
    meta.cabrilloNameMixed = CABRILLO_NAME;

    meta.adifContestIdCW = ADIF_CONTEST_ID;
    meta.adifContestIdSSB = "";
    meta.adifContestIdMixed = ADIF_CONTEST_ID;

    meta.schedule = "2nd Saturday of January and August";
    meta.floatingDates = {
        FloatingDate(1, "2nd Saturday"),  // January
        FloatingDate(8, "2nd Saturday")   // August
    };

    meta.website = "https://ncjweb.com/naqp/";
    meta.description = "12-hour low power CW sprint. Exchange: Name + State/Province. Max 100W.";

    return meta;
}

ContestBase* NAQPCWContest::create(ModeType mode, const StationInfo& myStation) {
    Q_UNUSED(mode);  // Mode is always CW for this contest
    return new NAQPCWContest(myStation);
}

} // namespace TR4QT

// Register contest
REGISTER_CONTEST(TR4QT::NAQPCWContest, "NAQP_CW");
