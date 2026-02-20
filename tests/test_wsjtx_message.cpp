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

#include <QTest>
#include <QColor>
#include <QDateTime>
#include <QTimeZone>
#include "network/WSJTXMessage.h"

using namespace TR4QT;

/**
 * Tests for WSJT-X binary protocol codec.
 *
 * Strategy: Build packets using the same QDataStream/Qt_5_4 encoding that
 * WSJT-X uses, then verify our parser extracts every field correctly.
 * Also test round-tripping our build* methods.
 */
class TestWSJTXMessage : public QObject {
    Q_OBJECT

private:
    // Helper: build a raw packet with correct header + payload via lambda
    QByteArray buildPacket(WSJTXMessageType type, const QString& id,
                           std::function<void(QDataStream&)> writePayload)
    {
        QByteArray buf;
        QDataStream stream(&buf, QIODevice::WriteOnly);
        WSJTXMessageCodec::configureStream(stream);

        // Header
        stream << WSJTXProtocol::MAGIC_NUMBER;
        stream << WSJTXProtocol::SCHEMA_VERSION;
        stream << static_cast<quint32>(type);
        writeUtf8(stream, id);

        // Payload
        if (writePayload)
            writePayload(stream);

        return buf;
    }

    // Mirror of WSJTXMessageCodec's private writeUtf8 for test packet construction
    static void writeUtf8(QDataStream& stream, const QString& str)
    {
        if (str.isNull()) {
            stream << static_cast<quint32>(0xFFFFFFFF);
            return;
        }
        QByteArray utf8 = str.toUtf8();
        stream << static_cast<quint32>(utf8.size());
        stream.writeRawData(utf8.constData(), utf8.size());
    }

private slots:
    // ── Header tests ────────────────────────────────────────────────────

    void testParseHeaderValid()
    {
        QByteArray pkt = buildPacket(WSJTXMessageType::Heartbeat, "WSJT-X", nullptr);
        QDataStream stream(pkt);
        WSJTXMessageCodec::configureStream(stream);

        WSJTXHeader hdr;
        QVERIFY(WSJTXMessageCodec::parseHeader(stream, hdr));
        QCOMPARE(hdr.magic, WSJTXProtocol::MAGIC_NUMBER);
        QCOMPARE(hdr.schema, WSJTXProtocol::SCHEMA_VERSION);
        QCOMPARE(hdr.type, WSJTXMessageType::Heartbeat);
        QCOMPARE(hdr.id, QString("WSJT-X"));
    }

    void testParseHeaderBadMagic()
    {
        QByteArray buf;
        QDataStream stream(&buf, QIODevice::WriteOnly);
        WSJTXMessageCodec::configureStream(stream);
        stream << static_cast<quint32>(0xDEADBEEF);  // wrong magic
        stream << WSJTXProtocol::SCHEMA_VERSION;
        stream << static_cast<quint32>(0);
        writeUtf8(stream, "WSJT-X");

        QDataStream readStream(buf);
        WSJTXMessageCodec::configureStream(readStream);
        WSJTXHeader hdr;
        QVERIFY(!WSJTXMessageCodec::parseHeader(readStream, hdr));
    }

    void testParseHeaderTruncated()
    {
        QByteArray buf;
        QDataStream stream(&buf, QIODevice::WriteOnly);
        WSJTXMessageCodec::configureStream(stream);
        stream << WSJTXProtocol::MAGIC_NUMBER;
        // No schema, type, or id — truncated

        QDataStream readStream(buf);
        WSJTXMessageCodec::configureStream(readStream);
        WSJTXHeader hdr;
        QVERIFY(!WSJTXMessageCodec::parseHeader(readStream, hdr));
    }

    // ── Heartbeat tests ─────────────────────────────────────────────────

    void testParseHeartbeat()
    {
        QByteArray pkt = buildPacket(WSJTXMessageType::Heartbeat, "WSJT-X",
            [](QDataStream& s) {
                s << static_cast<quint32>(3);             // maxSchema
                writeUtf8(s, "2.7.0-rc4");                // version
                writeUtf8(s, "abc123");                    // revision
            });

        QDataStream stream(pkt);
        WSJTXMessageCodec::configureStream(stream);
        WSJTXHeader hdr;
        QVERIFY(WSJTXMessageCodec::parseHeader(stream, hdr));
        QCOMPARE(hdr.type, WSJTXMessageType::Heartbeat);

        WSJTXHeartbeat msg;
        msg.id = hdr.id;
        QVERIFY(WSJTXMessageCodec::parseHeartbeat(stream, msg));
        QCOMPARE(msg.id, QString("WSJT-X"));
        QCOMPARE(msg.maxSchema, 3u);
        QCOMPARE(msg.version, QString("2.7.0-rc4"));
        QCOMPARE(msg.revision, QString("abc123"));
    }

