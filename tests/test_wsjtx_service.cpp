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
#include <QDateTime>
#include <QTimeZone>
#include "services/WSJTXService.h"

using namespace TR4QT;

class TestWSJTXService : public QObject {
    Q_OBJECT

private slots:
    // ── Mode mapping tests ──────────────────────────────────────────────

    void testMapModeFT8()
    {
        auto m = WSJTXService::mapMode("FT8");
        QCOMPARE(m.modeType, ModeType::FT8);
        QCOMPARE(m.adifMode, QString("FT8"));
        QCOMPARE(m.adifSubmode, QString("FT8"));
        QVERIFY(!m.rejected);
    }

    void testMapModeFT4()
    {
        auto m = WSJTXService::mapMode("FT4");
        QCOMPARE(m.modeType, ModeType::FT4);
        QCOMPARE(m.adifMode, QString("MFSK"));
        QCOMPARE(m.adifSubmode, QString("FT4"));
        QVERIFY(!m.rejected);
    }

    void testMapModeJT65()
    {
        auto m = WSJTXService::mapMode("JT65");
        QCOMPARE(m.modeType, ModeType::DATA);
        QCOMPARE(m.adifMode, QString("JT65"));
        QVERIFY(!m.rejected);
    }

    void testMapModeJT9()
    {
        auto m = WSJTXService::mapMode("JT9");
        QCOMPARE(m.modeType, ModeType::DATA);
        QCOMPARE(m.adifMode, QString("JT9"));
    }

    void testMapModeMSK144()
    {
        auto m = WSJTXService::mapMode("MSK144");
        QCOMPARE(m.modeType, ModeType::DATA);
        QCOMPARE(m.adifMode, QString("MSK144"));
    }

    void testMapModeQ65()
    {
        auto m = WSJTXService::mapMode("Q65");
        QCOMPARE(m.modeType, ModeType::DATA);
        QCOMPARE(m.adifMode, QString("Q65"));
    }

    void testMapModeFST4()
    {
        auto m = WSJTXService::mapMode("FST4");
        QCOMPARE(m.modeType, ModeType::DATA);
        QCOMPARE(m.adifMode, QString("FST4"));
        QVERIFY(!m.rejected);
    }

    void testMapModeFT2()
    {
        auto m = WSJTXService::mapMode("FT2");
        QCOMPARE(m.modeType, ModeType::DATA);
        QCOMPARE(m.adifMode, QString("MFSK"));
        QCOMPARE(m.adifSubmode, QString("FT2"));
    }

    void testMapModeWSPRRejected()
    {
        auto m = WSJTXService::mapMode("WSPR");
        QVERIFY(m.rejected);
    }

    void testMapModeFST4WRejected()
    {
        auto m = WSJTXService::mapMode("FST4W");
        QVERIFY(m.rejected);
    }

    void testMapModeCaseInsensitive()
    {
        auto m = WSJTXService::mapMode("ft8");
        QCOMPARE(m.modeType, ModeType::FT8);
    }

    void testMapModeUnknown()
    {
        auto m = WSJTXService::mapMode("NEWMODE");
        QCOMPARE(m.modeType, ModeType::DATA);
        QCOMPARE(m.adifMode, QString("NEWMODE"));
        QVERIFY(!m.rejected);
    }

    // ── Callsign extraction tests ───────────────────────────────────────

    void testExtractCallsignCQ()
    {
        auto info = WSJTXService::extractCallsign("CQ W1AW FN42");
        QVERIFY(info.valid);
        QVERIFY(info.isCQ);
        QCOMPARE(info.callsign, QString("W1AW"));
    }

    void testExtractCallsignCQDX()
    {
        auto info = WSJTXService::extractCallsign("CQ DX W1AW FN42");
        QVERIFY(info.valid);
        QVERIFY(info.isCQ);
        QCOMPARE(info.callsign, QString("W1AW"));
    }

    void testExtractCallsignCQContinent()
    {
        auto info = WSJTXService::extractCallsign("CQ NA W1AW FN42");
        QVERIFY(info.valid);
        QVERIFY(info.isCQ);
        QCOMPARE(info.callsign, QString("W1AW"));
    }

    void testExtractCallsignCQTest()
    {
        auto info = WSJTXService::extractCallsign("CQ TEST W1AW FN42");
        QVERIFY(info.valid);
        QVERIFY(info.isCQ);
        QCOMPARE(info.callsign, QString("W1AW"));
    }

    void testExtractCallsignResponse()
    {
        auto info = WSJTXService::extractCallsign("W1AW K3LR FN42");
        QVERIFY(info.valid);
        QVERIFY(!info.isCQ);
        QCOMPARE(info.callsign, QString("W1AW"));
    }

