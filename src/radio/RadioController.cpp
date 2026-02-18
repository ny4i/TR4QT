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

#include "RadioController.h"
#include "RadioFactory.h"
#include "RadioPreflightHelper.h"
#include "../logging/LogMacros.h"
#include <QMutexLocker>

namespace TR4QT {

RadioController::RadioController(QObject* parent)
    : QObject(parent)
    , m_radio(nullptr)
    , m_connected(false)
{
    // Create radio using RadioFactory (defaults to Hamlib for backward compatibility)
    RadioConfig defaultConfig;
    defaultConfig.radioType = static_cast<int>(RadioFactory::RadioType::HAMLIB);
    m_radio = RadioFactory::createRadio(RadioFactory::RadioType::HAMLIB, defaultConfig);

    LOG_DEBUG("RadioController", QString("Created radio: %1 (main thread: %2)")
              .arg(m_radio ? "valid" : "NULL")
              .arg(reinterpret_cast<quintptr>(QThread::currentThread())));

    // Cache max power BEFORE moveToThread (safe - just reads a constant, no I/O)
    m_cachedMaxPower = m_radio->maxPowerWatts();

    // Move to worker thread (QTimer moves with it as a child object)
    m_radio->moveToThread(&m_workerThread);

    LOG_DEBUG("RadioController", QString("Moved radio to worker thread: %1")
              .arg(reinterpret_cast<quintptr>(&m_workerThread)));

    // Connect radio signals
    connectRadioSignals();

    // Connect command signals (internal signals → m_radio slots)
    connectCommandSignals();

    // Start worker thread
    m_workerThread.start();

    LOG_DEBUG("RadioController", QString("Worker thread started: %1 (running: %2)")
              .arg(reinterpret_cast<quintptr>(&m_workerThread))
              .arg(m_workerThread.isRunning() ? "YES" : "NO"));
}

void RadioController::connectRadioSignals() {
    // Connect signals from radio (worker thread) to our signals (for UI)
    // Qt automatically uses QueuedConnection for cross-thread signals
    connect(m_radio, &RadioInterface::connectionStatusChanged,
            this, [this](bool connected) {
                LOG_DEBUG("RadioController", QString("Received connectionStatusChanged: %1").arg(connected ? "true" : "false"));

                {
                    QMutexLocker locker(&m_stateMutex);
                    m_connected = connected;

                    if (!connected) {
                        m_radioModel.clear();
                    }
                }
                // CRITICAL: Emit AFTER releasing mutex to prevent deadlock
                // Downstream slots may call isConnected()/getCurrentState() which lock m_stateMutex
                emit connectionStatusChanged(connected);
            });

    connect(m_radio, &RadioInterface::stateUpdated,
            this, [this](const RadioState& state) {
                QString newModel;
                {
                    QMutexLocker locker(&m_stateMutex);
                    m_lastState = state;

                    // Update cached radio model from state
                    if (!state.radioModel.isEmpty() && m_radioModel != state.radioModel) {
                        m_radioModel = state.radioModel;
                        newModel = m_radioModel;
                    }
                }
                // CRITICAL: Emit AFTER releasing mutex to prevent deadlock
                // Downstream slots may call isConnected()/getCurrentState() which lock m_stateMutex
                if (!newModel.isEmpty()) {
                    emit radioModelChanged(newModel);
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
    connect(m_radio, &RadioInterface::statusMessageReceived,
            this, &RadioController::statusMessageReceived);

    // Clean up radio when thread finishes
    connect(&m_workerThread, &QThread::finished,
            m_radio, &QObject::deleteLater);
}

void RadioController::connectCommandSignals() {
    // Connect our internal command signals to m_radio's slots
    // CRITICAL: Use old-style SLOT/SIGNAL macros for connecting to pure virtual slots
    // Qt's new-style connect syntax may fail with abstract base class pointers
    int failed = 0;

    // DEBUG: Connect test signal first to verify mechanism works
    if (!connect(this, SIGNAL(debugTestSignal(int)),
                 m_radio, SLOT(debugTestSlot(int)), Qt::QueuedConnection)) {
        LOG_ERROR("RadioController", "FAILED to connect debugTestSignal");
        failed++;
    } else {
        LOG_ERROR("RadioController", "***** DEBUG: Test signal connected successfully *****");
    }

    // CRITICAL: Old-style SLOT/SIGNAL macros may need the UNDERLYING type for typedefs
    // freq_t is typedef'd as double - try using the actual type
    if (!connect(this, SIGNAL(requestSetFrequency(double,VFO)),
                 m_radio, SLOT(setFrequency(double,VFO)), Qt::QueuedConnection)) {
        LOG_ERROR("RadioController", "FAILED to connect requestSetFrequency");
        failed++;
    }
    if (!connect(this, SIGNAL(requestSetMode(ModeType,VFO)),
                 m_radio, SLOT(setMode(ModeType,VFO)), Qt::QueuedConnection)) {
        LOG_ERROR("RadioController", "FAILED to connect requestSetMode");
        failed++;
    }
    if (!connect(this, SIGNAL(requestSetPTT(bool)),
                 m_radio, SLOT(setPTT(bool)), Qt::QueuedConnection)) {
        LOG_ERROR("RadioController", "FAILED to connect requestSetPTT");
        failed++;
    }
    if (!connect(this, SIGNAL(requestSendCW(QString)),
                 m_radio, SLOT(sendCW(QString)), Qt::QueuedConnection)) {
        LOG_ERROR("RadioController", "FAILED to connect requestSendCW");
        failed++;
    }
    if (!connect(this, SIGNAL(requestSetCWSpeed(int)),
                 m_radio, SLOT(setCWSpeed(int)), Qt::QueuedConnection)) {
        LOG_ERROR("RadioController", "FAILED to connect requestSetCWSpeed");
        failed++;
    }
    if (!connect(this, SIGNAL(requestStopCW()),
                 m_radio, SLOT(stopCW()), Qt::QueuedConnection)) {
        LOG_ERROR("RadioController", "FAILED to connect requestStopCW");
        failed++;
    }
    if (!connect(this, SIGNAL(requestEnableRIT(bool,VFO)),
                 m_radio, SLOT(enableRIT(bool,VFO)), Qt::QueuedConnection)) {
        LOG_ERROR("RadioController", "FAILED to connect requestEnableRIT");
        failed++;
    }
    if (!connect(this, SIGNAL(requestEnableXIT(bool,VFO)),
                 m_radio, SLOT(enableXIT(bool,VFO)), Qt::QueuedConnection)) {
        LOG_ERROR("RadioController", "FAILED to connect requestEnableXIT");
        failed++;
    }
    if (!connect(this, SIGNAL(requestSetSplit(bool,VFO)),
                 m_radio, SLOT(setSplit(bool,VFO)), Qt::QueuedConnection)) {
        LOG_ERROR("RadioController", "FAILED to connect requestSetSplit");
        failed++;
    }

    if (failed == 0) {
        LOG_DEBUG("RadioController", "Command signals connected to m_radio slots - ALL 10 SUCCESS (including debug test)");
    } else {
        LOG_ERROR("RadioController", QString("FAILED to connect %1 command signals!").arg(failed));
    }

    // DEBUG: Test both signal/slot and invokeMethod paths
    LOG_ERROR("RadioController", "***** DEBUG: Testing signal/slot path by emitting debugTestSignal(42) *****");
    emit debugTestSignal(42);

    LOG_ERROR("RadioController", "***** DEBUG: Testing invokeMethod path by calling debugTestSlot(99) *****");
    QMetaObject::invokeMethod(m_radio, "debugTestSlot", Qt::QueuedConnection, Q_ARG(int, 99));
}

void RadioController::recreateRadio(int radioType, const RadioConfig& config) {
    LOG_INFO("RadioController", QString("Recreating radio with type %1").arg(radioType));

    // Disconnect old radio
    if (m_radio) {
        // CRITICAL: Disconnect signals FIRST before calling disconnect()
        // Otherwise, disconnect() will emit connectionStatusChanged(false) which is still connected
        // This would cause RadioManager to think the radio disconnected when we're actually
        // just switching radio types (e.g., from Hamlib to Icom Direct)
        QObject::disconnect(m_radio, nullptr, this, nullptr);

        // Now disconnect in worker thread (won't emit signals since they're disconnected)
        QMetaObject::invokeMethod(m_radio, [this]() {
            static_cast<RadioInterface*>(m_radio)->disconnect();
        }, Qt::BlockingQueuedConnection);

        // Delete old radio (will be deleted when thread processes deleteLater)
        m_radio->deleteLater();
        m_radio = nullptr;
    }

    // Clear cached state from old radio
    // This ensures MainWindow doesn't display stale frequency/mode from previous radio
    {
        QMutexLocker locker(&m_stateMutex);
        m_lastState = RadioState();  // Reset to default state
        m_connected = false;
        m_radioModel.clear();
        LOG_DEBUG("RadioController", "Cleared cached radio state");
    }

    // Determine actual radio type to create
    RadioFactory::RadioType factoryType;
    if (radioType == -1) {
        // Auto: Use recommendedTypeForModel
        factoryType = RadioFactory::recommendedTypeForModel(config.hamlibModelId);
        LOG_INFO("RadioController", QString("Auto mode selected recommended type: %1")
                 .arg(RadioFactory::radioTypeName(factoryType)));
    } else {
        factoryType = static_cast<RadioFactory::RadioType>(radioType);
    }

    // Create new radio with RadioFactory
    m_radio = RadioFactory::createRadio(factoryType, config);

    // Cache max power BEFORE moveToThread (safe - just reads a constant, no I/O)
    m_cachedMaxPower = m_radio->maxPowerWatts();

    // Move to worker thread
    m_radio->moveToThread(&m_workerThread);

    // Reconnect signals
    connectRadioSignals();
    connectCommandSignals();  // Reconnect command signals to new radio

    LOG_INFO("RadioController", "Radio recreation complete");
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

int RadioController::maxPowerWatts() const {
    // Return cached value - no mutex needed, only written on main thread
    // (in constructor and recreateRadio, both before moveToThread)
    return m_cachedMaxPower;
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
    LOG_DEBUG("RadioController", QString("connectToRadio called with model %1 port %2 radioType %3")
              .arg(config.hamlibModelId).arg(config.port).arg(config.radioType));

    // CRITICAL: Reset shutdown flag before new connection attempt
    // This flag is set during disconnectFromRadio() and must be cleared for new connections
    m_shutdownRequested.store(false);

    // ALWAYS recreate radio via RadioFactory
    // This is the correct use of the factory pattern - let it handle polymorphism
    // Radio connection/disconnection is infrequent, so recreation cost is negligible
    LOG_INFO("RadioController", QString("Creating radio via factory: type=%1 model=%2 port=%3")
             .arg(config.radioType).arg(config.hamlibModelId).arg(config.port));
    recreateRadio(config.radioType, config);

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
    double freqKHz = freq / 1000.0;
    LOG_DEBUG("RadioController", QString("setFrequency called: freq=%1 kHz, VFO %2")
              .arg(freqKHz, 0, 'f', 1)
              .arg(vfo == VFO::VFO_A ? "A" : "B"));

    // Use invokeMethod instead of signal/slot - this DOES work (proven by connect() method)
    bool queued = QMetaObject::invokeMethod(m_radio, "setFrequency", Qt::QueuedConnection,
                                            Q_ARG(freq_t, freq),
                                            Q_ARG(VFO, vfo));

    LOG_DEBUG("RadioController", QString("setFrequency invokeMethod returned: %1").arg(queued));
}

void RadioController::setBand(BandType band, VFO vfo) {
    LOG_DEBUG("RadioController", QString("setBand called: band=%1, VFO %2")
              .arg(bandToString(band))
              .arg(vfo == VFO::VFO_A ? "A" : "B"));

    // Use invokeMethod to call setBand() on the radio in the worker thread
    bool queued = QMetaObject::invokeMethod(m_radio, "setBand", Qt::QueuedConnection,
                                            Q_ARG(BandType, band),
                                            Q_ARG(VFO, vfo));

    LOG_DEBUG("RadioController", QString("setBand invokeMethod returned: %1").arg(queued));
}

void RadioController::setMode(ModeType mode, VFO vfo) {
    emit requestSetMode(mode, vfo);
}

void RadioController::setPTT(bool transmit) {
    emit requestSetPTT(transmit);
}

void RadioController::sendCW(const QString& text) {
    emit requestSendCW(text);
}

void RadioController::setCWSpeed(int wpm) {
    emit requestSetCWSpeed(wpm);
}

void RadioController::setDetailedRigInfoEnabled(bool enabled) {
    if (!m_radio) return;
    QMetaObject::invokeMethod(m_radio, [this, enabled]() {
        m_radio->setDetailedRigInfoEnabled(enabled);
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

void RadioController::getCWSpeedRange(int& minWpm, int& maxWpm) const {
    // Call synchronously since we need the return values
    QMetaObject::invokeMethod(m_radio, [this, &minWpm, &maxWpm]() {
        m_radio->getCWSpeedRange(minWpm, maxWpm);
    }, Qt::BlockingQueuedConnection);
}

void RadioController::stopCW() {
    emit requestStopCW();
}

bool RadioController::waitForMorseComplete() {
    bool result = false;
    QMetaObject::invokeMethod(m_radio, [this, &result]() {
        result = m_radio->waitForMorseComplete();
    }, Qt::BlockingQueuedConnection);
    return result;
}

void RadioController::enableRIT(bool enable, VFO vfo) {
    emit requestEnableRIT(enable, vfo);
}

void RadioController::enableXIT(bool enable, VFO vfo) {
    emit requestEnableXIT(enable, vfo);
}

void RadioController::setSplit(bool enable, VFO txVfo) {
    emit requestSetSplit(enable, txVfo);
}

void RadioController::sendKeyDown() {
    // Key the transmitter via PTT for CW keying (used by software iambic keyer)
    emit requestSetPTT(true);
}

void RadioController::sendKeyUp() {
    // Un-key the transmitter (used by software iambic keyer)
    emit requestSetPTT(false);
}

} // namespace TR4QT
