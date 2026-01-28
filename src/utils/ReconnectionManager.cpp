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
