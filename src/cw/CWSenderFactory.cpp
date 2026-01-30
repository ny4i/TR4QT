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

#include "CWSenderFactory.h"
#include "CWSender.h"
#include "HamlibCWSender.h"
#include "../logging/LogMacros.h"

namespace TR4QT {

CWSender* CWSenderFactory::create(Backend backend,
                                  RadioController* radio,
                                  QObject* parent) {
    switch (backend) {
        case Backend::Hamlib:
            if (!radio) {
                LOG_ERROR("CWSenderFactory", "RadioController required for Hamlib backend");
                return nullptr;
            }
            return new HamlibCWSender(radio, parent);

        case Backend::Winkeyer:
            LOG_WARN("CWSenderFactory", "Winkeyer backend not yet implemented");
            return nullptr;

        case Backend::Simulated:
            LOG_WARN("CWSenderFactory", "Simulated backend not yet implemented");
            return nullptr;

        default:
            LOG_ERROR("CWSenderFactory", "Unknown backend type");
            return nullptr;
    }
}

CWSender* CWSenderFactory::create(const QString& backendName,
                                  RadioController* radio,
                                  QObject* parent) {
    return create(stringToBackend(backendName), radio, parent);
}

QString CWSenderFactory::backendToString(Backend backend) {
    switch (backend) {
        case Backend::Hamlib:    return "hamlib";
        case Backend::Winkeyer:  return "winkeyer";
        case Backend::Simulated: return "simulated";
        default:                 return "unknown";
    }
}

CWSenderFactory::Backend CWSenderFactory::stringToBackend(const QString& name) {
    QString lower = name.toLower();
    if (lower == "hamlib")    return Backend::Hamlib;
    if (lower == "winkeyer")  return Backend::Winkeyer;
    if (lower == "simulated") return Backend::Simulated;
    return Backend::Hamlib;  // Default
}

} // namespace TR4QT
