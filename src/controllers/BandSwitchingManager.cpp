/*
    TR4QT - An Amateur Radio Contesting Logger inspired by TR4W and TRLog.
    Copyright (C) 2026 Thomas M. Schaefer NY4I ny4i@ny4i.com

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#include "BandSwitchingManager.h"
#include "../core/BandConstants.h"
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
    freq_t freq = BandConstants::bandToFrequency(band);

    // If invalid band, default to 20m
    if (freq == 0) {
        return BandConstants::BAND_20M_EDGE;
    }

    return freq;
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
