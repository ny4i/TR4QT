# TR4QT File Storage Locations

This document explains where TR4QT stores your contest logs, configuration files, backups, and other application data on different operating systems.

## Overview

As of version 3.18.0, TR4QT follows platform-native conventions for storing application data. This means TR4QT uses the standard locations that your operating system expects, making it easier to find your data and ensuring compatibility with system backup tools.

## Storage Locations by Platform

### Windows

TR4QT stores all data in your **AppData\Local** folder:

```
C:\Users\<YourUsername>\AppData\Local\TR4QT\
```

**What's stored where:**

| Data Type | Location | Description |
|-----------|----------|-------------|
| Contest Logs | `C:\Users\<YourUsername>\AppData\Local\TR4QT\logs\` | SQLite database files (`.db`) for each contest |
| Backups | `C:\Users\<YourUsername>\AppData\Local\TR4QT\backups\` | Automatic and manual contest log backups |
| Country File | `C:\Users\<YourUsername>\AppData\Local\TR4QT\cty.dat` | DXCC country/prefix database |
| LOTW Users | `C:\Users\<YourUsername>\AppData\Local\TR4QT\lotw-user-activity.csv` | List of active LoTW users |
| Global Database | `C:\Users\<YourUsername>\AppData\Local\TR4QT\tr4qt_global.db` | Shared application data |
| Application Log | `C:\Users\<YourUsername>\AppData\Local\TR4QT\logs\tr4qt.log` | Debug/diagnostic logs |
| Settings | `%APPDATA%\TR4QT\TR4QT.ini` | User preferences (in Roaming profile) |

**How to access:**
1. Press `Win + R` to open Run dialog
2. Type `%LOCALAPPDATA%\TR4QT` and press Enter
3. Windows Explorer will open the TR4QT data directory

### macOS

TR4QT stores all data in your **Application Support** folder:

```
~/Library/Application Support/TR4QT/
```

**What's stored where:**

| Data Type | Location | Description |
|-----------|----------|-------------|
| Contest Logs | `~/Library/Application Support/TR4QT/logs/` | SQLite database files (`.db`) for each contest |
| Backups | `~/Library/Application Support/TR4QT/backups/` | Automatic and manual contest log backups |
| Country File | `~/Library/Application Support/TR4QT/cty.dat` | DXCC country/prefix database |
| LOTW Users | `~/Library/Application Support/TR4QT/lotw-user-activity.csv` | List of active LoTW users |
| Global Database | `~/Library/Application Support/TR4QT/tr4qt_global.db` | Shared application data |
| Application Log | `~/Library/Application Support/TR4QT/logs/tr4qt.log` | Debug/diagnostic logs |
| Settings | `~/Library/Preferences/com.tr4qt.TR4QT.plist` | User preferences |

**How to access:**
1. In Finder, press `Cmd + Shift + G` (Go to Folder)
2. Type `~/Library/Application Support/TR4QT` and press Enter
3. Finder will open the TR4QT data directory

**Note:** The `~/Library` folder is hidden by default in macOS. Use the method above or make hidden files visible in Finder (press `Cmd + Shift + .`).

### Linux

TR4QT stores all data in your **XDG data directory**:

```
~/.local/share/TR4QT/
```

**What's stored where:**

| Data Type | Location | Description |
|-----------|----------|-------------|
| Contest Logs | `~/.local/share/TR4QT/logs/` | SQLite database files (`.db`) for each contest |
| Backups | `~/.local/share/TR4QT/backups/` | Automatic and manual contest log backups |
| Country File | `~/.local/share/TR4QT/cty.dat` | DXCC country/prefix database |
| LOTW Users | `~/.local/share/TR4QT/lotw-user-activity.csv` | List of active LoTW users |
| Global Database | `~/.local/share/TR4QT/tr4qt_global.db` | Shared application data |
| Application Log | `~/.local/share/TR4QT/logs/tr4qt.log` | Debug/diagnostic logs |
| Settings | `~/.config/TR4QT/TR4QT.conf` | User preferences |

**How to access:**
1. Open a terminal
2. Type `cd ~/.local/share/TR4QT` and press Enter
3. Type `ls -la` to see all files

Or use your file manager:
1. Open your file manager (Nautilus, Dolphin, etc.)
2. Press `Ctrl + H` to show hidden files
3. Navigate to `.local/share/TR4QT` in your home directory

## Migration from Legacy Storage (v3.17.0 and earlier)

### What Changed in v3.18.0

Prior to version 3.18.0, TR4QT used a Unix-style `~/.tr4qt/` directory on all platforms:

```
~/.tr4qt/              # Old location (all platforms)
~/.tr4qt/logs/         # Contest databases
~/.tr4qt/backups/      # Backups
~/.tr4qt/cty.dat       # Country file
```

This worked, but wasn't the native Windows way and created visible clutter in your home directory.

### Automatic Migration (Windows Only)

**On Windows**, TR4QT automatically migrates your data on first run after upgrading to v3.18.0:

1. TR4QT checks if `C:\Users\<YourUsername>\.tr4qt\` exists
2. If found, it copies all contest logs, backups, and files to the new location
3. **Your original `~/.tr4qt` directory is preserved** (not deleted)
4. After verifying the migration worked, you can safely delete `~/.tr4qt` manually

**On macOS and Linux**, automatic migration is **not performed** because the old and new paths are similar enough that manual migration isn't necessary. However, you can manually move your data if desired.

### Manual Migration (Optional)

If you want to manually move your data to the new location:

**Windows:**
```cmd
# Copy contest logs
xcopy /E /I "%USERPROFILE%\.tr4qt\logs" "%LOCALAPPDATA%\TR4QT\logs"

