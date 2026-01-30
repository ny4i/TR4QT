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

#include "ADIFExporter.h"
#include "../core/Types.h"
#include "../core/Constants.h"
#include <QFile>
#include <QTextStream>
#include <QDateTime>

namespace TR4QT {

bool ADIFExporter::exportToFile(const QList<QSO>& qsos,
                                const QString& filePath,
                                ContestBase* contest,
                                const QString& operatorCall) {
    m_lastError.clear();

    // Generate ADIF text
    QString adifText = generateADIF(qsos, contest, operatorCall);

    // Write to file
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        m_lastError = QString("Failed to open file for writing: %1").arg(file.errorString());
        return false;
    }

    QTextStream out(&file);
    out << adifText;
    file.close();

    return true;
}

QString ADIFExporter::generateADIF(const QList<QSO>& qsos,
                                  ContestBase* contest,
                                  const QString& operatorCall) {
    QString result;
    QTextStream stream(&result);

    // Write header
    stream << generateHeader();

    stream << "<EOH>\n\n";

    // Get official ADIF contest ID from contest instance
    QString adifContestId;
    if (contest) {
        adifContestId = contest->getADIFContestId();
    }

    // Write each QSO (CONTEST_ID goes in each record)
    for (const QSO& qso : qsos) {
        if (!qso.isDupe) {  // Skip dupes in export
            stream << formatQSO(qso, adifContestId) << "\n";
        }
    }

    return result;
}

QString ADIFExporter::generateHeader() {
    QString header;
    QTextStream stream(&header);

    stream << "ADIF Export from " << APP_NAME << " v" << APP_VERSION << "\n";
    stream << "<ADIF_VER:5>3.1.4\n";
    stream << "<CREATED_TIMESTAMP:15>" << QDateTime::currentDateTimeUtc().toString("yyyyMMdd HHmmss") << "\n";
    stream << "<PROGRAMID:" << QString(APP_NAME).length() << ">" << APP_NAME << "\n";
    stream << "<PROGRAMVERSION:" << QString(APP_VERSION).length() << ">" << APP_VERSION << "\n";

    return header;
}

QString ADIFExporter::formatQSO(const QSO& qso, const QString& contestId) {
    QString result;
    QTextStream stream(&result);

    // Callsign (required)
    stream << formatField("CALL", qso.callsign);

    // Date and time (required)
    QString qsoDate = qso.timestamp.toUTC().toString("yyyyMMdd");
    QString qsoTime = qso.timestamp.toUTC().toString("HHmmss");
    stream << formatField("QSO_DATE", qsoDate);
    stream << formatField("TIME_ON", qsoTime);

    // Contest ID (official ADIF contest name)
    if (!contestId.isEmpty()) {
        stream << formatField("CONTEST_ID", contestId);
    }

    // Band
    stream << formatField("BAND", bandToString(qso.band).remove('M').toLower() + "m");

    // Mode and Submode (ADIF spec: some modes are parent/child pairs)
    // e.g., MODE=SSB SUBMODE=USB, MODE=MFSK SUBMODE=FT4
    QString adifMode;
    QString adifSubmode;
    switch (qso.mode) {
    case ModeType::CW:    adifMode = "CW";   break;
    case ModeType::CWR:   adifMode = "CW";   adifSubmode = "CW-R"; break;
    case ModeType::USB:   adifMode = "SSB";   adifSubmode = "USB";  break;
    case ModeType::LSB:   adifMode = "SSB";   adifSubmode = "LSB";  break;
    case ModeType::FM:    adifMode = "FM";    break;
    case ModeType::AM:    adifMode = "AM";    break;
    case ModeType::RTTY:  adifMode = "RTTY";  break;
    case ModeType::RTTYR: adifMode = "RTTY";  adifSubmode = "RTTY-R"; break;
    case ModeType::PSK:   adifMode = "PSK";   break;
    case ModeType::PSKR:  adifMode = "PSK";   adifSubmode = "PSK-R";  break;
    case ModeType::FT8:   adifMode = "MFSK";  adifSubmode = "FT8";  break;
    case ModeType::FT4:   adifMode = "MFSK";  adifSubmode = "FT4";  break;
    case ModeType::DATA:  adifMode = "DATA";  break;
    case ModeType::DATAR: adifMode = "DATA";  adifSubmode = "DATA-R"; break;
    default:              adifMode = modeToString(qso.mode); break;
    }
    stream << formatField("MODE", adifMode);

    // Write SUBMODE: prefer stored submode from import, fall back to derived
    if (!qso.submode.isEmpty()) {
        stream << formatField("SUBMODE", qso.submode);
    } else if (!adifSubmode.isEmpty()) {
        stream << formatField("SUBMODE", adifSubmode);
    }

    // Frequency (in MHz)
    if (qso.frequency > 0) {
        double freqMhz = qso.frequency / 1000000.0;
        stream << formatField("FREQ", QString::number(freqMhz, 'f', 6));
    }

    // RST sent and received
    stream << formatField("RST_SENT", qso.rstSent);
    stream << formatField("RST_RCVD", qso.rstReceived);

    // Exchange sent and received
    if (!qso.exchangeSent.isEmpty()) {
        stream << formatField("STX_STRING", qso.exchangeSent);
    }
    if (!qso.exchangeReceived.isEmpty()) {
        stream << formatField("SRX_STRING", qso.exchangeReceived);
    }

    // DXCC and zone information (use ADIF DXCC Entity Code enumeration)
    if (qso.dxccEntityCode > 0) {
        stream << formatField("DXCC", QString::number(qso.dxccEntityCode));
    }
    if (qso.cqZone > 0) {
        stream << formatField("CQZ", QString::number(qso.cqZone));
    }
    if (qso.ituZone > 0) {
        stream << formatField("ITUZ", QString::number(qso.ituZone));
    }
    if (!qso.state.isEmpty()) {
        stream << formatField("STATE", qso.state);
    }

    // Contest class (e.g., "1O", "2I" for Winter Field Day)
    if (!qso.contestClass.isEmpty()) {
        stream << formatField("CLASS", qso.contestClass);
    }

    // ARRL/RAC section (e.g., "WMA", "WCF")
    if (!qso.arrlSection.isEmpty()) {
        stream << formatField("ARRL_SECT", qso.arrlSection);
    }

    // Operator
    if (!qso.operatorCall.isEmpty()) {
        stream << formatField("OPERATOR", qso.operatorCall);
    }

    // QSO points (contest-specific)
    if (qso.qsoPoints > 0) {
        stream << formatField("POINTS", QString::number(qso.qsoPoints));
    }

    // End of record
    stream << "<EOR>\n";

    return result;
}

QString ADIFExporter::formatField(const QString& fieldName, const QString& value) {
    if (value.isEmpty()) {
        return QString();
    }

    // ADIF field format: <FIELD_NAME:LENGTH>VALUE
    return QString("<%1:%2>%3 ").arg(fieldName).arg(value.length()).arg(value);
}

} // namespace TR4QT