    void testParseHeartbeatMinimal()
    {
        // Older WSJT-X versions may not send version/revision
        QByteArray pkt = buildPacket(WSJTXMessageType::Heartbeat, "WSJT-X",
            [](QDataStream& s) {
                s << static_cast<quint32>(2);  // maxSchema only
            });

        QDataStream stream(pkt);
        WSJTXMessageCodec::configureStream(stream);
        WSJTXHeader hdr;
        QVERIFY(WSJTXMessageCodec::parseHeader(stream, hdr));

        WSJTXHeartbeat msg;
        msg.id = hdr.id;
        QVERIFY(WSJTXMessageCodec::parseHeartbeat(stream, msg));
        QCOMPARE(msg.maxSchema, 2u);
        QVERIFY(msg.version.isEmpty());
        QVERIFY(msg.revision.isEmpty());
    }

    // ── Status tests ────────────────────────────────────────────────────

    void testParseStatus()
    {
        QByteArray pkt = buildPacket(WSJTXMessageType::Status, "WSJT-X",
            [](QDataStream& s) {
                s << static_cast<quint64>(14074000);  // dialFrequency Hz
                writeUtf8(s, "FT8");                   // mode
                writeUtf8(s, "K3LR");                  // dxCall
                writeUtf8(s, "-12");                   // report
                writeUtf8(s, "FT8");                   // txMode
                s << true;                             // txEnabled
                s << false;                            // transmitting
                s << true;                             // decoding
                s << static_cast<quint32>(1500);       // rxDF
                s << static_cast<quint32>(1500);       // txDF
                writeUtf8(s, "NY4I");                  // deCall
                writeUtf8(s, "EM85");                  // deGrid
                writeUtf8(s, "EN91");                  // dxGrid
                s << false;                            // txWatchdog
                writeUtf8(s, "");                      // subMode
                s << false;                            // fastMode
                s << static_cast<quint8>(0);           // specialOpMode
                s << static_cast<quint32>(500);        // freqTolerance
                s << static_cast<quint32>(15);         // trPeriod
                writeUtf8(s, "Default");               // configName
                writeUtf8(s, "CQ NY4I EM85");          // txMessage
            });

        QDataStream stream(pkt);
        WSJTXMessageCodec::configureStream(stream);
        WSJTXHeader hdr;
        QVERIFY(WSJTXMessageCodec::parseHeader(stream, hdr));

        WSJTXStatus msg;
        msg.id = hdr.id;
        QVERIFY(WSJTXMessageCodec::parseStatus(stream, msg));
        QCOMPARE(msg.dialFrequency, static_cast<quint64>(14074000));
        QCOMPARE(msg.mode, QString("FT8"));
        QCOMPARE(msg.dxCall, QString("K3LR"));
        QCOMPARE(msg.report, QString("-12"));
        QCOMPARE(msg.txMode, QString("FT8"));
        QVERIFY(msg.txEnabled);
        QVERIFY(!msg.transmitting);
        QVERIFY(msg.decoding);
        QCOMPARE(msg.rxDF, 1500u);
        QCOMPARE(msg.txDF, 1500u);
        QCOMPARE(msg.deCall, QString("NY4I"));
        QCOMPARE(msg.deGrid, QString("EM85"));
        QCOMPARE(msg.dxGrid, QString("EN91"));
        QVERIFY(!msg.txWatchdog);
        QVERIFY(msg.subMode.isEmpty());
        QVERIFY(!msg.fastMode);
        QCOMPARE(msg.specialOpMode, static_cast<quint8>(0));
        QCOMPARE(msg.freqTolerance, 500u);
        QCOMPARE(msg.trPeriod, 15u);
        QCOMPARE(msg.configName, QString("Default"));
        QCOMPARE(msg.txMessage, QString("CQ NY4I EM85"));
    }

