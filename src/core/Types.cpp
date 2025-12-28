#include "Types.h"

namespace TR4QT {

QString bandToString(BandType band) {
    switch (band) {
        case BandType::Band160M: return "160M";
        case BandType::Band80M:  return "80M";
        case BandType::Band60M:  return "60M";
        case BandType::Band40M:  return "40M";
        case BandType::Band30M:  return "30M";
        case BandType::Band20M:  return "20M";
        case BandType::Band17M:  return "17M";
        case BandType::Band15M:  return "15M";
        case BandType::Band12M:  return "12M";
        case BandType::Band10M:  return "10M";
        case BandType::Band6M:   return "6M";
        case BandType::Band4M:   return "4M";
        case BandType::Band2M:   return "2M";
        case BandType::Band70CM: return "70CM";
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
    if (str == "160M") return BandType::Band160M;
    if (str == "80M")  return BandType::Band80M;
    if (str == "60M")  return BandType::Band60M;
    if (str == "40M")  return BandType::Band40M;
    if (str == "30M")  return BandType::Band30M;
    if (str == "20M")  return BandType::Band20M;
    if (str == "17M")  return BandType::Band17M;
    if (str == "15M")  return BandType::Band15M;
    if (str == "12M")  return BandType::Band12M;
    if (str == "10M")  return BandType::Band10M;
    if (str == "6M")   return BandType::Band6M;
    if (str == "4M")   return BandType::Band4M;
    if (str == "2M")   return BandType::Band2M;
    if (str == "70CM") return BandType::Band70CM;
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
        case BandType::Band70CM: return 420000;
        default: return 0;
    }
}

} // namespace TR4QT
