#ifndef ARRLDXCWCONTEST_H
#define ARRLDXCWCONTEST_H

#include "ARRLDXBase.h"
#include "ContestRegistry.h"

namespace TR4QT {

/**
 * ARRL International DX Contest - CW
 * Date: 3rd weekend of February (Saturday-Sunday, 48 hours)
 * Mode: CW only
 * Bands: 160m-10m (all HF bands)
 */
class ARRLDXCWContest : public ARRLDXBase {
public:
    // ===== Contest Identifiers =====
    static constexpr int WA7BNM_ID = 8;
    static constexpr const char* CABRILLO_NAME = "ARRL-DX-CW";
    static constexpr const char* ADIF_CONTEST_ID = "ARRL-DX-CW";

    explicit ARRLDXCWContest(const StationInfo& myStation)
        : ARRLDXBase(myStation) {}

    ~ARRLDXCWContest() override = default;

    // ===== Factory Methods =====
    static ContestMetadata getMetadata();
    static ContestBase* create(ModeType mode, const StationInfo& myStation);

    // ===== Contest Identity =====
    QString getContestId() const override { return "ARRL_DX_CW"; }
    QString getContestName() const override { return "ARRL International DX Contest - CW"; }
    ModeType getContestMode() const override { return ModeType::CW; }
    QString getADIFContestId() const override { return ADIF_CONTEST_ID; }

    // ===== Mode Validation =====
    bool isValidMode(ModeType mode, QString& errorMsg) const override {
        if (mode != ModeType::CW && mode != ModeType::CWR) {
            errorMsg = "ARRL DX CW only allows CW mode";
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

#endif // ARRLDXCWCONTEST_H