    void testExtractCallsignReport()
    {
        auto info = WSJTXService::extractCallsign("W1AW K3LR R-12");
        QVERIFY(info.valid);
        QCOMPARE(info.callsign, QString("W1AW"));
    }

    void testExtractCallsignRR73()
    {
        // Standalone RR73 — no callsign
        auto info = WSJTXService::extractCallsign("RR73");
        QVERIFY(!info.valid);
    }

    void testExtractCallsign73()
    {
        auto info = WSJTXService::extractCallsign("73");
        QVERIFY(!info.valid);
    }

    void testExtractCallsignRRR()
    {
        auto info = WSJTXService::extractCallsign("RRR");
        QVERIFY(!info.valid);
    }

    void testExtractCallsignEmpty()
    {
        auto info = WSJTXService::extractCallsign("");
        QVERIFY(!info.valid);
    }

    void testExtractCallsignCompound()
    {
        // Compound callsign with slash
        auto info = WSJTXService::extractCallsign("CQ DL/W1AW JN49");
        QVERIFY(info.valid);
        QCOMPARE(info.callsign, QString("DL/W1AW"));
    }

    void testExtractCallsignRR73InQSO()
    {
        // RR73 as part of a full QSO message (has callsigns before it)
        auto info = WSJTXService::extractCallsign("W1AW K3LR RR73");
        QVERIFY(info.valid);
        QCOMPARE(info.callsign, QString("W1AW"));
    }

    // ── QSO conversion tests ───────────────────────────────────────────

    void testConvertToQSOBasic()
    {
        WSJTXQSOLogged msg;
        msg.id = "WSJT-X";
        msg.dxCall = "K3LR";
        msg.dxGrid = "EN91";
        msg.txFrequency = 14074000;
        msg.mode = "FT8";
        msg.reportSent = "-12";
        msg.reportReceived = "-08";
        msg.exchangeSent = "05";
        msg.exchangeReceived = "03";
        msg.dateTimeOn = QDateTime(QDate(2026, 1, 15), QTime(14, 30, 0), QTimeZone::UTC);
        msg.dateTimeOff = QDateTime(QDate(2026, 1, 15), QTime(14, 31, 0), QTimeZone::UTC);
        msg.myCall = "NY4I";
        msg.operatorCall = "NY4I";

        QSO qso = WSJTXService::convertToQSO(msg, nullptr);

        QCOMPARE(qso.callsign, QString("K3LR"));
        QCOMPARE(qso.gridSquare, QString("EN91"));
        QCOMPARE(qso.frequency, static_cast<freq_t>(14074000));
        QCOMPARE(qso.band, BandType::Band20M);
        QCOMPARE(qso.mode, ModeType::FT8);
        QCOMPARE(qso.submode, QString("FT8"));
        QCOMPARE(qso.rstSent, QString("-12"));
        QCOMPARE(qso.rstReceived, QString("-08"));
        QCOMPARE(qso.exchangeSent, QString("05"));
        QCOMPARE(qso.exchangeReceived, QString("03"));
        QCOMPARE(qso.operatorCall, QString("NY4I"));
        QCOMPARE(qso.timestamp, msg.dateTimeOn);
        QVERIFY(!qso.guid.isEmpty());
    }

    void testConvertToQSOFT4()
    {
        WSJTXQSOLogged msg;
        msg.dxCall = "JA1ABC";
        msg.txFrequency = 7047500;
        msg.mode = "FT4";
        msg.reportSent = "-10";
        msg.reportReceived = "+05";
        msg.dateTimeOn = QDateTime::currentDateTimeUtc();

        QSO qso = WSJTXService::convertToQSO(msg, nullptr);

        QCOMPARE(qso.mode, ModeType::FT4);
        QCOMPARE(qso.submode, QString("FT4"));
        QCOMPARE(qso.band, BandType::Band40M);
    }

    void testConvertToQSO6m()
    {
        WSJTXQSOLogged msg;
        msg.dxCall = "VK3ABC";
        msg.txFrequency = 50313000;
        msg.mode = "FT8";
        msg.reportSent = "-20";
        msg.reportReceived = "-18";
        msg.dateTimeOn = QDateTime::currentDateTimeUtc();

        QSO qso = WSJTXService::convertToQSO(msg, nullptr);

        QCOMPARE(qso.band, BandType::Band6M);
    }

    void testConvertToQSOCallsignNormalized()
    {
        WSJTXQSOLogged msg;
        msg.dxCall = " k3lr ";  // lowercase with spaces
        msg.txFrequency = 14074000;
        msg.mode = "FT8";
        msg.dateTimeOn = QDateTime::currentDateTimeUtc();

        QSO qso = WSJTXService::convertToQSO(msg, nullptr);

        QCOMPARE(qso.callsign, QString("K3LR"));
    }
};

QTEST_GUILESS_MAIN(TestWSJTXService)
#include "test_wsjtx_service.moc"
