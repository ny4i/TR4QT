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
