# Getting Started with TR4QT

## Installation

### macOS

1. Download `TR4QT-macOS.dmg` from the latest release
2. Open the DMG file
3. Drag `tr4qt.app` to your Applications folder
4. First launch: Right-click the app and select "Open" (bypasses Gatekeeper)
5. Click "Open" when prompted about unverified developer

**Note**: TR4QT is not code-signed, so you must use "Open" from right-click menu on first launch.

### Windows

1. Download `TR4QT-Setup.exe` from the latest release
2. Run the installer
3. Follow the installation wizard
4. Launch from Start Menu or desktop shortcut

**Note**: Windows Defender may warn about unrecognized app - click "More info" then "Run anyway"

### Linux

1. Download `TR4QT-Linux-x86_64.tar.gz` from the latest release
2. Extract: `tar -xzf TR4QT-Linux-x86_64.tar.gz`
3. Run: `./tr4qt`

**Dependencies**: Ensure Qt 6.2+ libraries are installed on your system.

## First Launch

When you launch TR4QT for the first time:

1. **Contest Chooser** dialog appears
2. Select a contest type or click "Cancel" to explore the interface
3. The main window opens with an empty log

## Initial Configuration

### Set Your Station Information

1. Open **File → Preferences** (macOS: TR4QT → Preferences)
2. Configure the following:

**Station Tab:**
- **Callsign**: Your amateur radio callsign (required)
- **Name**: Your name (optional)
- **Grid Square**: Your Maidenhead grid locator
- **CQ Zone**: Your CQ Zone (1-40)
- **ITU Zone**: Your ITU Zone (1-90)
- **ARRL Section**: Your ARRL section (e.g., NC, ORG)
- **Continent**: Your continent (NA, SA, EU, AF, AS, OC)

**Important**: Many contests use your station info for exchange auto-population and scoring.

### Download Country File (Recommended)

TR4QT uses `cty.dat` for callsign lookups (country, zone, continent):

1. Go to **Tools → Download CTY.DAT**
2. Wait for download to complete
3. Status bar shows: "CTY.DAT downloaded successfully"

The country file enables:
- Automatic country/zone lookup
- Exchange auto-population
- Multiplier detection

### Download LoTW User List (Optional)

Highlight LoTW users in your log:

1. Go to **Tools → Download LOTW Users**
2. Wait for download (large file, may take a minute)
3. LoTW users will be highlighted in the log

## Understanding the Main Window

### Top Section: Band Summary Grid
Shows QSO counts and multipliers by band:
- **Click a band button** to change bands (if radio connected)
- **Color coding**: Green = active band, Gray = no QSOs yet

### Upper Right: Needs Display
Shows worked bands for current callsign:
- **Green**: Worked on this band
- **Gray**: Not worked yet
- Helps with "work all bands" contests

### Center: Log Entry
- **Callsign field**: Enter the station you're working
- **Exchange fields**: Contest-specific (RST, Zone, Section, etc.)
- **Log button**: Click or press Enter to log the QSO

### QSO Table
Displays all logged contacts with:
- **Time** (UTC)
- **Frequency**
- **Mode**
- **Callsign**
- **Exchange received**
- **Exchange sent**
- **Points** and **Multiplier** status

### Bottom Right: Statistics Panel
- **Score**: Current contest score
- **QSOs**: Total contacts logged
- **Mults**: Multiplier count
- **Rate**: QSOs per hour
- **This Hr**: Contacts in the current hour

### Bottom: Radio Status Grid
When radio connected:
- **Frequency/Band/Mode**: Current radio state
- **Date/Time**: UTC time
- **Status indicator**: Green = connected, Red flashing = disconnected

## Next Steps

- [Configure radio control](USER_GUIDE_RADIO_SETUP.md)
- [Create your first contest](USER_GUIDE_CONTEST_OPERATION.md)
- [Learn keyboard shortcuts](USER_GUIDE_SHORTCUTS.md)
