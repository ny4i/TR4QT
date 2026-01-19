#include "IC9700Radio.h"
#include "../logging/LogMacros.h"

namespace TR4QT {

IC9700Radio::IC9700Radio(QObject* parent)
    : IcomRadio(parent)
{
    LOG_DEBUG("IC9700Radio", "IC-9700 radio instance created");
}

} // namespace TR4QT
