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

#include "MapDataProvider.h"
#include "../ui/models/QSOTableModel.h"
#include "../models/QSO.h"
#include <QJsonArray>
#include <QDateTime>
#include <QMap>

using TR4QT::QSO;
using TR4QT::QSOTableModel;

QJsonObject MapDataProvider::getWorkedSections(QSOTableModel* model) {
    if (!model) {
        return QJsonObject();
    }

    // Build map of sections → QSO count
    QMap<QString, int> sectionCounts;

    int qsoCount = model->count();
    for (int row = 0; row < qsoCount; ++row) {
        QSO qso = model->getQSO(row);
        QString section = qso.arrlSection.trimmed().toUpper();

        if (!section.isEmpty()) {
            sectionCounts[section]++;
        }
    }

    // Convert to JSON array
    QJsonArray sectionsArray;
    for (auto it = sectionCounts.begin(); it != sectionCounts.end(); ++it) {
        QJsonObject sectionObj;
        sectionObj["section"] = it.key();
        sectionObj["count"] = it.value();
        sectionsArray.append(sectionObj);
    }

    QJsonObject json;
    json["sections"] = sectionsArray;
    json["totalSections"] = sectionCounts.size();
    json["totalQsos"] = qsoCount;
    json["lastUpdate"] = QDateTime::currentDateTimeUtc().toString(Qt::ISODate);

    return json;
}

QJsonObject MapDataProvider::getWorkedStates(QSOTableModel* model) {
    if (!model) {
        return QJsonObject();
    }

    // Build map of states → QSO count
    QMap<QString, int> stateCounts;

    int qsoCount = model->count();
    for (int row = 0; row < qsoCount; ++row) {
        QSO qso = model->getQSO(row);
        QString state = qso.state.trimmed().toUpper();

        if (!state.isEmpty()) {
            stateCounts[state]++;
        }
    }

    // Convert to JSON array
    QJsonArray statesArray;
    for (auto it = stateCounts.begin(); it != stateCounts.end(); ++it) {
        QJsonObject stateObj;
        stateObj["state"] = it.key();
        stateObj["count"] = it.value();
        statesArray.append(stateObj);
    }

    QJsonObject json;
    json["states"] = statesArray;
    json["totalStates"] = stateCounts.size();
    json["totalQsos"] = qsoCount;
    json["lastUpdate"] = QDateTime::currentDateTimeUtc().toString(Qt::ISODate);

    return json;
}

QJsonObject MapDataProvider::getWorkedDXCCEntities(QSOTableModel* model) {
    if (!model) {
        return QJsonObject();
    }

    // Build map of DXCC prefix → QSO count
    QMap<QString, int> dxccCounts;

    int qsoCount = model->count();
    for (int row = 0; row < qsoCount; ++row) {
        QSO qso = model->getQSO(row);
        QString dxcc = qso.dxccPrefix.trimmed().toUpper();

        if (!dxcc.isEmpty()) {
            dxccCounts[dxcc]++;
        }
    }

    // Convert to JSON array
    QJsonArray entitiesArray;
    for (auto it = dxccCounts.begin(); it != dxccCounts.end(); ++it) {
        QJsonObject entityObj;
        entityObj["dxcc"] = it.key();
        entityObj["count"] = it.value();
        entitiesArray.append(entityObj);
    }

    QJsonObject json;
    json["entities"] = entitiesArray;
    json["totalEntities"] = dxccCounts.size();
    json["totalQsos"] = qsoCount;
    json["lastUpdate"] = QDateTime::currentDateTimeUtc().toString(Qt::ISODate);

    return json;
}
