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

#include "MorseEncoder.h"
#include "MorseTable.h"
#include "../logging/LogMacros.h"

namespace TR4QT {

static constexpr int PARIS_CONSTANT = 1200;  // dit duration = 1200 / WPM ms
static constexpr int DAH_MULTIPLIER = 3;     // dah = 3 * dit
static constexpr int INTRA_CHAR_DITS = 1;    // gap between elements of same char
static constexpr int INTER_CHAR_DITS = 3;    // gap between characters
static constexpr int INTER_WORD_DITS = 7;    // gap between words (spaces)
static constexpr int MIN_WPM = 5;
static constexpr int MAX_WPM = 60;

MorseEncoder::MorseEncoder(QObject* parent)
    : QObject(parent)
{
    m_timer.setSingleShot(true);
    m_timer.setTimerType(Qt::PreciseTimer);
    connect(&m_timer, &QTimer::timeout, this, &MorseEncoder::onTimerExpired);
}

void MorseEncoder::send(const QString& text, int wpm)
{
    // Stop any in-progress transmission
    if (m_state != State::Idle) {
        stop();
    }

    if (text.isEmpty()) {
        emit finished();
        return;
    }

    m_wpm = qBound(MIN_WPM, wpm, MAX_WPM);
    m_text = text.toUpper();
    m_charIndex = 0;
    m_elementIndex = 0;
    m_currentPattern.clear();

    LOG_DEBUG("MorseEncoder", QString("Sending: \"%1\" at %2 WPM (dit=%3ms)")
              .arg(m_text).arg(m_wpm).arg(ditDurationMs()));

    // Start encoding the first character
    advanceToNextCharacter();
}

void MorseEncoder::stop()
{
    m_timer.stop();

    if (m_state == State::Element) {
        // Key is currently down - release it
        emit keyUp();
    }

    bool wasSending = (m_state != State::Idle);
    m_state = State::Idle;
    m_text.clear();
    m_charIndex = 0;
    m_elementIndex = 0;
    m_currentPattern.clear();

    if (wasSending) {
        LOG_DEBUG("MorseEncoder", "Encoding stopped");
        emit stopped();
    }
}

void MorseEncoder::onTimerExpired()
{
    switch (m_state) {
    case State::Element:
        // Element (dit or dah) finished playing - key up
        emit keyUp();
        advanceToNextElement();
        break;

    case State::IntraCharGap:
        // Gap between elements finished - play next element
        startElement();
        break;

    case State::InterCharGap:
    case State::InterWordGap:
        // Gap between chars/words finished - advance to next character
        advanceToNextCharacter();
        break;

    case State::Idle:
        // Should not happen
        break;
    }
}

void MorseEncoder::advanceToNextElement()
{
    m_elementIndex++;

    if (m_elementIndex < m_currentPattern.length()) {
        // More elements in current character - insert intra-char gap
        startGap(State::IntraCharGap, INTRA_CHAR_DITS);
    } else {
        // Character complete - check for next character
        m_charIndex++;
        if (m_charIndex >= m_text.length()) {
            // All text sent
            m_state = State::Idle;
            LOG_DEBUG("MorseEncoder", "Encoding complete");
            emit finished();
        } else {
            // Check if next char is a space (word gap) or regular char (char gap)
            QChar nextChar = m_text.at(m_charIndex);
            if (MorseTable::isWordSpace(nextChar)) {
                // Word gap = 7 dits total. We already have 1 dit from the element end,
                // so we need 7 - INTRA_CHAR_DITS = 6 more dits of silence.
                // Actually, inter-char gap is normally 3 dits after the last element's
                // trailing 1-dit intra-element gap. But standard timing is:
                // element + 1 dit gap (intra-char) between elements.
                // After last element of a char, we add inter-char gap (3 dits) or inter-word (7 dits).
                // The 1-dit gap is only between elements WITHIN a character.
                // Between characters, the full inter-char gap (3 dits) applies.
                // Between words, the full inter-word gap (7 dits) applies.
                startGap(State::InterWordGap, INTER_WORD_DITS);
                m_charIndex++;  // Skip the space character itself
            } else {
                startGap(State::InterCharGap, INTER_CHAR_DITS);
            }
        }
    }
}

void MorseEncoder::advanceToNextCharacter()
{
    // Skip any leading spaces (already handled as word gaps)
    while (m_charIndex < m_text.length() && MorseTable::isWordSpace(m_text.at(m_charIndex))) {
        m_charIndex++;
    }

    if (m_charIndex >= m_text.length()) {
        // All text sent
        m_state = State::Idle;
        LOG_DEBUG("MorseEncoder", "Encoding complete");
        emit finished();
        return;
    }

    QChar ch = m_text.at(m_charIndex);
    m_currentPattern = MorseTable::pattern(ch);

    if (m_currentPattern.isEmpty()) {
        // Unknown character - skip it
        LOG_DEBUG("MorseEncoder", QString("Skipping unknown character: '%1'").arg(ch));
        m_charIndex++;
        advanceToNextCharacter();
        return;
    }

    m_elementIndex = 0;
    startElement();
}

void MorseEncoder::startElement()
{
    if (m_elementIndex >= m_currentPattern.length()) {
        // Safety: shouldn't happen
        advanceToNextElement();
        return;
    }

    QChar element = m_currentPattern.at(m_elementIndex);
    int durationMs = (element == '-') ? ditDurationMs() * DAH_MULTIPLIER : ditDurationMs();

    m_state = State::Element;
    emit keyDown();
    m_timer.start(durationMs);
}

void MorseEncoder::startGap(State gapType, int ditMultiplier)
{
    m_state = gapType;
    m_timer.start(ditDurationMs() * ditMultiplier);
}

int MorseEncoder::ditDurationMs() const
{
    return PARIS_CONSTANT / m_wpm;
}

} // namespace TR4QT
