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

#include "KeyerCWSender.h"
#include "../keyers/KeyerController.h"
#include "../logging/LogMacros.h"

namespace TR4QT {

KeyerCWSender::KeyerCWSender(KeyerController* keyer, QObject* parent)
    : CWSender(parent)
    , m_keyer(keyer)
{
    // Forward echo text from keyer as transmission status
    connect(m_keyer, &KeyerController::echoText, this, [this](const QString& text) {
        Q_UNUSED(text);
        // WinKeyer echoes each character as it's sent
    });

    // Track WPM changes from keyer (e.g., WinKeyer speed pot)
    connect(m_keyer, &KeyerController::wpmChanged, this, [this](int wpm) {
        m_wpm = wpm;
        emit wpmChanged(wpm);
    });
}

CWSender::State KeyerCWSender::state() const {
    return m_state;
}

bool KeyerCWSender::isAvailable() const {
    return m_keyer && m_keyer->isConnected() && m_keyer->canSendText();
}

int KeyerCWSender::wpm() const {
    return m_wpm;
}

void KeyerCWSender::setWpm(int wpm) {
    m_wpm = wpm;
    if (m_keyer) {
        m_keyer->setWpm(wpm);
    }
    emit wpmChanged(wpm);
}

void KeyerCWSender::send(const QString& text) {
    if (!isAvailable()) {
        LOG_WARN("KeyerCWSender", "Cannot send: keyer not available");
        emit error("External keyer not connected or doesn't support text sending");
        return;
    }

    m_state = State::Sending;
    emit stateChanged(m_state);
    emit transmissionStarted(text);

    m_keyer->sendText(text);

    // WinKeyer sends asynchronously - we don't have a reliable completion signal
    // Mark as idle after sending (WinKeyer handles the timing internally)
    m_state = State::Idle;
    emit stateChanged(m_state);
}

void KeyerCWSender::stop() {
    if (m_keyer) {
        m_keyer->stopSending();
    }

    m_state = State::Idle;
    emit stateChanged(m_state);
    emit transmissionStopped();
}

} // namespace TR4QT
