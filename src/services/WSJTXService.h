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

#ifndef WSJTXSERVICE_H
#define WSJTXSERVICE_H

#include <QString>
#include "../core/Types.h"
#include "../models/QSO.h"
#include "../network/WSJTXMessage.h"

namespace TR4QT {

class CountryFile;

/**
 * Result of extracting a callsign from a WSJT-X decode message.
 */
struct WSJTXCallsignInfo {
    QString callsign;
    bool isCQ{false};
    bool valid{false};  // false if no callsign could be extracted
};

/**
 * Result of mapping a WSJT-X mode string.
 */
struct WSJTXModeMapping {
    ModeType modeType{ModeType::None};
    QString adifMode;
    QString adifSubmode;
    bool rejected{false};  // true for non-QSO modes (WSPR, FST4W)
};

/**
 * Service layer for WSJT-X data conversion.
 *
 * Pure functions — no state, no I/O, fully testable.
 * - Mode mapping (WSJT-X mode string → ModeType + ADIF mode/submode)
 * - Callsign extraction from FT8/FT4 decode message text
 * - QSO conversion (WSJTXQSOLogged → QSO struct)
 */
class WSJTXService {
public:
    /**
     * Map WSJT-X mode string to TR4QT ModeType and ADIF mode/submode.
     * Returns rejected=true for non-QSO modes (WSPR, FST4W).
     */
    static WSJTXModeMapping mapMode(const QString& wsjtxMode);

    /**
     * Extract callsign from FT8/FT4 decoded message text.
     *
     * Supported formats:
     *   "CQ W1AW FN42"         → callsign=W1AW, isCQ=true
     *   "CQ DX W1AW FN42"      → callsign=W1AW, isCQ=true
     *   "CQ NA W1AW FN42"      → callsign=W1AW, isCQ=true
     *   "W1AW K3LR FN42"       → callsign=W1AW (first call)
     *   "RR73" / "73" / "RRR"  → valid=false (no callsign)
     */
    static WSJTXCallsignInfo extractCallsign(const QString& decodeMessage);

    /**
     * Convert a WSJT-X QSOLogged message to a TR4QT QSO struct.
     * @param msg The WSJT-X QSO logged message
     * @param countryFile Country lookup for DXCC enrichment (may be nullptr)
     * @return Populated QSO struct ready for logging
     */
    static QSO convertToQSO(const WSJTXQSOLogged& msg, CountryFile* countryFile);

private:
    /**
     * Check if a token looks like a valid amateur radio callsign.
     * Basic heuristic: contains at least one digit and one letter,
     * only alphanumeric and slash characters, length 3-15.
     */
    static bool looksLikeCallsign(const QString& token);
};

} // namespace TR4QT

#endif // WSJTXSERVICE_H
