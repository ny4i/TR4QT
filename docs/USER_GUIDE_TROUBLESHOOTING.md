# Troubleshooting Guide

Common issues and solutions for TR4QT.

## Installation Issues

### macOS: "App can't be opened because it is from an unidentified developer"

**Cause**: TR4QT is not code-signed with Apple Developer certificate

**Solution**:
1. Right-click (or Control+click) on `tr4qt.app`
2. Select **Open** from context menu
3. Click **Open** when warned about unidentified developer
4. App is now whitelisted - future launches work normally

**Alternative**:
```bash
# Remove quarantine attribute
xattr -cr /Applications/tr4qt.app
```

### Windows: "Windows protected your PC" (SmartScreen)

**Cause**: TR4QT exe is not code-signed

**Solution**:
1. Click **More info** link
2. Click **Run anyway** button
3. TR4QT launches normally

### Linux: Missing Qt libraries

**Error**: `error while loading shared libraries: libQt6Core.so.6`

**Solution** (Ubuntu/Debian):
```bash
sudo apt-get update
sudo apt-get install qt6-base-dev libqt6sql6-sqlite
```

**Solution** (Fedora):
```bash
sudo dnf install qt6-qtbase qt6-qtbase-gui
```

## First Launch Issues

### Menu bar missing (macOS)

**Symptom**: No menu bar visible in application

**Solutions**:
1. **Restart TR4QT**: Quit (Cmd+Q) and relaunch
2. **Check system menu**: Sometimes macOS shows app menu in system bar
3. **Full screen mode**: Exit full screen (Control+Cmd+F)
4. **Multiple monitors**: Move window to primary display

### Application window off-screen

**Symptom**: Can't see TR4QT window after launch

**Solution**:
1. **Tools → Reset Window Positions** (if you can access menu)
2. Or delete preferences:
   ```bash
   rm ~/.tr4qt/tr4qt_config.ini
   ```
3. Relaunch TR4QT - windows reset to defaults

### Database initialization failed

**Error**: "Failed to initialize database"

**Cause**: Permission issues with `~/.tr4qt` directory

**Solution** (macOS/Linux):
```bash
mkdir -p ~/.tr4qt/logs
chmod 755 ~/.tr4qt
chmod 755 ~/.tr4qt/logs
```

**Solution** (Windows):
Check that `%USERPROFILE%\.tr4qt` directory exists and is writable.

## Radio Control Issues

