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

#ifndef MORSEENCODER_H
#define MORSEENCODER_H

#include <QObject>
#include <QTimer>
#include <QString>

namespace TR4QT {

/**
 * Converts text into timed key-down/key-up events for Morse code transmission.
 *
 * QTimer-driven state machine that walks through each character's dit/dah pattern,
 * emitting keyDown()/keyUp() signals with correct timing for the configured WPM.
 *
 * State machine:
 *   Idle -> Element (key down) -> IntraCharGap (key up, 1 dit) ->
 *     next element or InterCharGap (3 dits) or InterWordGap (7 dits) -> Idle
 *
 * Timing (standard PARIS method):
 *   dit duration  = 1200 / WPM ms
 *   dah duration  = 3 * dit ms
 *   intra-char gap = 1 dit (between elements of same character)
 *   inter-char gap = 3 dits (between characters)
 *   inter-word gap = 7 dits (between words / spaces)
 *
 * Usage:
 *   MorseEncoder encoder;
 *   connect(&encoder, &MorseEncoder::keyDown, ...);
 *   connect(&encoder, &MorseEncoder::keyUp, ...);
 *   encoder.send("CQ TEST NY4I", 25);
 */
class MorseEncoder : public QObject {
    Q_OBJECT

public:
    enum class State {
        Idle,
        Element,         // Key is down (dit or dah playing)
        IntraCharGap,    // Key up, gap between elements of same character
        InterCharGap,    // Key up, gap between characters
        InterWordGap     // Key up, gap between words (space)
    };
    Q_ENUM(State)

    explicit MorseEncoder(QObject* parent = nullptr);
    ~MorseEncoder() override = default;

    State state() const { return m_state; }
    bool isSending() const { return m_state != State::Idle; }

public slots:
    /**
     * Start encoding text into timed key events.
     * @param text Text to send as Morse code
     * @param wpm Speed in words per minute
     */
    void send(const QString& text, int wpm);

    /**
     * Stop encoding immediately. Emits keyUp() if key is currently down.
     */
    void stop();

signals:
    /** Emitted when the key should go down (start tone/transmit). */
    void keyDown();

    /** Emitted when the key should go up (stop tone/transmit). */
    void keyUp();

    /** Emitted when all text has been sent. */
    void finished();

    /** Emitted when encoding is stopped before completion. */
    void stopped();

private slots:
    void onTimerExpired();

private:
    void advanceToNextElement();
    void advanceToNextCharacter();
    void startElement();
    void startGap(State gapType, int ditMultiplier);
    int ditDurationMs() const;

    QTimer m_timer;

    State m_state = State::Idle;

    // Text being encoded
    QString m_text;
    int m_charIndex = 0;      // Current character position in m_text
    int m_elementIndex = 0;   // Current element position in current character's pattern
    QString m_currentPattern; // Dit/dah pattern for current character

    // Speed
    int m_wpm = 25;
};

} // namespace TR4QT

#endif // MORSEENCODER_H
