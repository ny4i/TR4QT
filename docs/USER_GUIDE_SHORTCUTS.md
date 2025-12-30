# Keyboard Shortcuts Reference

TR4QT is designed for keyboard-focused operation during contests. This reference covers all keyboard shortcuts.

**Platform notes**:
- **Alt** on Windows/Linux = **Option (⌥)** on macOS
- **Ctrl** on Windows/Linux = **Command (⌘)** on macOS
- Shortcuts display correctly for your platform in menus

## General / File Menu

| Shortcut | Action | Description |
|----------|--------|-------------|
| Ctrl+N / Cmd+N | New/Open Contest | Open contest chooser dialog |
| Ctrl+W / Cmd+W | Close Contest | Close current contest |
| Ctrl+I / Cmd+I | Import ADIF | Import QSOs from ADIF file |
| Ctrl+E / Cmd+E | Export ADIF | Export log to ADIF format |
| Ctrl+Shift+E | Export Cabrillo | Export contest log for submission |
| Ctrl+P / Cmd+P | Preferences | Open preferences dialog |
| Ctrl+Q / Cmd+Q | Quit | Exit TR4QT |

## Edit Menu

| Shortcut | Action | Description |
|----------|--------|-------------|
| Ctrl+L / Cmd+L | View/Edit Log | Open log editing window (planned) |
| Ctrl+D / Cmd+D | Clear Dupes | Clear duplicate flags (planned) |
| Ctrl+O / Cmd+O | Add Note | Add note to log (planned) |
| Ctrl+Y / Cmd+Y | Recall Last | Recall last entry (planned) |

## Radio Menu

| Shortcut | Action | Description |
|----------|--------|-------------|
| Ctrl+R / Cmd+R | Configure Radio | Open radio setup dialog |
| Ctrl+Shift+C | Connect Radio | Connect to radio |
| Ctrl+Shift+D | Disconnect Radio | Disconnect from radio |

## Operating Menu

| Shortcut | Action | Description |
|----------|--------|-------------|
| Alt+C / Option+C | Auto CQ | Start automatic CQ mode (planned) |
| Alt+R / Option+R | Auto CQ Resume | Resume auto CQ (planned) |
| Alt+K / Option+K | Kill CW | Abort CW transmission (planned) |
| Alt+D / Option+D | Dupe Check | Check for duplicates |
| Alt+F / Option+F | Search Log | Search contest log |
| Alt+Backspace | Delete Last QSO | Remove most recent contact |
| Alt+N / Option+N | Increment Number | Increment serial number |
| Alt+I / Option+I | Initial Exchange | Set initial exchange (planned) |
| Alt+W / Option+W | CW Speed | Adjust CW speed (planned) |
| Alt+T / Option+T | Toggle Sidetone | Enable/disable sidetone (planned) |
| Alt+A / Option+A | Toggle Autosend | Enable/disable autosend (planned) |

## Band Menu

| Shortcut | Action | Description |
|----------|--------|-------------|
| Alt+B / Option+B | Band Up | Switch to next higher band |
| Alt+V / Option+V | Band Down | Switch to next lower band |
| Alt+G / Option+G | Toggle Rigs | Switch between radios (SO2R, planned) |
| Alt+E / Option+E | Edit SO2R | Configure SO2R settings (planned) |

## Window Menu

| Shortcut | Action | Description |
|----------|--------|-------------|
| Ctrl+B / Cmd+B | Band Map | Show/hide band map window |
| Ctrl+X / Cmd+X | DX Cluster | Show/hide DX cluster window |
| Ctrl+Shift+R | Radio Control | Show/hide radio control panel |
| Ctrl+M / Cmd+M | Multipliers | Show/hide multiplier window |
| Ctrl+S / Cmd+S | Statistics | Show/hide statistics window |
| Ctrl+Shift+M | Sections Map | Show ARRL sections map |
| Ctrl+Shift+S | States Map | Show US states map (WAS) |
| Ctrl+Shift+W | Swap Mult View | Swap multiplier display (planned) |
| Ctrl+Shift+N | Missing Mults | Report missing multipliers (planned) |

## Tools Menu

| Shortcut | Action | Description |
|----------|--------|-------------|
| Alt+W / Option+W | Toggle Web Server | Start/stop web server |
| Alt+M / Option+M | Send Morse | Open morse code send dialog |
| Alt+Z / Option+Z | Backup Log | Create manual backup (planned) |
| Alt+Shift+D | Download CTY | Download CTY.DAT country file |
| Alt+Shift+L | Download LOTW | Download LOTW user list |
| Alt+Shift+T | Set Date/Time | Set system date/time (planned) |
| Alt+Shift+I | Initialize | Initialize system (planned) |
| Alt+Shift+R | Reset Windows | Reset all window positions to defaults |

