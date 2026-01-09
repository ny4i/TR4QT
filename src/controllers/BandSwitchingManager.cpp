#include "BandSwitchingManager.h"
#include "../utils/AppSettings.h"
#include "../logging/LogMacros.h"
#include <QtMath>

namespace TR4QT {

BandSwitchingManager::BandSwitchingManager(QObject* parent)
    : QObject(parent)
    , m_lastFrequency(0)
{
    LOG_DEBUG("BandSwitchingManager", "Initialized");
}

BandSwitchingManager::~BandSwitchingManager() {
    LOG_DEBUG("BandSwitchingManager", "Destroyed");
}

void BandSwitchingManager::selectBand(BandType band, const RadioState& currentState, bool isRadioConnected) {
    if (isRadioConnected) {
        // Radio connected: Caller should send band change to radio
        // This manager doesn't control hardware directly
        LOG_DEBUG("BandSwitchingManager", QString("Band clicked: %1 (radio connected - caller handles radio command)")
            .arg(bandToString(band)));
        return;
    }

    // Radio not connected: Manual band selection for logging
    LOG_DEBUG("BandSwitchingManager", QString("Manual band selection: %1").arg(bandToString(band)));

    // Calculate frequency for this band (band edge for manual selection)
    freq_t frequency = getFrequencyForBand(band, currentState.modeA);

    // Emit signals for UI/state updates
    double freqKHz = frequency / 1000.0;
    LOG_DEBUG("BandSwitchingManager", QString("Manual band selection emitting - band: %1, frequency: %2 kHz")
        .arg(bandToString(band)).arg(freqKHz, 0, 'f', 1));

    emit frequencyChanged(frequency);
    emit bandChanged(band);
    emit statusMessage(QString("Band: %1 (manual)").arg(bandToString(band)));
}

BandType BandSwitchingManager::getNextBand(BandType currentBand, const ContestBase* contest) const {
    QList<BandType> allowedBands = getAllowedBands(contest);

    // Find current band in allowed list
    int currentIndex = allowedBands.indexOf(currentBand);
    if (currentIndex == -1 || currentIndex >= allowedBands.size() - 1) {
        return currentBand;  // Already at highest or invalid band
    }

    return allowedBands[currentIndex + 1];
}

BandType BandSwitchingManager::getPreviousBand(BandType currentBand, const ContestBase* contest) const {
    QList<BandType> allowedBands = getAllowedBands(contest);

    // Find current band in allowed list
    int currentIndex = allowedBands.indexOf(currentBand);
    if (currentIndex <= 0) {
        return currentBand;  // Already at lowest or invalid band
    }

    return allowedBands[currentIndex - 1];
}

freq_t BandSwitchingManager::getFrequencyForBand(BandType band, ModeType mode) const {
    Q_UNUSED(mode);  // Not used - we return band edge for manual selection

    // Return low band edge as visual reminder this is manually set, not from radio
    // Real radio would show frequency within CW/SSB segments
    switch (band) {
    case BandType::Band160M:
        return 1800000;   // 1.800 MHz (band edge)
    case BandType::Band80M:
        return 3500000;   // 3.500 MHz (band edge)
    case BandType::Band60M:
        return 5330000;   // 5.330 MHz (band edge)
    case BandType::Band40M:
        return 7000000;   // 7.000 MHz (band edge)
    case BandType::Band30M:
        return 10100000;  // 10.100 MHz (band edge)
    case BandType::Band20M:
        return 14000000;  // 14.000 MHz (band edge)
    case BandType::Band17M:
        return 18068000;  // 18.068 MHz (band edge)
    case BandType::Band15M:
        return 21000000;  // 21.000 MHz (band edge)
    case BandType::Band12M:
        return 24890000;  // 24.890 MHz (band edge)
    case BandType::Band10M:
        return 28000000;  // 28.000 MHz (band edge)
    case BandType::Band6M:
        return 50000000;  // 50.000 MHz (band edge)
    case BandType::Band4M:
        return 70000000;  // 70.000 MHz (band edge)
    case BandType::Band2M:
        return 144000000; // 144.000 MHz (band edge)
    case BandType::Band70CM:
        return 420000000; // 420.000 MHz (band edge)
    default:
        return 14000000;  // Default to 20m band edge
    }
}

BandType BandSwitchingManager::getBandFromFrequency(freq_t frequency) const {
    // Use existing frequencyToBand utility from Types.h
    return frequencyToBand(frequency);
}

bool BandSwitchingManager::checkAutoSPCondition(freq_t newFrequency, bool autoSPEnabled, bool alreadyInSPMode) {
    // Only check if AUTO S&P is enabled
    if (!autoSPEnabled) {
        return false;
    }

    // Ignore if already in S&P mode
    if (alreadyInSPMode) {
        return false;
    }

    // Need at least two frequency samples to calculate rate
    if (m_lastFrequency == 0) {
        m_lastFrequency = newFrequency;
        m_lastFrequencyTime = QDateTime::currentDateTime();
        return false;
    }

    // Calculate time difference in seconds
    QDateTime now = QDateTime::currentDateTime();
    qint64 msecs = m_lastFrequencyTime.msecsTo(now);

    // Ignore if time difference is too small (avoid division by zero and spurious triggers)
    if (msecs < AUTO_SP_MIN_TIME_MS) {
        return false;
    }

    double seconds = msecs / 1000.0;

    // Calculate Hz/sec
    qint64 freqDelta = qAbs(static_cast<qint64>(newFrequency) - static_cast<qint64>(m_lastFrequency));
    double hzPerSec = freqDelta / seconds;

    // Get sensitivity threshold from settings
    int threshold = AppSettings::instance().getAutoSPSensitivity();

    // Update tracking
    m_lastFrequency = newFrequency;
    m_lastFrequencyTime = now;

    // Check if movement exceeds threshold
    if (hzPerSec >= threshold) {
        LOG_DEBUG("BandSwitchingManager", QString("AUTO S&P triggered: %.1f Hz/sec (threshold: %2 Hz/sec)")
            .arg(hzPerSec).arg(threshold));
        emit autoSPTriggered(hzPerSec, threshold);
        return true;
    }

    return false;
}

QList<BandType> BandSwitchingManager::getAllowedBands(const ContestBase* contest) const {
    if (contest) {
        return contest->getAllowedBands();
    } else {
        // Default: standard HF contest bands
        return { BandType::Band160M, BandType::Band80M, BandType::Band40M,
                 BandType::Band20M, BandType::Band15M, BandType::Band10M };
    }
}

} // namespace TR4QT
