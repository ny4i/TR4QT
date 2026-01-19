#include "Types.h"

namespace TR4QT {

QString bandToString(BandType band) {
    switch (band) {
        case BandType::Band160M: return "160m";
        case BandType::Band80M:  return "80m";
        case BandType::Band60M:  return "60m";
        case BandType::Band40M:  return "40m";
        case BandType::Band30M:  return "30m";
        case BandType::Band20M:  return "20m";
        case BandType::Band17M:  return "17m";
        case BandType::Band15M:  return "15m";
        case BandType::Band12M:  return "12m";
        case BandType::Band10M:  return "10m";
        case BandType::Band6M:   return "6m";
        case BandType::Band4M:   return "4m";
        case BandType::Band2M:   return "2m";
        case BandType::Band1_25M: return "1.25m";
        case BandType::Band70CM: return "70cm";
        case BandType::Band23CM: return "23cm";
        default: return "Unknown";
    }
}

QString modeToString(ModeType mode) {
    switch (mode) {
        case ModeType::CW:    return "CW";
        case ModeType::CWR:   return "CW-R";
        case ModeType::LSB:   return "LSB";
        case ModeType::USB:   return "USB";
        case ModeType::FM:    return "FM";
        case ModeType::AM:    return "AM";
        case ModeType::RTTY:  return "RTTY";
        case ModeType::RTTYR: return "RTTY-R";
        case ModeType::PSK:   return "PSK";
        case ModeType::PSKR:  return "PSK-R";
        case ModeType::FT8:   return "FT8";
        case ModeType::FT4:   return "FT4";
        case ModeType::DATA:  return "DATA";
        case ModeType::DATAR: return "DATA-R";
        default: return "Unknown";
    }
}

QString modeGroupToString(ModeGroup group) {
    switch (group) {
        case ModeGroup::Phone:   return "Phone";
        case ModeGroup::CW:      return "CW";
        case ModeGroup::Digital: return "Digital";
        default: return "Unknown";
    }
}

ModeGroup modeTypeToModeGroup(ModeType mode) {
    switch (mode) {
        case ModeType::CW:
        case ModeType::CWR:
            return ModeGroup::CW;

        case ModeType::LSB:
        case ModeType::USB:
        case ModeType::FM:
        case ModeType::AM:
            return ModeGroup::Phone;

        case ModeType::RTTY:
        case ModeType::RTTYR:
        case ModeType::PSK:
        case ModeType::PSKR:
        case ModeType::FT8:
        case ModeType::FT4:
        case ModeType::DATA:
        case ModeType::DATAR:
            return ModeGroup::Digital;

        default:
            return ModeGroup::Phone;  // Default to phone
    }
}

BandType stringToBand(const QString& str) {
    // Support both uppercase (legacy) and lowercase (ADIF) formats
    QString upper = str.toUpper();
    if (upper == "160M") return BandType::Band160M;
    if (upper == "80M")  return BandType::Band80M;
    if (upper == "60M")  return BandType::Band60M;
    if (upper == "40M")  return BandType::Band40M;
    if (upper == "30M")  return BandType::Band30M;
    if (upper == "20M")  return BandType::Band20M;
    if (upper == "17M")  return BandType::Band17M;
    if (upper == "15M")  return BandType::Band15M;
    if (upper == "12M")  return BandType::Band12M;
    if (upper == "10M")  return BandType::Band10M;
    if (upper == "6M")   return BandType::Band6M;
    if (upper == "4M")   return BandType::Band4M;
    if (upper == "2M")   return BandType::Band2M;
    if (upper == "1.25M") return BandType::Band1_25M;
    if (upper == "70CM") return BandType::Band70CM;
    if (upper == "23CM") return BandType::Band23CM;
    return BandType::None;
}

ModeType stringToMode(const QString& str) {
    if (str == "CW")     return ModeType::CW;
    if (str == "CW-R")   return ModeType::CWR;
    if (str == "LSB")    return ModeType::LSB;
    if (str == "USB")    return ModeType::USB;
    if (str == "FM")     return ModeType::FM;
    if (str == "AM")     return ModeType::AM;
    if (str == "RTTY")   return ModeType::RTTY;
    if (str == "RTTY-R") return ModeType::RTTYR;
    if (str == "PSK")    return ModeType::PSK;
    if (str == "PSK-R")  return ModeType::PSKR;
    if (str == "FT8")    return ModeType::FT8;
    if (str == "FT4")    return ModeType::FT4;
    if (str == "DATA")   return ModeType::DATA;
    if (str == "DATA-R") return ModeType::DATAR;
    return ModeType::None;
}

QString continentToString(Continent cont) {
    switch (cont) {
        case Continent::AF: return "AF";
        case Continent::AS: return "AS";
        case Continent::EU: return "EU";
        case Continent::NA: return "NA";
        case Continent::SA: return "SA";
        case Continent::OC: return "OC";
        default: return "";
    }
}

unsigned long bandToBaseFrequency(BandType band) {
    // Returns base frequency in kHz (band edge for CW portion)
    switch (band) {
        case BandType::Band160M: return 1800;
        case BandType::Band80M:  return 3500;
        case BandType::Band60M:  return 5330;
        case BandType::Band40M:  return 7000;
        case BandType::Band30M:  return 10100;
        case BandType::Band20M:  return 14000;
        case BandType::Band17M:  return 18068;
        case BandType::Band15M:  return 21000;
        case BandType::Band12M:  return 24890;
        case BandType::Band10M:  return 28000;
        case BandType::Band6M:   return 50000;
        case BandType::Band4M:   return 70000;
        case BandType::Band2M:   return 144000;
        case BandType::Band1_25M: return 222000;
        case BandType::Band70CM: return 420000;
        case BandType::Band23CM: return 1240000;
        default: return 0;
    }
}

BandType frequencyToBand(unsigned long frequencyHz) {
    // Convert frequency in Hz to BandType
    // Uses standard amateur radio band allocations

    if (frequencyHz >= 1800000 && frequencyHz <= 2000000) return BandType::Band160M;
    if (frequencyHz >= 3500000 && frequencyHz <= 4000000) return BandType::Band80M;
    if (frequencyHz >= 5330000 && frequencyHz <= 5405000) return BandType::Band60M;
    if (frequencyHz >= 7000000 && frequencyHz <= 7300000) return BandType::Band40M;
    if (frequencyHz >= 10100000 && frequencyHz <= 10150000) return BandType::Band30M;
    if (frequencyHz >= 14000000 && frequencyHz <= 14350000) return BandType::Band20M;
    if (frequencyHz >= 18068000 && frequencyHz <= 18168000) return BandType::Band17M;
    if (frequencyHz >= 21000000 && frequencyHz <= 21450000) return BandType::Band15M;
    if (frequencyHz >= 24890000 && frequencyHz <= 24990000) return BandType::Band12M;
    if (frequencyHz >= 28000000 && frequencyHz <= 29700000) return BandType::Band10M;
    if (frequencyHz >= 50000000 && frequencyHz <= 54000000) return BandType::Band6M;
    if (frequencyHz >= 70000000 && frequencyHz <= 71000000) return BandType::Band4M;
    if (frequencyHz >= 144000000 && frequencyHz <= 148000000) return BandType::Band2M;
    if (frequencyHz >= 222000000 && frequencyHz <= 225000000) return BandType::Band1_25M;
    if (frequencyHz >= 420000000 && frequencyHz <= 450000000) return BandType::Band70CM;
    if (frequencyHz >= 1240000000 && frequencyHz <= 1300000000) return BandType::Band23CM;

    return BandType::None;
}

} // namespace TR4QT
