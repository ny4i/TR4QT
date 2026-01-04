#include "RadioController.h"
#include "RadioPreflightHelper.h"
#include "../logging/LogMacros.h"
#include <QMutexLocker>

namespace TR4QT {

RadioController::RadioController(QObject* parent)
    : QObject(parent)
    , m_radio(nullptr)
    , m_connected(false)
{
    // Create HamlibRadio (will be moved to worker thread)
    m_radio = new HamlibRadio();

    // Move to worker thread (QTimer moves with it as a child object)
    m_radio->moveToThread(&m_workerThread);

    // Connect signals from radio (worker thread) to our signals (for UI)
    // Qt automatically uses QueuedConnection for cross-thread signals
    connect(m_radio, &RadioInterface::connectionStatusChanged,
            this, [this](bool connected) {
                LOG_DEBUG("RadioController", QString("Received connectionStatusChanged: %1").arg(connected ? "true" : "false"));

                QMutexLocker locker(&m_stateMutex);
                m_connected = connected;

                if (!connected) {
                    m_radioModel.clear();
                }

                emit connectionStatusChanged(connected);
            });

    connect(m_radio, &RadioInterface::stateUpdated,
            this, [this](const RadioState& state) {
                QMutexLocker locker(&m_stateMutex);
                m_lastState = state;

                // Update cached radio model from state
                if (!state.radioModel.isEmpty() && m_radioModel != state.radioModel) {
                    m_radioModel = state.radioModel;
                    emit radioModelChanged(m_radioModel);
                }

                emit stateUpdated(state);
            });

    connect(m_radio, &RadioInterface::frequencyChanged,
            this, &RadioController::frequencyChanged);
    connect(m_radio, &RadioInterface::modeChanged,
            this, &RadioController::modeChanged);
    connect(m_radio, &RadioInterface::pttChanged,
            this, &RadioController::pttChanged);
    connect(m_radio, &RadioInterface::errorOccurred,
            this, &RadioController::errorOccurred);

    // Clean up radio when thread finishes
    connect(&m_workerThread, &QThread::finished,
            m_radio, &QObject::deleteLater);

    // Start worker thread
    m_workerThread.start();
}

RadioController::~RadioController() {
    // Signal shutdown to worker thread (prevents new connection attempts)
    m_shutdownRequested.store(true);

    LOG_DEBUG("RadioController", "Destructor: Stopping worker thread");

    // CRITICAL FIX: Close connection FIRST to unblock any pending I/O
    // This is essential for TCP connections which block on read()
    // Use BlockingQueuedConnection to ensure disconnect completes before we terminate
    bool disconnected = QMetaObject::invokeMethod(m_radio, [this]() {
        static_cast<RadioInterface*>(m_radio)->disconnect();
        LOG_DEBUG("RadioController", "Hamlib connection closed in destructor");
    }, Qt::BlockingQueuedConnection);

    if (!disconnected) {
        LOG_WARN("RadioController", "Failed to invoke disconnect on worker thread");
    }

    // Give thread a moment to exit naturally after disconnect
    if (m_workerThread.wait(500)) {
        LOG_DEBUG("RadioController", "Worker thread exited cleanly after disconnect");
        return;  // Clean exit!
    }

    // Thread didn't exit naturally - try quit() signal
    m_workerThread.quit();
    if (m_workerThread.wait(500)) {
        LOG_DEBUG("RadioController", "Worker thread exited after quit()");
        return;
    }

    // Still running - resort to terminate() as last resort
    LOG_WARN("RadioController", "Worker thread didn't exit cleanly, using terminate()");
    m_workerThread.terminate();

    // CRITICAL: Must wait for termination to complete before QThread destructor runs
    for (int attempt = 0; attempt < 3; attempt++) {
        if (m_workerThread.wait(1000)) {
            LOG_DEBUG("RadioController", QString("Worker thread terminated (attempt %1)").arg(attempt + 1));
            return;
        }

        LOG_WARN("RadioController", QString("Worker thread still alive after terminate, attempt %1/3").arg(attempt + 1));
        m_workerThread.terminate();  // Try again
    }

    // If we get here, thread won't die - this is VERY bad
    LOG_ERROR("RadioController", "CRITICAL: Worker thread refused to die after all attempts!");
    LOG_ERROR("RadioController", "Proceeding anyway - QThread destructor may hang...");
}

bool RadioController::isConnected() const {
    QMutexLocker locker(&m_stateMutex);
    return m_connected;
}

RadioState RadioController::getCurrentState() const {
    QMutexLocker locker(&m_stateMutex);
    return m_lastState;
}

QString RadioController::getRadioModel() const {
    QMutexLocker locker(&m_stateMutex);
    return m_radioModel;
}

QList<ModeType> RadioController::getSupportedModes() const {
    // Call into worker thread to get supported modes
    QList<ModeType> modes;
    if (m_radio) {
        // Use blocking queued connection to safely call across threads
        QMetaObject::invokeMethod(m_radio, "getSupportedModes",
                                 Qt::BlockingQueuedConnection,
                                 Q_RETURN_ARG(QList<ModeType>, modes));
    }
    return modes;
}

bool RadioController::supportsCWSending() const {
    // Call into worker thread to check CW capability
    bool supported = false;
    if (m_radio) {
        // Use blocking queued connection to safely call across threads
        QMetaObject::invokeMethod(m_radio, "supportsCWSending",
                                 Qt::BlockingQueuedConnection,
                                 Q_RETURN_ARG(bool, supported));
    }
    return supported;
}

void RadioController::connectToRadio(const RadioConfig& config) {
    LOG_DEBUG("RadioController", QString("connectToRadio called with model %1 port %2").arg(config.hamlibModelId).arg(config.port));

    // CRITICAL: Reset shutdown flag before new connection attempt
    // This flag is set during disconnectFromRadio() and must be cleared for new connections
    m_shutdownRequested.store(false);

    // Pre-flight check: If this is a network connection (host:port format),
    // verify the radio is reachable BEFORE attempting Hamlib connection.
    // This prevents worker thread from blocking in connect() syscall for 75-120 seconds.
    if (config.port.contains(':')) {
        // Parse host:port from config
        QStringList parts = config.port.split(':');
        if (parts.size() == 2) {
            QString host = parts[0];
            bool ok = false;
            quint16 port = parts[1].toUShort(&ok);

            if (ok && !host.isEmpty()) {
                // Run radio-specific pre-flight check (2000ms timeout)
                // This performs radio-specific verification (e.g., K4 ID command)
                // or falls back to general TCP connectivity test
                // Note: Increased from 500ms to 2000ms to prevent false negatives
                // on slower networks or radios that take longer to respond
                if (!RadioPreflightHelper::radioSpecificPreflight(config.hamlibModelId, host, port, 2000)) {
                    // Radio not reachable or verification failed - abort connection attempt
                    QString errorMsg = QString("Radio not reachable at %1:%2 (pre-flight check failed)").arg(host).arg(port);
                    LOG_WARN("RadioController", errorMsg);

                    // Emit error signal so UI knows connection failed
                    emit errorOccurred(errorMsg);

                    // DO NOT proceed to Hamlib connection
                    return;
                }
                // Pre-flight check passed - proceed with Hamlib connection
                LOG_DEBUG("RadioController", QString("Pre-flight check passed for %1:%2 (model %3)")
                    .arg(host).arg(port).arg(config.hamlibModelId));
            }
        }
    }

    // Invoke connect method in worker thread using lambda
    bool queued = QMetaObject::invokeMethod(m_radio, [this, config]() {
        LOG_DEBUG("RadioController", "Lambda executing in worker thread");

        // Check if shutdown was requested (prevents blocking on rig_open during shutdown)
        if (m_shutdownRequested.load()) {
            LOG_DEBUG("RadioController", "Shutdown requested, aborting connection attempt");
            return;
        }

        // Call the RadioInterface::connect method (not QObject::connect)
        bool success = static_cast<RadioInterface*>(m_radio)->connect(config);
        LOG_DEBUG("RadioController", QString("connect() returned %1").arg(success ? "true" : "false"));
        if (!success) {
            LOG_WARN("RadioController", "HamlibRadio::connect returned false");
        }
    }, Qt::QueuedConnection);

    LOG_DEBUG("RadioController", QString("connectToRadio: invokeMethod returned %1").arg(queued ? "true" : "false"));
}

void RadioController::disconnectFromRadio() {
    // Set shutdown flag to abort any pending connection attempts
    m_shutdownRequested.store(true);

    // Try to disconnect with timeout (don't block indefinitely if worker thread is stuck)
    // Use Qt::QueuedConnection instead of BlockingQueuedConnection to avoid deadlock
    // when worker thread is blocked in rig_open() during shutdown
    QMetaObject::invokeMethod(m_radio, [this]() {
        // Only disconnect if not already in the middle of a blocking connect()
        // The destructor will handle forceful shutdown if needed
        static_cast<RadioInterface*>(m_radio)->disconnect();
    }, Qt::QueuedConnection);

    // Give worker thread a moment to process disconnect (non-blocking)
    // If it's stuck in rig_open(), the destructor will terminate it after 3 seconds
    LOG_DEBUG("RadioController", "Disconnect queued (non-blocking), destructor will ensure cleanup");
}

void RadioController::setFrequency(freq_t freq, VFO vfo) {
    QMetaObject::invokeMethod(m_radio, [this, freq, vfo]() {
        m_radio->setFrequency(freq, vfo);
    }, Qt::QueuedConnection);
}

void RadioController::setMode(ModeType mode, VFO vfo) {
    QMetaObject::invokeMethod(m_radio, [this, mode, vfo]() {
        m_radio->setMode(mode, vfo);
    }, Qt::QueuedConnection);
}

void RadioController::setPTT(bool transmit) {
    QMetaObject::invokeMethod(m_radio, [this, transmit]() {
        m_radio->setPTT(transmit);
    }, Qt::QueuedConnection);
}

void RadioController::sendCW(const QString& text) {
    QMetaObject::invokeMethod(m_radio, [this, text]() {
        m_radio->sendCW(text);
    }, Qt::QueuedConnection);
}

void RadioController::setCWSpeed(int wpm) {
    QMetaObject::invokeMethod(m_radio, [this, wpm]() {
        m_radio->setCWSpeed(wpm);
    }, Qt::QueuedConnection);
}

int RadioController::getCWSpeed() const {
    // Call synchronously since we need the return value
    int wpm = 0;
    QMetaObject::invokeMethod(m_radio, [this, &wpm]() {
        wpm = m_radio->getCWSpeed();
    }, Qt::BlockingQueuedConnection);
    return wpm;
}

void RadioController::stopCW() {
    QMetaObject::invokeMethod(m_radio, [this]() {
        m_radio->stopCW();
    }, Qt::QueuedConnection);
}

bool RadioController::waitForMorseComplete() {
    bool result = false;
    QMetaObject::invokeMethod(m_radio, [this, &result]() {
        result = m_radio->waitForMorseComplete();
    }, Qt::BlockingQueuedConnection);
    return result;
}

void RadioController::enableRIT(bool enable, VFO vfo) {
    QMetaObject::invokeMethod(m_radio, [this, enable, vfo]() {
        m_radio->enableRIT(enable, vfo);
    }, Qt::QueuedConnection);
}

void RadioController::enableXIT(bool enable, VFO vfo) {
    QMetaObject::invokeMethod(m_radio, [this, enable, vfo]() {
        m_radio->enableXIT(enable, vfo);
    }, Qt::QueuedConnection);
}

void RadioController::setSplit(bool enable, VFO txVfo) {
    QMetaObject::invokeMethod(m_radio, [this, enable, txVfo]() {
        m_radio->setSplit(enable, txVfo);
    }, Qt::QueuedConnection);
}

} // namespace TR4QT
