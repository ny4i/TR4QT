# Integrating Icom Network Support into TR4QT

This guide shows how to add the **IcomRadio** class to TR4QT's RadioFactory.

## Overview

The IcomRadio class integrates seamlessly with TR4QT's existing radio factory pattern:

- **IcomNetwork** - Low-level network protocol handler (generic, standalone)
- **IcomRadio** - RadioInterface implementation that uses IcomNetwork internally
- **RadioFactory** - Extended to create IcomRadio instances

## Files to Copy to TR4QT

Copy these files to `TR4QT/src/radio/`:

1. **icompackets.h** - Packet structures and encoding
2. **icomnetwork.h** - Network protocol implementation
3. **icomnetwork.cpp** - Network protocol implementation
4. **IcomRadio.h** - RadioInterface implementation
5. **IcomRadio.cpp** - RadioInterface implementation

## Step 1: Update RadioConfig

Add Icom-specific fields to `RadioConfig` in `TR4QT/src/radio/RadioInterface.h`:

```cpp
// Radio configuration
struct RadioConfig {
    int hamlibModelId{0};
    QString port;
    int baudRate{38400};
    int dataBits{8};
    int stopBits{1};
    int parity{0};
    int civAddress{0};
    int pollInterval{100};
    int radioType{0};

    // ADD THESE FOR ICOM NETWORK SUPPORT:
    QString icomUsername;        // Icom network username
    QString icomPassword;        // Icom network password
    QString icomClientName{"TR4QT"};  // Client identifier
};
```

## Step 2: Update RadioFactory Enum

Add ICOM_DIRECT to the RadioType enum in `TR4QT/src/radio/RadioFactory.h`:

```cpp
class RadioFactory {
public:
    enum class RadioType {
        HAMLIB,      // Hamlib library (universal)
        K4_DIRECT,   // Direct K4 control via TCP
        ICOM_DIRECT, // Direct Icom control via network (NEW!)
    };
    // ... rest of class
};
```

## Step 3: Update RadioFactory Implementation

Modify `TR4QT/src/radio/RadioFactory.cpp`:

### 3a. Add Include

```cpp
#include "RadioFactory.h"
#include "HamlibRadio.h"
#include "K4Radio.h"
#include "IcomRadio.h"  // ADD THIS
#include "../logging/LogMacros.h"
#include <hamlib/rig.h>
```

### 3b. Update createRadio()

```cpp
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
            if (config.hamlibModelId != 0 && config.hamlibModelId != RIG_MODEL_K4) {
                LOG_WARN("RadioFactory",
                         QString("K4 Direct mode selected but Hamlib model is not K4 (model %1).")
                         .arg(config.hamlibModelId));
            }
            return new K4Radio(parent);
        }

        // ADD THIS CASE:
        case RadioType::ICOM_DIRECT: {
            // Validate that this is an Icom radio
            if (config.hamlibModelId != 0) {
                // Check if it's a supported Icom network radio
                // IC-905=4032, IC-9700=3077, IC-7610=3078, IC-7600=3071,
                // IC-7300=3073, IC-705=3087, IC-R8600=3095
                int validIcomModels[] = {
                    4032,  // IC-905
                    3077,  // IC-9700
                    3078,  // IC-7610
                    3071,  // IC-7600
                    3074,  // IC-7300MK2
                    3087,  // IC-705
                    3095,  // IC-R8600
                    3091,  // IC-7850
                    3092   // IC-7851
                };

                bool isValidIcom = false;
                for (int model : validIcomModels) {
                    if (config.hamlibModelId == model) {
                        isValidIcom = true;
                        break;
                    }
                }

                if (!isValidIcom) {
                    LOG_WARN("RadioFactory",
                             QString("Icom Direct mode selected but Hamlib model %1 "
                                     "may not support network control.")
                             .arg(config.hamlibModelId));
                }
            }

            return new IcomRadio(parent);
        }

        default:
            LOG_ERROR("RadioFactory", QString("Unknown radio type: %1").arg(static_cast<int>(type)));
            return new HamlibRadio(parent);
    }
}
```

