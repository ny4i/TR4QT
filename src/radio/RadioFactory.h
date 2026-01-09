#ifndef RADIOFACTORY_H
#define RADIOFACTORY_H

#include "RadioInterface.h"
#include <QObject>

namespace TR4QT {

/**
 * @brief Factory for creating radio interface instances
 *
 * Provides runtime selection between Hamlib and direct radio implementations.
 * Allows choosing the optimal radio control method based on radio model and user preference.
 */
class RadioFactory {
public:
    enum class RadioType {
        HAMLIB,      // Hamlib library (universal, works with all radios)
        K4_DIRECT,   // Direct K4 control via TCP (bypasses Hamlib for performance)
        ICOM_DIRECT, // Direct Icom control via network (native Icom protocol)
        // Future: K3_DIRECT, etc.
    };

    /**
     * @brief Create a radio interface instance
     * @param type Radio type (Hamlib or direct implementation)
     * @param config Radio configuration
     * @param parent QObject parent for the created radio
     * @return RadioInterface pointer (caller takes ownership)
     *
     * Example usage:
     * @code
     * RadioConfig config;
     * config.radioType = RadioFactory::RadioType::K4_DIRECT;
     * config.port = "192.168.1.100:12345";
     *
     * RadioInterface* radio = RadioFactory::createRadio(
     *     RadioFactory::RadioType::K4_DIRECT,
     *     config,
     *     this
     * );
     * @endcode
     */
    static RadioInterface* createRadio(
        RadioType type,
        const RadioConfig& config,
        QObject* parent = nullptr
    );

    /**
     * @brief Get radio type name for display
     * @param type Radio type enum
     * @return Human-readable name (e.g., "Hamlib", "K4 Direct")
     */
    static QString radioTypeName(RadioType type);

    /**
     * @brief Get radio type description
     * @param type Radio type enum
     * @return Detailed description for UI tooltips
     */
    static QString radioTypeDescription(RadioType type);

    /**
     * @brief Check if radio type supports the given Hamlib model
     * @param type Radio type enum
     * @param hamlibModelId Hamlib RIG_MODEL_* constant
     * @return true if type supports this radio model
     *
     * Used to validate user selections and provide recommendations.
     * For example, K4_DIRECT only supports Hamlib model RIG_MODEL_K4.
     */
    static bool supportsRadioModel(RadioType type, int hamlibModelId);

    /**
     * @brief Get recommended radio type for a Hamlib model
     * @param hamlibModelId Hamlib RIG_MODEL_* constant
     * @return Recommended RadioType for best performance
     *
     * Returns K4_DIRECT for K4, HAMLIB for all others.
     * Used for "Auto" selection in UI.
     */
    static RadioType recommendedTypeForModel(int hamlibModelId);
};

} // namespace TR4QT

#endif // RADIOFACTORY_H
