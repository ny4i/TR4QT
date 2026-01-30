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

#include "ContactInfo.h"
#include <QXmlStreamWriter>

namespace TR4QT {

ContactInfo::ContactInfo()
{
}

QByteArray ContactInfo::toXml() const
{
    QByteArray data;
    QXmlStreamWriter xml(&data);

    xml.writeStartDocument();
    xml.writeStartElement("contactinfo");

    // === N1MM+ ContactInfo fields in spec order ===
    // All fields included per N1MM+ spec, blank if no data

    // Application identity
    xml.writeTextElement("app", app);
    xml.writeTextElement("contestname", contestName);  // ADIF Contest-ID
    xml.writeTextElement("contestnr", QString::number(contestNr));  // WA7BNM Contest Calendar ID

    // Timestamp (N1MM+ format: "YYYY-MM-DD HH:MM:SS")
    xml.writeTextElement("timestamp", timestamp);

    // Station identification
    xml.writeTextElement("mycall", mycall);
    xml.writeTextElement("band", band);
    xml.writeTextElement("rxfreq", QString::number(rxfreq));  // RX freq in tens of Hz
    xml.writeTextElement("txfreq", QString::number(txfreq));  // TX freq in tens of Hz
    xml.writeTextElement("operator", operator_);
    xml.writeTextElement("mode", mode);

    // Contact information
    xml.writeTextElement("call", call);
    xml.writeTextElement("countryprefix", dxccPrefix);  // N1MM+ name for DXCC prefix
    xml.writeTextElement("wpxprefix", wpxPrefix);       // WPX prefix (e.g., "W1")
    xml.writeTextElement("stationprefix", stationPrefix);  // Station prefix
    xml.writeTextElement("continent", continent);
    xml.writeTextElement("snt", rstSent);               // N1MM+ name for RST sent
    xml.writeTextElement("sntnr", QString::number(serialNumber));  // Sent serial number
    xml.writeTextElement("rcv", rstRcvd);               // N1MM+ name for RST received
    xml.writeTextElement("rcvnr", QString::number(serialNumberRcvd));  // Received serial number
    xml.writeTextElement("gridsquare", gridsquare);     // Maidenhead grid
    xml.writeTextElement("exchange1", exchangeRcvd);    // Primary exchange received
    xml.writeTextElement("section", section);           // ARRL section
    xml.writeTextElement("comment", comment);           // Comment field
    xml.writeTextElement("qth", qth);                   // Worked station's QTH
    xml.writeTextElement("name", name);                 // Worked station's name
    xml.writeTextElement("power", power);               // Worked station's power
    xml.writeTextElement("misctext", misctext);         // Miscellaneous text
    xml.writeTextElement("zone", QString::number(cqZone));  // N1MM+ name for CQ zone
    xml.writeTextElement("prec", prec);                 // Sweepstakes precedence
    xml.writeTextElement("ck", ck);                     // Sweepstakes check
    xml.writeTextElement("ismultiplier1", isMultiplier ? "1" : "0");  // Primary multiplier
    xml.writeTextElement("ismultiplier2", isMultiplier2 ? "1" : "0"); // Secondary multiplier
    xml.writeTextElement("ismultiplier3", isMultiplier3 ? "1" : "0"); // Tertiary multiplier
    xml.writeTextElement("points", QString::number(points));
    xml.writeTextElement("radionr", QString::number(radioNr));  // lowercase per N1MM+ spec
    xml.writeTextElement("run1run2", run1run2);         // Run/S&P indicator
    xml.writeTextElement("RoverLocation", roverLocation);  // Rover grid
    xml.writeTextElement("RadioInterfaced", radioInterfaced);  // Radio interfaced flag
    xml.writeTextElement("NetworkedCompNr", QString::number(networkedCompNr));  // Networked computer number
    xml.writeTextElement("IsOriginal", isOriginal ? "True" : "False");  // Original QSO flag
    xml.writeTextElement("NetBiosName", netBiosName);   // Computer name
    xml.writeTextElement("IsRunQSO", isRunQSO ? "1" : "0");  // Run mode QSO
    xml.writeTextElement("StationName", stationName);   // Station name
    xml.writeTextElement("ID", id);                     // GUID (no hyphens)
    xml.writeTextElement("IsClaimedQso", isClaimedQso ? "1" : "0");  // Claimed QSO flag

    // Additional fields (not always present in spec but useful)
    xml.writeTextElement("oldtimestamp", oldtimestamp);  // Original timestamp (for edits)
    xml.writeTextElement("oldcall", oldcall);            // Original call (for edits)

    // Fields we track but not in N1MM+ spec (keep for compatibility)
    if (ituZone > 0) {
        xml.writeTextElement("ituzone", QString::number(ituZone));  // ITU zone (TR4QT extension)
    }
    if (!state.isEmpty()) {
        xml.writeTextElement("state", state);  // US state (TR4QT extension)
    }
    xml.writeTextElement("isdupe", isDupe ? "True" : "False");  // Duplicate flag

    xml.writeEndElement(); // contactinfo
    xml.writeEndDocument();

    return data;
}

} // namespace TR4QT
