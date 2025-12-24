#include "RadioEnumerator.h"
#include "../logging/LogMacros.h"
#include <QMap>
#include <algorithm>

namespace TR4QT {

QString RadioEnumerator::statusToString(enum rig_status_e status) {
    switch (status) {
    case RIG_STATUS_ALPHA:
        return "Alpha";
    case RIG_STATUS_BETA:
        return "Beta";
    case RIG_STATUS_STABLE:
        return "Stable";
    case RIG_STATUS_UNTESTED:
        return "Untested";
    default:
        return "Unknown";
    }
}

int RadioEnumerator::enumerateCallback(const struct rig_caps* caps, rig_ptr_t data) {
    if (!caps || !data) {
        return -1;
    }

    QList<RadioModelInfo>* radios = static_cast<QList<RadioModelInfo>*>(data);

    // Filter out dummy/test rigs and utility backends
    if (caps->rig_model == RIG_MODEL_NONE ||
        caps->rig_model == RIG_MODEL_DUMMY ||
        caps->rig_model == RIG_MODEL_NETRIGCTL ||
        caps->rig_model == RIG_MODEL_DUMMY_NOVFO) {
        // These are intentionally filtered - don't log
        return 1;  // Continue enumeration
    }

    // Check for NULL manufacturer or model name
    if (!caps->model_name || !caps->mfg_name) {
        LOG_WARN("RadioEnumerator", QString("Filtering radio %1 - NULL data: mfg=%2 model=%3")
                 .arg(caps->rig_model)
                 .arg(caps->mfg_name ? caps->mfg_name : "NULL")
                 .arg(caps->model_name ? caps->model_name : "NULL"));
        return 1;  // Continue enumeration
    }

    RadioModelInfo info;
    info.modelId = caps->rig_model;
    info.manufacturer = QString::fromLatin1(caps->mfg_name);
    info.modelName = QString::fromLatin1(caps->model_name);
    info.status = statusToString(caps->status);

    radios->append(info);

    return 1;  // Continue enumeration
}

QList<RadioModelInfo> RadioEnumerator::getAvailableRadios() {
    QList<RadioModelInfo> radios;

    // Call hamlib to enumerate all registered rigs
    int result = rig_list_foreach(enumerateCallback, &radios);

    if (result != RIG_OK) {
        LOG_WARN("RadioEnumerator", QString("Failed to enumerate radios from hamlib: %1").arg(result));
    }

    // Sort by manufacturer, then model name
    std::sort(radios.begin(), radios.end(),
        [](const RadioModelInfo& a, const RadioModelInfo& b) {
            if (a.manufacturer != b.manufacturer) {
                return a.manufacturer < b.manufacturer;
            }
            return a.modelName < b.modelName;
        });

    LOG_DEBUG("RadioEnumerator", QString("Enumerated %1 radios from hamlib").arg(radios.size()));

    // Log manufacturer counts for diagnostic purposes
    QMap<QString, int> mfgCounts;
    for (const RadioModelInfo& radio : radios) {
        mfgCounts[radio.manufacturer]++;
    }

    // Log counts for major manufacturers
    QStringList majorMfgs = {"Icom", "Yaesu", "Kenwood", "Elecraft", "FlexRadio"};
    for (const QString& mfg : majorMfgs) {
        if (mfgCounts.contains(mfg)) {
            LOG_DEBUG("RadioEnumerator", QString("  - %1: %2 radios").arg(mfg).arg(mfgCounts[mfg]));
        } else {
            LOG_DEBUG("RadioEnumerator", QString("  - %1: 0 radios (NOT FOUND)").arg(mfg));
        }
    }

    return radios;
}

} // namespace TR4QT
