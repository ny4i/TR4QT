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
    xml.writeStartElement("ContactInfo");

    // Application identity
    xml.writeTextElement("app", app);
    xml.writeTextElement("contestname", contestName);
    xml.writeTextElement("StationName", stationName);

    // Timestamp (ISO 8601 format)
    xml.writeTextElement("timestamp", timestamp);

    // Station identification
    xml.writeTextElement("mycall", mycall);
    xml.writeTextElement("call", call);

    // Frequency and mode
    xml.writeTextElement("freq", QString::number(freq, 'f', 0));
    xml.writeTextElement("band", band);
    xml.writeTextElement("mode", mode);

    // Radio number
    xml.writeTextElement("RadioNr", QString::number(radioNr));

    // Exchange
    xml.writeTextElement("rstSent", rstSent);
    xml.writeTextElement("rstRcvd", rstRcvd);
    xml.writeTextElement("exchangeSent", exchangeSent);
    xml.writeTextElement("exchangeRcvd", exchangeRcvd);

    // DXCC/Geographic info
    xml.writeTextElement("dxccPrefix", dxccPrefix);
    xml.writeTextElement("continent", continent);
    xml.writeTextElement("cqZone", QString::number(cqZone));
    xml.writeTextElement("ituZone", QString::number(ituZone));
    xml.writeTextElement("state", state);

    // Contest scoring
    xml.writeTextElement("points", QString::number(points));
    xml.writeTextElement("isDupe", isDupe ? "True" : "False");
    xml.writeTextElement("isMultiplier", isMultiplier ? "True" : "False");

    // Operator (if specified)
    if (!operator_.isEmpty()) {
        xml.writeTextElement("operator", operator_);
    }

    // Serial numbers (if used)
    if (serialNumber > 0) {
        xml.writeTextElement("serialNumber", QString::number(serialNumber));
    }
    if (serialNumberRcvd > 0) {
        xml.writeTextElement("serialNumberRcvd", QString::number(serialNumberRcvd));
    }

    // Unique identifier (GUID)
    if (!id.isEmpty()) {
        xml.writeTextElement("ID", id);
    }

    xml.writeEndElement(); // ContactInfo
    xml.writeEndDocument();

    return data;
}

} // namespace TR4QT
