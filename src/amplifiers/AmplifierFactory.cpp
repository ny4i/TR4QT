#include "AmplifierFactory.h"
#include "HamlibAmplifier.h"
#include "KPA1500Direct.h"
#include "../logging/LogMacros.h"
#include <hamlib/amplifier.h>

namespace TR4QT {

IAmplifierController* AmplifierFactory::createAmplifier(
    AmplifierType type,
    const AmplifierConfig& config,
    QObject* parent)
{
    LOG_INFO("AmplifierFactory", QString("Creating amplifier interface: %1").arg(amplifierTypeName(type)));

    switch (type) {
        case AmplifierType::HAMLIB:
            return new HamlibAmplifier(parent);

        case AmplifierType::KPA1500_DIRECT: {
            // Validate that KPA1500_DIRECT is appropriate for this amplifier
            if (config.hamlibModelId != 0 && config.hamlibModelId != AMP_MODEL_ELECRAFT_KPA1500) {
                LOG_WARN("AmplifierFactory",
                         QString("KPA1500 Direct mode selected but Hamlib model is not KPA1500 (model %1). "
                                 "KPA1500 Direct will attempt to connect anyway.")
                         .arg(config.hamlibModelId));
            }

            return new KPA1500Direct(parent);
        }

        default:
            LOG_ERROR("AmplifierFactory", QString("Unknown amplifier type: %1").arg(static_cast<int>(type)));
            // Fallback to Hamlib
            return new HamlibAmplifier(parent);
    }
}

QString AmplifierFactory::amplifierTypeName(AmplifierType type)
{
    switch (type) {
        case AmplifierType::HAMLIB:
            return "Hamlib";
        case AmplifierType::KPA1500_DIRECT:
            return "KPA1500 Direct";
        default:
            return "Unknown";
    }
}

QString AmplifierFactory::amplifierTypeDescription(AmplifierType type)
{
    switch (type) {
        case AmplifierType::HAMLIB:
            return "Hamlib library (universal compatibility, works with all amplifiers)";

        case AmplifierType::KPA1500_DIRECT:
            return "Direct KPA1500 control via UDP (optimized performance, "
                   "requires Elecraft KPA1500 amplifier)";

        default:
            return "Unknown amplifier type";
    }
}

bool AmplifierFactory::supportsAmplifierModel(AmplifierType type, int hamlibModelId)
{
    switch (type) {
        case AmplifierType::HAMLIB:
            // Hamlib supports all amplifiers
            return true;

        case AmplifierType::KPA1500_DIRECT:
            // KPA1500 Direct only supports KPA1500
            return (hamlibModelId == AMP_MODEL_ELECRAFT_KPA1500 || hamlibModelId == 0);

        default:
            return false;
    }
}

AmplifierFactory::AmplifierType AmplifierFactory::recommendedTypeForModel(int hamlibModelId)
{
    // Recommend KPA1500 Direct for KPA1500 (better performance via UDP)
    if (hamlibModelId == AMP_MODEL_ELECRAFT_KPA1500) {
        return AmplifierType::KPA1500_DIRECT;
    }

    // Default to Hamlib for all other amplifiers
    return AmplifierType::HAMLIB;
}

} // namespace TR4QT
