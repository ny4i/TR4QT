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

#include "WSJTXService.h"
#include "../utils/CountryFile.h"

#include <QRegularExpression>
#include <QStringList>
#include <QUuid>

namespace TR4QT {

// ─── Mode mapping ───────────────────────────────────────────────────────────
// Reference: WSJT-X NetworkMessage.hpp + ADIF specification

WSJTXModeMapping WSJTXService::mapMode(const QString& wsjtxMode)
{
    WSJTXModeMapping result;
    const QString mode = wsjtxMode.trimmed().toUpper();

    if (mode == "FT8") {
        result.modeType = ModeType::FT8;
        result.adifMode = "FT8";
        result.adifSubmode = "FT8";
    } else if (mode == "FT4") {
        result.modeType = ModeType::FT4;
        result.adifMode = "MFSK";
        result.adifSubmode = "FT4";
    } else if (mode == "JT65") {
        result.modeType = ModeType::DATA;
        result.adifMode = "JT65";
        result.adifSubmode = "JT65";
    } else if (mode == "JT9") {
        result.modeType = ModeType::DATA;
        result.adifMode = "JT9";
        result.adifSubmode = "JT9";
    } else if (mode == "MSK144") {
        result.modeType = ModeType::DATA;
        result.adifMode = "MSK144";
        result.adifSubmode = "MSK144";
    } else if (mode == "Q65") {
        result.modeType = ModeType::DATA;
        result.adifMode = "Q65";
        result.adifSubmode = "Q65";
    } else if (mode == "FST4") {
        result.modeType = ModeType::DATA;
        result.adifMode = "FST4";
        result.adifSubmode = "FST4";
    } else if (mode == "FT2") {
        // Per issue requirement: "mode MFSK, submode FT2"
        result.modeType = ModeType::DATA;
        result.adifMode = "MFSK";
        result.adifSubmode = "FT2";
    } else if (mode == "FST4W" || mode == "WSPR") {
        // Non-QSO modes — reject
        result.rejected = true;
    } else {
        // Unknown mode — map to DATA as fallback
        result.modeType = ModeType::DATA;
        result.adifMode = mode;
        result.adifSubmode = mode;
    }

    return result;
}

// ─── Callsign extraction ────────────────────────────────────────────────────
// FT8/FT4 message formats:
//   "CQ W1AW FN42"           - Standard CQ
//   "CQ DX W1AW FN42"        - Directed CQ
//   "CQ NA W1AW FN42"        - Directed CQ with continent/prefix
//   "W1AW K3LR FN42"         - Response/report
//   "W1AW K3LR R-12"         - Report with R
//   "W1AW K3LR RR73"         - End of QSO
//   "RR73"                   - Standalone (no callsign)
//   "73"                     - Standalone (no callsign)
//   "CQ TEST W1AW FN42"     - Contest CQ

WSJTXCallsignInfo WSJTXService::extractCallsign(const QString& decodeMessage)
{
    WSJTXCallsignInfo result;

    const QString msg = decodeMessage.trimmed();
    if (msg.isEmpty())
        return result;

    const QStringList tokens = msg.split(' ', Qt::SkipEmptyParts);
    if (tokens.isEmpty())
        return result;

    // Reject standalone signoff messages
    if (tokens.size() == 1) {
        const QString& single = tokens[0];
        if (single == "RR73" || single == "73" || single == "RRR")
            return result;
    }

    // CQ message: "CQ [modifier] CALLSIGN [grid]"
    if (tokens[0] == "CQ") {
        result.isCQ = true;

        // Find the callsign — it's the first token after CQ (and optional modifier) that looks like a callsign
        for (int i = 1; i < tokens.size(); ++i) {
            if (looksLikeCallsign(tokens[i])) {
                result.callsign = tokens[i].toUpper();
                result.valid = true;
                return result;
            }
        }
        // No valid callsign found in CQ message
        return result;
    }

    // Non-CQ message: "CALL1 CALL2 [grid/report/RR73/73]"
    // First token should be a callsign
    if (looksLikeCallsign(tokens[0])) {
        result.callsign = tokens[0].toUpper();
        result.valid = true;
        return result;
    }

    return result;
}

bool WSJTXService::looksLikeCallsign(const QString& token)
{
    if (token.length() < 3 || token.length() > 15)
        return false;

    // Must contain only alphanumeric + slash
    static const QRegularExpression callPattern(
        QStringLiteral("^[A-Za-z0-9/]+$"));
    if (!callPattern.match(token).hasMatch())
        return false;

    // Must contain at least one digit and one letter
    bool hasDigit = false;
    bool hasLetter = false;
    for (const QChar& c : token) {
        if (c.isDigit()) hasDigit = true;
        if (c.isLetter()) hasLetter = true;
        if (hasDigit && hasLetter) return true;
    }

    return false;
}

// ─── QSO conversion ────────────────────────────────────────────────────────

QSO WSJTXService::convertToQSO(const WSJTXQSOLogged& msg, CountryFile* countryFile)
{
    QSO qso;
    qso.guid = QUuid::createUuid().toString(QUuid::WithoutBraces);

    // Callsign
    qso.callsign = msg.dxCall.trimmed().toUpper();

    // Timing
    qso.timestamp = msg.dateTimeOn;

    // Frequency and band
    qso.frequency = static_cast<freq_t>(msg.txFrequency);
    qso.band = frequencyToBand(static_cast<unsigned long>(msg.txFrequency));

    // Mode mapping
    auto modeMap = mapMode(msg.mode);
    qso.mode = modeMap.modeType;
    qso.submode = modeMap.adifSubmode;

    // RST from signal reports
    qso.rstSent = msg.reportSent;
    qso.rstReceived = msg.reportReceived;

    // Exchange
    qso.exchangeSent = msg.exchangeSent;
    qso.exchangeReceived = msg.exchangeReceived;

    // Grid
    qso.gridSquare = msg.dxGrid;

    // Operator
    qso.operatorCall = msg.operatorCall;

    // Country lookup for DXCC enrichment
    if (countryFile && !qso.callsign.isEmpty()) {
        CountryData countryData = countryFile->lookup(qso.callsign);
        if (countryData.isValid()) {
            qso.dxccEntity = countryData.name;
            qso.dxccPrefix = countryData.primaryPrefix;
            qso.dxccEntityCode = countryData.dxccEntity;
            qso.cqZone = countryData.cqZone;
            qso.ituZone = countryData.ituZone;
            qso.continent = continentToString(countryData.continent);
        }
    }

    return qso;
}

} // namespace TR4QT
