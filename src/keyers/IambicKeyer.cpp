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

#include "IambicKeyer.h"
#include "../logging/LogMacros.h"

namespace TR4QT {

IambicKeyer::IambicKeyer(QObject* parent)
    : QObject(parent)
{
    m_elementTimer.setSingleShot(true);
    m_elementTimer.setTimerType(Qt::PreciseTimer);
    m_spaceTimer.setSingleShot(true);
    m_spaceTimer.setTimerType(Qt::PreciseTimer);

    connect(&m_elementTimer, &QTimer::timeout, this, &IambicKeyer::onElementTimerExpired);
    connect(&m_spaceTimer, &QTimer::timeout, this, &IambicKeyer::onSpaceTimerExpired);
}

void IambicKeyer::setWpm(int wpm) {
    if (wpm <= 0) wpm = 25;
    m_wpm = wpm;
    m_ditLengthMs = 1200 / wpm;
    LOG_DEBUG("IambicKeyer", QString("WPM=%1, dit=%2ms").arg(wpm).arg(m_ditLengthMs));
}

void IambicKeyer::updatePaddleState(bool dit, bool dah) {
    // High-resolution timing diagnostics
    if (!m_timingStarted) {
        m_timingTimer.start();
        m_timingStarted = true;
    }
    const qint64 elapsedUs = m_timingTimer.nsecsElapsed() / 1000;
    LOG_INFO("TIMING:KEYER", QString("updatePaddle dit=%1 dah=%2 state=%3 elapsed=%4us")
             .arg(dit).arg(dah).arg(static_cast<int>(m_state)).arg(elapsedUs));

    LOG_TRACE("IambicKeyer", QString("Paddle: dit=%1 dah=%2 state=%3")
              .arg(dit).arg(dah).arg(static_cast<int>(m_state)));

    // Safety check: force reset if state machine stuck
    if (m_state != KeyerState::Idle && m_stateTimer.elapsed() > SAFETY_TIMEOUT_MS) {
        LOG_WARN("IambicKeyer", "State timeout detected, forcing reset");
        stop();
    }

    m_currentDit = dit;
    m_currentDah = dah;

    if (m_state == KeyerState::Idle && (dit || dah)) {
        // Start sending
        startNextElement();
    } else if (m_state == KeyerState::TonePlaying) {
        // Set alternation latch for opposite paddle pressed during tone
        if (m_lastWasDit && dah && !m_dahAtToneStart && !m_dahLatched) {
            m_dahLatched = true;
            LOG_TRACE("IambicKeyer", "DAH latch set (opposite during dit)");
        }
        if (!m_lastWasDit && dit && !m_ditAtToneStart && !m_ditLatched) {
            m_ditLatched = true;
            LOG_TRACE("IambicKeyer", "DIT latch set (opposite during dah)");
        }
    } else if (m_state == KeyerState::InterElementSpace) {
        // Latch paddles newly pressed during silence
        if (dit && !m_ditAtSpaceStart && !m_ditLatched) {
            m_ditLatched = true;
            LOG_TRACE("IambicKeyer", "DIT latch set (during silence)");
        }
        if (dah && !m_dahAtSpaceStart && !m_dahLatched) {
            m_dahLatched = true;
            LOG_TRACE("IambicKeyer", "DAH latch set (during silence)");
        }
    }
}

void IambicKeyer::stop() {
    LOG_DEBUG("IambicKeyer", "Stop called");

    m_elementTimer.stop();
    m_spaceTimer.stop();

    if (m_state == KeyerState::TonePlaying) {
        emit keyUp();
    }

    m_state = KeyerState::Idle;
    m_ditLatched = false;
    m_dahLatched = false;
    m_ditAtToneStart = false;
    m_dahAtToneStart = false;
    m_ditAtSpaceStart = false;
    m_dahAtSpaceStart = false;
}

void IambicKeyer::onElementTimerExpired() {
    // Tone finished → key up, enter inter-element space
    {
        const qint64 elapsedUs = m_timingTimer.nsecsElapsed() / 1000;
        LOG_INFO("TIMING:KEYER", QString("emit keyUp (element done) elapsed=%1us").arg(elapsedUs));
    }
    LOG_TRACE("IambicKeyer", "Element timer expired → key up");

    emit keyUp();

    m_state = KeyerState::InterElementSpace;
    m_stateTimer.restart();

    // Capture paddle states at start of silence
    m_ditAtSpaceStart = m_currentDit;
    m_dahAtSpaceStart = m_currentDah;

    // Start inter-element space timer (1 dit length)
    m_spaceTimer.start(m_ditLengthMs);
}

void IambicKeyer::onSpaceTimerExpired() {
    // Inter-element space finished → decide next element
    {
        const qint64 elapsedUs = m_timingTimer.nsecsElapsed() / 1000;
        LOG_INFO("TIMING:KEYER", QString("spaceExpired elapsed=%1us").arg(elapsedUs));
    }
    LOG_TRACE("IambicKeyer", "Space timer expired → deciding next");

    int nextDuration = determineNextToneDuration();

    if (nextDuration > 0) {
        startTone(nextDuration);
    } else {
        // Nothing to send, go idle
        LOG_TRACE("IambicKeyer", "No next element, going idle");
        m_state = KeyerState::Idle;
        m_ditLatched = false;
        m_dahLatched = false;
        m_ditAtToneStart = false;
        m_dahAtToneStart = false;
        m_ditAtSpaceStart = false;
        m_dahAtSpaceStart = false;
    }
}

void IambicKeyer::startNextElement() {
    int duration = determineNextToneDuration();

    if (duration > 0) {
        startTone(duration);
    }
}

int IambicKeyer::determineNextToneDuration() const {
    bool sendDit = false;
    bool sendDah = false;

    if (m_lastWasDit || m_state == KeyerState::Idle) {
        // After dit (or starting from idle):
        // Priority 1: Alternation — opposite paddle latched or pressed
        if (m_dahLatched || m_currentDah) {
            sendDah = true;
        }
        // Priority 2: Repetition — same paddle latched or pressed
        else if (m_ditLatched || m_currentDit) {
            sendDit = true;
        }
        // Priority 3 (Mode B): Squeeze — both held at tone start, both now released
        else if (m_modeB && m_dahAtToneStart && !m_currentDah && !m_currentDit) {
            sendDah = true;
        }
    } else {
        // After dah:
        // Priority 1: Alternation
        if (m_ditLatched || m_currentDit) {
            sendDit = true;
        }
        // Priority 2: Repetition
        else if (m_dahLatched || m_currentDah) {
            sendDah = true;
        }
        // Priority 3 (Mode B): Squeeze
        else if (m_modeB && m_ditAtToneStart && !m_currentDit && !m_currentDah) {
            sendDit = true;
        }
    }

    // Special case: both paddles from idle → dit first
    if (m_state == KeyerState::Idle && m_currentDit && m_currentDah) {
        sendDit = true;
        sendDah = false;
    }

    if (sendDit) return m_ditLengthMs;
    if (sendDah) return m_ditLengthMs * 3;
    return 0;  // Nothing to send
}

void IambicKeyer::startTone(int durationMs) {
    bool isDit = (durationMs == m_ditLengthMs);

    LOG_TRACE("IambicKeyer", QString("Starting %1 (%2ms)")
              .arg(isDit ? "dit" : "dah").arg(durationMs));

    // Capture paddle states at tone start (critical for Mode B)
    m_ditAtToneStart = m_currentDit;
    m_dahAtToneStart = m_currentDah;

    // Clear latches
    m_ditLatched = false;
    m_dahLatched = false;

    // Track element type
    m_lastWasDit = isDit;

    // Key down
    m_state = KeyerState::TonePlaying;
    m_stateTimer.restart();
    {
        const qint64 elapsedUs = m_timingTimer.nsecsElapsed() / 1000;
        LOG_INFO("TIMING:KEYER", QString("emit keyDown (%1, %2ms) elapsed=%3us")
                 .arg(isDit ? "dit" : "dah").arg(durationMs).arg(elapsedUs));
    }
    emit keyDown();

    // Start element timer
    m_elementTimer.start(durationMs);
}

} // namespace TR4QT
