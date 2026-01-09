# Icom Network Control for TR4QT

This is a simplified Qt-based class for controlling Icom radios over the network using their proprietary UDP protocol. It handles authentication and CI-V command transmission/reception without any audio functionality.

## Supported Radios

The following Icom radios have network/Ethernet interfaces compatible with this implementation:
- IC-905
- IC-9700
- IC-7850
- IC-7851
- IC-7610
- IC-7600
- IC-7300MK2 (note: IC-7300 original is serial only, not supported)
- IC-705
- IC-R8600

**Note:** The original IC-7300 does NOT have network capability - only serial. The IC-7300MK2 added Ethernet support.

## Files

- **icompackets.h** - Packet structure definitions and password encoding
- **icomnetwork.h** - Main class interface
- **icomnetwork.cpp** - Implementation
- **icomnetwork_example.cpp** - Usage example

## Features

✅ **Authentication** - Handles username/password login with token management
✅ **CI-V Commands** - Send and receive CI-V commands over the network
✅ **Multiple Radios** - Auto-discovery and selection when multiple radios are available
✅ **Reliable Delivery** - Packet sequence tracking and retransmission
✅ **Connection Management** - Automatic keepalive and reconnection
❌ **Audio** - Not implemented (control only)

## Quick Start

### 1. Add to your Qt project

Add these files to your .pro file:
```qmake
HEADERS += icompackets.h \
           icomnetwork.h

SOURCES += icomnetwork.cpp
```

Or for CMake:
```cmake
target_sources(your_target PRIVATE
    icompackets.h
    icomnetwork.h
    icomnetwork.cpp
)
```

### 2. Basic usage

```cpp
#include "icomnetwork.h"

// Create the network object
IcomNetwork* icom = new IcomNetwork(this);

// Connect signals
connect(icom, &IcomNetwork::connected, this, &MyClass::onConnected);
connect(icom, &IcomNetwork::civDataReceived, this, &MyClass::onCivData);

// Configure connection
IcomConnectionConfig config;
config.ipAddress = "192.168.1.100";  // Your radio's IP
config.controlPort = 50001;           // Standard Icom port
config.username = "username";
config.password = "password";
config.clientName = "TR4QT";

// Connect
icom->connectToRadio(config);
```

### 3. Send CI-V commands

```cpp
void MyClass::onConnected()
{
    // Get frequency (CI-V command 0x03)
    QByteArray cmd;
    cmd.append(0xFE);  // Preamble
    cmd.append(0xFE);  // Preamble
    cmd.append(icom->currentRadio().civAddress);  // Radio address
    cmd.append(0xE0);  // Controller address
    cmd.append(0x03);  // Command: Get frequency
    cmd.append(0xFD);  // End marker

    icom->sendCivCommand(cmd);
}
```

### 4. Receive CI-V data

```cpp
void MyClass::onCivData(const QByteArray& data)
{
    qDebug() << "Received:" << data.toHex(' ');

    // Parse the response
    // Format: FE FE E0 <radio_addr> <cmd> <data...> FD
}
```

## Connection Flow

The class handles the complex connection sequence automatically:

1. **Discovery** - Sends "Are You There" packets
2. **Handshake** - Receives "I Am Here" response
3. **Login** - Sends encoded username/password
4. **Authentication** - Receives token
5. **Token Confirmation** - Confirms token receipt
6. **Radio Discovery** - Receives list of available radios
7. **Stream Request** - Requests CI-V data stream
8. **Connected** - CI-V socket ready for commands

## Signals

### Connection Signals
- `connected()` - Successfully connected and ready
- `disconnected()` - Connection closed
- `authenticationFailed(QString reason)` - Login failed
- `connectionError(QString error)` - Connection error occurred

### Data Signals
- `civDataReceived(QByteArray data)` - CI-V data received from radio
- `radiosDiscovered(QList<IcomRadioInfo>)` - Available radios discovered

### Status Signals
- `statusUpdate(QString message)` - Status messages
- `statisticsUpdated(IcomConnectionStats)` - Connection statistics

## Radio Configuration

### Find your radio's IP address

1. Radio menu → SET → Network
2. Look for "IP Address" setting
3. Default is usually DHCP - check your router

### Default credentials

Most Icom radios ship with:
- **Username**: (blank or "user")
- **Password**: (blank or model-specific)
- **Control Port**: 50001

Check your radio's manual for specific defaults.

## CI-V Command Reference

