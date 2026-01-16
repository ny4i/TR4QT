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
        // Future: GS232, Yaesu, DCU-1, etc.
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
};

} // namespace TR4QT

#endif // ROTATORFACTORY_H
