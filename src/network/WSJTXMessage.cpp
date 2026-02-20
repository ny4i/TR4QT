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

#include "WSJTXMessage.h"
#include <QIODevice>

namespace TR4QT {

// ─── String helpers ─────────────────────────────────────────────────────────
// WSJT-X encodes strings as: quint32 length + UTF-8 bytes.
// A length of 0xFFFFFFFF means null/empty string.

bool WSJTXMessageCodec::readUtf8(QDataStream& stream, QString& out)
{
    quint32 len;
    stream >> len;
    if (stream.status() != QDataStream::Ok) return false;

    if (len == 0xFFFFFFFF) {
        out.clear();
        return true;
    }

    if (len == 0) {
        out.clear();
        return true;
    }

    QByteArray raw(len, Qt::Uninitialized);
    if (stream.readRawData(raw.data(), len) != static_cast<int>(len))
        return false;

    out = QString::fromUtf8(raw);
    return true;
}

void WSJTXMessageCodec::writeUtf8(QDataStream& stream, const QString& str)
{
    if (str.isNull()) {
        stream << static_cast<quint32>(0xFFFFFFFF);
        return;
    }
    QByteArray utf8 = str.toUtf8();
    stream << static_cast<quint32>(utf8.size());
    stream.writeRawData(utf8.constData(), utf8.size());
}

// ─── Header parsing ─────────────────────────────────────────────────────────

bool WSJTXMessageCodec::parseHeader(QDataStream& stream, WSJTXHeader& header)
{
    stream >> header.magic;
    if (stream.status() != QDataStream::Ok) return false;
    if (header.magic != WSJTXProtocol::MAGIC_NUMBER) return false;

    stream >> header.schema;
    if (stream.status() != QDataStream::Ok) return false;

    quint32 typeVal;
    stream >> typeVal;
    if (stream.status() != QDataStream::Ok) return false;
    header.type = static_cast<WSJTXMessageType>(typeVal);

    if (!readUtf8(stream, header.id)) return false;

    return true;
}

// ─── Heartbeat (type 0) ─────────────────────────────────────────────────────

bool WSJTXMessageCodec::parseHeartbeat(QDataStream& stream, WSJTXHeartbeat& msg)
{
    stream >> msg.maxSchema;
    if (stream.status() != QDataStream::Ok) return false;

    // version and revision are optional (added in later protocol revisions)
    if (!stream.atEnd()) {
        if (!readUtf8(stream, msg.version)) return false;
    }
    if (!stream.atEnd()) {
        if (!readUtf8(stream, msg.revision)) return false;
    }

    return true;
}

// ─── Status (type 1) ────────────────────────────────────────────────────────

bool WSJTXMessageCodec::parseStatus(QDataStream& stream, WSJTXStatus& msg)
{
    stream >> msg.dialFrequency;
    if (stream.status() != QDataStream::Ok) return false;

    if (!readUtf8(stream, msg.mode)) return false;
    if (!readUtf8(stream, msg.dxCall)) return false;
    if (!readUtf8(stream, msg.report)) return false;
    if (!readUtf8(stream, msg.txMode)) return false;

    // bool fields are encoded as QDataStream bool (1 byte)
    stream >> msg.txEnabled;
    if (stream.status() != QDataStream::Ok) return false;

    stream >> msg.transmitting;
    if (stream.status() != QDataStream::Ok) return false;

    stream >> msg.decoding;
    if (stream.status() != QDataStream::Ok) return false;

    stream >> msg.rxDF;
    if (stream.status() != QDataStream::Ok) return false;

    stream >> msg.txDF;
    if (stream.status() != QDataStream::Ok) return false;

    if (!readUtf8(stream, msg.deCall)) return false;
    if (!readUtf8(stream, msg.deGrid)) return false;
    if (!readUtf8(stream, msg.dxGrid)) return false;

    stream >> msg.txWatchdog;
    if (stream.status() != QDataStream::Ok) return false;

    if (!readUtf8(stream, msg.subMode)) return false;

    stream >> msg.fastMode;
    if (stream.status() != QDataStream::Ok) return false;

    // Optional fields added in later schema revisions
    if (!stream.atEnd()) {
        stream >> msg.specialOpMode;
        if (stream.status() != QDataStream::Ok) return false;
    }
    if (!stream.atEnd()) {
        stream >> msg.freqTolerance;
        if (stream.status() != QDataStream::Ok) return false;
    }
    if (!stream.atEnd()) {
        stream >> msg.trPeriod;
        if (stream.status() != QDataStream::Ok) return false;
    }
    if (!stream.atEnd()) {
        if (!readUtf8(stream, msg.configName)) return false;
    }
    if (!stream.atEnd()) {
        if (!readUtf8(stream, msg.txMessage)) return false;
    }

    return true;
}

// ─── Decode (type 2) ────────────────────────────────────────────────────────

bool WSJTXMessageCodec::parseDecode(QDataStream& stream, WSJTXDecode& msg)
{
    stream >> msg.isNew;
    if (stream.status() != QDataStream::Ok) return false;

    stream >> msg.time;
    if (stream.status() != QDataStream::Ok) return false;

    stream >> msg.snr;
    if (stream.status() != QDataStream::Ok) return false;

    stream >> msg.deltaTime;
    if (stream.status() != QDataStream::Ok) return false;

    stream >> msg.deltaFreq;
    if (stream.status() != QDataStream::Ok) return false;

    if (!readUtf8(stream, msg.mode)) return false;
    if (!readUtf8(stream, msg.message)) return false;

    stream >> msg.lowConfidence;
    if (stream.status() != QDataStream::Ok) return false;

    // offAir is optional
    if (!stream.atEnd()) {
        stream >> msg.offAir;
    }

    return true;
}

// ─── QSO Logged (type 5) ────────────────────────────────────────────────────

bool WSJTXMessageCodec::parseQSOLogged(QDataStream& stream, WSJTXQSOLogged& msg)
{
    stream >> msg.dateTimeOff;
    if (stream.status() != QDataStream::Ok) return false;

    if (!readUtf8(stream, msg.dxCall)) return false;
    if (!readUtf8(stream, msg.dxGrid)) return false;

    stream >> msg.txFrequency;
    if (stream.status() != QDataStream::Ok) return false;

    if (!readUtf8(stream, msg.mode)) return false;
    if (!readUtf8(stream, msg.reportSent)) return false;
    if (!readUtf8(stream, msg.reportReceived)) return false;
    if (!readUtf8(stream, msg.txPower)) return false;
    if (!readUtf8(stream, msg.comments)) return false;
    if (!readUtf8(stream, msg.name)) return false;

    stream >> msg.dateTimeOn;
    if (stream.status() != QDataStream::Ok) return false;

    if (!readUtf8(stream, msg.operatorCall)) return false;
    if (!readUtf8(stream, msg.myCall)) return false;
    if (!readUtf8(stream, msg.myGrid)) return false;

    // Optional exchange fields
    if (!stream.atEnd()) {
        if (!readUtf8(stream, msg.exchangeSent)) return false;
    }
    if (!stream.atEnd()) {
        if (!readUtf8(stream, msg.exchangeReceived)) return false;
    }
    if (!stream.atEnd()) {
        if (!readUtf8(stream, msg.adifPropMode)) return false;
    }

    return true;
}

// ─── Logged ADIF (type 12) ──────────────────────────────────────────────────

bool WSJTXMessageCodec::parseLoggedADIF(QDataStream& stream, WSJTXLoggedADIF& msg)
{
    if (!readUtf8(stream, msg.adifText)) return false;
    return true;
}

// ─── Build HighlightCallsign (type 13) ──────────────────────────────────────

QByteArray WSJTXMessageCodec::buildHighlightCallsign(const QString& id,
                                                       const QString& callsign,
                                                       const QColor& bgColor,
                                                       const QColor& fgColor,
                                                       bool highlightLast)
{
    QByteArray buffer;
    QDataStream stream(&buffer, QIODevice::WriteOnly);
    configureStream(stream);

    // Header
    stream << WSJTXProtocol::MAGIC_NUMBER;
    stream << WSJTXProtocol::SCHEMA_VERSION;
    stream << static_cast<quint32>(WSJTXMessageType::HighlightCallsign);
    writeUtf8(stream, id);

    // Payload
    writeUtf8(stream, callsign);
    stream << bgColor;
    stream << fgColor;
    stream << highlightLast;

    return buffer;
}

// ─── Build Clear Highlights ─────────────────────────────────────────────────

QByteArray WSJTXMessageCodec::buildClearHighlights(const QString& id)
{
    // Per WSJT-X protocol: send empty callsign with invalid colors to clear all
    return buildHighlightCallsign(id, QString{}, QColor{}, QColor{}, false);
}

} // namespace TR4QT
