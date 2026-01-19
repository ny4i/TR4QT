#ifndef IC9700RADIO_H
#define IC9700RADIO_H

#include "IcomRadio.h"

namespace TR4QT {

/**
 * @brief IC-9700 specific implementation
 *
 * Model-specific features:
 * - VFO B commands use standard format (no sub-command byte)
 * - Transceive parameter: 0x01 0x27
 * - Supports dual VFO
 * - CI-V address typically 0xA2
 */
class IC9700Radio : public IcomRadio
{
    Q_OBJECT

public:
    explicit IC9700Radio(QObject* parent = nullptr);
    ~IC9700Radio() override = default;

    // Model identification
    QString modelName() const override { return "IC-9700"; }

    // VFO B format: IC-9700 uses standard format (no sub-command)
    bool vfoBUsesSubCommand() const override { return false; }

    // Transceive command parameter (0x1A 0x05 0x01 <param2> 0x01/00)
    quint8 transceiveParameter2() const override { return 0x27; }

    // Model capabilities
    bool supportsTransceive() const override { return true; }
    bool supportsVfoB() const override { return true; }
    bool supportsScope() const override { return true; }
};

} // namespace TR4QT

#endif // IC9700RADIO_H