    void testParseStatusMinimalFields()
    {
        // Status without optional trailing fields (older WSJT-X)
        QByteArray pkt = buildPacket(WSJTXMessageType::Status, "WSJT-X",
            [](QDataStream& s) {
                s << static_cast<quint64>(7074000);
                writeUtf8(s, "FT8");
                writeUtf8(s, "");
                writeUtf8(s, "");
                writeUtf8(s, "FT8");
                s << false;    // txEnabled
                s << false;    // transmitting
                s << false;    // decoding
                s << static_cast<quint32>(0);
                s << static_cast<quint32>(0);
                writeUtf8(s, "NY4I");
                writeUtf8(s, "EM85");
                writeUtf8(s, "");
                s << false;    // txWatchdog
                writeUtf8(s, "");
                s << false;    // fastMode
                // No specialOpMode, freqTolerance, trPeriod, configName, txMessage
            });

        QDataStream stream(pkt);
        WSJTXMessageCodec::configureStream(stream);
        WSJTXHeader hdr;
        QVERIFY(WSJTXMessageCodec::parseHeader(stream, hdr));

        WSJTXStatus msg;
        msg.id = hdr.id;
        QVERIFY(WSJTXMessageCodec::parseStatus(stream, msg));
        QCOMPARE(msg.dialFrequency, static_cast<quint64>(7074000));
        QCOMPARE(msg.specialOpMode, static_cast<quint8>(0));
    }

    // ── Decode tests ────────────────────────────────────────────────────

    void testParseDecode()
    {
        QByteArray pkt = buildPacket(WSJTXMessageType::Decode, "WSJT-X",
            [](QDataStream& s) {
                s << true;                                  // isNew
                s << QTime(12, 30, 15);                     // time
                s << static_cast<qint32>(-14);              // snr
                s << 0.3;                                   // deltaTime
                s << static_cast<quint32>(1234);            // deltaFreq
                writeUtf8(s, "~");                          // mode (FT8 = "~")
                writeUtf8(s, "CQ W1AW FN42");              // message
                s << false;                                 // lowConfidence
                s << false;                                 // offAir
            });

        QDataStream stream(pkt);
        WSJTXMessageCodec::configureStream(stream);
        WSJTXHeader hdr;
        QVERIFY(WSJTXMessageCodec::parseHeader(stream, hdr));

        WSJTXDecode msg;
        msg.id = hdr.id;
        QVERIFY(WSJTXMessageCodec::parseDecode(stream, msg));
        QVERIFY(msg.isNew);
        QCOMPARE(msg.time, QTime(12, 30, 15));
        QCOMPARE(msg.snr, -14);
        QVERIFY(qAbs(msg.deltaTime - 0.3) < 0.001);
        QCOMPARE(msg.deltaFreq, 1234u);
        QCOMPARE(msg.mode, QString("~"));
        QCOMPARE(msg.message, QString("CQ W1AW FN42"));
        QVERIFY(!msg.lowConfidence);
        QVERIFY(!msg.offAir);
    }

    void testParseDecodeWithoutOffAir()
    {
        // Older versions don't send offAir
        QByteArray pkt = buildPacket(WSJTXMessageType::Decode, "WSJT-X",
            [](QDataStream& s) {
                s << false;
                s << QTime(1, 2, 3);
                s << static_cast<qint32>(5);
                s << 0.0;
                s << static_cast<quint32>(800);
                writeUtf8(s, "+");
                writeUtf8(s, "K3LR NY4I EM85");
                s << false;
                // no offAir
            });

        QDataStream stream(pkt);
        WSJTXMessageCodec::configureStream(stream);
        WSJTXHeader hdr;
        QVERIFY(WSJTXMessageCodec::parseHeader(stream, hdr));

        WSJTXDecode msg;
        msg.id = hdr.id;
        QVERIFY(WSJTXMessageCodec::parseDecode(stream, msg));
        QCOMPARE(msg.message, QString("K3LR NY4I EM85"));
        QVERIFY(!msg.offAir);  // default
    }

    // ── QSO Logged tests ────────────────────────────────────────────────

