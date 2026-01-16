#include "RotatorFactory.h"
#include "PSTRotatorController.h"
#include "../logging/LogMacros.h"

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
    default:
        return "Unknown";
    }
}

QString RotatorFactory::rotatorTypeDescription(RotatorType type)
{
    switch (type) {
    case RotatorType::PSTROTATOR:
        return "PSTRotator UDP control (XML-like protocol, default port 12000)";
    default:
        return "Unknown rotator type";
    }
}

bool RotatorFactory::isNetworkRotator(RotatorType type)
{
    switch (type) {
    case RotatorType::PSTROTATOR:
        return true;  // UDP network protocol
    default:
        return false;
    }
}

} // namespace TR4QT
