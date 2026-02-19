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

#include "SpotCollectorService.h"
#include "../logging/LogMacros.h"
#include "../utils/AppSettings.h"

namespace TR4QT {

SpotCollectorService::SpotCollectorService(QObject* parent)
    : QObject(parent)
    , m_pathfinder(new DXLabPathfinder(this))
{
    connect(m_pathfinder, &DXLabPathfinder::callsignReceived,
            this, &SpotCollectorService::onPathfinderCallsign);
    connect(m_pathfinder, &DXLabPathfinder::error,
            this, &SpotCollectorService::onPathfinderError);
}

SpotCollectorService::~SpotCollectorService() = default;

void SpotCollectorService::loadSettings()
{
#ifndef Q_OS_WIN
    // DDE is Windows-only — never start on other platforms regardless of saved setting
    return;
#else
    bool enabled = AppSettings::instance().getDXLabDDEEnabled();
    m_qsyEnabled = AppSettings::instance().getDXLabDDEQSY();
    if (enabled && !m_pathfinder->isRunning()) {
        m_pathfinder->start();
    } else if (!enabled && m_pathfinder->isRunning()) {
        m_pathfinder->stop();
    }
#endif
}

bool SpotCollectorService::isRunning() const
{
    return m_pathfinder->isRunning();
}

void SpotCollectorService::setFrequencyLookup(FrequencyLookupCallback callback)
{
    m_frequencyLookup = std::move(callback);
}

void SpotCollectorService::onPathfinderCallsign(const QString& callsign)
{
    LOG_DEBUG("SpotCollectorService", QString("Callsign received: %1").arg(callsign));

    // Only accept if fields are empty or callsign was previously set by DDE
    if (!m_fieldsEmpty && !m_callsignFromDDE) {
        LOG_DEBUG("SpotCollectorService", "Ignoring, user is working a contact");
        return;
    }

    m_callsignFromDDE = true;
    emit callsignReceived(callsign);

    // QSY if enabled and we have a frequency lookup
    if (m_qsyEnabled && m_frequencyLookup) {
        double freq = m_frequencyLookup(callsign);
        if (freq > 0) {
            LOG_DEBUG("SpotCollectorService", QString("QSY to %1 Hz").arg(freq));
            emit qsyRequested(freq);
        }
    }
}

void SpotCollectorService::onPathfinderError(const QString& message)
{
    LOG_WARN("SpotCollectorService", QString("DDE error: %1").arg(message));
    emit error(message);
}

} // namespace TR4QT