    void testParseQSOLogged()
    {
        QDateTime dtOn = QDateTime(QDate(2026, 1, 15), QTime(14, 30, 0), QTimeZone::UTC);
        QDateTime dtOff = QDateTime(QDate(2026, 1, 15), QTime(14, 31, 15), QTimeZone::UTC);

        QByteArray pkt = buildPacket(WSJTXMessageType::QSOLogged, "WSJT-X",
            [&](QDataStream& s) {
                s << dtOff;
                writeUtf8(s, "K3LR");
                writeUtf8(s, "EN91");
                s << static_cast<quint64>(14074000);
                writeUtf8(s, "FT8");
                writeUtf8(s, "-12");
                writeUtf8(s, "-08");
                writeUtf8(s, "50");
                writeUtf8(s, "");           // comments
                writeUtf8(s, "");           // name
                s << dtOn;
                writeUtf8(s, "NY4I");       // operatorCall
                writeUtf8(s, "NY4I");       // myCall
                writeUtf8(s, "EM85");       // myGrid
                writeUtf8(s, "05");         // exchangeSent
                writeUtf8(s, "03");         // exchangeReceived
                writeUtf8(s, "");           // adifPropMode
            });

        QDataStream stream(pkt);
        WSJTXMessageCodec::configureStream(stream);
        WSJTXHeader hdr;
        QVERIFY(WSJTXMessageCodec::parseHeader(stream, hdr));

        WSJTXQSOLogged msg;
        msg.id = hdr.id;
        QVERIFY(WSJTXMessageCodec::parseQSOLogged(stream, msg));
        QCOMPARE(msg.dxCall, QString("K3LR"));
        QCOMPARE(msg.dxGrid, QString("EN91"));
        QCOMPARE(msg.txFrequency, static_cast<quint64>(14074000));
        QCOMPARE(msg.mode, QString("FT8"));
        QCOMPARE(msg.reportSent, QString("-12"));
        QCOMPARE(msg.reportReceived, QString("-08"));
        QCOMPARE(msg.txPower, QString("50"));
        QCOMPARE(msg.myCall, QString("NY4I"));
        QCOMPARE(msg.myGrid, QString("EM85"));
        QCOMPARE(msg.exchangeSent, QString("05"));
        QCOMPARE(msg.exchangeReceived, QString("03"));
        QCOMPARE(msg.dateTimeOn, dtOn);
        QCOMPARE(msg.dateTimeOff, dtOff);
    }

    void testParseQSOLoggedWithoutExchanges()
    {
        // WSJT-X older versions may not send exchange fields
        QDateTime dt = QDateTime(QDate(2026, 2, 1), QTime(20, 0, 0), QTimeZone::UTC);

        QByteArray pkt = buildPacket(WSJTXMessageType::QSOLogged, "WSJT-X",
            [&](QDataStream& s) {
                s << dt;
                writeUtf8(s, "JA1ABC");
                writeUtf8(s, "PM95");
                s << static_cast<quint64>(7074000);
                writeUtf8(s, "FT8");
                writeUtf8(s, "-15");
                writeUtf8(s, "-10");
                writeUtf8(s, "100");
                writeUtf8(s, "comment");
                writeUtf8(s, "Taro");
                s << dt;
                writeUtf8(s, "NY4I");
                writeUtf8(s, "NY4I");
                writeUtf8(s, "EM85");
                // No exchange fields
            });

        QDataStream stream(pkt);
        WSJTXMessageCodec::configureStream(stream);
        WSJTXHeader hdr;
        QVERIFY(WSJTXMessageCodec::parseHeader(stream, hdr));

        WSJTXQSOLogged msg;
        msg.id = hdr.id;
        QVERIFY(WSJTXMessageCodec::parseQSOLogged(stream, msg));
        QCOMPARE(msg.dxCall, QString("JA1ABC"));
        QCOMPARE(msg.name, QString("Taro"));
        QVERIFY(msg.exchangeSent.isEmpty());
        QVERIFY(msg.exchangeReceived.isEmpty());
    }

    // ── Logged ADIF tests ───────────────────────────────────────────────

    void testParseLoggedADIF()
    {
        QString adif = "<call:4>K3LR<band:3>20m<mode:3>FT8<eor>";
        QByteArray pkt = buildPacket(WSJTXMessageType::LoggedADIF, "WSJT-X",
            [&](QDataStream& s) {
                writeUtf8(s, adif);
            });

        QDataStream stream(pkt);
        WSJTXMessageCodec::configureStream(stream);
        WSJTXHeader hdr;
        QVERIFY(WSJTXMessageCodec::parseHeader(stream, hdr));

        WSJTXLoggedADIF msg;
        msg.id = hdr.id;
        QVERIFY(WSJTXMessageCodec::parseLoggedADIF(stream, msg));
        QCOMPARE(msg.adifText, adif);
    }

    // ── HighlightCallsign build tests ───────────────────────────────────

