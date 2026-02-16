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

#ifndef IAMBICKEYER_H
#define IAMBICKEYER_H

#include <QObject>
#include <QTimer>
#include <QElapsedTimer>
#include "KeyerConfig.h"

namespace TR4QT {

/**
 * Software iambic keyer engine.
 *
 * Converts paddle press/release events into timed key-down/key-up signals.
 * Supports Iambic Mode A and Mode B.
 *
 * State machine:
 *   Idle → TonePlaying → InterElementSpace → (next element or Idle)
 *
 * Timing:
 *   dit = 1200 / WPM ms
 *   dah = 3 × dit ms
 *   inter-element space = 1 dit
 *
 * Iambic alternation: opposite paddle pressed during tone queues alternate element.
 * Iambic repetition: same paddle held continues same element.
 * Mode B squeeze: both paddles held then released sends one more alternate element.
 *
 * Emits keyDown()/keyUp() signals for the RadioController to send CAT commands.
 *
 * Based on NetKeyer's IambicKeyer implementation.
 */
class IambicKeyer : public QObject {
    Q_OBJECT

public:
    explicit IambicKeyer(QObject* parent = nullptr);
    ~IambicKeyer() override = default;

    // Configuration
    void setWpm(int wpm);
    int wpm() const { return m_wpm; }

    void setMode(IambicMode mode) { m_modeB = (mode == IambicMode::IambicB); }
    IambicMode mode() const { return m_modeB ? IambicMode::IambicB : IambicMode::IambicA; }

public slots:
    /**
     * Update paddle state. Call whenever paddle state changes.
     * @param dit true if dit paddle is pressed
     * @param dah true if dah paddle is pressed
     */
    void updatePaddleState(bool dit, bool dah);

    /**
     * Stop keyer immediately and reset to idle.
     */
    void stop();

signals:
    /**
     * Emitted when the transmitter should key down (start sending).
     * Connect to RadioController::sendKeyDown().
     */
    void keyDown();

    /**
     * Emitted when the transmitter should key up (stop sending).
     * Connect to RadioController::sendKeyUp().
     */
    void keyUp();

private slots:
    void onElementTimerExpired();
    void onSpaceTimerExpired();

private:
    enum class KeyerState {
        Idle,               // Nothing playing, nothing queued
        TonePlaying,        // Key is down, element timing running
        InterElementSpace   // Key is up, inter-element space timing running
    };

    void startNextElement();
    int determineNextToneDuration() const;  // Returns 0 if no element to send
    void startTone(int durationMs);

    // State machine
    KeyerState m_state = KeyerState::Idle;

    // Timers for element and space timing
    QTimer m_elementTimer;  // Times the key-down portion (dit or dah duration)
    QTimer m_spaceTimer;    // Times the inter-element space (1 dit)

    // CW speed
    int m_wpm = 25;
    int m_ditLengthMs = 48;  // 1200 / 25 = 48ms

    // Mode
    bool m_modeB = true;  // Default to Iambic B

    // Current paddle states (updated by updatePaddleState)
    bool m_currentDit = false;
    bool m_currentDah = false;

    // Latches for alternation detection
    bool m_ditLatched = false;
    bool m_dahLatched = false;

    // Paddle states at various points for Mode B logic
    bool m_ditAtToneStart = false;
    bool m_dahAtToneStart = false;
    bool m_ditAtSpaceStart = false;
    bool m_dahAtSpaceStart = false;

    // Track what element was last sent
    bool m_lastWasDit = true;

    // Safety timeout
    QElapsedTimer m_stateTimer;
    static constexpr int SAFETY_TIMEOUT_MS = 1000;

    // Timing diagnostics
    QElapsedTimer m_timingTimer;
    bool m_timingStarted = false;
};

} // namespace TR4QT

#endif // IAMBICKEYER_H
