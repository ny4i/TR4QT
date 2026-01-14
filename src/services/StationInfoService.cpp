/**
 * @file StationInfoService.cpp
 * @brief Implementation of StationInfoService
 *
 * Phase 7 extraction from MainWindow.
 */

#include "StationInfoService.h"
#include "../utils/GeographicUtils.h"
#include "../utils/ThemeManager.h"
#include "../contests/ContestBase.h"
#include "../models/QSO.h"
#include <QDate>
#include <cmath>

namespace TR4QT {

StationInfoService::StationInfoService(const CountryFile* countryFile)
    : m_countryFile(countryFile)
{
}

StationInfoResult StationInfoService::calculateStationInfo(
    const QString& callsign,
    const QString& myGrid,
    bool useMetric) const
{
    StationInfoResult result;

    if (!m_countryFile) {
        return result;
    }

    // Lookup country data from CTY.DAT
    CountryData countryData = m_countryFile->lookup(callsign);
    if (!countryData.isValid()) {
        return result;
    }

    result.valid = true;
    result.countryName = countryData.name;
    result.primaryPrefix = countryData.primaryPrefix;

    // If no grid square configured, return basic info
    if (myGrid.isEmpty()) {
        result.displayInfo = countryData.primaryPrefix;
        return result;
    }

    // Convert my grid square to lat/lon
    double myLat, myLon;
    if (!GeographicUtils::gridToLatLon(myGrid, myLat, myLon)) {
        result.displayInfo = countryData.primaryPrefix;
        return result;
    }

    // Calculate target coordinates
    // For US mainland callsigns (DXCC 291), use call area grid squares (more precise)
    if (CountryFile::getUSCallAreaCoordinates(callsign, countryData.dxccEntity,
                                               result.targetLat, result.targetLon)) {
        // US mainland callsign - use call area center grid
    } else {
        // Non-US or Alaska/Hawaii - use country center from CTY.DAT
        result.targetLat = countryData.latitude;
        result.targetLon = countryData.longitude;
    }

    // Calculate distance and bearing
    result.distance = GeographicUtils::haversineDistance(
        myLat, myLon, result.targetLat, result.targetLon, useMetric);
    result.bearing = GeographicUtils::calculateBearing(
        myLat, myLon, result.targetLat, result.targetLon);

    // Calculate sunrise/sunset for DX station
    QDate today = QDate::currentDate();
    QTime dxSunrise = GeographicUtils::calculateSunrise(result.targetLat, result.targetLon, today);
    QTime dxSunset = GeographicUtils::calculateSunset(result.targetLat, result.targetLon, today);

    // Calculate sunrise/sunset for Home station
    QTime homeSunrise = GeographicUtils::calculateSunrise(myLat, myLon, today);
    QTime homeSunset = GeographicUtils::calculateSunset(myLat, myLon, today);

    // Check grayline status
    QDateTime now = QDateTime::currentDateTimeUtc();
    QTime currentTime = now.time();

    // Check which specific times are in grayline
    result.homeInGrayline = GeographicUtils::isInGraylineWindow(
        now, homeSunrise, homeSunset, GRAYLINE_WINDOW_MINUTES);

    // Check DX sunrise grayline
    bool dxSunriseInGrayline = false;
    if (dxSunrise.isValid()) {
        int secondsToSunrise = currentTime.secsTo(dxSunrise);
        dxSunriseInGrayline = (std::abs(secondsToSunrise) <= GRAYLINE_WINDOW_MINUTES * 60);
    }

    // Check DX sunset grayline
    bool dxSunsetInGrayline = false;
    if (dxSunset.isValid()) {
        int secondsToSunset = currentTime.secsTo(dxSunset);
        dxSunsetInGrayline = (std::abs(secondsToSunset) <= GRAYLINE_WINDOW_MINUTES * 60);
    }

    result.dxInGrayline = dxSunriseInGrayline || dxSunsetInGrayline;
    result.doubleGrayline = result.homeInGrayline && result.dxInGrayline;

    // Format display string: "PREFIX  BEARING°  DISTANCE  SR/SS"
    QString distUnit = useMetric ? "km" : "mi";
    QString info = QString("%1  %2°  %3%4")
        .arg(countryData.primaryPrefix, -6)
        .arg(static_cast<int>(result.bearing), 3)
        .arg(static_cast<int>(result.distance), 4)
        .arg(distUnit);

    // Add sunrise/sunset times if valid (with rich text for grayline highlighting)
    if (dxSunrise.isValid() && dxSunset.isValid()) {
        QString srText = dxSunrise.toString("HH:mm") + "z";
        QString ssText = dxSunset.toString("HH:mm") + "z";

        // Color highlight if in grayline (orange for enhanced propagation)
        QString graylineColor = ThemeManager::instance().colorName(ColorRole::AgingSpotText);

        if (dxSunriseInGrayline) {
            srText = QString("<span style='color:%1;font-weight:bold;'>%2</span>")
                .arg(graylineColor).arg(srText);
            result.tooltip = "DX station in sunrise grayline window (enhanced propagation)";
        }

        if (dxSunsetInGrayline) {
            ssText = QString("<span style='color:%1;font-weight:bold;'>%2</span>")
                .arg(graylineColor).arg(ssText);
            if (!result.tooltip.isEmpty()) {
                result.tooltip = "DX station in sunrise/sunset grayline window (enhanced propagation)";
            } else {
                result.tooltip = "DX station in sunset grayline window (enhanced propagation)";
            }
        }

        info += "  " + srText + "/" + ssText;
    }

    // Add grayline indicators
    if (result.doubleGrayline) {
        info += "  ⚡DOUBLE⚡";
        result.tooltip = "Both home and DX stations in grayline window (exceptional propagation!)";
    } else if (result.homeInGrayline) {
        info += "  [HOME GRAYLINE]";
        if (result.tooltip.isEmpty()) {
            result.tooltip = "Home station in grayline window (enhanced propagation)";
        } else {
            result.tooltip += " + Home station also in grayline";
        }
    }

    result.displayInfo = info;
    return result;
}

SCPDisplayResult StationInfoService::formatSCPMatches(
    const QStringList& matches,
    const QSet<QString>& workedCallsigns,
    BandType currentBand,
    ModeType currentMode,
    std::function<bool(const QString&, BandType, ModeType, QString&)> dupeChecker) const
{
    SCPDisplayResult result;
    result.totalMatches = matches.size();

    if (matches.isEmpty()) {
        return result;
    }

    // Get colors for different states from ThemeManager
    QString dupeColorStr = ThemeManager::instance().colorName(ColorRole::DupeText);
    QString workedColorStr = ThemeManager::instance().colorName(ColorRole::WorkedStationText);
    QString notWorkedColorStr = ThemeManager::instance().colorName(ColorRole::MultiplierText);

    // Sort matches: worked/dupe calls FIRST, then not-worked calls
    QStringList workedMatches;
    QStringList notWorkedMatches;

    for (const QString& match : matches) {
        if (workedCallsigns.contains(match)) {
            workedMatches.append(match);
            result.workedCount++;
        } else {
            notWorkedMatches.append(match);
        }
    }

    // Combine: worked calls first, then not-worked
    QStringList sortedMatches = workedMatches + notWorkedMatches;

    // Format matches in 2 columns with color coding:
    // RED = duplicate on current band/mode
    // GRAY = worked but not a duplicate
    // BLUE = not worked yet (only in MASTER.SCP)
    QStringList rows;
    for (int i = 0; i < sortedMatches.size(); i += 2) {
        QString dupeInfo;
        bool isWorked1 = workedCallsigns.contains(sortedMatches[i]);
        bool isDupe1 = isWorked1 && dupeChecker(sortedMatches[i], currentBand, currentMode, dupeInfo);

        if (isDupe1) result.dupeCount++;

        QString color1;
        if (isDupe1) {
            color1 = dupeColorStr;
        } else if (isWorked1) {
            color1 = workedColorStr;
        } else {
            color1 = notWorkedColorStr;
        }

        QString call1 = QString("<span style='color: %1;'>%2</span>")
            .arg(color1)
            .arg(sortedMatches[i]);

        QString row = call1;

        // Check second match if exists
        if (i + 1 < sortedMatches.size()) {
            bool isWorked2 = workedCallsigns.contains(sortedMatches[i + 1]);
            bool isDupe2 = isWorked2 && dupeChecker(sortedMatches[i + 1], currentBand, currentMode, dupeInfo);

            if (isDupe2) result.dupeCount++;

            QString color2;
            if (isDupe2) {
                color2 = dupeColorStr;
            } else if (isWorked2) {
                color2 = workedColorStr;
            } else {
                color2 = notWorkedColorStr;
            }

            QString call2 = QString("<span style='color: %1;'>%2</span>")
                .arg(color2)
                .arg(sortedMatches[i + 1]);
            row += "  " + call2;
        }

        rows.append(row);
    }

    result.htmlContent = rows.join("<br>");
    return result;
}

QString StationInfoService::getMultiplierValueForCallsign(
    const QString& callsign,
    const ContestBase* contest) const
{
    if (!contest || !m_countryFile || callsign.isEmpty()) {
        return QString();
    }

    // Build a temporary QSO and populate DXCC fields
    QSO tempQso;
    tempQso.callsign = callsign;
    m_countryFile->populateQSODXCCFields(tempQso);

    if (tempQso.dxccEntity.isEmpty()) {
        return QString();
    }

    // Get the contest's primary multiplier type
    QList<MultiplierDefinition> multDefs = contest->getMultiplierTypes();
    if (multDefs.isEmpty()) {
        return QString();
    }

    // Use the first multiplier type (most contests have Country or Zone as primary)
    MultiplierType primaryMultType = multDefs.first().type;

    // Get the multiplier value
    return contest->getMultiplierValue(tempQso, primaryMultType, QStringList());
}

} // namespace TR4QT