# Copy backups
xcopy /E /I "%USERPROFILE%\.tr4qt\backups" "%LOCALAPPDATA%\TR4QT\backups"

# Copy country file
copy "%USERPROFILE%\.tr4qt\cty.dat" "%LOCALAPPDATA%\TR4QT\cty.dat"

# Copy LOTW file (if exists)
copy "%USERPROFILE%\.tr4qt\lotw-user-activity.csv" "%LOCALAPPDATA%\TR4QT\lotw-user-activity.csv"

# Copy global database
copy "%USERPROFILE%\.tr4qt\tr4qt_global.db" "%LOCALAPPDATA%\TR4QT\tr4qt_global.db"
```

**macOS/Linux:**
```bash
# The new location is similar to the old one, but you can move if desired:
mkdir -p ~/Library/Application\ Support/TR4QT
cp -R ~/.tr4qt/* ~/Library/Application\ Support/TR4QT/
```

## Backup Recommendations

### What to Backup

**Essential (contains all your contest data):**
- Contest logs directory (`logs/`)
- Backups directory (`backups/`)
- Global database (`tr4qt_global.db`)

**Optional (can be re-downloaded):**
- Country file (`cty.dat`)
- LOTW user file (`lotw-user-activity.csv`)

**Not needed (will be recreated):**
- Application log (`tr4qt.log`)
- Settings file (only if you want to preserve custom preferences)

### Backup Methods

**Windows:**
- Use File History to automatically backup `%LOCALAPPDATA%\TR4QT`
- Or manually copy the entire `TR4QT` folder to external drive/cloud storage

**macOS:**
- Time Machine automatically includes `~/Library/Application Support`
- Or manually copy the `TR4QT` folder to external drive/cloud storage

**Linux:**
- Use your preferred backup tool (rsync, duplicity, etc.)
- Or manually copy `~/.local/share/TR4QT` to backup location

## Finding Your Contest Logs

All contest logs are SQLite databases stored in the `logs/` subdirectory.

**File naming format:**
```
<ContestType>_<StartDate>.db
```

**Examples:**
- `CQWW_20231125.db` - CQ WW DX Contest starting Nov 25, 2023
- `CQWPX_20240330.db` - CQ WPX Contest starting Mar 30, 2024
- `ARRL_DX_20240217.db` - ARRL DX Contest starting Feb 17, 2024

These are standard SQLite databases and can be opened with any SQLite tool (DB Browser for SQLite, sqlite3 command-line, etc.) for advanced analysis or export.

## Changing Storage Locations

TR4QT allows you to customize where backups are stored:

1. Open **Preferences** (Menu → Settings → Preferences)
2. Go to **Backup/Restore** tab
3. Click **Browse** to select a custom backup directory
4. Click **Save**

**Note:** Contest logs and other core data cannot be relocated - they must stay in the platform-native location for TR4QT to function correctly.

## Troubleshooting

### Can't Find TR4QT Data Directory

**Windows:**
- Make sure hidden files/folders are visible in Windows Explorer
- Use the Run dialog method: `Win + R` → `%LOCALAPPDATA%\TR4QT`

**macOS:**
- Make sure hidden files are visible: Press `Cmd + Shift + .` in Finder
- Or use the "Go to Folder" method: `Cmd + Shift + G` → `~/Library/Application Support/TR4QT`

**Linux:**
- Make sure hidden files are visible: Press `Ctrl + H` in file manager
- Or use terminal: `cd ~/.local/share/TR4QT`

### Lost Contest Logs After Upgrade

If you upgraded to v3.18.0 and can't find your contest logs:

**On Windows:**
- Check if `C:\Users\<YourUsername>\.tr4qt\logs` still exists
- If it does, the automatic migration didn't run
- Follow the manual migration steps above

**On macOS/Linux:**
- Check the old location: `~/.tr4qt/logs`
- If found, manually copy to new location (see manual migration above)

### Application Log is Too Large

TR4QT rotates log files automatically, but you can safely delete `tr4qt.log` at any time - it will be recreated on next run.

Maximum log file size can be configured in **Preferences** → **Advanced Settings** → **Log File Settings**.

## Support

If you have questions about file storage locations or need help with migration:

- **GitHub Issues:** https://github.com/n4kin/TR4QT/issues
- **Documentation:** https://github.com/n4kin/TR4QT/wiki

---

**Document Version:** 1.0
**Last Updated:** January 1, 2026
**Applies to TR4QT:** v3.18.0 and later
