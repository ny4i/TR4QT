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
