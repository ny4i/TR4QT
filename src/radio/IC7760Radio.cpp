#include "IC7760Radio.h"
#include "../logging/LogMacros.h"

namespace TR4QT {

IC7760Radio::IC7760Radio(QObject* parent)
    : IcomRadio(parent)
{
    LOG_DEBUG("IC7760Radio", "IC-7760 radio instance created");
}

} // namespace TR4QT