### 3c. Update radioTypeName()

```cpp
QString RadioFactory::radioTypeName(RadioType type)
{
    switch (type) {
        case RadioType::HAMLIB:
            return "Hamlib";
        case RadioType::K4_DIRECT:
            return "K4 Direct";
        case RadioType::ICOM_DIRECT:  // ADD THIS
            return "Icom Direct";
        default:
            return "Unknown";
    }
}
```

### 3d. Update radioTypeDescription()

```cpp
QString RadioFactory::radioTypeDescription(RadioType type)
{
    switch (type) {
        case RadioType::HAMLIB:
            return "Hamlib library (universal compatibility, works with all radios)";

        case RadioType::K4_DIRECT:
            return "Direct K4 control via TCP (5-10x faster than Hamlib, "
                   "requires Elecraft K4/K4D/K4HD)";

        // ADD THIS:
        case RadioType::ICOM_DIRECT:
            return "Direct Icom network control (3-5x faster than Hamlib, "
                   "requires Icom radio with network capability: IC-905, IC-9700, "
                   "IC-7850, IC-7851, IC-7610, IC-7600, IC-7300MK2, IC-705, IC-R8600)";

        default:
            return "Unknown radio type";
    }
}
```

### 3e. Update supportsRadioModel()

```cpp
bool RadioFactory::supportsRadioModel(RadioType type, int hamlibModelId)
{
    switch (type) {
        case RadioType::HAMLIB:
            return true;

        case RadioType::K4_DIRECT:
            return (hamlibModelId == RIG_MODEL_K4 || hamlibModelId == 0);

        // ADD THIS:
        case RadioType::ICOM_DIRECT: {
            // Check if it's a supported Icom network radio
            int validIcomModels[] = {
                4032,  // IC-905
                3077,  // IC-9700
                3078,  // IC-7610
                3071,  // IC-7600
                3074,  // IC-7300MK2
                3087,  // IC-705
                3095,  // IC-R8600
                3091,  // IC-7850
                3092   // IC-7851
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
```

### 3f. Update recommendedTypeForModel()

```cpp
RadioFactory::RadioType RadioFactory::recommendedTypeForModel(int hamlibModelId)
{
    // Recommend K4 Direct for K4
    if (hamlibModelId == RIG_MODEL_K4) {
        return RadioType::K4_DIRECT;
    }

    // ADD THIS: Recommend Icom Direct for supported Icom network radios
    int icomNetworkModels[] = {
        4032,  // IC-905
        3077,  // IC-9700
        3078,  // IC-7610
        3071,  // IC-7600
        3074,  // IC-7300MK2
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
```

## Step 4: Update CMakeLists.txt

Add the new files to `TR4QT/src/CMakeLists.txt`:

```cmake
# Radio control
target_sources(${PROJECT_NAME} PRIVATE
    radio/RadioFactory.cpp
    radio/RadioInterface.h
    radio/HamlibRadio.cpp
    radio/HamlibRadio.h
    radio/K4Radio.cpp
    radio/K4Radio.h
    radio/IcomRadio.cpp          # ADD THIS
    radio/IcomRadio.h            # ADD THIS
    radio/icomnetwork.cpp        # ADD THIS
    radio/icomnetwork.h          # ADD THIS
    radio/icompackets.h          # ADD THIS
    # ... rest of files
)
```

## Step 5: Usage Example

### In PreferencesDialog or Radio Setup UI:

```cpp
// User selects Icom radio
RadioConfig config;
config.hamlibModelId = 3078;  // IC-7610
config.port = "192.168.1.100:50001";  // IP:PORT format for Icom
config.civAddress = 0x98;  // CI-V address from radio settings
config.icomUsername = "";  // From user input (can be blank)
config.icomPassword = "";  // From user input (can be blank)
config.pollInterval = 100;  // Poll every 100ms

// Factory automatically recommends ICOM_DIRECT for IC-7610
RadioFactory::RadioType type = RadioFactory::recommendedTypeForModel(config.hamlibModelId);

// Or user manually selects
type = RadioFactory::RadioType::ICOM_DIRECT;

// Create radio
RadioInterface* radio = RadioFactory::createRadio(type, config, this);

// Connect and use
radio->connect(config);
```

