# K4Discovery - Standalone Version

Self-contained Elecraft K4 radio network discovery class for Qt6 projects.

## What It Does

Discovers Elecraft K4 radios on your local network using UDP broadcast discovery protocol.

- Broadcasts "findk4" message to all network interfaces
- Listens for K4 responses in format: `k4:index:ip:serial`
- Returns radio information: IP address, serial number, hostname

## Requirements

- Qt 6.x
- Qt modules: Core, Network

## Installation

Copy these two files into your Qt project:
- `K4Discovery_standalone.h`
- `K4Discovery_standalone.cpp`

Add to your `.pro` file (qmake):
```qmake
QT += core network
SOURCES += K4Discovery_standalone.cpp
HEADERS += K4Discovery_standalone.h
```

Or `CMakeLists.txt` (CMake):
```cmake
find_package(Qt6 REQUIRED COMPONENTS Core Network)
target_sources(your_target PRIVATE
    K4Discovery_standalone.cpp
    K4Discovery_standalone.h
)
target_link_libraries(your_target PRIVATE Qt6::Core Qt6::Network)
```

## Basic Usage

```cpp
#include "K4Discovery_standalone.h"

// In your class:
K4Discovery* discovery = new K4Discovery(this);

// Connect signals
connect(discovery, &K4Discovery::radioFound, this, [](const K4RadioInfo& radio) {
    qDebug() << "Found K4:" << radio.ipAddress << "Serial:" << radio.serialNumber;
});

connect(discovery, &K4Discovery::discoveryFinished, this, [](int count) {
    qDebug() << "Discovery finished. Found" << count << "radio(s)";
});

// Start discovery (runs for 3 seconds)
discovery->startDiscovery();
```

## K4RadioInfo Structure

```cpp
struct K4RadioInfo {
    QString rigType;      // "k4"
    int rigIndex;         // Radio index (typically 0)
    QString ipAddress;    // IP address (e.g., "192.168.1.100")
    QString serialNumber; // Serial number (e.g., "278")

    QString hostname() const; // Returns "K4-SN00278.local" format
};
```

## Signals

- `radioFound(const K4RadioInfo& radio)` - Emitted when a K4 is discovered
- `discoveryFinished(int count)` - Emitted when discovery completes (after 3 seconds)
- `error(const QString& errorMessage)` - Emitted on error (e.g., no network interfaces)

## Logging

Uses Qt's standard logging system via `QLoggingCategory`.

By default, only warnings and info messages are shown. To enable debug messages:

```cpp
// In your main.cpp or initialization code:
QLoggingCategory::setFilterRules("K4Discovery.debug=true");
```

## How It Works

The K4 discovery protocol requires careful network handling:

1. **Per-Interface Sockets**: Creates one UDP socket per network interface
2. **Interface Binding**: Each socket binds to the specific IP of its interface
3. **Subnet Broadcast**: Sends to subnet broadcast address (e.g., 192.168.1.255)
4. **Ephemeral Ports**: OS assigns temporary port for each socket
5. **Response Handling**: K4 responds to the source IP:port of the discovery packet

This approach ensures the K4 sees the correct source IP and can respond back to the same socket.

## Example: Complete Application

```cpp
#include <QCoreApplication>
#include "K4Discovery_standalone.h"

int main(int argc, char *argv[]) {
    QCoreApplication app(argc, argv);

    // Enable debug logging (optional)
    QLoggingCategory::setFilterRules("K4Discovery.debug=true");

    K4Discovery discovery;

    QObject::connect(&discovery, &K4Discovery::radioFound, [](const K4RadioInfo& radio) {
        qInfo() << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━";
        qInfo() << "Found K4 Radio:";
        qInfo() << "  IP Address:" << radio.ipAddress;
        qInfo() << "  Serial:    " << radio.serialNumber;
        qInfo() << "  Hostname:  " << radio.hostname();
        qInfo() << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━";
    });

    QObject::connect(&discovery, &K4Discovery::discoveryFinished, [&app](int count) {
        qInfo() << "\nDiscovery complete. Found" << count << "K4 radio(s).";
        app.quit();
    });

    QObject::connect(&discovery, &K4Discovery::error, [&app](const QString& msg) {
        qWarning() << "Error:" << msg;
        app.quit();
    });

    qInfo() << "Starting K4 discovery (3 second timeout)...\n";
    discovery.startDiscovery();

    return app.exec();
}
```

## Troubleshooting

**No radios found?**
- Ensure K4 is powered on and connected to the same network
- Check firewall settings (UDP port 9100)
- Enable debug logging to see network interface details
- K4 must be on the same subnet as one of your network interfaces

**Multiple network interfaces?**
- The class automatically sends discovery on all active interfaces
- This is by design - ensures discovery works regardless of network topology

## License

Originally developed for TR4QT (https://github.com/ny4i/TR4QT)

Standalone version: Use freely in your projects. Attribution appreciated but not required.

## Credits

Based on the `findk4.py` utility and refined through extensive testing with real K4 radios.
