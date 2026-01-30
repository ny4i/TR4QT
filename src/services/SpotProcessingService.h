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
 * @file SpotProcessingService.h
 * @brief Service for processing DX cluster spots
 *
 * Extracted from MainWindow::onDXSpotReceived() - 103 lines.
 * Handles spot enrichment (LOTW, QSX parsing) and forwarding.
 */

#ifndef SPOTPROCESSINGSERVICE_H
#define SPOTPROCESSINGSERVICE_H

#include <QString>
#include <QDateTime>
#include <hamlib/rig.h>  // For freq_t type

namespace TR4QT {

// Forward declarations
struct Spot;

/**
 * @brief Service for processing and enriching DX cluster spots
 *
 * Responsibilities:
 * - Parse QSX/UP split frequency from spot comments
 * - Look up LOTW user status
 * - Create fully-enriched Spot struct ready for display
 */
class SpotProcessingService {
public:
    SpotProcessingService() = default;

    /**
     * @brief Process a raw DX cluster spot into enriched Spot struct
     *
     * @param callsign Spotted callsign
     * @param frequency Spot frequency in Hz
     * @param spotter Who spotted it
     * @param comment Spot comment (may contain QSX/UP info)
     * @return Enriched Spot struct ready for band map
     */
    Spot processSpot(const QString& callsign, double frequency,
                     const QString& spotter, const QString& comment) const;

private:
    /**
     * @brief Parse QSX (split frequency) from spot comment
     *
     * Supports: "QSX 210" (kHz fragment) or "QSX 14.210" (full MHz)
     *
     * @param comment Spot comment text
     * @param spotFrequency Base spot frequency in Hz
     * @return Parsed QSX frequency in Hz, or 0 if not found
     */
    freq_t parseQSX(const QString& comment, freq_t spotFrequency) const;

    /**
     * @brief Parse UP (offset) from spot comment
     *
     * Supports: "UP 5" (offset in kHz from spot frequency)
     *
     * @param comment Spot comment text
     * @param spotFrequency Base spot frequency in Hz
     * @return Calculated QSX frequency in Hz, or 0 if not found
     */
    freq_t parseUP(const QString& comment, freq_t spotFrequency) const;

    /**
     * @brief Check if callsign is an LOTW user
     *
     * @param callsign Callsign to check
     * @return true if LOTW user (and lookup enabled), false otherwise
     */
    bool checkLotwUser(const QString& callsign) const;
};

} // namespace TR4QT

#endif // SPOTPROCESSINGSERVICE_H
