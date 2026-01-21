#include "RotatorFactory.h"
#include "PSTRotatorController.h"
#include "HamlibRotator.h"
#include "../logging/LogMacros.h"
#include <hamlib/rotator.h>

namespace TR4QT {

IRotatorController* RotatorFactory::createRotator(
    RotatorType type,
    const RotatorConfig& config,
    QObject* parent)
{
    LOG_INFO("RotatorFactory", QString("Creating rotator type: %1")
        .arg(rotatorTypeName(type)));

    IRotatorController* controller = nullptr;

    switch (type) {
    case RotatorType::PSTROTATOR:
        controller = new PSTRotatorController(parent);
        break;

    case RotatorType::HAMLIB:
        controller = new HamlibRotator(parent);
        break;

    default:
        LOG_ERROR("RotatorFactory", QString("Unknown rotator type: %1")
            .arg(static_cast<int>(type)));
        return nullptr;
    }

    // Connect rotator with configuration
    if (controller) {
        if (!controller->connect(config)) {
            LOG_ERROR("RotatorFactory", "Failed to connect rotator");
            delete controller;
            return nullptr;
        }
    }

    return controller;
}

QString RotatorFactory::rotatorTypeName(RotatorType type)
{
    switch (type) {
    case RotatorType::PSTROTATOR:
        return "PSTRotator";
    case RotatorType::HAMLIB:
        return "Hamlib";
    default:
        return "Unknown";
    }
}

QString RotatorFactory::rotatorTypeDescription(RotatorType type)
{
    switch (type) {
    case RotatorType::PSTROTATOR:
        return "PSTRotator UDP control (XML-like protocol, default port 12000)";
    case RotatorType::HAMLIB:
        return "Hamlib library (universal compatibility - supports EASYCOMM, ROTOREZ, "
               "GS-232A/B, NETROTCTL, and many other rotator protocols)";
    default:
        return "Unknown rotator type";
    }
}

bool RotatorFactory::isNetworkRotator(RotatorType type)
{
    switch (type) {
    case RotatorType::PSTROTATOR:
        return true;  // UDP network protocol
    case RotatorType::HAMLIB:
        return false;  // Hamlib supports both serial and network (depends on model)
    default:
        return false;
    }
}

bool RotatorFactory::supportsRotatorModel(RotatorType type, int hamlibModelId)
{
    switch (type) {
    case RotatorType::HAMLIB:
        // Hamlib supports all rotators
        return true;

    case RotatorType::PSTROTATOR:
        // PSTRotator doesn't use Hamlib model IDs
        return (hamlibModelId == 0);  // Only allow if no Hamlib model specified

    default:
        return false;
    }
}

RotatorFactory::RotatorType RotatorFactory::recommendedTypeForModel(int hamlibModelId)
{
    // For now, default to Hamlib for all Hamlib-supported rotators
    // PSTRotator users should select manually (it doesn't have a Hamlib model ID)
    if (hamlibModelId == 0) {
        return RotatorType::PSTROTATOR;  // No Hamlib model = assume PSTRotator
    }

    return RotatorType::HAMLIB;  // Default to Hamlib for all other models
}

} // namespace TR4QT
