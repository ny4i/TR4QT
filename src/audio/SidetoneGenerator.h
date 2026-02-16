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

#ifndef SIDETONEGENERATOR_H
#define SIDETONEGENERATOR_H

#include <QObject>
#include <QIODevice>
#include <QAudioSink>
#include <QMediaDevices>
#include <QTimer>
#include <QtMath>
#include <QVector>
#include <QElapsedTimer>
#include <memory>

namespace TR4QT {

/**
 * Software sidetone generator using Qt Multimedia (push mode).
 *
 * Generates a sine wave tone with raised-cosine envelope ramps to avoid clicks.
 * Used by KeyerSetupDialog for practice mode and paddle feedback.
 *
 * Uses push mode with aggressive buffering:
 * - QAudioSink configured with 200ms buffer (9600 samples)
 * - QTimer fires every 10ms and fills ALL available buffer space
 * - This tolerates timer jitter up to ~190ms without underrun
 *
 * Envelope state machine:
 *   Silent -> RampUp -> Sustain -> RampDown -> Silent
 */
class SidetoneGenerator : public QObject {
    Q_OBJECT

public:
    explicit SidetoneGenerator(QObject* parent = nullptr);
    ~SidetoneGenerator() override;

    void start();
    void stop();

    void setFrequency(int hz);
    int frequency() const { return m_frequencyHz; }

    void setVolume(int percent);
    int volume() const { return m_volumePercent; }

public slots:
    void keyDown();
    void keyUp();

private slots:
    void pushSamples();

private:
    enum class EnvelopeState {
        Silent,
        RampUp,
        Sustain,
        RampDown
    };

    // Audio format constants
    static constexpr int SAMPLE_RATE = 48000;
    static constexpr int RAMP_SAMPLES = 240;          // 5ms at 48kHz (click-free transitions)
    static constexpr int PUSH_INTERVAL_MS = 10;        // Timer interval
    static constexpr int BUFFER_DURATION_MS = 200;     // Audio sink buffer size
    static constexpr int BUFFER_SAMPLES = SAMPLE_RATE * BUFFER_DURATION_MS / 1000;  // 9600
    static constexpr int IDLE_STOP_TICKS = BUFFER_DURATION_MS / PUSH_INTERVAL_MS;  // 20 ticks to flush buffer

    // Envelope state
    EnvelopeState m_envelopeState = EnvelopeState::Silent;
    int m_rampPosition = 0;

    // Tone parameters
    int m_frequencyHz = 600;
    int m_volumePercent = 50;
    double m_phase = 0.0;

    // Audio output (push mode)
    std::unique_ptr<QAudioSink> m_audioSink;
    QIODevice* m_audioOutput = nullptr;  // Owned by QAudioSink
    QTimer m_pushTimer;

    // Reusable sample buffer (avoids per-tick allocation)
    QVector<float> m_sampleBuffer;

    // Idle detection — stop push timer when silent to prevent App Nap hang
    int m_silentTickCount = 0;

    float generateSample();

    // Timing diagnostics
    QElapsedTimer m_timingTimer;
    bool m_timingStarted = false;
};

} // namespace TR4QT

#endif // SIDETONEGENERATOR_H
