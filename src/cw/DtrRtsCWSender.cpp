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

#include "DtrRtsCWSender.h"
#include "MorseEncoder.h"
#include "../logging/LogMacros.h"

#ifdef Q_OS_WIN
#include <windows.h>
#else
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#endif

namespace TR4QT {

// ============================================================================
// DtrRtsWorker - runs in dedicated high-priority thread
// ============================================================================

DtrRtsWorker::DtrRtsWorker(const QString& portName, Pin pin)
    : m_portName(portName)
    , m_pin(pin)
{
    // Don't create MorseEncoder or open port here.
    // init() must be called AFTER moveToThread() so that QTimers
    // are created on the worker thread (member QTimers don't move
    // with moveToThread since they have no QObject parent).
}

void DtrRtsWorker::init()
{
    m_encoder = new MorseEncoder(this);

    connect(m_encoder, &MorseEncoder::keyDown, this, &DtrRtsWorker::onEncoderKeyDown);
    connect(m_encoder, &MorseEncoder::keyUp, this, &DtrRtsWorker::onEncoderKeyUp);
    connect(m_encoder, &MorseEncoder::finished, this, &DtrRtsWorker::onEncoderFinished);
    connect(m_encoder, &MorseEncoder::stopped, this, &DtrRtsWorker::onEncoderFinished);

    if (!m_portName.isEmpty()) {
        openPort();
    }

    emit initialized(m_portOpen);
}

DtrRtsWorker::~DtrRtsWorker()
{
    closePort();
}

void DtrRtsWorker::doSend(const QString& text, int wpm)
{
    if (text.isEmpty()) return;

    if (!m_portOpen) {
        emit portError("DTR/RTS port not open");
        return;
    }

    // Stop any in-progress sending
    if (m_encoder->isSending()) {
        m_encoder->stop();
    }

    emit sendingStarted(text);
    m_encoder->send(text, wpm);
}

void DtrRtsWorker::doStop()
{
    m_encoder->stop();
    setKeyLine(false);
}

void DtrRtsWorker::doKeyDown()
{
    setKeyLine(true);
}

void DtrRtsWorker::doKeyUp()
{
    setKeyLine(false);
}

void DtrRtsWorker::onEncoderKeyDown()
{
    setKeyLine(true);
}

void DtrRtsWorker::onEncoderKeyUp()
{
    setKeyLine(false);
}

void DtrRtsWorker::onEncoderFinished()
{
    setKeyLine(false);
    emit sendingComplete();
}

void DtrRtsWorker::setKeyLine(bool active)
{
    if (!m_portHandle || !m_portOpen) return;

#ifdef Q_OS_WIN
    DWORD func;
    if (m_pin == Pin::DTR) {
        func = active ? SETDTR : CLRDTR;
    } else {
        func = active ? SETRTS : CLRRTS;
    }
    EscapeCommFunction(static_cast<HANDLE>(m_portHandle), func);
#else
    int fd = static_cast<int>(reinterpret_cast<intptr_t>(m_portHandle));
    int bits = 0;
    ioctl(fd, TIOCMGET, &bits);
    int pin = (m_pin == Pin::DTR) ? TIOCM_DTR : TIOCM_RTS;
    if (active) {
        bits |= pin;
    } else {
        bits &= ~pin;
    }
    ioctl(fd, TIOCMSET, &bits);
#endif
}

bool DtrRtsWorker::openPort()
{
    if (m_portName.isEmpty()) {
        LOG_WARN("DtrRtsWorker", "Cannot open port: no port name configured");
        return false;
    }

    closePort();

#ifdef Q_OS_WIN
    // Use CreateFile directly to open the port WITHOUT raising DTR.
    // QSerialPort's open() briefly raises DTR which causes unwanted keying.
    QString devicePath = QString("\\\\.\\%1").arg(m_portName);
    HANDLE h = CreateFileW(
        reinterpret_cast<LPCWSTR>(devicePath.utf16()),
        GENERIC_READ | GENERIC_WRITE,
        0,
        NULL,
        OPEN_EXISTING,
        0,
        NULL);

    if (h == INVALID_HANDLE_VALUE) {
        DWORD err = GetLastError();
        LOG_ERROR("DtrRtsWorker", QString("Failed to open port %1: Windows error %2")
                  .arg(m_portName).arg(err));
        return false;
    }

    // Force DTR and RTS low IMMEDIATELY after CreateFile.
    // The driver may raise DTR during open - clear it as fast as possible.
    EscapeCommFunction(h, CLRDTR);
    EscapeCommFunction(h, CLRRTS);

    // Configure DCB to disable automatic DTR/RTS control so Windows
    // doesn't raise them again behind our back.
    DCB dcb;
    ZeroMemory(&dcb, sizeof(DCB));
    dcb.DCBlength = sizeof(DCB);

    if (!GetCommState(h, &dcb)) {
        LOG_ERROR("DtrRtsWorker", QString("Failed to get comm state for %1").arg(m_portName));
        CloseHandle(h);
        return false;
    }

    dcb.fDtrControl = DTR_CONTROL_DISABLE;
    dcb.fRtsControl = RTS_CONTROL_DISABLE;

    if (!SetCommState(h, &dcb)) {
        LOG_ERROR("DtrRtsWorker", QString("Failed to set comm state for %1").arg(m_portName));
        CloseHandle(h);
        return false;
    }

    m_portHandle = static_cast<void*>(h);
#else
    // Linux/macOS: open with native API, same approach (no DTR raise)
    QByteArray path = m_portName.toLocal8Bit();
    int fd = ::open(path.constData(), O_RDWR | O_NOCTTY | O_NONBLOCK);
    if (fd < 0) {
        LOG_ERROR("DtrRtsWorker", QString("Failed to open port %1: errno %2")
                  .arg(m_portName).arg(errno));
        return false;
    }

    // Clear DTR and RTS
    int bits = 0;
    ioctl(fd, TIOCMGET, &bits);
    bits &= ~(TIOCM_DTR | TIOCM_RTS);
    ioctl(fd, TIOCMSET, &bits);

    m_portHandle = reinterpret_cast<void*>(static_cast<intptr_t>(fd));
#endif

    m_portOpen = true;
    LOG_INFO("DtrRtsWorker", QString("Opened port %1 for %2 keying (native API)")
             .arg(m_portName)
             .arg(m_pin == Pin::DTR ? "DTR" : "RTS"));
    return true;
}

void DtrRtsWorker::closePort()
{
    if (m_portHandle && m_portOpen) {
        // Ensure keying line is low before closing
        setKeyLine(false);

#ifdef Q_OS_WIN
        CloseHandle(static_cast<HANDLE>(m_portHandle));
#else
        int fd = static_cast<int>(reinterpret_cast<intptr_t>(m_portHandle));
        ::close(fd);
#endif
    }
    m_portHandle = nullptr;
    m_portOpen = false;
}

// ============================================================================
// DtrRtsCWSender - main thread interface
// ============================================================================

DtrRtsCWSender::DtrRtsCWSender(const Config& config, QObject* parent)
    : CWSender(parent)
{
    // Create worker (on main thread initially, but only stores config)
    auto workerPin = static_cast<DtrRtsWorker::Pin>(config.pin);
    m_worker = new DtrRtsWorker(config.portName, workerPin);

    // Move worker to dedicated thread
    m_worker->moveToThread(&m_workerThread);

    // Worker init runs on the worker thread (creates MorseEncoder + QSerialPort there
    // so QTimers are on the correct thread)
    connect(&m_workerThread, &QThread::started, m_worker, &DtrRtsWorker::init);

    // Connect dispatch signals → worker slots (auto QueuedConnection cross-thread)
    connect(this, &DtrRtsCWSender::requestSend, m_worker, &DtrRtsWorker::doSend);
    connect(this, &DtrRtsCWSender::requestStop, m_worker, &DtrRtsWorker::doStop);
    connect(this, &DtrRtsCWSender::requestKeyDown, m_worker, &DtrRtsWorker::doKeyDown);
    connect(this, &DtrRtsCWSender::requestKeyUp, m_worker, &DtrRtsWorker::doKeyUp);

    // Connect worker signals → our slots (auto QueuedConnection cross-thread)
    connect(m_worker, &DtrRtsWorker::initialized, this, [this](bool portOpen) {
        m_portOpen = portOpen;
        LOG_DEBUG("DtrRtsCWSender", QString("Worker initialized, portOpen=%1").arg(portOpen));
    });
    connect(m_worker, &DtrRtsWorker::sendingStarted, this, &DtrRtsCWSender::onWorkerSendingStarted);
    connect(m_worker, &DtrRtsWorker::sendingComplete, this, &DtrRtsCWSender::onWorkerSendingComplete);
    connect(m_worker, &DtrRtsWorker::sendingStopped, this, &DtrRtsCWSender::onWorkerSendingStopped);
    connect(m_worker, &DtrRtsWorker::portError, this, &DtrRtsCWSender::onWorkerPortError);

    // Clean up worker when thread finishes
    connect(&m_workerThread, &QThread::finished, m_worker, &QObject::deleteLater);

    // Start with high priority for precise CW timing
    m_workerThread.start(QThread::HighPriority);

    LOG_DEBUG("DtrRtsCWSender", QString("Created: pin=%1, port=%2, thread=HighPriority")
              .arg(config.pin == Pin::DTR ? "DTR" : "RTS")
              .arg(config.portName));
}

DtrRtsCWSender::~DtrRtsCWSender()
{
    // Stop any in-progress sending
    emit requestStop();

    m_workerThread.quit();
    m_workerThread.wait(2000);

    if (m_workerThread.isRunning()) {
        LOG_WARN("DtrRtsCWSender", "Worker thread didn't exit cleanly, terminating");
        m_workerThread.terminate();
        m_workerThread.wait(1000);
    }
}

CWSender::State DtrRtsCWSender::state() const
{
    return m_state;
}

bool DtrRtsCWSender::isAvailable() const
{
    return m_portOpen;
}

int DtrRtsCWSender::wpm() const
{
    return m_wpm;
}

void DtrRtsCWSender::setWpm(int wpm)
{
    if (m_wpm != wpm) {
        m_wpm = wpm;
        emit wpmChanged(wpm);
    }
}

void DtrRtsCWSender::send(const QString& text)
{
    if (text.isEmpty()) return;

    if (!m_portOpen) {
        emit error("DTR/RTS port not available");
        return;
    }

    m_state = State::Sending;
    emit stateChanged(m_state);

    // Dispatch to worker thread
    emit requestSend(text, m_wpm);
}

void DtrRtsCWSender::stop()
{
    if (m_state == State::Idle) return;

    m_state = State::Stopping;
    emit stateChanged(m_state);

    // Dispatch to worker thread
    emit requestStop();
}

void DtrRtsCWSender::keyDown()
{
    emit requestKeyDown();
}

void DtrRtsCWSender::keyUp()
{
    emit requestKeyUp();
}

void DtrRtsCWSender::onWorkerSendingStarted(const QString& text)
{
    emit transmissionStarted(text);
}

void DtrRtsCWSender::onWorkerSendingComplete()
{
    m_state = State::Idle;
    emit stateChanged(m_state);
    emit transmissionComplete();
}

void DtrRtsCWSender::onWorkerSendingStopped()
{
    m_state = State::Idle;
    emit stateChanged(m_state);
    emit transmissionStopped();
}

void DtrRtsCWSender::onWorkerPortError(const QString& errorMsg)
{
    m_state = State::Error;
    emit stateChanged(m_state);
    emit error(errorMsg);
}

} // namespace TR4QT
