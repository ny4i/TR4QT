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

#include "RadioFactory.h"
#include "HamlibRadio.h"
#include "K4Radio.h"
#include "IcomRadio.h"
#include "IC7760Radio.h"
#include "IC9700Radio.h"
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
            // Instantiate model-specific Icom radio class based on Hamlib model ID
            // IC-7760: Hamlib ID 3092
            // IC-9700: Hamlib ID 3077 or 3081

            if (config.hamlibModelId == 3092) {
                LOG_INFO("RadioFactory", "Creating IC-7760 radio instance");
                return new IC7760Radio(parent);
            } else if (config.hamlibModelId == 3077 || config.hamlibModelId == 3081) {
                LOG_INFO("RadioFactory", "Creating IC-9700 radio instance");
                return new IC9700Radio(parent);
            } else {
                // For other Icom radios without specific implementations, fall back to Hamlib
                // Supported but not yet implemented: IC-905, IC-7610, IC-7600, IC-7300MK2, IC-705, IC-R8600, IC-7850
                LOG_WARN("RadioFactory",
                         QString("Icom Direct mode selected for Hamlib model %1, "
                                 "but no model-specific implementation exists yet. "
                                 "Falling back to Hamlib. "
                                 "Supported models: IC-7760 (3092), IC-9700 (3077/3081)")
                         .arg(config.hamlibModelId));
                return new HamlibRadio(parent);
            }
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
                3090,  // IC-905
                3077,  // IC-9700
                3081,  // IC-9700 (alternative Hamlib ID)
                3078,  // IC-7610
                3071,  // IC-7600
                3094,  // IC-7300MK2
                3087,  // IC-705
                3095,  // IC-R8600
                3075,  // IC-7850/7851
                3092   // IC-7760
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
        3090,  // IC-905
        3077,  // IC-9700
        3081,  // IC-9700 (alternative Hamlib ID)
        3078,  // IC-7610
        3071,  // IC-7600
        3094,  // IC-7300MK2
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

QList<SupportedRadio> RadioFactory::getImplementedRadios(RadioType type)
{
    QList<SupportedRadio> radios;

    switch (type) {
        case RadioType::K4_DIRECT:
            // K4Radio class supports all K4 variants
            radios.append({RIG_MODEL_K4, "Elecraft K4 / K4D / K4HD"});
            break;

        case RadioType::ICOM_DIRECT:
            // Only include radios with actual class implementations
            // IC7760Radio and IC9700Radio are the currently implemented classes
            radios.append({3092, "Icom IC-7760"});
            radios.append({3081, "Icom IC-9700"});
            // Future implementations would be added here:
            // radios.append({3078, "Icom IC-7610"});
            // radios.append({3087, "Icom IC-705"});
            // etc.
            break;

        case RadioType::HAMLIB:
            // Hamlib supports all radios - use Hamlib's enumeration instead
            // Return empty list; caller should use rig_list_foreach()
            break;

        default:
            break;
    }

    return radios;
}

} // namespace TR4QT
