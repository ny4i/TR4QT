#ifndef RADIOENUMERATOR_H
#define RADIOENUMERATOR_H

#include <QList>
#include <QString>
#include <hamlib/rig.h>

namespace TR4QT {

/**
 * Information about an available radio model from hamlib
 */
struct RadioModelInfo {
    int modelId;
    QString manufacturer;
    QString modelName;
    QString status;  // "Stable", "Beta", "Alpha", "Untested"

    QString displayName() const {
        return QString("%1 %2").arg(manufacturer).arg(modelName);
    }
};

/**
 * Utility class to enumerate available radios from hamlib
 */
class RadioEnumerator {
public:
    /**
     * Get list of all available radio models from hamlib
     * Sorted by manufacturer, then model name
     */
    static QList<RadioModelInfo> getAvailableRadios();

private:
    // Callback for rig_list_foreach
    static int enumerateCallback(const struct rig_caps* caps, rig_ptr_t data);

    // Convert hamlib status enum to string
    static QString statusToString(enum rig_status_e status);
};

} // namespace TR4QT

#endif // RADIOENUMERATOR_H
