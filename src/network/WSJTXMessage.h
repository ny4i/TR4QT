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

#ifndef WSJTXMESSAGE_H
#define WSJTXMESSAGE_H

#include <QByteArray>
#include <QColor>
#include <QDataStream>
#include <QDateTime>
#include <QString>
#include <QTime>

namespace TR4QT {

// WSJT-X protocol constants
namespace WSJTXProtocol {
    constexpr quint32 MAGIC_NUMBER = 0xadbccbda;
    constexpr quint32 SCHEMA_VERSION = 3;

    // QDataStream version MUST match WSJT-X (Qt 5.4 wire format).
    // Qt 6 defaults to Qt_6_0 which is binary-incompatible for QColor, QDateTime, etc.
    constexpr int QDATASTREAM_VERSION = QDataStream::Qt_5_4;
}

/**
 * Message types sent/received by WSJT-X.
 * Only types we care about are listed; others are silently ignored.
 */
enum class WSJTXMessageType : quint32 {
    Heartbeat    = 0,
    Status       = 1,
    Decode       = 2,
    Clear        = 3,
    Reply        = 4,   // We send this
    QSOLogged    = 5,
    Close        = 6,
    Replay       = 7,
    HaltTx       = 8,
    FreeText     = 9,
    WSPRDecode   = 10,
    Location     = 11,
    LoggedADIF   = 12,
    HighlightCallsign = 13,  // We send this
    SwitchConfig = 14,
    Configure    = 15,
    Unknown      = 0xFFFFFFFF
};

/**
 * Parsed message header (common to all WSJT-X messages).
 */
struct WSJTXHeader {
    quint32 magic{0};
    quint32 schema{0};
    WSJTXMessageType type{WSJTXMessageType::Unknown};
    QString id;  // WSJT-X instance identifier
};

/**
 * Heartbeat message (type 0).
 * Sent every 15 seconds by WSJT-X.
 */
struct WSJTXHeartbeat {
    QString id;
    quint32 maxSchema{0};
    QString version;
    QString revision;
};

/**
 * Status message (type 1).
 * Sent on every significant state change in WSJT-X.
 */
struct WSJTXStatus {
    QString id;
    quint64 dialFrequency{0};  // Hz
    QString mode;
    QString dxCall;
    QString report;
    QString txMode;
    bool txEnabled{false};
    bool transmitting{false};
    bool decoding{false};
    quint32 rxDF{0};
    quint32 txDF{0};
    QString deCall;
    QString deGrid;
    QString dxGrid;
    bool txWatchdog{false};
    QString subMode;
    bool fastMode{false};
    quint8 specialOpMode{0};
    quint32 freqTolerance{0};
    quint32 trPeriod{0};
    QString configName;
    QString txMessage;
};

/**
 * Decode message (type 2).
 * Sent for each decoded signal.
 */
struct WSJTXDecode {
    QString id;
    bool isNew{false};
    QTime time;
    qint32 snr{0};
    double deltaTime{0.0};
    quint32 deltaFreq{0};
    QString mode;
    QString message;
    bool lowConfidence{false};
    bool offAir{false};
};

/**
 * QSO Logged message (type 5).
 * Sent when user clicks "Log QSO" in WSJT-X.
 */
struct WSJTXQSOLogged {
    QString id;
    QDateTime dateTimeOff;
    QString dxCall;
    QString dxGrid;
    quint64 txFrequency{0};  // Hz
    QString mode;
    QString reportSent;
    QString reportReceived;
    QString txPower;
    QString comments;
    QString name;
    QDateTime dateTimeOn;
    QString operatorCall;
    QString myCall;
    QString myGrid;
    QString exchangeSent;
    QString exchangeReceived;
    QString adifPropMode;
};

/**
 * Logged ADIF message (type 12).
 * Contains full ADIF record text.
 */
struct WSJTXLoggedADIF {
    QString id;
    QString adifText;
};

/**
 * Static codec for WSJT-X binary protocol.
 *
 * CRITICAL: All QDataStream instances MUST use Qt_5_4 version.
 * WSJT-X uses QDataStream::Qt_5_4 (schema 3). Qt 6 defaults to Qt_6_0
 * which is binary-incompatible for QColor, QDateTime, etc.
 *
 * This class enforces the correct version in all factory methods.
 */
class WSJTXMessageCodec {
public:
    /**
     * Configure an existing QDataStream for WSJT-X protocol.
     * ALWAYS call this before reading/writing WSJT-X messages.
     */
    static void configureStream(QDataStream& stream) {
        stream.setVersion(WSJTXProtocol::QDATASTREAM_VERSION);
    }

    /**
     * Parse message header. Returns false if magic/schema invalid.
     * On success, the stream is positioned after the header (ready to read payload).
     */
    static bool parseHeader(QDataStream& stream, WSJTXHeader& header);

    /**
     * Parse specific message types from a stream positioned after the header.
     * Each method reads only the fields it knows about and silently ignores
     * any trailing data (forward compatibility per protocol spec).
     */
    static bool parseHeartbeat(QDataStream& stream, WSJTXHeartbeat& msg);
    static bool parseStatus(QDataStream& stream, WSJTXStatus& msg);
    static bool parseDecode(QDataStream& stream, WSJTXDecode& msg);
    static bool parseQSOLogged(QDataStream& stream, WSJTXQSOLogged& msg);
    static bool parseLoggedADIF(QDataStream& stream, WSJTXLoggedADIF& msg);

    /**
     * Build HighlightCallsign message (type 13) to send to WSJT-X.
     * @param id WSJT-X instance id (from heartbeat)
     * @param callsign Callsign to highlight
     * @param bgColor Background color
     * @param fgColor Foreground (text) color
     * @param highlightLast If true, highlight last CQ/QRZ/DE message
     * @return Serialized message bytes ready for UDP send
     */
    static QByteArray buildHighlightCallsign(const QString& id,
                                              const QString& callsign,
                                              const QColor& bgColor,
                                              const QColor& fgColor,
                                              bool highlightLast = true);

    /**
     * Build a "clear all highlights" message.
     * Per protocol: send callsign="", invalid background and foreground colors.
     */
    static QByteArray buildClearHighlights(const QString& id);

private:
    // Helper: read a UTF-8 string from QDataStream (WSJT-X wire format)
    static bool readUtf8(QDataStream& stream, QString& out);

    // Helper: write a UTF-8 string to QDataStream (WSJT-X wire format)
    static void writeUtf8(QDataStream& stream, const QString& str);
};

} // namespace TR4QT

#endif // WSJTXMESSAGE_H
