# Radio Configuration Guide

TR4QT supports over 200 radio models through the Hamlib library. Radio control enables automatic frequency/mode logging, band switching, and CW keying.

## Prerequisites

**macOS**: Hamlib is bundled with TR4QT - no additional installation needed.

**Windows**: Hamlib is bundled with TR4QT - no additional installation needed.

**Linux**: Install Hamlib system package:
```bash
sudo apt-get install libhamlib4  # Ubuntu/Debian
sudo dnf install hamlib           # Fedora
```

## Configure Your Radio

### Step 1: Connect Your Radio

Choose one of these connection methods:

**Serial (RS-232):**
- Connect radio's CAT/CI-V port to computer's serial port
- Or use USB-to-Serial adapter
- Note the port name (e.g., `/dev/tty.usbserial` on macOS, `COM3` on Windows)

**USB:**
- Connect radio's USB port to computer
- Install radio manufacturer's USB driver if required
- Note the virtual COM port created

**Network:**
- Use Hamlib's `rigctld` network server
- Or connect to radio's built-in network interface (e.g., Icom RS-BA1)

### Step 2: Open Radio Configuration

1. Go to **Radio → Configure Radio**
2. The Radio Configuration dialog opens

### Step 3: Select Radio Model

1. Click the **Model** dropdown
2. Search for your radio manufacturer and model
3. Common models:
   - Icom IC-7300: "Icom IC-7300"
   - Yaesu FT-991A: "Yaesu FT-991A"
   - Kenwood TS-590SG: "Kenwood TS-590SG"
   - Elecraft K3: "Elecraft K3/KX3"

**Tip**: Use Ctrl+F (Cmd+F on Mac) to search in the dropdown.

### Step 4: Configure Connection

**For Serial/USB connections:**
- **Device**: Enter port name
  - macOS: `/dev/tty.usbserial-XXXXX` or `/dev/cu.usbserial-XXXXX`
  - Windows: `COM3`, `COM4`, etc.
  - Linux: `/dev/ttyUSB0`, `/dev/ttyACM0`, etc.
- **Baud Rate**: Set to match radio (common: 9600, 19200, 38400, 115200)
- **Data Bits**: Usually 8
- **Stop Bits**: Usually 1
- **Parity**: Usually None
- **RTS/CTS**: Enable if radio requires hardware flow control

**For Network connections:**
- **Device**: `localhost:4532` (if using rigctld)
- Or radio's IP:PORT (e.g., `192.168.1.100:4532`)

### Step 5: Test Connection

1. Click **Test** button
2. TR4QT attempts to communicate with radio
3. Success: Shows radio model and firmware version
4. Failure: Shows error message (see Troubleshooting below)

### Step 6: Enable Auto-Reconnect (Recommended)

- Check **Auto-reconnect on disconnect**
- TR4QT will automatically retry if radio connection is lost
- Useful for USB radios that disconnect during computer sleep

### Step 7: Save and Connect

1. Click **OK** to save configuration
2. Go to **Radio → Connect**
3. Status bar shows "Radio connected"
4. Radio status grid (bottom) shows current frequency/mode

## Verifying Radio Control

When connected, you should see:

1. **Bottom status grid** updates with:
   - Current frequency (e.g., "14.025")
   - Current band/mode (e.g., "20CW")
   - Green connection indicator

2. **Band switching works**:
   - Click a band button in Band Summary Grid
   - Radio changes frequency
   - New band is highlighted

3. **Frequency changes reflected**:
   - Change frequency on radio
   - TR4QT updates band/frequency display (polled every 1 second)

## Common Radio Models - Quick Reference

### Icom Radios

**IC-7300 / IC-7610:**
- Model: "Icom IC-7300" or "Icom IC-7610"
- Baud: 19200 or 115200 (set in radio menu)
- Device: USB creates virtual serial port
- CI-V Address: 0x94 (IC-7300), 0x98 (IC-7610)

**IC-9700:**
- Model: "Icom IC-9700"
- Baud: 19200 or 115200
- Device: USB virtual serial port

