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

/**
 * @file SpotProcessingService.cpp
 * @brief Implementation of SpotProcessingService
 *
 * Extracted from MainWindow::onDXSpotReceived() - 103 lines.
 */

#include "SpotProcessingService.h"
#include "../ui/widgets/BandMapWidget.h"  // For Spot struct
#include "../data/LOTWUserRepository.h"
#include "../utils/AppSettings.h"
#include "../logging/LogMacros.h"

#include <QRegularExpression>

namespace TR4QT {

Spot SpotProcessingService::processSpot(const QString& callsign, double frequency,
                                         const QString& spotter, const QString& comment) const {
    LOG_DEBUG("SpotProcessingService", QString("Processing spot: %1 at %2 Hz from %3")
        .arg(callsign)
        .arg(QString::number(static_cast<qint64>(frequency)))
        .arg(spotter));

    Spot spot;
    spot.callsign = callsign;
    spot.frequency = static_cast<freq_t>(frequency);
    spot.timestamp = QDateTime::currentDateTime();
    spot.isMultiplier = false;  // TODO: Check if this is a needed multiplier
    spot.isWorked = false;      // TODO: Check if we've worked this station
    spot.comment = comment;
    spot.source = QString("DX Cluster (%1)").arg(spotter);

    // Parse split frequency from comment
    spot.qsx = parseQSX(comment, spot.frequency);
    if (spot.qsx == 0) {
        spot.qsx = parseUP(comment, spot.frequency);
    }

    // Check LOTW status
    spot.isLotwUser = checkLotwUser(callsign);

    // Log enriched spot details
    QString logMsg = QString("Processed spot: %1").arg(callsign);
    logMsg += QString(" | TX: %1 kHz").arg(spot.frequency / 1000.0, 0, 'f', 1);
    if (spot.qsx > 0) {
        logMsg += QString(" | RX: %1 kHz").arg(spot.qsx / 1000.0, 0, 'f', 1);
    }
    logMsg += QString(" | LOTW: %1").arg(spot.isLotwUser ? "YES" : "NO");
    LOG_DEBUG("SpotProcessingService", logMsg);

    return spot;
}

freq_t SpotProcessingService::parseQSX(const QString& comment, freq_t spotFrequency) const {
    // Pattern: "QSX 210" (kHz fragment) or "QSX 14.210" (full MHz)
    static QRegularExpression qsxRegex(R"(\bQSX\s+(\d+(?:\.\d+)?)\b)",
                                        QRegularExpression::CaseInsensitiveOption);
    QRegularExpressionMatch match = qsxRegex.match(comment);

    if (!match.hasMatch()) {
        return 0;
    }

    double qsxValue = match.captured(1).toDouble();

    if (qsxValue < 1000) {
        // kHz fragment (e.g., 210 = 14.210 MHz on 20m)
        freq_t spotMHz = (spotFrequency / 1000000) * 1000000;
        freq_t result = spotMHz + static_cast<freq_t>(qsxValue * 1000);
        LOG_DEBUG("SpotProcessingService", QString("Parsed QSX fragment: %1 kHz = %2 kHz")
            .arg(qsxValue).arg(result / 1000.0, 0, 'f', 1));
        return result;
    } else {
        // Full frequency in MHz
        freq_t result = static_cast<freq_t>(qsxValue * 1000000);
        LOG_DEBUG("SpotProcessingService", QString("Parsed QSX full: %1 MHz = %2 kHz")
            .arg(qsxValue).arg(result / 1000.0, 0, 'f', 1));
        return result;
    }
}

freq_t SpotProcessingService::parseUP(const QString& comment, freq_t spotFrequency) const {
    // Pattern: "UP 5" (offset in kHz)
    static QRegularExpression upRegex(R"(\bUP\s+(\d+(?:\.\d+)?)\b)",
                                       QRegularExpression::CaseInsensitiveOption);
    QRegularExpressionMatch match = upRegex.match(comment);

    if (!match.hasMatch()) {
        return 0;
    }

    double offsetKHz = match.captured(1).toDouble();
    freq_t result = spotFrequency + static_cast<freq_t>(offsetKHz * 1000);

    LOG_DEBUG("SpotProcessingService", QString("Parsed UP: TX=%1 kHz + %2 kHz = RX=%3 kHz")
        .arg(spotFrequency / 1000.0, 0, 'f', 1)
        .arg(offsetKHz)
        .arg(result / 1000.0, 0, 'f', 1));

    return result;
}

bool SpotProcessingService::checkLotwUser(const QString& callsign) const {
    AppSettings& settings = AppSettings::instance();
    if (!settings.getEnableLotwLookup()) {
        LOG_DEBUG("SpotProcessingService", QString("%1 - LOTW lookup disabled").arg(callsign));
        return false;
    }

    LOTWUserRepository lotwRepo;
    bool isLotwUser = lotwRepo.isLotwUser(callsign);

    if (isLotwUser) {
        LOTWUser lotwUser = lotwRepo.findByCallsign(callsign);
        LOG_DEBUG("SpotProcessingService", QString("%1 is LOTW user (last upload: %2 %3)")
            .arg(callsign)
            .arg(lotwUser.lastUploadDate)
            .arg(lotwUser.lastUploadTime));
    } else {
        LOG_DEBUG("SpotProcessingService", QString("%1 is NOT an LOTW user").arg(callsign));
    }

    return isLotwUser;
}

} // namespace TR4QT
