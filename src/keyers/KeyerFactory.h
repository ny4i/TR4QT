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

#ifndef KEYERFACTORY_H
#define KEYERFACTORY_H

#include <QString>
#include <QStringList>
#include "KeyerConfig.h"

namespace TR4QT {

class ICWKeyerDevice;

/**
 * Factory for creating CW keyer device instances
 */
class KeyerFactory {
public:
    /**
     * Create a keyer device of the specified type
     * @param type Device type to create
     * @param parent QObject parent for memory management
     * @return New device instance (caller takes ownership), or nullptr on failure
     */
    static ICWKeyerDevice* createKeyer(KeyerDeviceType type, QObject* parent = nullptr);

    /**
     * Get human-readable name for a device type
     */
    static QString deviceTypeName(KeyerDeviceType type);

    /**
     * Get list of available device type names
     */
    static QStringList availableDeviceTypes();
};

} // namespace TR4QT

#endif // KEYERFACTORY_H
