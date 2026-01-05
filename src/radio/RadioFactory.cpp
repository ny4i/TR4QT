#include "RadioFactory.h"
#include "HamlibRadio.h"
#include "K4Radio.h"
#include "../logging/LogMacros.h"
#include <hamlib/rig.h>

namespace TR4QT {

RadioInterface* RadioFactory::createRadio(
    RadioType type,
    const RadioConfig& config,
    QObject* parent)
{
    LOG_INFO("RadioFactory", QString("Creating radio interface: %1").arg(radioTypeName(type)));

    switch (type) {
        case RadioType::HAMLIB:
            return new HamlibRadio(parent);

        case RadioType::K4_DIRECT: {
            // Validate that K4_DIRECT is appropriate for this radio
            if (config.hamlibModelId != 0 && config.hamlibModelId != RIG_MODEL_K4) {
                LOG_WARN("RadioFactory",
                         QString("K4 Direct mode selected but Hamlib model is not K4 (model %1). "
                                 "K4 Direct will attempt to connect anyway.")
                         .arg(config.hamlibModelId));
            }

            return new K4Radio(parent);
        }

        default:
            LOG_ERROR("RadioFactory", QString("Unknown radio type: %1").arg(static_cast<int>(type)));
            // Fallback to Hamlib
            return new HamlibRadio(parent);
    }
}

QString RadioFactory::radioTypeName(RadioType type)
{
    switch (type) {
        case RadioType::HAMLIB:
            return "Hamlib";
        case RadioType::K4_DIRECT:
            return "K4 Direct";
        default:
            return "Unknown";
    }
}

QString RadioFactory::radioTypeDescription(RadioType type)
{
    switch (type) {
        case RadioType::HAMLIB:
            return "Hamlib library (universal compatibility, works with all radios)";

        case RadioType::K4_DIRECT:
            return "Direct K4 control via TCP (5-10x faster than Hamlib, "
                   "requires Elecraft K4/K4D/K4HD)";

        default:
            return "Unknown radio type";
    }
}

bool RadioFactory::supportsRadioModel(RadioType type, int hamlibModelId)
{
    switch (type) {
        case RadioType::HAMLIB:
            // Hamlib supports all radios
            return true;

        case RadioType::K4_DIRECT:
            // K4 Direct only supports K4
            return (hamlibModelId == RIG_MODEL_K4 || hamlibModelId == 0);

        default:
            return false;
    }
}

RadioFactory::RadioType RadioFactory::recommendedTypeForModel(int hamlibModelId)
{
    // Recommend K4 Direct for K4 (better performance)
    if (hamlibModelId == RIG_MODEL_K4) {
        return RadioType::K4_DIRECT;
    }

    // Default to Hamlib for all other radios
    return RadioType::HAMLIB;
}

} // namespace TR4QT