## In-Application Shortcuts

### Logging Window

| Key | Action | Description |
|-----|--------|-------------|
| Enter | Log QSO | Add QSO to log (from any field) |
| Tab | Next Field | Move to next exchange field |
| Shift+Tab | Previous Field | Move to previous field |
| Escape | Clear Entry | Clear callsign and exchange fields |
| F1-F12 | Function Keys | CW/Voice message macros (planned) |

### QSO Table

| Key | Action | Description |
|-----|--------|-------------|
| Double-click | Edit QSO | Open edit dialog for selected QSO |
| Delete | Delete QSO | Remove selected contact from log |
| Ctrl+C / Cmd+C | Copy | Copy selected QSO data |
| Ctrl+V / Cmd+V | Paste | Paste QSO data (planned) |
| Up/Down | Navigate | Move through log entries |
| Home | First QSO | Jump to first contact in log |
| End | Last QSO | Jump to most recent contact |

### Band Summary Grid

| Action | Description |
|--------|-------------|
| Click | Change Band | QSY to clicked band (if radio connected) |
| | Or manual band selection (without radio) |

## Function Keys (Planned)

TR4QT will support programmable function keys for CW and voice messages:

| Key | Typical Use |
|-----|------------|
| F1 | CQ message |
| F2 | Exchange |
| F3 | TU message |
| F4 | My call |
| F5 | His call |
| F6 | QSL message |
| F7 | Question mark (repeat) |
| F8 | AGN (again) |
| F9-F12 | Custom messages |

**Note**: Function key support requires CW/voice keying implementation.

## Special Key Combinations

### Contest Operation

- **Enter in Callsign**: Logs QSO with current exchange
- **Enter in Exchange**: Logs QSO
- **Tab through fields**: Auto-advance to next field
- **Escape**: Clear entry and return to callsign field

### Band Switching

When radio connected:
- **Click band**: Immediate QSY
- **Alt+B/Alt+V**: Step through bands sequentially

Without radio:
- **Click band**: Manual selection (frequency set to band edge)

### Duplicate Warning

When "DUPE" warning appears:
- **Press Enter anyway**: Logs duplicate (0 points)
- **Escape**: Clears entry
- **Edit exchange**: Override if not actually a dupe

## Customizing Shortcuts (Future)

TR4QT currently uses fixed shortcuts matching TR4W conventions. Future versions may support:
- User-defined keyboard shortcuts
- Programmable function keys
- Voice keyer DVK messages
- Macro recording and playback

## Shortcuts vs Mouse Operation

**Keyboard-first design**:
- ✅ **Faster**: No hand movement from keyboard to mouse
- ✅ **Contest-ready**: Designed for high-rate operation
- ✅ **Accessibility**: Full operation without mouse
- ✅ **Flexible**: Mouse available for setup and configuration

**When to use mouse**:
- Initial setup and configuration
- Reviewing log entries
- Accessing preferences
- Window management

**When to use keyboard**:
- Active contest operation
- High-rate running
- Search and pounce
- Band changes during run

## Quick Reference Card

### Essential Contest Shortcuts

**Logging**:
- Enter → Log QSO
- Tab → Next field
- Esc → Clear

**Band Changes**:
- Alt+B → Band up
- Alt+V → Band down

**Windows**:
- Ctrl+B → Band map
- Ctrl+M → Multipliers
- Ctrl+X → DX cluster

**Radio**:
- Ctrl+Shift+C → Connect
- Ctrl+Shift+D → Disconnect

**Tools**:
- Alt+W → Web server
- Ctrl+I → Import ADIF
- Ctrl+E → Export ADIF

**Print this section** and keep near your operating position!

## Keyboard-Only Operation Challenge

Can you operate an entire contest without touching the mouse?

**Required skills**:
1. Tab between fields
2. Alt+B / Alt+V for band changes
3. Enter to log QSOs
4. Ctrl+M to check multipliers
5. Alt+F to search log

**Pro tip**: Try keyboard-only operation in a small contest (NAQP, Sprint) to build muscle memory before major contests.

## Next Steps

- [Understand contest operation](USER_GUIDE_CONTEST_OPERATION.md)
- [Troubleshoot common issues](USER_GUIDE_TROUBLESHOOTING.md)
