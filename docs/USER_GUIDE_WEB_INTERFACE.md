# Web Browser Interface Guide

TR4QT includes a built-in web server that allows you to view your contest log from any device with a web browser - phone, tablet, laptop, or another computer on your network.

## Quick Start

1. **Start web server**: Tools → Start Web Server (or press Alt+W)
2. **Note the URL**: Status bar shows "Web server running at http://192.168.1.100:8080"
3. **Open browser**: On any device, navigate to the displayed URL
4. **View your log**: See real-time contest progress

## Starting the Web Server

### From Menu

1. Go to **Tools → Start Web Server**
2. Web server starts on port 8080 (default)
3. Status bar displays: "Web server running at http://YOUR_IP:8080"

### Keyboard Shortcut

- Press **Alt+W** (Option+W on Mac)
- Toggles web server on/off

### Auto-start on Launch (Optional)

Currently manual start only. Auto-start planned for future release.

## Accessing the Web Interface

### From Same Computer

Open browser to:
- `http://localhost:8080`
- Or `http://127.0.0.1:8080`

### From Local Network

1. **Note your IP address**:
   - macOS: System Settings → Network → (your connection) → IP Address
   - Windows: `ipconfig` in Command Prompt, look for "IPv4 Address"
   - Linux: `ip addr show` or `hostname -I`

2. **Open browser** on any device on same network:
   - `http://192.168.1.100:8080` (replace with your IP)

3. **Bookmark it** for easy access during contest

### Example URLs

- From logging station: `http://localhost:8080`
- From phone on WiFi: `http://192.168.1.100:8080`
- From tablet: `http://192.168.1.100:8080`

## Web Interface Features

The web interface shows:

### Header
- **Contest name** (e.g., "CQ WW DX CW 2025")
- **Your callsign** (station info)
- **Auto-refresh** indicator (updates every 2 seconds)

### Score Summary
- **Score**: Current contest score
- **QSOs**: Total contacts
- **Mults**: Total multipliers
- **Rate**: QSOs per hour (last 60 minutes)

### QSO Log Table
All contacts in chronological order:
- **Time** (UTC)
- **Frequency**
- **Mode**
- **Callsign**
- **Received Exchange** (contest-specific fields)
- **Sent Exchange**
- **Points**

**Note**: Table auto-scrolls to show latest QSO at bottom.

### Band Summary
QSO counts by band/mode:
- **Band**: 160m through 10m
- **Mode**: CW, SSB, Digital
- **Count**: QSOs on each band

## Network Configuration

### Firewall Settings

**macOS:**
- System Settings → Network → Firewall
- Allow incoming connections for TR4QT
- Or disable firewall temporarily during contest

**Windows:**
- Windows Defender Firewall → Allow an app
- Add TR4QT to allowed apps
- Or create inbound rule for port 8080

**Linux:**
```bash
sudo ufw allow 8080/tcp  # Ubuntu/Debian with ufw
sudo firewall-cmd --add-port=8080/tcp  # Fedora/RHEL
```

### Router Configuration (for Internet access)

To access from outside your network:

1. **Find your public IP**: Visit https://whatismyip.com
2. **Configure port forwarding** on router:
   - External port: 8080
   - Internal IP: Your TR4QT computer (e.g., 192.168.1.100)
   - Internal port: 8080
   - Protocol: TCP

3. **Access from anywhere**:
   - `http://YOUR_PUBLIC_IP:8080`

**Security Warning**: Port forwarding exposes your log to the internet. Only do this if you understand the security implications. Consider using a VPN instead.

## Advanced: UDP Broadcast Mode

TR4QT also supports UDP broadcasts for multi-station viewing (e.g., club contest setups).

### Enable UDP Broadcasting

1. **File → Preferences → Advanced**
2. **UDP Broadcast Settings**:
   - Check "Enable UDP broadcast"
   - Port: 9871 (default, matches TR4W)
   - Broadcast address: 255.255.255.255 (local subnet)

3. **Click Apply**

### Receiving UDP Broadcasts

Other TR4QT instances on the same network can receive:
1. Enable "Listen for UDP broadcasts" in Preferences
2. View incoming QSOs from all stations in real-time
3. Useful for Multi-Multi operations

**Note**: UDP broadcast is one-way (send only from logging station). For full multi-op support, future TCP networking is planned.

## Use Cases

### 1. Multi-Op Contest Station

**Scenario**: Club contest with logging station and operators in different rooms

**Setup**:
- Main logging computer runs TR4QT with web server
- Operators check rates/mults on phones/tablets
- Multiplier coordinator monitors from laptop

