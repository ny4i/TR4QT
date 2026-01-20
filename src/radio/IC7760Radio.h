#ifndef IC7760RADIO_H
#define IC7760RADIO_H

#include "IcomRadio.h"

namespace TR4QT {

/**
 * @brief IC-7760 specific implementation
 *
 * Model-specific features:
 * - VFO B commands use sub-command byte 0x01 (dual receiver)
 * - Transceive parameter: 0x01 0x31
 * - Shared RIT/XIT offset (like K4)
 * - CI-V address typically 0xB2
 */
class IC7760Radio : public IcomRadio
{
    Q_OBJECT

public:
    explicit IC7760Radio(QObject* parent = nullptr);
    ~IC7760Radio() override = default;

    // Model identification
    QString modelName() const override { return "IC-7760"; }

    // VFO B format: IC-7760 uses sub-command byte
    bool vfoBUsesSubCommand() const override { return true; }

    // Transceive command parameter (0x1A 0x05 0x01 <param2> 0x01/00)
    quint8 transceiveParameter2() const override { return 0x31; }

    // Model capabilities
    bool supportsTransceive() const override { return true; }
    bool supportsVfoB() const override { return true; }
    bool supportsScope() const override { return true; }
    int maxPowerWatts() const override { return 200; }  // IC-7760 is 200W radio
};

} // namespace TR4QT

#endif // IC7760RADIO_H