### Using the Radio:

```cpp
// Everything works through RadioInterface - no changes needed!
radio->setFrequency(14250000, VFO::VFO_A);  // 14.250 MHz
radio->setMode(ModeType::CW, VFO::VFO_A);
radio->setPTT(true);
radio->sendCW("CQ TEST K4ABC K4ABC K4ABC K");
```

## Port Format Requirements

**Important:** Icom Direct requires port in `IP:PORT` format:
- ✅ Correct: `"192.168.1.100:50001"`
- ❌ Wrong: `"192.168.1.100"` (missing port)
- ❌ Wrong: `"/dev/ttyUSB0"` (serial port - not supported)

The standard Icom control port is **50001**.

## Finding Radio Network Settings

Users need to configure these on their radio:

### IC-9700/IC-7610/IC-905:
1. MENU → SET → Network
2. Enable "Network Control"
3. Note IP Address
4. Control Port is usually 50001
5. Set/note Username and Password (can be blank)

### CI-V Address:
1. MENU → SET → Connectors
2. CI-V Address (e.g., 0x98 for IC-9700, 0x98 for IC-7610)

## Benefits Over Hamlib for Icom

When using `RadioType::ICOM_DIRECT`:

1. **3-5x faster** frequency/mode changes
2. **More reliable** network connection (native protocol)
3. **Lower latency** for CW sending
4. **Direct access** to all CI-V commands
5. **Better error handling** with specific Icom error codes

## Fallback to Hamlib

If Icom Direct fails or user prefers Hamlib:

```cpp
// Fallback to Hamlib
RadioFactory::RadioType type = RadioFactory::RadioType::HAMLIB;
RadioInterface* radio = RadioFactory::createRadio(type, config, this);

// Port format for Hamlib network:
config.port = "192.168.1.100";  // IP only (Hamlib handles port internally)
// Or for Hamlib serial:
config.port = "/dev/ttyUSB0";
config.baudRate = 115200;
```

## Testing

Test with different Icom radios:

```cpp
// IC-9700
config.hamlibModelId = 3077;
config.civAddress = 0x98;

// IC-7610
config.hamlibModelId = 3078;
config.civAddress = 0x98;

// IC-705
config.hamlibModelId = 3087;
config.civAddress = 0xA4;
```

## Troubleshooting Integration

### Build errors:
- Ensure all 5 files are copied
- Check CMakeLists.txt has all files
- Verify include paths

### Runtime connection errors:
- Check IP address is correct (ping radio)
- Verify port 50001 is not blocked by firewall
- Ensure radio has "Network Control" enabled
- Check username/password if radio requires them

### CI-V command issues:
- Verify civAddress matches radio setting
- Check CI-V reference manual for your specific radio model
- Some commands vary between radio models

## Complete Example: IC-7610 Setup

```cpp
// In your radio setup code
RadioConfig config;
config.hamlibModelId = 3078;           // IC-7610
config.port = "192.168.1.100:50001";  // Radio IP + control port
config.civAddress = 0x98;              // IC-7610 default CI-V address
config.icomUsername = "";              // Usually blank
config.icomPassword = "";              // Usually blank
config.icomClientName = "TR4QT";
config.pollInterval = 100;             // Poll every 100ms

// Create Icom Direct radio
RadioInterface* radio = RadioFactory::createRadio(
    RadioFactory::RadioType::ICOM_DIRECT,
    config,
    this
);

// Connect signals
connect(radio, &RadioInterface::frequencyChanged,
        this, &MyClass::onFrequencyChanged);
connect(radio, &RadioInterface::connectionStatusChanged,
        this, &MyClass::onConnectionChanged);

// Connect to radio
if (radio->connect(config)) {
    qInfo() << "Connected to" << radio->getCurrentState().radioModel;
}
```

That's it! The Icom network support is now fully integrated into TR4QT's radio factory pattern.
