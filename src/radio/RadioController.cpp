#include "RadioController.h"
#include <QDebug>
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
                qDebug() << "RadioController: Received connectionStatusChanged:" << connected;

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
    // Disconnect radio if still connected
    if (m_connected) {
        disconnectFromRadio();
        // Give it a moment to disconnect
        QThread::msleep(100);
    }

    // Stop worker thread
    m_workerThread.quit();
    m_workerThread.wait(3000);  // Wait up to 3 seconds

    if (m_workerThread.isRunning()) {
        qWarning() << "Worker thread did not stop gracefully, terminating";
        m_workerThread.terminate();
        m_workerThread.wait();
    }
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

void RadioController::connectToRadio(const RadioConfig& config) {
    qDebug() << "RadioController::connectToRadio called with model" << config.hamlibModelId << "port" << config.port;

    // Invoke connect method in worker thread using lambda
    bool queued = QMetaObject::invokeMethod(m_radio, [this, config]() {
        qDebug() << "RadioController: Lambda executing in worker thread";
        // Call the RadioInterface::connect method (not QObject::connect)
        bool success = static_cast<RadioInterface*>(m_radio)->connect(config);
        qDebug() << "RadioController: connect() returned" << success;
        if (!success) {
            qWarning() << "HamlibRadio::connect returned false";
        }
    }, Qt::QueuedConnection);

    qDebug() << "RadioController::connectToRadio: invokeMethod returned" << queued;
}

void RadioController::disconnectFromRadio() {
    // Invoke disconnect method in worker thread (blocking to ensure it completes)
    QMetaObject::invokeMethod(m_radio, [this]() {
        static_cast<RadioInterface*>(m_radio)->disconnect();
    }, Qt::BlockingQueuedConnection);
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

} // namespace TR4QT
