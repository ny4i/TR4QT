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

#ifndef HAMLIBAMPLIFIER_H
#define HAMLIBAMPLIFIER_H

#include "IAmplifierController.h"
#include <hamlib/amplifier.h>
#include <QMutex>
#include <QTimer>

namespace TR4QT {

/**
 * Hamlib Amplifier Controller
 *
 * Concrete implementation of IAmplifierController using Hamlib amplifier library.
 * Supports all amplifiers through Hamlib, including:
 * - Elecraft KPA1500 (AMP_MODEL_ELECRAFT_KPA1500, model 1201)
 * - Elecraft KPA500 (future support via Hamlib)
 * - Other amplifiers supported by Hamlib
 *
 * Threading:
 * - Uses QMutex for thread-safe access to AMP* handle
 * - Polling timer runs in same thread as controller
 * - All Hamlib calls protected by mutex
 */
class HamlibAmplifier : public IAmplifierController {
    Q_OBJECT

public:
    explicit HamlibAmplifier(QObject* parent = nullptr);
    ~HamlibAmplifier() override;

public slots:
    // IAmplifierController slot overrides (must be in slots section for MOC)
    bool connect(const AmplifierConfig& config) override;
    void disconnect() override;
    void setFrequency(freq_t freq) override;
    void queryStatus() override;
    void sendRawCommand(const QString& command) override;

public:
    // Query methods (const, not slots)
    bool isConnected() const override;
    AmplifierState getState() const override;

private slots:
    void pollAmplifier();

private:
    // Hamlib error logging helper
    void logHamlibError(const char* operation, int retcode);

    // Pointer check helper (for const methods)
    bool checkAmpPointer(const char* context) const;

    // Update state from Hamlib queries
    void updateState();

    // Hamlib handle
    AMP* m_amp{nullptr};

    // Thread safety
    mutable QMutex m_ampMutex;

    // Polling timer
    QTimer* m_pollTimer{nullptr};
    int m_pollIntervalMs{100};  // Default: 100ms polling interval

    // Connection state
    bool m_connected{false};
    AmplifierConfig m_config;

    // Current state (cached from last successful query)
    AmplifierState m_currentState;

    // Error tracking (for auto-disconnect on persistent errors)
    int m_consecutiveErrors{0};
    static constexpr int MAX_CONSECUTIVE_ERRORS = 10;
};

} // namespace TR4QT

#endif // HAMLIBAMPLIFIER_H
