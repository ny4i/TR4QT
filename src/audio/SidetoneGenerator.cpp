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

#include "SidetoneGenerator.h"
#include "../logging/LogMacros.h"
#include <QAudioFormat>

namespace TR4QT {

SidetoneGenerator::SidetoneGenerator(QObject* parent)
    : QObject(parent)
{
    m_sampleBuffer.resize(BUFFER_SAMPLES);
    connect(&m_pushTimer, &QTimer::timeout, this, &SidetoneGenerator::pushSamples);
}

SidetoneGenerator::~SidetoneGenerator() {
    stop();
}

void SidetoneGenerator::start() {
    if (m_audioSink) return;  // Already started

    QAudioFormat format;
    format.setSampleRate(SAMPLE_RATE);
    format.setChannelCount(1);
    format.setSampleFormat(QAudioFormat::Float);

    QAudioDevice defaultDevice = QMediaDevices::defaultAudioOutput();
    if (defaultDevice.isNull()) {
        LOG_WARN("SidetoneGenerator", "No audio output device available");
        return;
    }

    if (!defaultDevice.isFormatSupported(format)) {
        LOG_WARN("SidetoneGenerator", "Float32 mono 48kHz not supported, trying nearest format");
        format = defaultDevice.preferredFormat();
    }

    m_audioSink = std::make_unique<QAudioSink>(defaultDevice, format);
    m_audioSink->setVolume(m_volumePercent / 100.0);

    // Set a large buffer (200ms) to absorb timer jitter
    const int bufferBytes = BUFFER_SAMPLES * static_cast<int>(sizeof(float));
    m_audioSink->setBufferSize(bufferBytes);

    // Start in push mode: QAudioSink::start() returns a QIODevice* we write to
    m_audioOutput = m_audioSink->start();
    if (!m_audioOutput) {
        LOG_ERROR("SidetoneGenerator", "Failed to start audio sink in push mode");
        m_audioSink.reset();
        return;
    }

    // Don't start push timer here — it starts on-demand when keyDown() is called.
    // This prevents App Nap from queuing thousands of timer events when idle.

    LOG_INFO("SidetoneGenerator", QString("Ready (push mode): %1 Hz, %2% vol, buffer=%3 bytes, sink state=%4")
             .arg(m_frequencyHz).arg(m_volumePercent)
             .arg(m_audioSink->bufferSize())
             .arg(static_cast<int>(m_audioSink->state())));
}

void SidetoneGenerator::stop() {
    m_pushTimer.stop();

    if (m_audioSink) {
        m_audioSink->stop();
        m_audioSink.reset();
    }
    m_audioOutput = nullptr;
    m_envelopeState = EnvelopeState::Silent;
    m_phase = 0.0;
}

void SidetoneGenerator::setFrequency(int hz) {
    m_frequencyHz = qBound(200, hz, 1200);
}

void SidetoneGenerator::setVolume(int percent) {
    m_volumePercent = qBound(0, percent, 100);
    if (m_audioSink) {
        m_audioSink->setVolume(m_volumePercent / 100.0);
    }
}

void SidetoneGenerator::keyDown() {
    if (!m_timingStarted) {
        m_timingTimer.start();
        m_timingStarted = true;
    }
    const qint64 elapsedUs = m_timingTimer.nsecsElapsed() / 1000;
    LOG_INFO("TIMING:TONE", QString("keyDown envState=%1 elapsed=%2us")
             .arg(static_cast<int>(m_envelopeState)).arg(elapsedUs));

    if (m_envelopeState == EnvelopeState::Silent || m_envelopeState == EnvelopeState::RampDown) {
        m_envelopeState = EnvelopeState::RampUp;
        m_rampPosition = 0;
    }

    // Start push timer on-demand (avoids App Nap hang when idle)
    if (!m_pushTimer.isActive() && m_audioOutput) {
        m_silentTickCount = 0;
        pushSamples();  // Pre-fill buffer to avoid initial underrun
        m_pushTimer.start(PUSH_INTERVAL_MS);
    }
}

void SidetoneGenerator::keyUp() {
    const qint64 elapsedUs = m_timingStarted ? m_timingTimer.nsecsElapsed() / 1000 : 0;
    LOG_INFO("TIMING:TONE", QString("keyUp envState=%1 elapsed=%2us")
             .arg(static_cast<int>(m_envelopeState)).arg(elapsedUs));

    if (m_envelopeState == EnvelopeState::Sustain || m_envelopeState == EnvelopeState::RampUp) {
        m_envelopeState = EnvelopeState::RampDown;
        m_rampPosition = 0;
    }
}

void SidetoneGenerator::pushSamples() {
    if (!m_audioOutput || !m_audioSink) return;

    // Fill ALL available buffer space — this is the key to avoiding underruns.
    // bytesFree() tells us how much room the sink has; we generate exactly that much.
    const qint64 bytesFree = m_audioSink->bytesFree();
    if (bytesFree <= 0) return;

    const int samplesToWrite = static_cast<int>(bytesFree / static_cast<qint64>(sizeof(float)));
    if (samplesToWrite <= 0) return;

    // Ensure our reusable buffer is large enough
    if (m_sampleBuffer.size() < samplesToWrite) {
        m_sampleBuffer.resize(samplesToWrite);
    }

    // Generate samples
    for (int i = 0; i < samplesToWrite; ++i) {
        m_sampleBuffer[i] = generateSample();
    }

    // Push to audio sink
    const auto* data = reinterpret_cast<const char*>(m_sampleBuffer.constData());
    const qint64 bytesToWrite = samplesToWrite * static_cast<qint64>(sizeof(float));
    m_audioOutput->write(data, bytesToWrite);

    // Stop push timer when silent long enough to flush the buffer.
    // This prevents App Nap from queuing thousands of timer events when idle.
    if (m_envelopeState == EnvelopeState::Silent) {
        m_silentTickCount++;
        // Wait enough ticks to flush the 200ms buffer (200ms / 10ms = 20 ticks)
        if (m_silentTickCount >= IDLE_STOP_TICKS) {
            m_pushTimer.stop();
            LOG_DEBUG("SidetoneGenerator", "Push timer stopped (idle)");
        }
    } else {
        m_silentTickCount = 0;
    }
}

float SidetoneGenerator::generateSample() {
    // Calculate envelope amplitude
    float envelope = 0.0f;

    switch (m_envelopeState) {
        case EnvelopeState::Silent:
            return 0.0f;

        case EnvelopeState::RampUp:
            // Raised cosine ramp up: 0.5 * (1 - cos(pi * i / RAMP_SAMPLES))
            envelope = 0.5f * (1.0f - static_cast<float>(qCos(M_PI * m_rampPosition / RAMP_SAMPLES)));
            m_rampPosition++;
            if (m_rampPosition >= RAMP_SAMPLES) {
                m_envelopeState = EnvelopeState::Sustain;
            }
            break;

        case EnvelopeState::Sustain:
            envelope = 1.0f;
            break;

        case EnvelopeState::RampDown:
            // Raised cosine ramp down: 0.5 * (1 + cos(pi * i / RAMP_SAMPLES))
            envelope = 0.5f * (1.0f + static_cast<float>(qCos(M_PI * m_rampPosition / RAMP_SAMPLES)));
            m_rampPosition++;
            if (m_rampPosition >= RAMP_SAMPLES) {
                m_envelopeState = EnvelopeState::Silent;
                return 0.0f;
            }
            break;
    }

    // Generate sine wave sample
    const double phaseIncrement = 2.0 * M_PI * m_frequencyHz / SAMPLE_RATE;
    float sample = envelope * static_cast<float>(qSin(m_phase));
    m_phase += phaseIncrement;

    // Keep phase in [0, 2*PI) to avoid floating point drift
    if (m_phase >= 2.0 * M_PI) {
        m_phase -= 2.0 * M_PI;
    }

    return sample;
}

} // namespace TR4QT
