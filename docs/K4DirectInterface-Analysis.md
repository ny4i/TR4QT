# K4 Direct Interface Analysis

## Overview

Analysis of TR4W's `TK4Radio` class for implementing a direct K4 interface in TR4QT, bypassing Hamlib for improved performance and feature access.

## TR4W Implementation Analysis

### Architecture

**Class Hierarchy:**
```pascal
TK4Radio = class(TNetRadioBase)
```

- Extends network-based radio base class
- Direct TCP socket communication with K4
- Event-driven message processing
- Automatic status updates via AI (Auto Information) mode

### K4 Command Protocol

**Message Format:**
- Commands: Semicolon-terminated ASCII strings (e.g., `FA00014200000;`)
- Read Terminator: `;` character
- VFO A: Standard commands (`FA;`, `MD;`, etc.)
- VFO B: Commands with `$` suffix (`FA$;`, `MD$;`, etc.)

**Key Commands Used:**

| Command | Description | Example |
|---------|-------------|---------|
| `FA` | Set/Get Frequency VFO A | `FA00014200000;` (14.200 MHz) |
| `FB` | Set/Get Frequency VFO B | `FB00007100000;` (7.100 MHz) |
| `MD` | Set/Get Mode | `MD3;` (CW) |
| `DT` | Set/Get Data Sub-mode | `DT0;` (DATA A) |
| `IF` | Get Transceiver Info (comprehensive) | `IF;` |
| `KS` | Set/Get CW Speed | `KS025;` (25 WPM) |
| `KY` | Send CW | `KY CQ TEST;` |
| `RT` | RIT On/Off | `RT1;` (on), `RT0;` (off) |
| `XT` | XIT On/Off | `XT1;` (on), `XT0;` (off) |
| `RO` | Set RIT/XIT Offset | `RO+0100;` (+100 Hz) |
| `FT` | Split On/Off | `FT1;` (on), `FT0;` (off) |
| `BN` | Set Band | `BN05;` (20m) |
| `FP` | Filter Preset | `FP3;` (filter 3) |
| `TX` | Transmit | `TX;` |
| `RX` | Receive | `RX;` |
| `AI` | Auto Information Mode | `AI5;` (comprehensive updates) |
| `ID` | Radio ID | `ID;` → `ID017;` (K4) |
| `OM` | Option Modules | `OM;` → `OM APXSHML14---;` |
| `DA` | DVK (Voice Keyer) | `DAMP100000;` (play msg 1) |
| `RC` | RIT/XIT Clear | `RC;` |
| `UP`/`DN` | VFO Up/Down | `UP;`, `DN;` |
| `UPB`/`DNB` | VFO B Up/Down | `UPB;`, `DNB;` |

### IF Command (Comprehensive Status)

**Format:** `IF[f]*****+yyyyrx*00tmvspbd1*;`

**Fields:**
- `[f]`: Operating frequency (11 digits, Hz)
- `+yyyy`: RIT/XIT offset (-9999 to +9999 Hz)
- `r`: RIT on/off (1/0)
- `x`: XIT on/off (1/0)
- `t`: TX/RX state (1/0)
- `m`: Operating mode (see MD command)
- `v`: Active VFO (0=A, 1=B)
- `s`: Scan in progress (1/0)
- `p`: Split mode (1/0)
- `d`: Data sub-mode (0=DATA A, 1=AFSK A, 2=FSK D, 3=PSK D)

**Example:**
```
IF00014200000     +0000001001000301;
   └─14.200 MHz  └─RIT +0Hz, RIT on, XIT off, RX, mode 3 (CW), VFO A, no split, data mode 0
```

### Auto Information (AI) Mode

**TR4W uses AI5:**
```pascal
Self.SetAIMode(5);
```

**AI Modes:**
- `AI0`: Off (no automatic updates)
- `AI1`: Basic updates
- `AI2`: Extended updates
- `AI3`: More comprehensive
- **`AI5`: Most comprehensive** (used by TR4W)

**Benefit:** Radio automatically sends status updates on changes, eliminating need for polling.

### Mode Handling

**K4 Mode Numbers:**
```
0 = None
1 = LSB
2 = USB
3 = CW
4 = FM
5 = AM
6 = Data (with DT sub-mode: 0=DATA A, 1=AFSK A, 2=FSK D, 3=PSK D)
7 = CW Reverse
9 = Data Reverse
```

**Data Sub-modes** (DT command):
- 0: DATA A
- 1: AFSK A
- 2: FSK D
- 3: PSK D

**Setting Mode:**
```pascal
// Set USB
SendToRadio('MD2;');

// Set PSK-D
SendToRadio('MD6;');  // Data mode
SendToRadio('DT3;');  // PSK sub-mode
```

