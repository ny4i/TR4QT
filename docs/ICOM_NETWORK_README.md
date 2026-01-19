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
| Get Frequency | 0x03 | Read VFO A frequency |
| Get Mode | 0x04 | Read VFO A operating mode |
| Set Frequency | 0x05 | Set VFO A frequency |
| Set Mode | 0x06 | Set VFO A operating mode |
| Read S-Meter | 0x15 0x02 | Read signal strength |
| Get TX Status | 0x1C 0x00 | Read PTT status |
| Get VFO B Frequency | 0x25 | Read VFO B frequency (see VFO B Support section) |
| Get VFO B Mode | 0x26 | Read VFO B operating mode (see VFO B Support section) |

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
- Icom IC-7760 CI-V Reference Manual
- TR4W Icom CI-V Information: https://github.com/n4af/TR4W/wiki/Icom-CI-V-Information
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

## Contest Operation Requirements

The following CI-V commands are essential for contest operation and must be verified on each supported radio:

| Function | Command | Status (IC-7760) | Notes |
|----------|---------|------------------|-------|
| Set Frequency | 0x05 | ✅ Implemented | |
| Get Frequency | 0x03 | ✅ Implemented | |
| Set Mode | 0x06 | ✅ Implemented | |
| Get Mode | 0x04 | ✅ Implemented | |
| Get Data Setting | 0x1A 0x06 | ⚠️ Not tested | Data mode sub-mode |
| Split Mode | 0x0F | ✅ Implemented | Get/Set split on/off |
| Set RIT | 0x21 0x00 | ✅ Implemented | Shared RIT/XIT offset |
| Get RIT | 0x21 0x00 | ✅ Implemented | Shared RIT/XIT offset |
| Get Extended VFO | 0x25 | ✅ Implemented | VFO B frequency |
| Get Extended Mode | 0x26 | ✅ Implemented | VFO B mode |
| CW Send | 0x17 | ✅ Implemented | Send CW message |
| Set CW Keyer Speed | 0x14 0x0C | ✅ Implemented | See IC-7760 notes for BCD format |
| Set Filter Width | 0x1A 0x03 | ❌ TODO | Needs implementation |
| Send Voice Memory | 0x28 0x00 0x01-08 | ❌ TODO | Voice memory 1-8 |
| Stop Voice Keyer | 0x28 0x00 0x00 | ❌ TODO | Stop playback |

**Legend:**
- ✅ Implemented and tested
- ⚠️ Implemented but not fully tested
- ❌ Not yet implemented

## General Icom Network Protocol Notes

### Sequence Number Management

**CRITICAL:** All Icom network radios require proper sequence number management (verified on IC-7760, likely applies to IC-7610, IC-9700, etc.):

- **Outer sequence** (UDP packet): Starts at 1, increments per packet
- **Inner sequence** (CI-V stream): Starts at 0, increments per CI-V command
- **Must reset both sequences** when opening a new CI-V connection
- **Common bug:** Double-incrementing the outer sequence causes the radio to request retransmits for "missing" packets

**Implementation:** Increment outer sequence ONLY in `sendTrackedPacket()`, not in the calling functions.

### CI-V Response Timing

After sending the CI-V open command (`0x04`), wait ~200ms before sending additional commands. The radio needs time to process the open packet. Without this delay, commands sent immediately after open are ignored.

## Radio-Specific Protocol Notes

### IC-7760

The IC-7760 has some specific protocol quirks and implementation details:

#### CW Speed (Command 0x14 0x0C)

**Format:** The IC-7760 uses a 2-byte BCD encoding of a 0-255 value range (NOT direct WPM)

**Get Response:**
```
0x14 0x0C <bcd-high> <bcd-low>
```

Example: `0x14 0x0C 0x01 0x08` = BCD "0108" = decimal 108

**Conversion Formula:**
```cpp
// BCD to WPM
int value = ((bcdHigh >> 4) * 10 + (bcdHigh & 0x0F)) * 100 +
            ((bcdLow >> 4) * 10 + (bcdLow & 0x0F));
int wpm = 6 + (value * 42 + 127) / 255;  // Round properly

// WPM to BCD
int value = ((wpm - 6) * 255) / 42;
int hundreds = value / 100;
int tens = (value % 100) / 10;
int ones = value % 10;
quint8 bcdHigh = ((hundreds / 10) << 4) | (hundreds % 10);
quint8 bcdLow = (tens << 4) | ones;
```

**Known Firmware Bug:** The IC-7760 sends value 250 (0x02 0x50) for 48 WPM instead of the correct 255 (0x02 0x55). This causes the calculated speed to be ~47 WPM when the radio displays 48 WPM. Similar off-by-one errors occur at 46-47 WPM. This is a radio firmware issue, not a protocol implementation bug.

#### RIT/XIT (Command 0x21)

The IC-7760 uses a **shared offset** for both RIT and XIT (similar to Elecraft K4 behavior).

