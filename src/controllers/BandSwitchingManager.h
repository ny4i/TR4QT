#ifndef BANDSWITCHINGMANAGER_H
#define BANDSWITCHINGMANAGER_H

#include <QObject>
#include <QDateTime>
#include "../core/Types.h"
#include "../radio/RadioInterface.h"
#include "../contests/ContestBase.h"

namespace TR4QT {

/**
 * BandSwitchingManager
 *
 * Handles band switching logic and navigation.
 * Separates band selection and frequency conversion from MainWindow.
 *
 * Responsibilities:
 * - Handle manual band selection (when radio disconnected)
 * - Navigate band list (up/down through allowed bands)
 * - Convert bands to frequencies (with mode-aware band edges)
 * - Convert frequencies to bands
 * - Detect AUTO S&P band changes (frequency movement detection)
 *
 * This manager does NOT:
 * - Control the radio hardware (use RadioManager/RadioController)
 * - Update UI directly (emits signals for UI updates)
 * - Store radio state long-term (receives RadioState references)
 */
class BandSwitchingManager : public QObject {
    Q_OBJECT

public:
    /**
     * Construct a BandSwitchingManager
     * @param parent Parent QObject
     */
    explicit BandSwitchingManager(QObject* parent = nullptr);

    /**
     * Destructor
     */
    ~BandSwitchingManager() override;

    /**
     * Handle band selection (clicked in band buttons)
     * For manual operation (radio disconnected), updates internal state
     * and emits signals for frequency/band change.
     *
     * For radio-connected operation, the caller should send the band
     * change to the radio directly (via RadioController::setBand).
     *
     * @param band Selected band
     * @param currentState Current radio state (contains mode for frequency calculation)
     * @param isRadioConnected Whether radio is connected
     */
    void selectBand(BandType band, const RadioState& currentState, bool isRadioConnected);

    /**
     * Navigate to next higher band
     * Uses contest's allowed bands if available, otherwise defaults to HF bands.
     *
     * @param currentBand Current band
     * @param contest Active contest (for allowed bands) or nullptr
     * @return Next band, or currentBand if already at highest
     */
    BandType getNextBand(BandType currentBand, const ContestBase* contest) const;

    /**
     * Navigate to previous lower band
     * Uses contest's allowed bands if available, otherwise defaults to HF bands.
     *
     * @param currentBand Current band
     * @param contest Active contest (for allowed bands) or nullptr
     * @return Previous band, or currentBand if already at lowest
     */
    BandType getPreviousBand(BandType currentBand, const ContestBase* contest) const;

    /**
     * Get frequency for a band (band edge)
     * Returns low band edge as visual indicator for manual selection.
     * Real radio would show frequency within CW/SSB segments.
     *
     * @param band Band to convert
     * @param mode Current mode (unused, reserved for future mode-specific edges)
     * @return Frequency in Hz (band edge)
     */
    freq_t getFrequencyForBand(BandType band, ModeType mode) const;

    /**
     * Convert frequency to band
     * Uses standard amateur radio band allocations.
     *
     * @param frequency Frequency in Hz
     * @return Detected band, or BandType::None if not a ham band
     */
    BandType getBandFromFrequency(freq_t frequency) const;

    /**
     * Check AUTO S&P condition (band change detection)
     * Monitors frequency movement rate and triggers S&P mode if threshold exceeded.
     * Only checks if AUTO S&P is enabled in settings and not already in S&P mode.
     *
     * @param newFrequency New frequency from radio (Hz)
     * @param autoSPEnabled Whether AUTO S&P automation is enabled
     * @param alreadyInSPMode Whether already in S&P operating mode
     * @return true if AUTO S&P should trigger (frequency movement > threshold)
     */
    bool checkAutoSPCondition(freq_t newFrequency, bool autoSPEnabled, bool alreadyInSPMode);

signals:
    /**
     * Emitted when band selection causes frequency change
     * (manual band selection only - radio disconnected)
     * @param frequency New frequency in Hz
     */
    void frequencyChanged(freq_t frequency);

    /**
     * Emitted when band changes
     * (manual band selection only - radio disconnected)
     * @param band New band
     */
    void bandChanged(BandType band);

    /**
     * Emitted when AUTO S&P should trigger
     * @param hzPerSec Frequency movement rate that triggered AUTO S&P
     * @param threshold Threshold that was exceeded
     */
    void autoSPTriggered(double hzPerSec, int threshold);

    /**
     * Emitted when status message should be displayed
     * @param message Status message for UI
     */
    void statusMessage(const QString& message);

private:
    /**
     * Get allowed bands for current contest
     * @param contest Active contest or nullptr
     * @return List of allowed bands (defaults to HF bands if no contest)
     */
    QList<BandType> getAllowedBands(const ContestBase* contest) const;

    // AUTO S&P tracking
    freq_t m_lastFrequency;            // Last frequency for AUTO S&P detection
    QDateTime m_lastFrequencyTime;     // Timestamp of last frequency update

    // Constants
    static constexpr int AUTO_SP_MIN_TIME_MS = 100;  // Minimum time between samples (ms)
};

} // namespace TR4QT

#endif // BANDSWITCHINGMANAGER_H
