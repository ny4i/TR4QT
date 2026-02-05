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

#include "StatusNotifier.h"
#include "../logging/LogMacros.h"

namespace TR4QT {

StatusNotifier& StatusNotifier::instance() {
    static StatusNotifier instance;
    return instance;
}

StatusNotifier::StatusNotifier(QObject* parent)
    : QObject(parent)
{
}

void StatusNotifier::notify(StatusEvent event, const QVariant& data) {
    QString message = formatMessage(event, data);
    StatusStyle style = styleForEvent(event);

    LOG_DEBUG("StatusNotifier", QString("Event: %1 -> %2").arg(static_cast<int>(event)).arg(message));

    emit statusChanged(message, style);
}

void StatusNotifier::notifyCustom(const QString& message, StatusStyle style) {
    LOG_DEBUG("StatusNotifier", QString("Custom: %1").arg(message));
    emit statusChanged(message, style);
}

QString StatusNotifier::formatMessage(StatusEvent event, const QVariant& data) {
    switch (event) {
        // General
        case StatusEvent::Ready:
            return "Ready";
        case StatusEvent::Error:
            return data.isValid() ? QString("Error: %1").arg(data.toString()) : "Error";
        case StatusEvent::Warning:
            return data.isValid() ? QString("\u26A0 %1").arg(data.toString()) : "Warning";

        // Radio
        case StatusEvent::RadioConnecting:
            return "Auto-connecting to radio...";
        case StatusEvent::RadioConnected:
            return data.isValid() ? QString("Radio connected: %1").arg(data.toString()) : "Radio connected";
        case StatusEvent::RadioDisconnected:
            return "Radio disconnected";
        case StatusEvent::RadioConfigSaved:
            return "Radio configuration saved";
        case StatusEvent::RadioNotConfigured:
            return "No radio configuration found. Use Radio \u2192 Configure.";
        case StatusEvent::CWSpeedChanged:
            return QString("CW Speed: %1 WPM").arg(data.toInt());
        case StatusEvent::CWAborted:
            return "CW transmission aborted";
        case StatusEvent::CWAutoSendOff:
            return "\u26A0 CW Auto-Send is OFF - Enable in Radio menu";

        // Contest
        case StatusEvent::ContestCreated:
            return QString("Created new contest: %1").arg(data.toString());
        case StatusEvent::ContestResumed:
            return QString("Resumed contest: %1").arg(data.toString());
        case StatusEvent::ContestReopened:
            return QString("Reopened: %1").arg(data.toString());
        case StatusEvent::NoActiveContest:
            return "Error: No active contest - open a contest first";

        // QSO Logging
        case StatusEvent::QSOLogged:
            return data.isValid() ? QString("Logged: %1").arg(data.toString()) : "QSO logged";
        case StatusEvent::QSODuplicate:
            return QString("\u26A0 %1").arg(data.toString());
        case StatusEvent::QSOError:
            return data.toString();

        // Export/Import
        case StatusEvent::ExportComplete:
            return data.isValid() ? data.toString() : "Export complete";
        case StatusEvent::ImportComplete:
            return data.isValid() ? data.toString() : "Import complete";
        case StatusEvent::BackupCreated:
            return QString("Backup created: %1 - Log cleared").arg(data.toString());
        case StatusEvent::LogCleared:
            return "Log cleared";

        // Maintenance
        case StatusEvent::RescoreStarted:
            return "Rescoring contest...";
        case StatusEvent::RescoreComplete:
            return data.isValid() ? data.toString() : "Rescore complete";
        case StatusEvent::IntegrityCheckStarted:
            return "Running full integrity check...";
        case StatusEvent::IntegrityCheckComplete:
            return "Integrity check complete";
        case StatusEvent::ExchangeUpdateStarted:
            return "Updating contest exchange...";
        case StatusEvent::ExchangeUpdateComplete:
            return data.isValid() ? data.toString() : "Exchange update complete";
        case StatusEvent::PointsRecalculated:
            return QString("Recalculated points for %1 QSOs").arg(data.toInt());

        // Data Downloads
        case StatusEvent::CTYUpdateComplete:
            return "CTY.DAT updated successfully";
        case StatusEvent::CTYUpdateFailed:
            return QString("CTY download failed: %1").arg(data.toString());
        case StatusEvent::LOTWDownloadComplete:
            return data.isValid() ? data.toString() : "LOTW download complete";
        case StatusEvent::LOTWDownloadFailed:
            return QString("LOTW download failed: %1").arg(data.toString());
        case StatusEvent::SCPDownloadComplete:
            return data.isValid() ? data.toString() : "SCP download complete";
        case StatusEvent::SCPDownloadFailed:
            return QString("SCP download failed: %1").arg(data.toString());

        // Web Server
        case StatusEvent::WebServerStarted:
            return QString("Web server started: %1").arg(data.toString());
        case StatusEvent::WebServerStopped:
            return "Web server stopped";
        case StatusEvent::WebServerFailed:
            return "Failed to start web server";

        // UDP Broadcast
        case StatusEvent::UDPBroadcastStarted:
            return QString("Starting UDP rebroadcast of %1 QSOs...").arg(data.toInt());
        case StatusEvent::UDPBroadcastProgress:
            return data.toString();  // Pre-formatted progress message
        case StatusEvent::UDPBroadcastComplete:
            return QString("UDP rebroadcast complete: %1 QSOs sent").arg(data.toInt());
        case StatusEvent::UDPBroadcastDisabled:
            return "Error: UDP broadcasting is disabled";

        // SO2R
        case StatusEvent::SO2RNotEnabled:
            return data.isValid() ? data.toString() : "SO2R not enabled - configure in Preferences \u2192 Hardware \u2192 Radio";
        case StatusEvent::SO2REnabled:
            return "SO2R enabled - reconnect to activate second radio";

        // Operator
        case StatusEvent::OperatorChanged:
            return QString("Operator changed to: %1").arg(data.toString());
        case StatusEvent::OperatorChangeCancelled:
            return data.isValid() ? data.toString() : "Operator change cancelled";

        // Frequency/Band
        case StatusEvent::FrequencyError:
            return QString("Error: %1").arg(data.toString());
        case StatusEvent::FrequencySet:
            return data.toString();
        case StatusEvent::BandManuallySet:
            return QString("Band: %1 (manual)").arg(data.toString());

        // Search
        case StatusEvent::SearchCancelled:
            return "Search cancelled: no criteria specified";
        case StatusEvent::SearchComplete:
            return data.toString();

        // Preferences
        case StatusEvent::PreferencesSaved:
            return "Preferences saved";
        case StatusEvent::WindowPositionsReset:
            return "Window positions reset to defaults";

        default:
            return data.isValid() ? data.toString() : "Unknown status";
    }
}

StatusStyle StatusNotifier::styleForEvent(StatusEvent event) {
    switch (event) {
        case StatusEvent::Error:
        case StatusEvent::QSOError:
        case StatusEvent::FrequencyError:
        case StatusEvent::NoActiveContest:
        case StatusEvent::CTYUpdateFailed:
        case StatusEvent::LOTWDownloadFailed:
        case StatusEvent::SCPDownloadFailed:
        case StatusEvent::WebServerFailed:
        case StatusEvent::UDPBroadcastDisabled:
            return StatusStyle::Error;

        case StatusEvent::Warning:
        case StatusEvent::QSODuplicate:
        case StatusEvent::CWAutoSendOff:
            return StatusStyle::Warning;

        case StatusEvent::QSOLogged:
        case StatusEvent::ContestCreated:
        case StatusEvent::ContestResumed:
        case StatusEvent::CTYUpdateComplete:
        case StatusEvent::ExportComplete:
        case StatusEvent::ImportComplete:
        case StatusEvent::RescoreComplete:
        case StatusEvent::IntegrityCheckComplete:
            return StatusStyle::Success;

        case StatusEvent::RadioConnecting:
        case StatusEvent::RescoreStarted:
        case StatusEvent::IntegrityCheckStarted:
        case StatusEvent::ExchangeUpdateStarted:
        case StatusEvent::UDPBroadcastStarted:
        case StatusEvent::UDPBroadcastProgress:
            return StatusStyle::Highlight;

        default:
            return StatusStyle::Normal;
    }
}

} // namespace TR4QT