### CW Handling

**Buffer and Send:**
```pascal
procedure TK4Radio.BufferCW(cwChars: string);
begin
   Self.CWBuffer := Self.CWBuffer + cwChars;
end;

procedure TK4Radio.SendCW;
begin
   Self.SendToRadio('KY ' + Self.CWBuffer + ';');
   Self.CWBuffer := '';
end;

procedure TK4Radio.StopCW;
begin
   Self.SendToRadio(Chr(4) + ';RX;');  // Ctrl-D stops CW, then RX
end;
```

**CW Speed:**
```pascal
Self.SendToRadio(Format('KS%3d;',[speed]));  // 8-100 WPM
```

### DVK (Voice Keyer)

**Play Memory 1-8:**
```pascal
// Play message 1
Self.SendToRadio('DAMP100000;');  // DAMPmnnnnn; m=memory, nnnnn=repeat interval

// Stop DVK
Self.SendToRadio('DA0;');
```

## Benefits Over Hamlib

### 1. **Performance**
- **Direct TCP**: No Hamlib library overhead
- **AI Mode**: Radio pushes updates automatically (no polling needed)
- **Lower Latency**: Typical command round-trip <10ms vs Hamlib 50-100ms

### 2. **K4-Specific Features**
- **Option Module Detection**: `OM` command (KPA1500, P3, etc.)
- **Filter Presets**: `FP` command (5 filter slots)
- **DVK Control**: Voice keyer messages
- **Data Sub-modes**: Full PSK/FSK/AFSK control
- **VFO-specific Commands**: Independent A/B control with `$` suffix

### 3. **Reliability**
- **Simpler Stack**: Fewer failure points
- **Better Error Handling**: Direct protocol parsing, clearer errors
- **No Hamlib Version Issues**: No dependency on Hamlib release cycle

### 4. **Features Hamlib Doesn't Support**
From TR4W experience:
- Full dual VFO control
- K4-specific filter settings
- DVK memory playback
- Option module queries
- K4D/K4HD model detection

## TR4QT Implementation Proposal

### Architecture

```cpp
// RadioInterface.h
class RadioInterface {
public:
    virtual bool connect() = 0;
    virtual void disconnect() = 0;
    virtual bool setFrequency(freq_t freq, VFO vfo) = 0;
    virtual bool setMode(ModeType mode, VFO vfo) = 0;
    virtual RadioState pollCurrentState() = 0;
    // ... existing interface
};

// K4Radio.h (new class)
class K4Radio : public RadioInterface, public QObject {
    Q_OBJECT
public:
    K4Radio(const QString& host, quint16 port);
    ~K4Radio() override;

    // RadioInterface implementation
    bool connect() override;
    void disconnect() override;
    bool setFrequency(freq_t freq, VFO vfo) override;
    bool setMode(ModeType mode, VFO vfo) override;
    RadioState pollCurrentState() override;

    // K4-specific features
    bool enableAIMode(int level = 5);
    bool queryOptionModules(QStringList& modules);
    bool setFilterPreset(int preset, VFO vfo);
    bool playDVKMessage(int message);
    bool stopDVK();

    // CW
    bool bufferCW(const QString& text);
    bool sendCW();
    bool stopCW();
    bool setCWSpeed(int wpm);

signals:
    void stateChanged(const RadioState& newState);
    void errorOccurred(const QString& error);

private slots:
    void onSocketConnected();
    void onSocketDisconnected();
    void onSocketError(QAbstractSocket::SocketError error);
    void onReadyRead();

private:
    QTcpSocket* m_socket;
    QString m_host;
    quint16 m_port;
    RadioState m_state;
    QString m_receiveBuffer;
    QString m_cwBuffer;

    // K4 protocol helpers
    void sendCommand(const QString& cmd, VFO vfo = VFO_A);
    void processMessage(const QString& message);
    bool parseIFCommand(const QString& response);
    ModeType modeStringToMode(const QString& mode, const QString& dataMode);
    QString modeToModeString(ModeType mode, int& dataModeInt);
    BandType bandNumberToBand(int bandNum);
};
```

### RadioFactory Integration

```cpp
// RadioFactory.h (enhanced)
class RadioFactory {
public:
    enum RadioType {
        HAMLIB_RADIO,
        K4_DIRECT,
        // Future: K3_DIRECT, IC7300_DIRECT, etc.
    };

    static RadioInterface* createRadio(RadioType type, const RadioConfig& config);
};

// Usage
RadioConfig config;
config.type = RadioFactory::K4_DIRECT;
config.networkHost = "192.168.1.100";
config.networkPort = 12345;

RadioInterface* radio = RadioFactory::createRadio(RadioFactory::K4_DIRECT, config);
```

