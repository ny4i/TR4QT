#ifndef CQWWCWCONTEST_H
#define CQWWCWCONTEST_H

#include "CQWWBase.h"
#include "ContestRegistry.h"

namespace TR4QT {

/**
 * CQ World Wide DX Contest - CW
 * Date: Last full weekend of November
 * Mode: CW only
 * Bands: 160m-10m (all HF bands)
 *
 * CW gets higher QSO points than SSB
 */
class CQWWCWContest : public CQWWBase {
public:
    // ===== Contest Identifiers =====
    static constexpr int WA7BNM_ID = 3;
    static constexpr const char* CABRILLO_NAME = "CQ-WW-CW";
    static constexpr const char* ADIF_CONTEST_ID = "CQ-WW-CW";

    explicit CQWWCWContest(const StationInfo& myStation)
        : CQWWBase(myStation) {}

    ~CQWWCWContest() override = default;

    // ===== Factory Methods =====
    static ContestMetadata getMetadata();
    static ContestBase* create(ModeType mode, const StationInfo& myStation);

    // ===== Contest Identity =====
    QString getContestId() const override { return "CQWW_CW"; }
    QString getContestName() const override { return "CQ World Wide DX Contest - CW"; }
    ModeType getContestMode() const override { return ModeType::CW; }
    QString getADIFContestId() const override { return ADIF_CONTEST_ID; }
    int getWA7BNMContestId() const override { return WA7BNM_ID; }

    // ===== Mode Validation =====
    bool isValidMode(ModeType mode, QString& errorMsg) const override {
        if (mode != ModeType::CW && mode != ModeType::CWR) {
            errorMsg = "CQ WW CW only allows CW mode";
            return false;
        }
        return true;
    }

    // ===== Cabrillo =====
    QMap<QString, QString> getCabrilloHeaders() const override {
        QMap<QString, QString> headers = ContestBase::getCabrilloHeaders();
        headers["CONTEST"] = CABRILLO_NAME;
        return headers;
    }

protected:
    // ===== Mode Point Multiplier =====
    double getModePointMultiplier() const override {
        return 1.0;  // CW gets full points (3 for different continent)
    }
};

} // namespace TR4QT

#endif // CQWWCWCONTEST_H
