#ifndef NAQPSSBCONTEST_H
#define NAQPSSBCONTEST_H

#include "NAQPBase.h"
#include "ContestRegistry.h"

namespace TR4QT {

/**
 * NAQP SSB Contest
 * Dates: 3rd Saturday of January and August
 * Mode: SSB (USB/LSB) only
 * Bands: 160m-10m (all HF bands)
 */
class NAQPSSBContest : public NAQPBase {
public:
    // ===== Contest Identifiers =====
    static constexpr int WA7BNM_ID_JAN = 182;  // January SSB
    static constexpr int WA7BNM_ID_AUG = 185;  // August SSB
    static constexpr const char* CABRILLO_NAME = "NAQP-SSB";
    static constexpr const char* ADIF_CONTEST_ID = "NAQP-SSB";

    explicit NAQPSSBContest(const StationInfo& myStation)
        : NAQPBase(myStation) {}

    ~NAQPSSBContest() override = default;

    // ===== Factory Methods =====
    static ContestMetadata getMetadata();
    static ContestBase* create(ModeType mode, const StationInfo& myStation);

    // ===== Contest Identity =====
    QString getContestId() const override { return "NAQP_SSB"; }
    QString getContestName() const override { return "North American QSO Party - SSB"; }
    ModeType getContestMode() const override { return ModeType::USB; }
    QString getADIFContestId() const override { return ADIF_CONTEST_ID; }
    int getWA7BNMContestId() const override { return WA7BNM_ID_JAN; }  // Use January ID as default

    // ===== Bands =====
    QList<BandType> getAllowedBands() const override {
        return {BandType::Band160M, BandType::Band80M, BandType::Band40M,
                BandType::Band20M, BandType::Band15M, BandType::Band10M};
    }

    // ===== Mode Validation =====
    bool isValidMode(ModeType mode, QString& errorMsg) const override {
        if (mode != ModeType::USB && mode != ModeType::LSB) {
            errorMsg = "NAQP-SSB only allows SSB modes (USB/LSB)";
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

#endif // NAQPSSBCONTEST_H
