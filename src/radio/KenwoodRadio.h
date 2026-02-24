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

#ifndef KENWOODRADIO_H
#define KENWOODRADIO_H

#include "RadioInterface.h"
#include <QTcpSocket>
#include <QTimer>
#include <QMutex>

namespace TR4QT {

/**
 * @brief Base class for Kenwood network/serial radios (bypasses Hamlib)
 *
 * Implements the Kenwood ASCII CAT protocol (semicolon-terminated commands)
 * for improved performance and access to Kenwood-specific features.
 *
 * Supports LAN authentication (##CN/##ID handshake) for network radios.
 * Uses AI2 (Auto Information mode 2) for automatic push updates.
 *
 * Subclasses implement model-specific details:
 * - Mode encoding (OM command P2 values differ per model)
 * - Radio ID string (e.g., "024" for TS-890S)
 * - Post-authentication initialization
 *
 * Supported radios:
 * - TS-890S (via TS890Radio subclass)
 * - Future: TS-990S, etc.
 *
 * Protocol: ASCII commands terminated with ';' (e.g., "FA00014200000;")
 * Benefits over Hamlib:
 * - Direct TCP connection (no Hamlib abstraction layer)
 * - AI mode provides instant push updates
 * - Access to Kenwood-specific features (metering, bandscope)
 */
class KenwoodRadio : public RadioInterface {
    Q_OBJECT

public:
    explicit KenwoodRadio(QObject* parent = nullptr);
    ~KenwoodRadio() override;

public slots:
    // DEBUG: Test slot
    void debugTestSlot(int testValue) override;

    // RadioInterface slot overrides
    bool connect(const RadioConfig& config) override;
    void disconnect() override;

    bool setFrequency(freq_t freq, VFO vfo = VFO::VFO_A) override;
    bool setBand(BandType band, VFO vfo = VFO::VFO_A) override;
    bool setMode(ModeType mode, VFO vfo = VFO::VFO_A) override;
    bool setPTT(bool transmit) override;
    bool sendCW(const QString& text) override;
    bool setCWSpeed(int wpm) override;
    bool stopCW() override;
    bool waitForMorseComplete() override;
    bool setRIT(int offset_hz, VFO vfo = VFO::VFO_A) override;
    bool setXIT(int offset_hz, VFO vfo = VFO::VFO_A) override;
    bool clearRIT(VFO vfo = VFO::VFO_A) override;
    bool clearXIT(VFO vfo = VFO::VFO_A) override;
    bool enableRIT(bool enable, VFO vfo = VFO::VFO_A) override;
    bool enableXIT(bool enable, VFO vfo = VFO::VFO_A) override;
    bool setSplit(bool enable, VFO txVfo = VFO::VFO_B) override;
    bool vfoBumpUp(VFO vfo = VFO::VFO_A) override;
    bool vfoBumpDown(VFO vfo = VFO::VFO_A) override;
    bool setFilterWidth(int width_hz) override;

    // Detailed rig info (metering polls)
    void setDetailedRigInfoEnabled(bool enabled) override;

public:
    // Query methods (const, not slots)
    bool isConnected() const override;
    freq_t getFrequency(VFO vfo = VFO::VFO_A) const override;
    ModeType getMode(VFO vfo = VFO::VFO_A) const override;
    bool getPTT() const override;
    int getCWSpeed() const override;
    int getRIT(VFO vfo = VFO::VFO_A) const override;
    int getXIT(VFO vfo = VFO::VFO_A) const override;
    bool getSplit() const override;
    int getFilterWidth() const override;
    RadioState getCurrentState() const override;
    bool supportsDiscreteBandCommand() const override;

    // Radio information
    QList<ModeType> getSupportedModes() const;
    bool supportsCWSending() const;

protected:
    // Subclass hooks (must be implemented by model-specific subclasses)

    /// Convert Kenwood OM command P2 value to TR4QT ModeType
    virtual ModeType kenwoodModeToMode(const QString& modeChar) const = 0;

    /// Convert TR4QT ModeType to Kenwood OM command P2 value
    virtual QString modeToKenwoodMode(ModeType mode) const = 0;

    /// Radio ID string returned by ID command (e.g., "024" for TS-890S)
    virtual QString radioIdString() const = 0;

    /// Model-specific initialization after authentication completes
    virtual void onConnectedInitialize() = 0;

    // Shared helpers
    void sendCommand(const QString& cmd);
    void processMessage(const QString& message);

    // State (accessible to subclasses)
    QTcpSocket* m_socket{nullptr};
    QString m_host;
    quint16 m_port{0};
    RadioState m_state;
    mutable QMutex m_stateMutex;
    QString m_receiveBuffer;

private slots:
    void onSocketConnected();
    void onSocketDisconnected();
    void onSocketError(QAbstractSocket::SocketError error);
    void onReadyRead();
    void onCWTimeout();
    void onMeterPoll();

private:
    // LAN authentication
    QString m_adminId;
    QString m_adminPassword;
    bool m_isLanConnection{false};
    bool m_authenticated{false};

    // Authentication state machine
    enum class AuthState {
        None,
        WaitingForCN,   // Sent ##CN;, waiting for ##CN1;
        WaitingForID,   // Sent ##ID..;, waiting for ##ID1;
        Authenticated   // LAN auth complete
    };
    AuthState m_authState{AuthState::None};

    // CW buffer (24-char chunks, same pattern as K4)
    QString m_cwBuffer;
    QTimer* m_cwTimer{nullptr};
    bool m_cwInProgress{false};

    // Metering poll timer (SM not auto-pushed by AI mode)
    QTimer* m_meterTimer{nullptr};
    bool m_collectDetailedRigInfo{false};

    // Meter polling interval
    static constexpr int METER_POLL_INTERVAL_MS = 250;
};

} // namespace TR4QT

#endif // KENWOODRADIO_H
