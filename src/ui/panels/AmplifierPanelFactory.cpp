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