### Yaesu Radios

**FT-991 / FT-991A:**
- Model: "Yaesu FT-991A"
- Baud: 38400
- Device: USB virtual serial port (ENHANCED port)

**FT-DX10 / FT-DX101:**
- Model: "Yaesu FT-DX101"
- Baud: 38400
- Device: USB virtual serial port

### Kenwood Radios

**TS-590S / TS-590SG:**
- Model: "Kenwood TS-590SG"
- Baud: 115200
- Device: USB virtual serial port

**TS-890S:**
- Model: "Kenwood TS-890S"
- Baud: 115200
- Device: USB virtual serial port

### Elecraft Radios

**K3 / KX3:**
- Model: "Elecraft K3/KX3"
- Baud: 38400
- Device: USB virtual serial port

**K4:**
- Model: "Elecraft K4"
- Baud: 38400
- Device: USB virtual serial port

## Troubleshooting

### "Failed to open device"

**macOS**: Permission denied on `/dev/tty.xxx`
- Check device name with: `ls /dev/tty.*`
- Try alternate name: `/dev/cu.usbserial` instead of `/dev/tty.usbserial`

**Windows**: "COM3 not found"
- Open Device Manager → Ports (COM & LPT)
- Verify COM port number
- Check if driver installed for USB-to-Serial adapter

**Linux**: Permission denied on `/dev/ttyUSB0`
- Add user to dialout group: `sudo usermod -a -G dialout $USER`
- Log out and log back in
- Or run with sudo (not recommended)

### "Radio not responding" / "Timeout"

1. **Check cable**: Ensure CAT cable is firmly connected
2. **Verify baud rate**: Must match radio's CAT settings
3. **Check radio menu**: Ensure CAT is enabled in radio's menu
4. **Test with Hamlib directly**:
   ```bash
   # macOS/Linux
   rigctl -m 3073 -r /dev/tty.usbserial -s 19200 f

   # Windows (Git Bash or WSL)
   rigctl -m 3073 -r COM3 -s 19200 f
   ```
   (Replace model number -m and settings as needed)

### Wrong Frequency/Mode Displayed

1. **Polling interval**: TR4QT polls every 1 second, not real-time
2. **Band decode**: Ensure radio is in amateur band (not general coverage)
3. **Mode mapping**: Some modes may not map correctly (report as bug)

### Radio Disconnects Randomly

**USB radios**:
- Enable "Auto-reconnect on disconnect" in preferences
- On macOS: System Settings → Energy Saver → Disable "Put hard disks to sleep when possible"
- Check USB cable quality (ferrite cores help with RF interference)

**Serial radios**:
- Check for loose connections
- Verify ground between radio and computer
- Add ferrite cores to CAT cable if RF issues suspected

### CW Keying Not Working

TR4QT currently supports radio control only (frequency/mode).
CW keying support is planned for future release.

## Advanced: Using rigctld Network Server

For remote radio control or sharing one radio with multiple apps:

1. **Start rigctld on radio computer**:
   ```bash
   rigctld -m 3073 -r /dev/tty.usbserial -s 19200 -T 0.0.0.0
   ```

2. **Configure TR4QT to use network**:
   - Model: "Hamlib NET rigctl"
   - Device: `192.168.1.100:4532` (IP of computer running rigctld)
   - Baud: (ignored for network)

3. **Benefits**:
   - Multiple apps can control same radio
   - Radio computer can be different from logging computer
   - Useful for headless Raspberry Pi rig control

## Hamlib Debug Logging

If experiencing radio control issues:

1. Go to **File → Preferences → Advanced**
2. Check **Enable Hamlib Debug Output**
3. Restart TR4QT
4. Check `~/.tr4qt/tr4qt.log` for detailed Hamlib communication

**Warning**: Debug output is VERY verbose. Only enable when troubleshooting.

## Next Steps

- [Start logging contests](USER_GUIDE_CONTEST_OPERATION.md)
- [Learn keyboard shortcuts for band switching](USER_GUIDE_SHORTCUTS.md)
