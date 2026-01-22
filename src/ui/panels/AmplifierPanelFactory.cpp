#include "AmplifierPanelFactory.h"
#include "KPA1500PanelController.h"

namespace TR4QT {

std::unique_ptr<IAmplifierPanelController> AmplifierPanelFactory::createPanelController(AmplifierType type) {
    switch (type) {
        case AmplifierType::KPA1500:
            return std::make_unique<KPA1500PanelController>();

        case AmplifierType::KPA500:
            // TODO: Implement KPA500 panel controller
            return nullptr;

        case AmplifierType::GenericHamlib:
            // TODO: Implement generic Hamlib panel controller
            return nullptr;

        case AmplifierType::None:
        default:
            return nullptr;
    }
}

AmplifierPanelFactory::AmplifierType AmplifierPanelFactory::typeFromString(const QString& model) {
    QString lowerModel = model.toLower();

    if (lowerModel.contains("kpa1500") || lowerModel.contains("kpa-1500")) {
        return AmplifierType::KPA1500;
    } else if (lowerModel.contains("kpa500") || lowerModel.contains("kpa-500")) {
        return AmplifierType::KPA500;
    } else if (lowerModel.contains("hamlib")) {
        return AmplifierType::GenericHamlib;
    }

    return AmplifierType::None;
}

QString AmplifierPanelFactory::typeToString(AmplifierType type) {
    switch (type) {
        case AmplifierType::KPA1500:
            return "Elecraft KPA1500";
        case AmplifierType::KPA500:
            return "Elecraft KPA500";
        case AmplifierType::GenericHamlib:
            return "Hamlib Amplifier";
        case AmplifierType::None:
        default:
            return "Unknown";
    }
}

} // namespace TR4QT