Common CI-V commands (refer to your radio's CI-V manual for complete list):

| Command | Code | Description |
|---------|------|-------------|
| Get Frequency | 0x03 | Read VFO frequency |
| Get Mode | 0x04 | Read operating mode |
| Set Frequency | 0x05 | Set VFO frequency |
| Set Mode | 0x06 | Set operating mode |
| Read S-Meter | 0x15 0x02 | Read signal strength |
| Get TX Status | 0x1C 0x00 | Read PTT status |

### CI-V Packet Format

All CI-V commands follow this structure:
```
FE FE <radio_addr> E0 <command> [data...] FD
```

Where:
- `FE FE` - Preamble (2 bytes)
- `<radio_addr>` - Radio's CI-V address (from `currentRadio().civAddress`)
- `E0` - Controller address (always 0xE0)
- `<command>` - Command byte(s)
- `[data...]` - Optional command data
- `FD` - End marker

## Integration with TR4QT

To integrate this into your TR4QT project:

1. Copy the three header/source files to your project directory
2. Add them to your build system
3. Create an `IcomNetwork` instance in your main radio control class
4. Map your existing CAT command interface to CI-V commands
5. Connect the signals to your UI/logging

Example mapping:
```cpp
// TR4QT CAT command
void setFrequency(double freqMHz) {
    quint64 freqHz = freqMHz * 1000000;
    // Convert to CI-V and send via icom->sendCivCommand()
}

// CI-V response handler
void onCivDataReceived(const QByteArray& data) {
    // Parse and update TR4QT internal state
}
```

## Thread Safety

The class uses QMutex for packet buffer access but should generally be used from the main Qt event loop. All signals are emitted in the context of the QUdpSocket's thread.

## Debugging

Enable Qt debug output to see connection details:
```cpp
QLoggingCategory::setFilterRules("*.debug=true");
```

This will show:
- Packet transmission/reception
- Authentication steps
- CI-V data flow

## Limitations

- **No audio streaming** - Control commands only
- **Single connection** - One radio at a time
- **IPv4 only** - No IPv6 support
- **No encryption** - Password encoding is obfuscation, not security

## License

This code is derived from the wfview project and should maintain compatible licensing.

## References

- Icom IC-9700 CI-V Reference Manual
- Icom IC-7610 CI-V Reference Manual
- wfview project: https://gitlab.com/eliggett/wfview

## Troubleshooting

### "Failed to resolve hostname"
- Check the IP address is correct
- Try pinging the radio from command line
- Ensure radio is on same network

### "Authentication failed"
- Verify username/password in radio settings
- Check if radio has network login enabled
- Some radios require empty username/password

### "Radio not responding"
- Verify control port is 50001 (or your configured port)
- Check firewall isn't blocking UDP port
- Ensure radio's network control is enabled

### No CI-V data received
- Check CI-V address matches radio's configured address
- Verify radio has CI-V over LAN enabled
- Some radios require enabling "Network Control" in menu

## Example: Complete Frequency Control

```cpp
class FrequencyController : public QObject {
    Q_OBJECT
public:
    FrequencyController() {
        icom = new IcomNetwork(this);
        connect(icom, &IcomNetwork::connected, [this]() {
            qInfo() << "Connected!";
            getFrequency();
        });
        connect(icom, &IcomNetwork::civDataReceived,
                this, &FrequencyController::parseCivData);
    }

    void getFrequency() {
        QByteArray cmd;
        cmd.append(0xFE).append(0xFE);
        cmd.append(icom->currentRadio().civAddress);
        cmd.append(0xE0).append(0x03).append(0xFD);
        icom->sendCivCommand(cmd);
    }

    void setFrequency(quint64 hz) {
        // Convert to 10-digit BCD, LSB first
        QByteArray bcd;
        for (int i = 0; i < 5; i++) {
            quint8 lo = hz % 10; hz /= 10;
            quint8 hi = hz % 10; hz /= 10;
            bcd.append((hi << 4) | lo);
        }

        QByteArray cmd;
        cmd.append(0xFE).append(0xFE);
        cmd.append(icom->currentRadio().civAddress);
        cmd.append(0xE0).append(0x05);
        cmd.append(bcd);
        cmd.append(0xFD);
        icom->sendCivCommand(cmd);
    }

private slots:
    void parseCivData(const QByteArray& data) {
        if (data.length() < 6) return;
        if (data[0] != (char)0xFE || data[1] != (char)0xFE) return;

        quint8 cmd = data[4];
        if (cmd == 0x03 && data.length() >= 11) {
            // Parse BCD frequency
            quint64 freq = 0;
            quint64 multiplier = 1;
            for (int i = 5; i < 10; i++) {
                quint8 bcd = data[i];
                freq += (bcd & 0x0F) * multiplier;
                multiplier *= 10;
                freq += ((bcd >> 4) & 0x0F) * multiplier;
                multiplier *= 10;
            }
            qInfo() << "Frequency:" << freq << "Hz"
                    << "=" << (freq / 1000000.0) << "MHz";
        }
    }

private:
    IcomNetwork* icom;
};
```

This provides a complete, working implementation for Icom network control without any audio components!