### Message Processing (Event-Driven)

```cpp
void K4Radio::onReadyRead() {
    while (m_socket->canReadLine()) {
        QByteArray line = m_socket->readLine();
        QString message = QString::fromLatin1(line).trimmed();

        if (message.endsWith(';')) {
            message.chop(1);  // Remove trailing ;
            processMessage(message);
        }
    }
}

void K4Radio::processMessage(const QString& message) {
    if (message.length() < 2) return;

    QString command = message.left(2);
    bool isVFOB = (message.length() > 2 && message[2] == '$');
    QString data = isVFOB ? message.mid(3) : message.mid(2);
    VFO vfo = isVFOB ? VFO_B : VFO_A;

    if (command == "FA") {
        // Frequency VFO A
        bool ok;
        freq_t freq = data.toLongLong(&ok);
        if (ok) {
            m_state.frequencyA = freq;
            emit stateChanged(m_state);
        }
    }
    else if (command == "FB") {
        // Frequency VFO B
        // ...
    }
    else if (command == "MD") {
        // Mode
        m_state.modeA = modeStringToMode(data, "0");
        emit stateChanged(m_state);
    }
    else if (command == "IF") {
        // Comprehensive status
        parseIFCommand(message);
        emit stateChanged(m_state);
    }
    // ... handle other commands
}
```

### IF Command Parser

```cpp
bool K4Radio::parseIFCommand(const QString& cmd) {
    // IF[f]*****+yyyyrx*00tmvspbd1*;
    if (cmd.length() < 36) return false;

    int pos = 2;  // Skip "IF"

    // Frequency (11 digits)
    freq_t freq = cmd.mid(pos, 11).toLongLong();
    m_state.frequencyA = freq;
    pos += 11;

    // Skip 5 spaces
    pos += 5;

    // RIT/XIT offset sign and value
    int sign = (cmd[pos] == '-') ? -1 : 1;
    pos++;
    int ritOffset = cmd.mid(pos, 4).toInt() * sign;
    m_state.ritOffset = ritOffset;
    pos += 4;

    // RIT on/off
    m_state.ritEnabled = (cmd[pos] == '1');
    pos++;

    // XIT on/off
    m_state.xitEnabled = (cmd[pos] == '1');
    pos++;

    // Skip space and "00"
    pos += 3;

    // TX/RX
    m_state.transmitting = (cmd[pos] == '1');
    pos++;

    // Mode
    QString modeStr = cmd.mid(pos, 1);
    pos++;

    // Active VFO
    bool vfoB = (cmd[pos] == '1');
    pos++;

    // Skip scan
    pos++;

    // Split
    m_state.splitEnabled = (cmd[pos] == '1');
    pos++;

    // Skip band byte
    pos++;

    // Data sub-mode
    QString dataModeStr = cmd.mid(pos, 1);

    m_state.modeA = modeStringToMode(modeStr, dataModeStr);

    emit stateChanged(m_state);
    return true;
}
```

### Connection and Initialization

```cpp
bool K4Radio::connect() {
    m_socket = new QTcpSocket(this);

    connect(m_socket, &QTcpSocket::connected, this, &K4Radio::onSocketConnected);
    connect(m_socket, &QTcpSocket::disconnected, this, &K4Radio::onSocketDisconnected);
    connect(m_socket, QOverload<QAbstractSocket::SocketError>::of(&QTcpSocket::error),
            this, &K4Radio::onSocketError);
    connect(m_socket, &QTcpSocket::readyRead, this, &K4Radio::onReadyRead);

    m_socket->connectToHost(m_host, m_port);
    return m_socket->waitForConnected(2000);
}

void K4Radio::onSocketConnected() {
    LOG_INFO("K4Radio", "Connected to K4 at " + m_host + ":" + QString::number(m_port));

    // Set AI5 mode for automatic updates
    enableAIMode(5);

    // Query initial state
    sendCommand("RT;XT;RO;FT;ID;MD;DT;IF;");
    sendCommand("RT$;XT$;RO$;MD$;DT$;IF$;");  // VFO B
    sendCommand("KS;BN;FP;");
}

bool K4Radio::enableAIMode(int level) {
    if (level < 0 || level > 5) return false;
    sendCommand(QString("AI%1;").arg(level));
    return true;
}
```

### CW Implementation

