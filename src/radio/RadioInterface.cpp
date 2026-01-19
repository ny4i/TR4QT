#include "RadioInterface.h"
#include "../logging/LogMacros.h"

namespace TR4QT {

freq_t RadioInterface::getLastFrequencyForBand(BandType band, freq_t fallback) const
{
    if (m_bandMemory.contains(band)) {
        freq_t freq = m_bandMemory[band];
        LOG_DEBUG("RadioInterface", QString("Band memory: %1 -> %2 Hz (cached)")
            .arg(bandToString(band)).arg(freq));
        return freq;
    } else {
        LOG_DEBUG("RadioInterface", QString("Band memory: %1 -> %2 Hz (fallback, first time)")
            .arg(bandToString(band)).arg(fallback));
        // Store fallback for next time
        m_bandMemory[band] = fallback;
        return fallback;
    }
}

void RadioInterface::updateBandMemory(freq_t freq)
{
    BandType band = frequencyToBand(freq);
    if (band != BandType::None) {
        freq_t oldFreq = m_bandMemory.value(band, 0);
        m_bandMemory[band] = freq;

        if (oldFreq != freq) {
            LOG_TRACE("RadioInterface", QString("Band memory updated: %1 -> %2 Hz (was %3 Hz)")
                .arg(bandToString(band)).arg(freq).arg(oldFreq));
        }
    }
}

} // namespace TR4QT
