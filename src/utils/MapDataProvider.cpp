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