See [Radio Configuration Guide](USER_GUIDE_RADIO_SETUP.md#troubleshooting) for detailed radio troubleshooting.

### Quick radio troubleshooting checklist:

- [ ] Radio is powered on
- [ ] CAT cable firmly connected
- [ ] Correct COM port selected
- [ ] Baud rate matches radio
- [ ] CAT enabled in radio menu
- [ ] No other program using COM port
- [ ] Driver installed (USB radios)

## Logging Issues

### Exchange not auto-populating

**Cause**: Missing CTY.DAT file or exchange memory

**Solutions**:
1. **Download CTY.DAT**: Tools → Download CTY.DAT
2. **Check station info**: Preferences → Station (verify callsign, zone)
3. **Manual entry**: Tab to exchange field and type manually

### Duplicate warning for non-dupe

**Cause**: Previously logged same station on this band/mode

**Solutions**:
1. **Check band/mode**: Verify you're on different band or mode
2. **Review log**: Search for callsign (Alt+F) to confirm
3. **Override**: Press Enter anyway to log (investigates later)
4. **Delete old QSO**: If incorrect, right-click and delete

### Score not calculating correctly

**Symptoms**:
- Zero points for valid QSOs
- Multipliers not detected
- Unexpected score value

**Solutions**:
1. **Rescore contest**: Operating → Rescore Contest
2. **Check contest type**: Ensure correct contest selected
3. **Verify exchange**: Incorrect exchange = wrong scoring
4. **Check for duplicates**: Dupes always score 0 points

### QSO table not updating

**Symptom**: Logged QSO doesn't appear in table

**Solutions**:
1. **Scroll to bottom**: Newest QSOs at bottom of table
2. **Resize window**: Sometimes table needs refresh
3. **Check active contest**: Verify contest is open
4. **Database issue**: Check `~/.tr4qt/tr4qt.log` for errors

## Performance Issues

### Slow startup

**Cause**: Large logs or many contests in database

**Solutions**:
1. **Archive old contests**: Export ADIF and delete from database
2. **Clean backups**: Delete old backups from `~/.tr4qt/backups/`
3. **Database vacuum**: Close TR4QT, run:
   ```bash
   sqlite3 ~/.tr4qt/logs/tr4qt_global.db "VACUUM;"
   ```

### UI freezing/sluggish

**Symptoms**: Slow response to clicks, typing lag

**Solutions**:
1. **Close extra windows**: Band map, DX cluster, statistics
2. **Disable web server**: Tools → Stop Web Server
3. **Check system resources**: Close other applications
4. **Large log**: 10,000+ QSOs may slow down some operations

### High CPU usage

**Cause**: Hamlib polling, web server, or DX cluster

**Solutions**:
1. **Disconnect radio**: Radio → Disconnect (stops polling)
2. **Stop web server**: Tools → Stop Web Server
3. **Disconnect DX cluster**: Close DX cluster window
4. **Check for runaway process**: Activity Monitor (Mac) or Task Manager (Windows)

## Import/Export Issues

### ADIF import fails completely

**Error**: "Failed to parse ADIF file"

**Solutions**:
1. **Open ADIF in text editor**: Check for corruption
2. **Validate ADIF**: Use ADIFMaster or ADIF Editor tool
3. **Re-export**: From source software with ADIF 3.1 format
4. **Check file encoding**: Should be UTF-8 or ASCII

### Some QSOs not importing

**Symptom**: "Imported 100 QSOs, Failed 50"

**Cause**: Missing required fields (callsign, date, time)

**Solutions**:
1. **Review error messages**: Import results panel shows why
2. **Check ADIF file**: Ensure all QSOs have CALL, QSO_DATE, TIME_ON
3. **Enable auto-correct**: Check "Auto-correct common errors"

### Cabrillo export fails

**Error**: "Failed to export Cabrillo"

**Solutions**:
1. **Check station info**: Preferences → Station (callsign required)
2. **Verify contest**: Must have valid contest open
3. **Check exchange fields**: All QSOs must have complete exchanges
4. **Review log**: Look for QSOs with missing data

## Web Server Issues

### "Cannot start web server: Port already in use"

**Cause**: Port 8080 in use by another application

**Solutions**:
1. **Find conflicting app**:
   ```bash
   # macOS/Linux
   lsof -i :8080

   # Windows
   netstat -ano | findstr :8080
   ```
2. **Stop conflicting app** or change TR4QT port (in Preferences)
3. **Restart TR4QT**

### Can't access web interface from phone/tablet

**Symptoms**: Browser shows "Connection refused" or timeout

**Solutions**:
1. **Verify same network**: Phone and computer on same WiFi
2. **Check IP address**: Confirm correct IP (not localhost)
3. **Firewall**: Temporarily disable to test
4. **Router**: Some routers block device-to-device communication

### Web interface shows old data

**Symptom**: Browser doesn't update with new QSOs

**Solutions**:
1. **Refresh browser**: F5 or Cmd+R
2. **Check auto-refresh**: Should update every 2 seconds
3. **Clear browser cache**: Hard refresh (Ctrl+Shift+R)
4. **JavaScript enabled**: Required for auto-updates

## Database Issues

### "Database is locked"

**Error**: "Database is locked - unable to write QSO"

**Cause**: Multiple instances of TR4QT or filesystem issue

**Solutions**:
1. **Close extra instances**: Only one TR4QT per contest database
2. **Check for crash**: Look for stale lock files in `~/.tr4qt/logs/`
3. **Restart**: Close all TR4QT instances and relaunch
4. **Network drive**: Don't run TR4QT from network storage

### Contest log disappeared

**Symptom**: Contest list empty or missing expected contest

**Solutions**:
1. **Check correct user**: Running as different user?
2. **Restore from backup**:
   - File → Open Contest
   - Navigate to `~/.tr4qt/backups/`
   - Open most recent backup
3. **Database corruption**: Check for `.db-shm` or `.db-wal` files
4. **Last resort**: Recover from ADIF exports

### Backup restore not working

**Symptom**: Trying to open backup file fails

**Solutions**:
1. **Use correct method**: File → Open Contest (not Import ADIF)
2. **Check file extension**: Should be `.db` SQLite database
3. **Verify file integrity**:
   ```bash
   sqlite3 backup_file.db "PRAGMA integrity_check;"
   ```
4. **Extract from ADIF**: If backup corrupt, restore via ADIF export if available

## Country File (CTY.DAT) Issues

### CTY.DAT download fails

**Error**: "Failed to download CTY.DAT"

**Causes**:
1. **No internet connection**
2. **Firewall blocking HTTPS**
3. **TLS certificate issue**
4. **Source website down**

**Solutions**:
1. **Check internet**: Verify browser can reach https://www.country-files.com
2. **Retry**: Tools → Download CTY.DAT again
3. **Manual download**:
   - Download from https://www.country-files.com/cty/cty.dat
   - Save to `~/.tr4qt/cty.dat`
4. **Check firewall/antivirus**: May block download

### Callsign lookup not working

**Symptom**: Country/zone not auto-filling

**Solutions**:
1. **Verify CTY.DAT exists**:
   ```bash
   ls ~/.tr4qt/cty.dat
   ```
2. **Re-download**: Tools → Download CTY.DAT
3. **Check file size**: Should be ~300-500 KB
4. **Restart TR4QT**: Reload country file

## LoTW User List Issues

### LoTW download very slow

**Cause**: File is large (~50-100 MB)

**Solution**: Be patient - download may take 1-2 minutes on slow connections

### LoTW users not highlighted

**Symptom**: Downloaded LoTW list but no highlighting

**Solutions**:
1. **Restart TR4QT**: Reload LoTW data
2. **Check file**: Verify `~/.tr4qt/lotw-user-activity.csv` exists
3. **Re-download**: Tools → Download LOTW Users
4. **Feature not yet implemented**: Check TR4QT version (may be future feature)

## Crash/Hang Issues

### TR4QT crashes on startup

**Solutions**:
1. **Check logs**:
   ```bash
   tail -100 ~/.tr4qt/tr4qt.log
   ```
2. **Reset preferences**:
   ```bash
   mv ~/.tr4qt/tr4qt_config.ini ~/.tr4qt/tr4qt_config.ini.backup
   ```
3. **Clean database locks**:
   ```bash
   rm ~/.tr4qt/logs/*.db-shm
   rm ~/.tr4qt/logs/*.db-wal
   ```
4. **Reinstall**: Delete app and download fresh copy

### TR4QT hangs during operation

**Symptoms**: Spinner cursor, no response

**Solutions**:
1. **Wait**: Large operations (rescore, import) may take time
2. **Force quit**: Activity Monitor (Mac), Task Manager (Windows)
3. **Check logs**: `~/.tr4qt/tr4qt.log` for errors
4. **Report bug**: With log file and steps to reproduce

### Frequent crashes with radio connected

**Cause**: Hamlib communication issue

**Solutions**:
1. **Update Hamlib**: Use latest version
2. **Disable debug**: Preferences → Advanced → Hamlib Debug (uncheck)
3. **Test rigctl**:
   ```bash
   rigctl -m MODEL -r DEVICE -s BAUD f
   ```
4. **Report to Hamlib**: If rigctl also crashes

## Logging and Debug Information

### Enable debug logging

1. **Preferences → Advanced**
2. **Check "Enable Hamlib Debug Output"** (if radio issues)
3. **Restart TR4QT**
4. **Reproduce issue**
5. **Collect logs**: `~/.tr4qt/tr4qt.log`

### Log file locations

- **macOS/Linux**: `~/.tr4qt/tr4qt.log`
- **Windows**: `%USERPROFILE%\.tr4qt\tr4qt.log`
- **Backups**: `~/.tr4qt/backups/`
- **Contest databases**: `~/.tr4qt/logs/*.db`

### What to include in bug reports

1. **TR4QT version**: Help → About
2. **Operating system**: macOS 14.5, Windows 11, Ubuntu 22.04, etc.
3. **Steps to reproduce**: Exact sequence that causes problem
4. **Expected behavior**: What should happen
5. **Actual behavior**: What actually happens
6. **Log file**: Last 100 lines of `~/.tr4qt/tr4qt.log`
7. **Screenshots**: If UI issue

## Getting Help

### Official resources

- **GitHub Issues**: https://github.com/ny4i/TR4QT/issues
- **GitHub Discussions**: https://github.com/ny4i/TR4QT/discussions
- **Documentation**: https://github.com/ny4i/TR4QT/tree/master/docs

### Before asking for help

- [ ] Checked this troubleshooting guide
- [ ] Searched existing GitHub issues
- [ ] Tried restarting TR4QT
- [ ] Checked `~/.tr4qt/tr4qt.log` for errors
- [ ] Verified issue reproducible

### How to ask effective questions

**Good question**:
> **Title**: "CQ WW scoring gives 0 points for all Europe QSOs"
>
> **Description**: TR4QT 3.8.1 on macOS 14.5. Created CQ WW CW contest, logged 50 QSOs to Europe (W1AW calling from North America). All show 0 points in log. Expected 3 points per QSO. Attached log file and screenshot.
>
> Steps:
> 1. Create CQ WW CW contest
> 2. Set station: W1AW, zone 5, continent NA
> 3. Log QSO to DL1ABC
> 4. Points column shows 0 instead of 3

**Bad question**:
> "Scoring doesn't work"
>
> (No details, can't reproduce, unclear)

## Still stuck?

If this guide doesn't solve your issue:

1. **Search GitHub issues**: Someone may have reported it
2. **Create new issue**: Provide all details from "What to include" above
3. **Join discussion**: Ask in GitHub Discussions
4. **Be patient**: TR4QT is developed by volunteers

Most issues have simple solutions - often just a setting or restart needed!

## Next Steps

- [Return to main User Guide](USER_GUIDE.md)
- [Review getting started guide](USER_GUIDE_GETTING_STARTED.md)
- [Check keyboard shortcuts](USER_GUIDE_SHORTCUTS.md)
