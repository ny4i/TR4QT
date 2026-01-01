#include "HamlibCWSender.h"
#include "../radio/RadioController.h"
#include "../logging/Logger.h"
#include "../logging/LogMacros.h"
#include "../utils/AppSettings.h"

namespace TR4QT {

// ============================================================================
// HamlibCWSender Implementation
// ============================================================================

HamlibCWSender::HamlibCWSender(RadioController* radio, QObject* parent)
    : CWSender(parent)
    , m_radio(radio)
    , m_wpm(AppSettings::instance().getMorseWPM())
{
    // Create worker and move to thread
    HamlibCWWorker* worker = new HamlibCWWorker();
    worker->moveToThread(&m_workerThread);
    worker->setStopFlag(&m_stopRequested);

    // Connect worker signals
    connect(&m_workerThread, &QThread::finished, worker, &QObject::deleteLater);
    connect(this, &HamlibCWSender::startWorkerSend, worker, &HamlibCWWorker::doSend);
    connect(worker, &HamlibCWWorker::finished, this, &HamlibCWSender::onWorkerFinished);

    m_workerThread.start();

    LOG_DEBUG("HamlibCWSender", "CW sender initialized");
}

HamlibCWSender::~HamlibCWSender() {
    stop();
    m_workerThread.quit();
    m_workerThread.wait();
}

CWSender::State HamlibCWSender::state() const {
    return m_state.load();
}

bool HamlibCWSender::isAvailable() const {
    // Check if radio is connected AND supports CW sending via Hamlib
    return m_radio && m_radio->isConnected() && m_radio->supportsCWSending();
}

int HamlibCWSender::wpm() const {
    QMutexLocker locker(&m_mutex);
    return m_wpm;
}

void HamlibCWSender::setWpm(int wpm) {
    {
        QMutexLocker locker(&m_mutex);
        if (m_wpm == wpm) return;
        m_wpm = wpm;
    }

    // Update radio CW speed
    if (m_radio && m_radio->isConnected()) {
        m_radio->setCWSpeed(wpm);
    }

    emit wpmChanged(wpm);
}

void HamlibCWSender::send(const QString& text) {
    if (!isAvailable()) {
        emit error("Radio not connected");
        return;
    }

    if (m_state == State::Sending) {
        LOG_WARN("HamlibCWSender", "Already sending CW, ignoring new request");
        return;
    }

    m_stopRequested = false;
    m_state = State::Sending;
    emit stateChanged(State::Sending);

    // Set the WPM before sending
    int currentWpm = wpm();
    m_radio->setCWSpeed(currentWpm);

    // Start sending via radio controller
    m_radio->sendCW(text);

    LOG_INFO("HamlibCWSender", QString("Sending CW: '%1' at %2 WPM").arg(text).arg(currentWpm));

    // Emit the public signal
    emit transmissionStarted(text);

    // Emit signal to worker to wait for completion
    emit startWorkerSend(text, currentWpm);
}

void HamlibCWSender::stop() {
    if (m_state != State::Sending) {
        return;
    }

    m_stopRequested = true;
    m_state = State::Stopping;
    emit stateChanged(State::Stopping);

    // Stop the radio transmission
    if (m_radio) {
        m_radio->stopCW();
    }

    LOG_INFO("HamlibCWSender", "CW transmission stop requested");
}

void HamlibCWSender::onWorkerFinished(bool success, const QString& errorMsg) {
    if (m_state == State::Stopping || m_stopRequested) {
        m_state = State::Idle;
        emit stateChanged(State::Idle);
        emit transmissionStopped();
    } else if (success) {
        m_state = State::Idle;
        emit stateChanged(State::Idle);
        emit transmissionComplete();
    } else {
        m_state = State::Idle;
        emit stateChanged(State::Idle);
        // If rig_wait_morse is not supported, still emit complete
        // (the transmission likely finished, we just couldn't confirm)
        emit transmissionComplete();
    }
}

// ============================================================================
// HamlibCWWorker Implementation
// ============================================================================

HamlibCWWorker::HamlibCWWorker(QObject* parent)
    : QObject(parent)
{
}

void HamlibCWWorker::doSend(const QString& text, int wpm) {
    Q_UNUSED(wpm)

    // Estimate transmission time based on WPM
    // PARIS standard: 50 units per word, at 1 WPM = 60 seconds/word
    // At N WPM, time per char ≈ 1200/N ms for 10 units average per char
    double msPerUnit = 1200.0 / wpm;
    int estimatedMs = static_cast<int>(text.length() * 10 * msPerUnit) + 500;

    // Wait for the estimated transmission time
    // This is a fallback if rig_wait_morse is not available
    // In a real implementation, we would call rig_wait_morse here

    // For now, just sleep for estimated time
    // TODO: Integrate with RadioController::waitForMorseComplete()
    // The challenge is that waitForMorseComplete blocks the radio thread
    // and we need to handle the stop flag

    int elapsed = 0;
    const int sleepInterval = 100; // Check stop flag every 100ms

    while (elapsed < estimatedMs) {
        if (m_stopRequested && m_stopRequested->load()) {
            emit finished(false, "Stopped by user");
            return;
        }
        QThread::msleep(sleepInterval);
        elapsed += sleepInterval;
    }

    emit finished(true, QString());
}

} // namespace TR4QT