```cpp
bool K4Radio::bufferCW(const QString& text) {
    m_cwBuffer += text;
    return true;
}

bool K4Radio::sendCW() {
    if (m_cwBuffer.isEmpty()) return false;
    sendCommand("KY " + m_cwBuffer + ";");
    m_cwBuffer.clear();
    return true;
}

bool K4Radio::stopCW() {
    sendCommand(QChar(0x04) + QString(";RX;"));  // Ctrl-D + RX
    return true;
}

bool K4Radio::setCWSpeed(int wpm) {
    if (wpm < 8 || wpm > 100) {
        LOG_ERROR("K4Radio", QString("CW speed out of range: %1 (must be 8-100)").arg(wpm));
        return false;
    }
    sendCommand(QString("KS%1;").arg(wpm, 3, 10, QChar('0')));
    return true;
}
```

## Implementation Plan

### Phase 1: Core K4Radio Class (Week 1)
- [ ] Create `K4Radio.h` and `K4Radio.cpp`
- [ ] Implement TCP socket connection
- [ ] Implement basic command sending (`sendCommand()`)
- [ ] Implement message reception and parsing (`processMessage()`)
- [ ] Parse IF command for comprehensive status
- [ ] Emit `stateChanged()` signals

### Phase 2: RadioInterface Implementation (Week 1-2)
- [ ] Implement `setFrequency()`
- [ ] Implement `setMode()` with data sub-mode handling
- [ ] Implement `pollCurrentState()` (return cached state from AI updates)
- [ ] Implement RIT/XIT control
- [ ] Implement split mode
- [ ] Implement band selection

### Phase 3: CW Support (Week 2)
- [ ] Implement `bufferCW()`, `sendCW()`, `stopCW()`
- [ ] Implement `setCWSpeed()`
- [ ] Test CW sending in MainWindow

### Phase 4: K4-Specific Features (Week 2-3)
- [ ] Implement `queryOptionModules()`
- [ ] Implement `setFilterPreset()`
- [ ] Implement DVK control (`playDVKMessage()`, `stopDVK()`)
- [ ] Add VFO bump up/down

### Phase 5: RadioFactory Integration (Week 3)
- [ ] Add `RadioFactory::K4_DIRECT` type
- [ ] Add UI option in PreferencesDialog for radio type selection
- [ ] Test switching between Hamlib and K4Direct
- [ ] Add K4 detection logic (try K4Direct first, fall back to Hamlib)

### Phase 6: Testing and Polish (Week 3-4)
- [ ] Unit tests for K4Radio
- [ ] Integration tests with real K4
- [ ] Performance comparison (Hamlib vs K4Direct)
- [ ] Documentation

## Performance Expectations

Based on TR4W experience:

| Operation | Hamlib | K4 Direct | Improvement |
|-----------|--------|-----------|-------------|
| Frequency change | 50-100ms | 5-10ms | **5-10x faster** |
| Mode change | 50-100ms | 5-10ms | **5-10x faster** |
| Status poll | 80-120ms | 0ms (AI push) | **Infinite** |
| CW latency | 100-150ms | 10-20ms | **5-10x faster** |

## Risks and Mitigation

### Risk 1: K4 Firmware Changes
**Mitigation:** Elecraft's command set is stable, documented, backward-compatible

### Risk 2: Network Issues
**Mitigation:** Same as current Hamlib TCP implementation, already handled

### Risk 3: Maintenance Burden
**Mitigation:**
- Well-documented protocol
- Single radio model (K4/K4D/K4HD)
- Can fall back to Hamlib if needed

### Risk 4: Missing Hamlib Features
**Mitigation:**
- Keep HamlibRadio for other radio models
- RadioFactory allows choice at runtime
- K4 users can still use Hamlib if preferred

## Recommendations

1. **Implement K4Radio as optional enhancement**
   - Keep HamlibRadio as default
   - Add K4Radio as "K4 Direct Mode" option in preferences
   - Allow users to choose based on their needs

2. **Use AI5 mode for automatic updates**
   - Eliminate polling overhead
   - Reduce latency to minimum
   - Match TR4W's proven approach

3. **Start with core features, add K4-specific later**
   - Phase 1-3: Feature parity with HamlibRadio
   - Phase 4: K4-specific enhancements (OM, FP, DVK)
   - Validate approach before investing in advanced features

4. **Consider future expansion**
   - K3Radio class (similar protocol)
   - IC7300Radio class (Icom CI-V protocol)
   - Framework for direct radio support

## Conclusion

The TR4W `TK4Radio` implementation provides a proven blueprint for direct K4 control. Implementing this in TR4QT would:

- **Eliminate Hamlib overhead** (5-10x latency reduction)
- **Enable K4-specific features** (option modules, filters, DVK)
- **Improve user experience** (faster response, fewer delays)
- **Maintain compatibility** (RadioFactory allows Hamlib fallback)

**Estimated effort:** 3-4 weeks for full implementation and testing.

**Recommendation:** Proceed with implementation as optional K4 Direct Mode.
