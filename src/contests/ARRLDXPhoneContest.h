#ifndef ARRLDXPHONECONTEST_H
#define ARRLDXPHONECONTEST_H

#include "ARRLDXBase.h"
#include "ContestRegistry.h"

namespace TR4QT {

/**
 * ARRL International DX Contest - Phone
 * Date: 1st weekend of March (Saturday-Sunday, 48 hours)
 * Mode: Phone/SSB only (USB/LSB)
 * Bands: 160m-10m (all HF bands)
 */
class ARRLDXPhoneContest : public ARRLDXBase {
public:
    // ===== Contest Identifiers =====
    static constexpr int WA7BNM_ID = 9;
    static constexpr const char* CABRILLO_NAME = "ARRL-DX-SSB";
    static constexpr const char* ADIF_CONTEST_ID = "ARRL-DX-SSB";

    explicit ARRLDXPhoneContest(const StationInfo& myStation)
        : ARRLDXBase(myStation) {}

    ~ARRLDXPhoneContest() override = default;

    // ===== Factory Methods =====
    static ContestMetadata getMetadata();
    static ContestBase* create(ModeType mode, const StationInfo& myStation);

    // ===== Contest Identity =====
    QString getContestId() const override { return "ARRL_DX_SSB"; }
    QString getContestName() const override { return "ARRL International DX Contest - Phone"; }
    ModeType getContestMode() const override { return ModeType::USB; }
    QString getADIFContestId() const override { return ADIF_CONTEST_ID; }
    int getWA7BNMContestId() const override { return WA7BNM_ID; }

    // ===== Mode Validation =====
    bool isValidMode(ModeType mode, QString& errorMsg) const override {
        if (mode != ModeType::USB && mode != ModeType::LSB) {
            errorMsg = "ARRL DX Phone only allows Phone modes (USB/LSB)";
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

#endif // ARRLDXPHONECONTEST_H
