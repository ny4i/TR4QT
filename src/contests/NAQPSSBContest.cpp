#include "NAQPSSBContest.h"
#include "ContestMetadata.h"

namespace TR4QT {

ContestMetadata NAQPSSBContest::getMetadata() {
    ContestMetadata meta;
    meta.id = "NAQP_SSB";
    meta.displayName = "North American QSO Party - SSB";
    meta.shortName = "NAQP-SSB";
    meta.supportedModes = {ModeType::USB, ModeType::LSB};
    meta.hasSeparateContests = false;  // This IS the specific SSB contest

    meta.wa7bnmIdCW = 0;
    meta.wa7bnmIdSSB = WA7BNM_ID_JAN;
    meta.wa7bnmIdMixed = WA7BNM_ID_AUG;

    meta.cabrilloNameCW = "";
    meta.cabrilloNameSSB = CABRILLO_NAME;
    meta.cabrilloNameMixed = CABRILLO_NAME;

    meta.adifContestIdCW = "";
    meta.adifContestIdSSB = ADIF_CONTEST_ID;
    meta.adifContestIdMixed = ADIF_CONTEST_ID;

    meta.schedule = "3rd Saturday of January and August";
    meta.floatingDates = {
        FloatingDate(1, "3rd Saturday"),  // January
        FloatingDate(8, "3rd Saturday")   // August
    };

    meta.website = "https://ncjweb.com/naqp/";
    meta.description = "12-hour low power SSB sprint. Exchange: Name + State/Province. Max 100W.";

    return meta;
}

ContestBase* NAQPSSBContest::create(ModeType mode, const StationInfo& myStation) {
    Q_UNUSED(mode);  // Mode is always SSB for this contest
    return new NAQPSSBContest(myStation);
}

} // namespace TR4QT

// Register contest
REGISTER_CONTEST(TR4QT::NAQPSSBContest, "NAQP_SSB");
