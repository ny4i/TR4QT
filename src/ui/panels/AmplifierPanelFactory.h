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

#ifndef AMPLIFIERPANELFACTORY_H
#define AMPLIFIERPANELFACTORY_H

#include "IAmplifierPanelController.h"
#include <memory>

namespace TR4QT {

/**
 * @brief Factory for creating amplifier panel controllers
 *
 * Creates the appropriate panel controller based on amplifier type.
 * Currently supports:
 * - KPA1500 (Elecraft KPA1500)
 *
 * Future support:
 * - KPA500 (Elecraft KPA500)
 * - Generic Hamlib amplifier
 */
class AmplifierPanelFactory {
public:
    /**
     * @brief Amplifier types supported by the panel factory
     */
    enum class AmplifierType {
        KPA1500,        // Elecraft KPA1500
        KPA500,         // Elecraft KPA500 (future)
        GenericHamlib,  // Generic Hamlib amplifier (future)
        None            // No panel (placeholder)
    };

    /**
     * @brief Create a panel controller for the specified amplifier type
     * @param type The amplifier type
     * @return Unique pointer to panel controller, or nullptr if type not supported
     */
    static std::unique_ptr<IAmplifierPanelController> createPanelController(AmplifierType type);

    /**
     * @brief Convert string amplifier model to type enum
     * @param model Model name string (e.g., "KPA1500")
     * @return Corresponding amplifier type
     */
    static AmplifierType typeFromString(const QString& model);

    /**
     * @brief Get display name for amplifier type
     */
    static QString typeToString(AmplifierType type);
};

} // namespace TR4QT

#endif // AMPLIFIERPANELFACTORY_H
