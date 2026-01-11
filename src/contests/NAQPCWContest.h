#ifndef NAQPCWCONTEST_H
#define NAQPCWCONTEST_H

#include "NAQPBase.h"
#include "ContestRegistry.h"

namespace TR4QT {

/**
 * NAQP CW Contest
 * Dates: 2nd Saturday of January and August
 * Mode: CW only
 * Bands: 160m-10m (all HF bands)
 */
class NAQPCWContest : public NAQPBase {
public:
    // ===== Contest Identifiers =====
    static constexpr int WA7BNM_ID_JAN = 181;  // January CW
    static constexpr int WA7BNM_ID_AUG = 184;  // August CW
    static constexpr const char* CABRILLO_NAME = "NAQP-CW";
    static constexpr const char* ADIF_CONTEST_ID = "NAQP-CW";

    explicit NAQPCWContest(const StationInfo& myStation)
        : NAQPBase(myStation) {}

    ~NAQPCWContest() override = default;

    // ===== Factory Methods =====
    static ContestMetadata getMetadata();
    static ContestBase* create(ModeType mode, const StationInfo& myStation);

    // ===== Contest Identity =====
    QString getContestId() const override { return "NAQP_CW"; }
    QString getContestName() const override { return "North American QSO Party - CW"; }
    ModeType getContestMode() const override { return ModeType::CW; }
    QString getADIFContestId() const override { return ADIF_CONTEST_ID; }

    // ===== Bands =====
    QList<BandType> getAllowedBands() const override {
        return {BandType::Band160M, BandType::Band80M, BandType::Band40M,
                BandType::Band20M, BandType::Band15M, BandType::Band10M};
    }

    // ===== Mode Validation =====
    bool isValidMode(ModeType mode, QString& errorMsg) const override {
        if (mode != ModeType::CW) {
            errorMsg = "NAQP-CW only allows CW mode";
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

#endif // NAQPCWCONTEST_H