**Sub-commands:**
- `0x21 0x00` - Read/Write shared RIT/XIT offset (4 bytes)
- `0x21 0x01` - Read/Write RIT on/off (2 bytes: `01 XX` where XX=00/01)
- `0x21 0x02` - Read/Write XIT on/off (Icom calls this "Delta TX", 2 bytes: `02 XX`)
- `0x21 0x03` - NOT SUPPORTED (returns only echo, no offset data)

**Shared Offset Format (0x21 0x00):**
```
0x00 <bcd-high> <bcd-low> <sign>
```

Example: `0x00 0x48 0x00` = "0048" Hz positive offset

**Parsing:**
```cpp
quint8 bcdHigh = responseData[1];
quint8 bcdLow = responseData[2];
quint8 sign = responseData[3];

int offset = ((bcdHigh >> 4) & 0x0F) * 1000 +  // Thousands
             (bcdHigh & 0x0F) * 100 +           // Hundreds
             ((bcdLow >> 4) & 0x0F) * 10 +      // Tens
             (bcdLow & 0x0F);                   // Ones

if (sign != 0x00) offset = -offset;

// Both RIT and XIT use the same offset value
ritOffset = offset;
xitOffset = offset;
```

**Important:** There is only ONE offset value shared by both RIT and XIT. When RIT is enabled, the offset applies to receive. When XIT is enabled, the offset applies to transmit. Both can be enabled simultaneously with the same offset.

#### CI-V Address

- Default CI-V address: **0xB2**
- Controller address: **0xE1** (use 0xE1, not the typical 0xE0)

### IC-7610

*(To be documented after testing - likely shares general sequence number requirements, CW speed format may differ)*

## VFO B Support (Commands 0x25 and 0x26)

### Supported Radios

The following Icom radios support VFO B frequency and mode control via commands **0x25** (frequency) and **0x26** (mode):

- IC-705
- IC-7100
- IC-7300
- IC-7800
- IC-7850
- IC-7851
- IC-7600
- IC-7610
- IC-7700
- IC-7760
- IC-905
- IC-9700

**Important Notes:**
- **All network-capable Icom radios** in TR4QT support these commands (since all network radios are in the above list)
- These commands work via both **network (UDP)** and **serial (CI-V)** connections
- VFO B is sometimes called "Sub receiver" in Icom documentation
- Dual watch mode is **not required** - VFO B can be queried/set independently

### Protocol Format Differences

#### Standard Radios (IC-7610, IC-9700, etc.)
Most Icom radios use the standard format without sub-command bytes:

**Query VFO B frequency:**
```
FE FE <radio> E0 25 FD
```
**Response:**
```
FE FE E0 <radio> 25 <5 BCD bytes> FD
```

**Set VFO B frequency:**
```
FE FE <radio> E0 25 <5 BCD bytes> FD
```

#### IC-7760 Extended Format
The IC-7760 uses an **extended format** with a sub-command byte (0x01) to indicate VFO B:

**Query VFO B frequency:**
```
FE FE <radio> E0 25 01 FD
```
**Response:**
```
FE FE E0 <radio> 25 01 <5 BCD bytes> FD
                       ^-- Sub-command (01 = VFO B)
```

**Set VFO B frequency:**
```
FE FE <radio> E0 25 01 <5 BCD bytes> FD
```

**Set VFO B mode:**
```
FE FE <radio> E0 26 01 <mode> <filter> FD
```

### Implementation Notes

The `IcomRadio` class automatically handles both formats:
- Detects IC-7760 extended format (6 bytes for frequency, 3 bytes for mode)
- Falls back to standard format (5 bytes for frequency, 2 bytes for mode)
- Sends sub-command byte `0x01` when setting/querying VFO B

Example parsing code in `IcomRadio::parseFrequencyResponse()`:
```cpp
if (data.length() == 6 && vfo == VFO::VFO_B) {
    // IC-7760 format: skip first byte (sub-command)
    quint8 subCmd = (quint8)data[0];
    bcdData = data.mid(1, 5);
} else if (data.length() == 5) {
    // Standard format
    bcdData = data;
}
```

### Testing VFO B

To verify VFO B support on a new radio model:

1. **Query VFO B frequency** - Send `0x25` (with `0x01` sub-cmd for IC-7760)
2. **Check response format** - 5 bytes (standard) or 6 bytes (IC-7760)
3. **Set VFO B frequency** - Verify frequency changes
4. **Query/Set VFO B mode** - Verify mode changes with command `0x26`

If the radio responds with `0xFA` (NG/No Good), the radio may not support VFO B commands or requires a different format.

### Radios Without Native VFO B Commands

Older Icom radios that do **not** support commands 0x25/0x26 use a different approach:
1. Send a command to swap VFO A and VFO B (making VFO B the main VFO)
2. Read the frequency using standard command 0x03
3. Swap back to restore original configuration

**TR4QT does not implement this swap-based approach** because:
- It's complex and error-prone (requires state tracking and swap-back)
- Creates race conditions if multiple commands are sent
- All modern network-capable Icom radios support 0x25/0x26 natively
- The swap approach is considered obsolete and unreliable for contest operation

If you need VFO B support on older radios, consider using Hamlib which may implement the swap logic.

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
