#ifndef RECONNECTIONMANAGER_H
#define RECONNECTIONMANAGER_H

#include <QObject>
#include <QTimer>

namespace TR4QT {

/**
 * ReconnectionManager
 *
 * Encapsulates retry-with-timer logic used by RadioManager and DXClusterWindow.
 * Manages a single-shot QTimer that fires retryRequested(attemptNumber) until
 * maxAttempts is reached (0 = unlimited retries).
 *
 * Usage:
 *   auto* reconnect = new ReconnectionManager(10000, 0, this);  // 10s, unlimited
 *   connect(reconnect, &ReconnectionManager::retryRequested, this, &MyClass::onRetry);
 *   reconnect->start();   // begins retry cycle
 *   reconnect->recordSuccess();  // resets on success
 */
class ReconnectionManager : public QObject {
    Q_OBJECT

public:
    /**
     * Construct a ReconnectionManager
     * @param intervalMs Milliseconds between retry attempts
     * @param maxAttempts Maximum number of retries (0 = unlimited)
     * @param parent Parent QObject
     */
    explicit ReconnectionManager(int intervalMs, int maxAttempts = 0, QObject* parent = nullptr);

    /**
     * Start the reconnection timer.
     * The first retryRequested signal fires after intervalMs.
     */
    void start();

    /**
     * Stop the reconnection timer without resetting the attempt counter.
     */
    void stop();

    /**
     * Stop the timer and reset the attempt counter to zero.
     */
    void reset();

    /**
     * Record a successful connection.
     * Stops the timer and resets the attempt counter.
     */
    void recordSuccess();

    /**
     * @return true if the timer is currently running
     */
    bool isActive() const;

    /**
     * @return Number of retry attempts made since last reset
     */
    int attemptCount() const { return m_attemptCount; }

    /**
     * @return Maximum attempts (0 = unlimited)
     */
    int maxAttempts() const { return m_maxAttempts; }

signals:
    /**
     * Emitted when a retry should be attempted
     * @param attempt The attempt number (1-based)
     */
    void retryRequested(int attempt);

    /**
     * Emitted when maxAttempts is reached (only if maxAttempts > 0)
     * @param totalAttempts Total attempts made
     */
    void retriesExhausted(int totalAttempts);

private slots:
    void onTimeout();

private:
    QTimer* m_timer;
    int m_intervalMs;
    int m_maxAttempts;
    int m_attemptCount;
};

} // namespace TR4QT

#endif // RECONNECTIONMANAGER_H