**Benefits**:
- No additional software required
- Works with any device (iOS, Android, Windows, Mac, Linux)
- Real-time updates every 2 seconds

### 2. Remote Operating

**Scenario**: Radio at remote site, operator at home

**Setup**:
- Remote site computer runs TR4QT
- Operator views log from home via VPN or port forwarding
- Coordinate with remote radio control software

**Benefits**:
- Monitor log from anywhere
- Multiple observers possible
- No special client software needed

### 3. Contesting from RV/Portable

**Scenario**: Operating portable with helper/family nearby

**Setup**:
- Logging laptop runs TR4QT
- Family member checks progress on phone
- Share WiFi hotspot for network access

**Benefits**:
- Simple setup (just join same WiFi)
- Share the excitement of contesting
- Track progress toward goals

### 4. Contest Club Gatherings

**Scenario**: Club members operating from same location

**Setup**:
- Each station runs TR4QT with web server (different ports)
- Large display shows combined scores via browser tabs
- Club members monitor each other's progress

**Benefits**:
- Friendly competition
- Coordinate multiplier needs
- Shared experience

## Stopping the Web Server

### From Menu
1. **Tools → Stop Web Server**
2. Status bar shows: "Web server stopped"

### Keyboard Shortcut
- Press **Alt+W** (Option+W on Mac) again

### On Exit
Web server automatically stops when TR4QT closes.

## Security Considerations

### Local Network Only (Recommended)

For home/portable use:
- ✅ Web server on local network only
- ✅ No port forwarding on router
- ✅ Firewall allows only local network access
- **Risk**: Very low (trusted devices only)

### Internet-Accessible (Advanced Users)

If you port-forward for remote access:
- ⚠️ **No authentication** - anyone with URL can view your log
- ⚠️ **Read-only** - viewers cannot modify log
- ⚠️ **Sensitive info** - callsigns/exchanges visible to anyone
- **Risk**: Medium (publicly accessible, but read-only)

**Recommendation**: Use VPN (WireGuard, Tailscale, ZeroTier) instead of port forwarding for secure remote access.

## Troubleshooting

### "Cannot start web server: Port 8080 already in use"

**Cause**: Another application is using port 8080

**Solutions**:
1. Change TR4QT web server port in Preferences
2. Or stop the other application using port 8080:
   ```bash
   # macOS/Linux - find what's using port 8080
   lsof -i :8080

   # Windows
   netstat -ano | findstr :8080
   ```

### "Connection refused" from other devices

**Causes**:
1. Firewall blocking port 8080
2. Wrong IP address
3. Not on same network

**Solutions**:
1. Temporarily disable firewall to test
2. Verify IP with `ipconfig` or `ip addr`
3. Ensure devices on same WiFi/network

### Web page shows "No active contest"

**Cause**: No contest is open in TR4QT

**Solution**: Create or open a contest (File → New/Open Contest)

### Page doesn't auto-refresh

**Cause**: Browser JavaScript disabled or old browser

**Solution**:
- Enable JavaScript in browser settings
- Use modern browser (Chrome, Firefox, Safari, Edge)
- Or manually refresh page (F5 or Cmd+R)

### Slow updates / lag

**Causes**:
1. Network congestion
2. Too many connected clients
3. Large log (10,000+ QSOs)

**Solutions**:
1. Use wired Ethernet instead of WiFi if possible
2. Limit number of simultaneous viewers
3. Performance should be acceptable even with large logs

## Technical Details

### Technology Stack
- **Backend**: Qt HTTP Server (Qt6::HttpServer)
- **Frontend**: HTML + JavaScript (vanilla, no frameworks)
- **Protocol**: HTTP/1.1
- **Data format**: JSON API
- **Refresh**: Client-side polling every 2 seconds

### API Endpoints (for developers)

- `GET /` - Main web interface (HTML)
- `GET /api/status` - Contest status JSON
- `GET /api/qsos` - QSO log JSON
- `GET /api/stats` - Statistics JSON

**Example JSON response** (`/api/status`):
```json
{
  "contest": "CQ WW DX CW 2025",
  "callsign": "W1AW",
  "score": 12500,
  "qsos": 450,
  "mults": 75,
  "rate": 125
}
```

## Future Enhancements

Planned features for web interface:
- [ ] Authentication/password protection
- [ ] HTTPS/TLS support
- [ ] Configurable refresh rate
- [ ] Band map visualization
- [ ] Multiplier needed/worked display
- [ ] Rate graphs and statistics
- [ ] Mobile-optimized responsive design

## Next Steps

- [Learn about ADIF import/export](USER_GUIDE_ADIF.md)
- [Explore keyboard shortcuts](USER_GUIDE_SHORTCUTS.md)
