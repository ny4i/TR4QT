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

#include "RadioInfo.h"
#include <QXmlStreamWriter>

namespace TR4QT {

RadioInfo::RadioInfo()
{
}

QByteArray RadioInfo::toXml() const
{
    QByteArray data;
    QXmlStreamWriter xml(&data);

    xml.writeStartDocument();
    xml.writeStartElement("RadioInfo");

    // Application identity
    xml.writeTextElement("app", app);
    xml.writeTextElement("StationName", stationName);

    // Radio identification
    xml.writeTextElement("RadioNr", QString::number(radioNr));
    xml.writeTextElement("RadioName", radioName);

    // Frequencies (in tens of Hz)
    xml.writeTextElement("Freq", QString::number(freq, 'f', 0));
    xml.writeTextElement("TXFreq", QString::number(txFreq, 'f', 0));

    // Operating parameters
    xml.writeTextElement("Mode", mode);
    xml.writeTextElement("mycall", mycall);
    xml.writeTextElement("OpCall", opCall);

    // Status flags (N1MM+ expects "True"/"False" strings)
    xml.writeTextElement("IsRunning", isRunning ? "True" : "False");
    xml.writeTextElement("IsTransmitting", isTransmitting ? "True" : "False");
    xml.writeTextElement("IsSplit", isSplit ? "True" : "False");
    xml.writeTextElement("IsStereo", isStereo ? "True" : "False");
    xml.writeTextElement("IsConnected", isConnected ? "True" : "False");
    xml.writeTextElement("IsRunMode", isRunMode ? "True" : "False");

    // UI state (for N1MM+ compatibility)
    xml.writeTextElement("FocusEntry", QString::number(focusEntry));
    xml.writeTextElement("EntryWindowHwnd", QString::number(entryWindowHwnd));
    xml.writeTextElement("FocusRadioNr", QString::number(focusRadioNr));
    xml.writeTextElement("ActiveRadioNr", QString::number(activeRadioNr));
    xml.writeTextElement("FunctionKeyCaption", functionKeyCaption);

    // Antenna/Rotor
    xml.writeTextElement("Antenna", QString::number(antenna));
    xml.writeTextElement("Rotors", rotors);
    xml.writeTextElement("AuxAntSelected", QString::number(auxAntSelected));
    xml.writeTextElement("AuxAntSelectedName", auxAntSelectedName);

    xml.writeEndElement(); // RadioInfo
    xml.writeEndDocument();

    return data;
}

int RadioInfo::hzToTensOfHz(freq_t hz)
{
    // Convert Hz to tens of Hz
    // Example: 14,025,000 Hz → 1,402,500 tens of Hz
    return static_cast<int>(hz / 10);
}

freq_t RadioInfo::tensOfHzToHz(int tensOfHz)
{
    // Convert tens of Hz back to Hz
    // Example: 1,402,500 tens of Hz → 14,025,000 Hz
    return static_cast<freq_t>(tensOfHz) * 10;
}

int RadioInfo::mhzToTensOfHz(double mhz)
{
    // Convert MHz to tens of Hz
    // Example: 14.025 MHz → 1,402,500 tens of Hz
    return static_cast<int>(mhz * 100000.0);
}

double RadioInfo::tensOfHzToMhz(int tensOfHz)
{
    // Convert tens of Hz to MHz
    // Example: 1,402,500 tens of Hz → 14.025 MHz
    return tensOfHz / 100000.0;
}

} // namespace TR4QT
