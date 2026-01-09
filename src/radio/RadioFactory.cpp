#include "RadioFactory.h"
#include "HamlibRadio.h"
#include "K4Radio.h"
#include "IcomRadio.h"
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

        case RadioType::ICOM_DIRECT: {
            // Validate that this is an Icom radio
            if (config.hamlibModelId != 0) {
                // Check if it's a supported Icom network radio
                // IC-905=4032, IC-9700=3077, IC-7610=3078, IC-7600=3071,
                // IC-7300=3073, IC-705=3087, IC-R8600=3095
                int validIcomModels[] = {
                    4032,  // IC-905
                    3077,  // IC-9700
                    3078,  // IC-7610
                    3071,  // IC-7600
                    3074,  // IC-7300MK2
                    3087,  // IC-705
                    3095,  // IC-R8600
                    3091,  // IC-7850
                    3092   // IC-7851
                };

                bool isValidIcom = false;
                for (int model : validIcomModels) {
                    if (config.hamlibModelId == model) {
                        isValidIcom = true;
                        break;
                    }
                }

                if (!isValidIcom) {
                    LOG_WARN("RadioFactory",
                             QString("Icom Direct mode selected but Hamlib model %1 "
                                     "may not support network control.")
                             .arg(config.hamlibModelId));
                }
            }

            return new IcomRadio(parent);
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
        case RadioType::ICOM_DIRECT:
            return "Icom Direct";
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

        case RadioType::ICOM_DIRECT:
            return "Direct Icom network control (3-5x faster than Hamlib, "
                   "requires Icom radio with network capability: IC-905, IC-9700, "
                   "IC-7850, IC-7851, IC-7610, IC-7600, IC-7300MK2, IC-705, IC-R8600)";

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

        case RadioType::ICOM_DIRECT: {
            // Check if it's a supported Icom network radio
            int validIcomModels[] = {
                4032,  // IC-905
                3077,  // IC-9700
                3078,  // IC-7610
                3071,  // IC-7600
                3074,  // IC-7300MK2
                3087,  // IC-705
                3095,  // IC-R8600
                3091,  // IC-7850
                3092   // IC-7851
            };

            if (hamlibModelId == 0) return true;  // Allow unconfigured

            for (int model : validIcomModels) {
                if (hamlibModelId == model) {
                    return true;
                }
            }
            return false;
        }

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

    // Recommend Icom Direct for supported Icom network radios
    int icomNetworkModels[] = {
        4032,  // IC-905
        3077,  // IC-9700
        3078,  // IC-7610
        3071,  // IC-7600
        3074,  // IC-7300MK2
        3087,  // IC-705
        3095,  // IC-R8600
        3091,  // IC-7850
        3092   // IC-7851
    };

    for (int model : icomNetworkModels) {
        if (hamlibModelId == model) {
            return RadioType::ICOM_DIRECT;
        }
    }

    // Default to Hamlib for all other radios
    return RadioType::HAMLIB;
}

} // namespace TR4QT
