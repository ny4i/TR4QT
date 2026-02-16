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

#ifndef CWSENDERFACTORY_H
#define CWSENDERFACTORY_H

#include <memory>
#include <QString>

class QObject;  // Forward declaration

namespace TR4QT {

class CWSender;
class RadioController;
class KeyerController;

/**
 * Factory for creating CW sender instances
 *
 * Supports different backends:
 * - "hamlib": Uses Hamlib rig_send_morse (default)
 * - "winkeyer": Uses Winkeyer hardware (future)
 * - "simulated": For testing without hardware (future)
 */
class CWSenderFactory {
public:
    enum class Backend {
        Hamlib,        // Default: Use Hamlib via RadioController
        KeyerDevice,   // External keyer (WinKeyer) via KeyerController
        Simulated      // For testing
    };

    /**
     * Create a CW sender with the specified backend
     *
     * @param backend The backend to use
     * @param radio The radio controller (required for Hamlib backend)
     * @param parent QObject parent for memory management
     * @return Pointer to the created CW sender, or nullptr on failure
     */
    static CWSender* create(Backend backend,
                           RadioController* radio = nullptr,
                           KeyerController* keyer = nullptr,
                           QObject* parent = nullptr);

    /**
     * Create a CW sender by backend name
     *
     * @param backendName "hamlib", "winkeyer", or "simulated"
     * @param radio The radio controller (required for Hamlib backend)
     * @param parent QObject parent for memory management
     * @return Pointer to the created CW sender, or nullptr on failure
     */
    static CWSender* create(const QString& backendName,
                           RadioController* radio = nullptr,
                           KeyerController* keyer = nullptr,
                           QObject* parent = nullptr);

    /**
     * Get the default backend
     */
    static Backend defaultBackend() { return Backend::Hamlib; }

    /**
     * Convert backend enum to string
     */
    static QString backendToString(Backend backend);

    /**
     * Convert string to backend enum
     */
    static Backend stringToBackend(const QString& name);
};

} // namespace TR4QT

#endif // CWSENDERFACTORY_H