    void testBuildHighlightCallsign()
    {
        QColor bg(255, 0, 0);       // Red
        QColor fg(255, 255, 255);   // White

        QByteArray pkt = WSJTXMessageCodec::buildHighlightCallsign(
            "WSJT-X", "K3LR", bg, fg, true);

        // Parse it back and verify header
        QDataStream stream(pkt);
        WSJTXMessageCodec::configureStream(stream);
        WSJTXHeader hdr;
        QVERIFY(WSJTXMessageCodec::parseHeader(stream, hdr));
        QCOMPARE(hdr.type, WSJTXMessageType::HighlightCallsign);
        QCOMPARE(hdr.id, QString("WSJT-X"));

        // Read payload fields manually
        // Callsign
        quint32 len;
        stream >> len;
        QCOMPARE(len, 4u);
        QByteArray callRaw(len, Qt::Uninitialized);
        stream.readRawData(callRaw.data(), len);
        QCOMPARE(QString::fromUtf8(callRaw), QString("K3LR"));

        // Background color
        QColor readBg;
        stream >> readBg;
        QCOMPARE(readBg, bg);

        // Foreground color
        QColor readFg;
        stream >> readFg;
        QCOMPARE(readFg, fg);

        // highlightLast
        bool hl;
        stream >> hl;
        QVERIFY(hl);
    }

    void testBuildClearHighlights()
    {
        QByteArray pkt = WSJTXMessageCodec::buildClearHighlights("WSJT-X");

        QDataStream stream(pkt);
        WSJTXMessageCodec::configureStream(stream);
        WSJTXHeader hdr;
        QVERIFY(WSJTXMessageCodec::parseHeader(stream, hdr));
        QCOMPARE(hdr.type, WSJTXMessageType::HighlightCallsign);

        // Null callsign (0xFFFFFFFF = null string in WSJT-X protocol)
        quint32 len;
        stream >> len;
        QCOMPARE(len, static_cast<quint32>(0xFFFFFFFF));

        // Invalid colors (QColor default-constructed = invalid)
        QColor bg, fg;
        stream >> bg;
        stream >> fg;
        QVERIFY(!bg.isValid());
        QVERIFY(!fg.isValid());
    }

    // ── QDataStream version enforcement ─────────────────────────────────

    void testStreamVersionIsQt54()
    {
        QByteArray buf;
        QDataStream writeStream(&buf, QIODevice::WriteOnly);
        WSJTXMessageCodec::configureStream(writeStream);
        QCOMPARE(writeStream.version(), static_cast<int>(QDataStream::Qt_5_4));

        QDataStream readStream(buf);
        WSJTXMessageCodec::configureStream(readStream);
        QCOMPARE(readStream.version(), static_cast<int>(QDataStream::Qt_5_4));
    }

    // ── UTF-8 edge cases ────────────────────────────────────────────────

    void testNullStringEncoding()
    {
        // Null string should encode as 0xFFFFFFFF length
        QByteArray pkt = buildPacket(WSJTXMessageType::LoggedADIF, "WSJT-X",
            [](QDataStream& s) {
                writeUtf8(s, QString());  // null QString
            });

        QDataStream stream(pkt);
        WSJTXMessageCodec::configureStream(stream);
        WSJTXHeader hdr;
        QVERIFY(WSJTXMessageCodec::parseHeader(stream, hdr));

        WSJTXLoggedADIF msg;
        msg.id = hdr.id;
        QVERIFY(WSJTXMessageCodec::parseLoggedADIF(stream, msg));
        QVERIFY(msg.adifText.isEmpty());
    }

    void testUnicodeCallsign()
    {
        // Callsigns are ASCII but test UTF-8 robustness with comments
        QString unicodeComment = "QSO with José in México";
        QByteArray pkt = buildPacket(WSJTXMessageType::LoggedADIF, "WSJT-X",
            [&](QDataStream& s) {
                writeUtf8(s, unicodeComment);
            });

        QDataStream stream(pkt);
        WSJTXMessageCodec::configureStream(stream);
        WSJTXHeader hdr;
        QVERIFY(WSJTXMessageCodec::parseHeader(stream, hdr));

        WSJTXLoggedADIF msg;
        msg.id = hdr.id;
        QVERIFY(WSJTXMessageCodec::parseLoggedADIF(stream, msg));
        QCOMPARE(msg.adifText, unicodeComment);
    }

    // ── Unknown message type handling ───────────────────────────────────

    void testUnknownMessageType()
    {
        QByteArray pkt = buildPacket(static_cast<WSJTXMessageType>(99), "WSJT-X", nullptr);

        QDataStream stream(pkt);
        WSJTXMessageCodec::configureStream(stream);
        WSJTXHeader hdr;
        QVERIFY(WSJTXMessageCodec::parseHeader(stream, hdr));
        QCOMPARE(hdr.type, static_cast<WSJTXMessageType>(99));
        // Parser should succeed — unknown types are just ignored by caller
    }
};

QTEST_GUILESS_MAIN(TestWSJTXMessage)
#include "test_wsjtx_message.moc"
