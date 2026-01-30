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

#ifndef PSTROTATORCONTROLLER_H
#define PSTROTATORCONTROLLER_H

#include "IRotatorController.h"
#include <QUdpSocket>
#include <QMutex>
#include <QThread>
#include <QQueue>
#include <QWaitCondition>

namespace TR4QT {

/**
 * Concrete implementation of IRotatorController for PSTRotator
 *
 * Supports UDP control of PSTRotator antenna rotator software
 * Protocol: XML-like commands <PST>...</PST>
 * Default port: 12000 (responses on port+1: 12001)
 *
 * Commands supported:
 * - <PST><AZIMUTH>nnn</AZIMUTH></PST> - Set azimuth (0-360°)
 * - <PST><STOP>1</STOP></PST> - Stop rotation
 * - <PST>AZ?</PST> - Query current azimuth (response: AZ:nnn.n<CR>)
 *
 * Thread safety:
 * - Commands execute in background thread (non-blocking)
 * - Uses Qt signals for cross-thread communication
 * - Internal command queue with mutex protection
 */
class PSTRotatorController : public IRotatorController {
    Q_OBJECT

public:
    explicit PSTRotatorController(QObject* parent = nullptr);
    ~PSTRotatorController() override;

public slots:
    // IRotatorController slot overrides (must be in slots section for MOC)
    bool connect(const RotatorConfig& config) override;
    void disconnect() override;

    // Azimuth control (validates range, queues command for background execution)
    bool setAzimuth(int degrees) override;

    // Stop rotator (queues command for background execution)
    void stop() override;

    // Elevation control (future - not implemented for PSTRotator yet)
    bool setElevation(int degrees) override;

public:
    // Query methods (const, not slots)
    bool isConnected() const override;
    std::optional<int> getAzimuth(int timeoutMs = 1000) const override;
    RotatorState getCurrentState() const override;

private:
    // Command queue structure
    struct Command {
        enum Type { SetAzimuth, Stop, QueryAzimuth };
        Type type;
        int value{0};  // For SetAzimuth: degrees
    };

    // Background thread for UDP communication
    class WorkerThread : public QThread {
    public:
        explicit WorkerThread(PSTRotatorController* controller)
            : m_controller(controller) {}
        void run() override;
        void requestStop();

    private:
        PSTRotatorController* m_controller;
        QAtomicInt m_stopRequested{0};
    };

    // Command building
    QString buildSetAzimuthCommand(int degrees) const;
    QString buildStopCommand() const;
    QString buildQueryAzimuthCommand() const;

    // Response parsing
    std::optional<int> parseAzimuthResponse(const QString& response) const;

    // Command queue management
    void enqueueCommand(const Command& cmd);
    bool dequeueCommand(Command& cmd);  // Returns false if queue empty
    void processCommandQueue();  // Background thread main loop

    // UDP communication
    bool sendUdpCommand(const QString& command);
    QString receiveUdpResponse(int timeoutMs);

    // Validation
    bool isValidAzimuth(int degrees) const;

    // Member variables
    RotatorConfig m_config;
    mutable QMutex m_stateMutex;  // Protects m_currentState
    RotatorState m_currentState;
    bool m_connected{false};

    // Command queue and thread management
    QQueue<Command> m_commandQueue;
    mutable QMutex m_queueMutex;
    QWaitCondition m_queueCondition;
    WorkerThread* m_workerThread{nullptr};

    // UDP socket (created in worker thread)
    QUdpSocket* m_sendSocket{nullptr};     // For sending commands (fire-and-forget)
    // Note: getAzimuth() creates its own temporary socket for queries

    static constexpr int MIN_AZIMUTH = 0;
    static constexpr int MAX_AZIMUTH = 360;
    static constexpr int DEFAULT_TIMEOUT_MS = 1000;
};

} // namespace TR4QT

#endif // PSTROTATORCONTROLLER_H
