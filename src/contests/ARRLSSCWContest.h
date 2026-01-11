#ifndef ARRLSSCWCONTEST_H
#define ARRLSSCWCONTEST_H

#include "ARRLSSBase.h"
#include "ContestRegistry.h"

namespace TR4QT {

/**
 * ARRL Sweepstakes Contest - CW
 * Date: 1st full weekend of November
 * Mode: CW only
 * Bands: 160m-10m (all HF bands)
 */
class ARRLSSCWContest : public ARRLSSBase {
public:
    // ===== Contest Identifiers =====
    static constexpr int WA7BNM_ID = 1;
    static constexpr const char* CABRILLO_NAME = "ARRL-SS-CW";
    static constexpr const char* ADIF_CONTEST_ID = "ARRL-SS-CW";

    explicit ARRLSSCWContest(const StationInfo& myStation)
        : ARRLSSBase(myStation) {}

    ~ARRLSSCWContest() override = default;

    // ===== Factory Methods =====
    static ContestMetadata getMetadata();
    static ContestBase* create(ModeType mode, const StationInfo& myStation);

    // ===== Contest Identity =====
    QString getContestId() const override { return "ARRL_SS_CW"; }
    QString getContestName() const override { return "ARRL Sweepstakes - CW"; }
    ModeType getContestMode() const override { return ModeType::CW; }
    QString getADIFContestId() const override { return ADIF_CONTEST_ID; }

    // ===== Mode Validation =====
    bool isValidMode(ModeType mode, QString& errorMsg) const override {
        if (mode != ModeType::CW && mode != ModeType::CWR) {
            errorMsg = "ARRL Sweepstakes CW only allows CW mode";
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

#endif // ARRLSSCWCONTEST_H
