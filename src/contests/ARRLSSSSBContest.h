#ifndef ARRLSSSSBCONTEST_H
#define ARRLSSSSBCONTEST_H

#include "ARRLSSBase.h"
#include "ContestRegistry.h"

namespace TR4QT {

/**
 * ARRL Sweepstakes Contest - SSB
 * Date: 3rd full weekend of November
 * Mode: SSB/Phone only (USB/LSB)
 * Bands: 160m-10m (all HF bands)
 */
class ARRLSSSSBContest : public ARRLSSBase {
public:
    // ===== Contest Identifiers =====
    static constexpr int WA7BNM_ID = 2;
    static constexpr const char* CABRILLO_NAME = "ARRL-SS-SSB";
    static constexpr const char* ADIF_CONTEST_ID = "ARRL-SS-SSB";

    explicit ARRLSSSSBContest(const StationInfo& myStation)
        : ARRLSSBase(myStation) {}

    ~ARRLSSSSBContest() override = default;

    // ===== Factory Methods =====
    static ContestMetadata getMetadata();
    static ContestBase* create(ModeType mode, const StationInfo& myStation);

    // ===== Contest Identity =====
    QString getContestId() const override { return "ARRL_SS_SSB"; }
    QString getContestName() const override { return "ARRL Sweepstakes - SSB"; }
    ModeType getContestMode() const override { return ModeType::USB; }
    QString getADIFContestId() const override { return ADIF_CONTEST_ID; }
    int getWA7BNMContestId() const override { return WA7BNM_ID; }

    // ===== Mode Validation =====
    bool isValidMode(ModeType mode, QString& errorMsg) const override {
        if (mode != ModeType::USB && mode != ModeType::LSB) {
            errorMsg = "ARRL Sweepstakes SSB only allows Phone modes (USB/LSB)";
            return false;
        }
        return true;
    }

    // ===== Cabrillo =====
    QMap<QString, QString> getCabrilloHeaders() const override {
        QMap<QString, QString> headers;
        headers["CONTEST"] = CABRILLO_NAME;
        return headers;
    }
};

} // namespace TR4QT

#endif // ARRLSSSSBCONTEST_H
