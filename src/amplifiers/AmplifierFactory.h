#ifndef AMPLIFIERFACTORY_H
#define AMPLIFIERFACTORY_H

#include "IAmplifierController.h"
#include <QObject>

namespace TR4QT {

/**
 * @brief Factory for creating amplifier controller instances
 *
 * Provides runtime selection between Hamlib and direct amplifier implementations.
 * Allows choosing the optimal amplifier control method based on amplifier model and user preference.
 */
class AmplifierFactory {
public:
    enum class AmplifierType {
        HAMLIB,           // Hamlib library (universal, works with all amplifiers)
        KPA1500_DIRECT,   // Direct KPA1500 control via UDP (bypasses Hamlib for performance)
        // Future: KPA500_DIRECT, ACOM_DIRECT, etc.
    };

    /**
     * @brief Create an amplifier controller instance
     * @param type Amplifier type (Hamlib or direct implementation)
     * @param config Amplifier configuration
     * @param parent QObject parent for the created amplifier
     * @return IAmplifierController pointer (caller takes ownership)
     *
     * Example usage:
     * @code
     * AmplifierConfig config;
     * config.amplifierType = AmplifierFactory::AmplifierType::KPA1500_DIRECT;
     * config.port = "192.168.1.100:1500";
     *
     * IAmplifierController* amp = AmplifierFactory::createAmplifier(
     *     AmplifierFactory::AmplifierType::KPA1500_DIRECT,
     *     config,
     *     this
     * );
     * @endcode
     */
    static IAmplifierController* createAmplifier(
        AmplifierType type,
        const AmplifierConfig& config,
        QObject* parent = nullptr
    );

    /**
     * @brief Get amplifier type name for display
     * @param type Amplifier type enum
     * @return Human-readable name (e.g., "Hamlib", "KPA1500 Direct")
     */
    static QString amplifierTypeName(AmplifierType type);

    /**
     * @brief Get amplifier type description
     * @param type Amplifier type enum
     * @return Detailed description for UI tooltips
     */
    static QString amplifierTypeDescription(AmplifierType type);

    /**
     * @brief Check if amplifier type supports the given Hamlib model
     * @param type Amplifier type enum
     * @param hamlibModelId Hamlib AMP_MODEL_* constant
     * @return true if type supports this amplifier model
     *
     * Used to validate user selections and provide recommendations.
     * For example, KPA1500_DIRECT only supports AMP_MODEL_ELECRAFT_KPA1500.
     */
    static bool supportsAmplifierModel(AmplifierType type, int hamlibModelId);

    /**
     * @brief Get recommended amplifier type for a Hamlib model
     * @param hamlibModelId Hamlib AMP_MODEL_* constant
     * @return Recommended AmplifierType for best performance
     *
     * Returns KPA1500_DIRECT for KPA1500, HAMLIB for all others.
     * Used for "Auto" selection in UI.
     */
    static AmplifierType recommendedTypeForModel(int hamlibModelId);
};

} // namespace TR4QT

#endif // AMPLIFIERFACTORY_H
