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

#ifndef ROTATORFACTORY_H
#define ROTATORFACTORY_H

#include "IRotatorController.h"
#include <QObject>

namespace TR4QT {

/**
 * @brief Factory for creating rotator controller instances
 *
 * Provides runtime selection between different rotator control protocols.
 * Allows choosing the optimal rotator control method based on hardware and user preference.
 */
class RotatorFactory {
public:
    enum class RotatorType {
        PSTROTATOR,  // PSTRotator UDP protocol (XML-like commands)
        HAMLIB,      // Hamlib library (universal compatibility - EASYCOMM, ROTOREZ, GS-232, NETROTCTL, etc.)
        // Future: GREEN_HERON_DIRECT, GS232_DIRECT, etc.
    };

    /**
     * @brief Create a rotator controller instance
     * @param type Rotator type (protocol to use)
     * @param config Rotator configuration
     * @param parent QObject parent for the created controller
     * @return IRotatorController pointer (caller takes ownership)
     *
     * Example usage:
     * @code
     * RotatorConfig config;
     * config.rotatorType = RotatorFactory::RotatorType::PSTROTATOR;
     * config.ipAddress = "192.168.1.100";
     * config.port = 12000;
     *
     * IRotatorController* rotator = RotatorFactory::createRotator(
     *     RotatorFactory::RotatorType::PSTROTATOR,
     *     config,
     *     this
     * );
     * @endcode
     */
    static IRotatorController* createRotator(
        RotatorType type,
        const RotatorConfig& config,
        QObject* parent = nullptr
    );

    /**
     * @brief Get rotator type name for display
     * @param type Rotator type enum
     * @return Human-readable name (e.g., "PSTRotator", "GS-232")
     */
    static QString rotatorTypeName(RotatorType type);

    /**
     * @brief Get rotator type description
     * @param type Rotator type enum
     * @return Detailed description for UI tooltips
     */
    static QString rotatorTypeDescription(RotatorType type);

    /**
     * @brief Check if rotator type supports network control
     * @param type Rotator type enum
     * @return true if type uses IP/UDP, false if serial
     */
    static bool isNetworkRotator(RotatorType type);

    /**
     * @brief Check if rotator type supports the given Hamlib model
     * @param type Rotator type enum
     * @param hamlibModelId Hamlib ROT_MODEL_* constant
     * @return true if type supports this rotator model
     */
    static bool supportsRotatorModel(RotatorType type, int hamlibModelId);

    /**
     * @brief Get recommended rotator type for a Hamlib model
     * @param hamlibModelId Hamlib ROT_MODEL_* constant
     * @return Recommended RotatorType for best performance
     */
    static RotatorType recommendedTypeForModel(int hamlibModelId);
};

} // namespace TR4QT

#endif // ROTATORFACTORY_H
