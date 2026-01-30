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

#include "ReconnectionManager.h"

namespace TR4QT {

ReconnectionManager::ReconnectionManager(int intervalMs, int maxAttempts, QObject* parent)
    : QObject(parent)
    , m_timer(new QTimer(this))
    , m_intervalMs(intervalMs)
    , m_maxAttempts(maxAttempts)
    , m_attemptCount(0)
{
    m_timer->setSingleShot(true);
    m_timer->setInterval(m_intervalMs);
    connect(m_timer, &QTimer::timeout, this, &ReconnectionManager::onTimeout);
}

void ReconnectionManager::start()
{
    m_timer->start();
}

void ReconnectionManager::stop()
{
    m_timer->stop();
}

void ReconnectionManager::reset()
{
    m_timer->stop();
    m_attemptCount = 0;
}

void ReconnectionManager::recordSuccess()
{
    m_timer->stop();
    m_attemptCount = 0;
}

bool ReconnectionManager::isActive() const
{
    return m_timer->isActive();
}

void ReconnectionManager::onTimeout()
{
    m_attemptCount++;

    // Check if max attempts reached (0 = unlimited)
    if (m_maxAttempts > 0 && m_attemptCount > m_maxAttempts) {
        emit retriesExhausted(m_attemptCount - 1);
        return;
    }

    emit retryRequested(m_attemptCount);

    // Restart timer for next attempt (caller can call stop() if no longer needed)
    m_timer->start();
}

} // namespace TR4QT
