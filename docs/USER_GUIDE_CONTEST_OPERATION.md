# Contest Operation Guide

Learn how to create contests, log QSOs, track multipliers, and maximize your score with TR4QT.

## Creating a New Contest

### Method 1: Contest Chooser (Recommended)

1. **File → New/Open Contest** (or Ctrl+N / Cmd+N)
2. **Contest Chooser** dialog appears
3. **Select contest type** from the list:
   - CQ WW DX Contest (CW/SSB)
   - CQ WPX Contest (CW/SSB)
   - ARRL Field Day
   - ARRL Winter Field Day
   - ARRL Sweepstakes
   - NAQP (North American QSO Party)
   - More contests added regularly

4. **Enter contest details**:
   - **Name**: Contest instance name (e.g., "CQ WW CW 2025")
   - **Start Date/Time**: When contest begins (UTC)
   - **Mode**: CW, SSB, or Mixed (contest-specific)

5. **Click Create**

6. Contest opens with empty log ready for operation

### Method 2: Quick Contest Setup

For casual logging without formal contest:

1. **File → New/Open Contest**
2. **Select** "General Logging" or skip contest chooser
3. **Start logging** - TR4QT adapts exchange fields based on mode

## Understanding Contest Types

### CQ WW DX Contest

**Exchange Sent**: RST + CQ Zone (e.g., "599 5")
**Exchange Received**: RST + CQ Zone
**Scoring**: Points based on distance, multipliers = countries + zones
**Duplicate Rule**: Per band/mode

### CQ WPX Contest

**Exchange Sent**: RST + Serial Number (e.g., "599 001")
**Exchange Received**: RST + Serial Number
**Scoring**: Points for QSOs, multipliers = unique prefixes
**Duplicate Rule**: Per band/mode

### ARRL Field Day

**Exchange Sent**: Class + Section (e.g., "2A ORG")
**Exchange Received**: Class + Section
**Scoring**: Points per QSO, bonuses for modes/power
**Duplicate Rule**: Per band/mode

### ARRL Winter Field Day

**Exchange Sent**: Class + Section (e.g., "3O NC")
**Exchange Received**: Class + Section
**Scoring**: Points per QSO, mode multipliers
**Duplicate Rule**: Per band/mode

## Logging QSOs

### Basic Logging Workflow

1. **Enter callsign** in callsign field
2. **Check for duplicate**: Warning appears if worked before
3. **Review exchange**: Auto-populated from predictions
4. **Adjust exchange** if needed (Tab to move between fields)
5. **Press Enter** or click **Log** button
6. **QSO added to log** - callsign field clears for next contact

### Exchange Auto-Population

TR4QT predicts exchange values using:

1. **Exchange Memory**: Previously worked stations
2. **CTY.DAT Lookup**: Country/zone from callsign
3. **Contest Defaults**: Standard values (RST, etc.)

**Example (CQ WW DX)**:
- Enter: `UA9BA`
- Exchange auto-fills: `599 18` (CQ Zone 18 from CTY.DAT)
- Press Enter to log

### Duplicate Checking

TR4QT checks for duplicates in real-time:

**Visual Indicators**:
- **Warning label** appears: "DUPE: Worked on 20m CW"
- **Log anyway** if intentional (zero points)

**Duplicate Rules** (contest-specific):
- **Per Band/Mode**: Same call on same band/mode = dupe
- **Per Band**: Same call on same band (any mode) = dupe
- **All Band**: Same call once in contest = dupe

**Override**: You can log duplicates (they show 0 points)

### Multiplier Detection

New multipliers highlighted automatically:

**Visual Indicators**:
- **Mult column** shows ✓ for multiplier QSOs
- **Multiplier window** (Window → Multipliers) shows worked/needed
- **Needs display** (upper right) shows worked bands for callsign

**Multiplier Types** (contest-specific):
- **CQ WW**: Countries + CQ Zones
- **CQ WPX**: Unique prefixes (W1, W2, UA9, etc.)
- **Field Day**: ARRL Sections
- **Sweepstakes**: Sections, precedence

## Band Switching

### With Radio Connected

1. **Click band button** in Band Summary Grid
2. Radio changes to that band (preset frequency)
3. Logging continues on new band

**Keyboard Shortcuts**:
- **Alt+B** (Option+B on Mac): Band up
- **Alt+V** (Option+V on Mac): Band down

### Without Radio (Manual)

1. **Click band button** to select band
2. Frequency set to band edge (visual indicator)
3. Log QSOs - band recorded from manual selection

**Tip**: Manual band selection useful for paper logging or delayed entry

## Score Tracking

### Score Display (Bottom Right)

- **Score**: Total contest score (auto-calculated)
- **QSOs**: Total contacts
- **Mults**: Multipliers worked
- **Rate**: QSOs per hour (last 60 minutes)
- **This Hr**: Contacts in current UTC hour

### Real-Time Scoring

TR4QT calculates score automatically:
- **QSO points**: Based on contest rules and distance
- **Multipliers**: Detected from exchange and callsign
- **Duplicate handling**: Zero points for dupes
- **Instant feedback**: Score updates after each QSO

### Rescore Contest

If score seems incorrect:

1. **Operating → Rescore Contest** (or Ctrl+R / Cmd+R)
2. **Confirm**: Dialog explains what rescore does
3. **Wait**: Progress shown in status bar
4. **Results**: Summary shows QSOs updated, mults marked, dupes found

**When to rescore**:
- After importing ADIF file
- After updating contest rules
- If multipliers not detected correctly
- Suspect duplicate not marked

## Advanced Features

### Band Map

**Window → Band Map** (or Ctrl+B / Cmd+B)

- Shows spotted stations by frequency
- Click spot to QSY (if radio connected)
- DX Cluster integration required

### DX Cluster Integration

**Window → DX Cluster** (or Ctrl+D / Cmd+D)

1. **Connect to cluster**: Enter server (e.g., `dxc.w6rk.com:7373`)
2. **Login**: Enter your callsign
3. **Spots appear** in DX Cluster window and Band Map
4. **Click spot**: Radio QSYs to frequency (if connected)

### Multiplier Window

**Window → Multipliers** (or Ctrl+M / Cmd+M)

- **Worked mults**: Green checkmarks
- **Needed mults**: Empty boxes
- **Per-band view**: Shows multipliers by band
- **All-band view**: Total multipliers worked

**Tip**: Keep open during contest to track needed mults

### Statistics Window

**Window → Statistics** (or Ctrl+S / Cmd+S)

- **QSO counts by hour**: Bar graph
- **Rate trends**: Last hour, last 10 minutes
- **Band breakdown**: QSOs per band
- **Mode breakdown**: CW vs SSB vs Digital

### Partial Callsign Lookup

Type partial call to search log:

1. **Enter partial**: e.g., `W1A`
2. **Results show**: All matching calls in log
3. **Displays**: Previous exchanges, bands worked
4. **Helps**: Recall station info, check for dupes

**Keyboard Shortcut**: Ctrl+F / Cmd+F for search dialog

## Operating Tips

### Maximize Rate

1. **Use keyboard shortcuts**: Faster than mouse
2. **Auto-populate exchange**: Let TR4QT predict values
3. **Minimize typing**: Tab between fields, Enter to log
4. **Keep radio connected**: Auto frequency/mode logging
5. **Check dupes automatically**: Visual warnings prevent re-works

### Maximize Multipliers

1. **Open Multiplier window**: See what you need
2. **Monitor DX Cluster**: Spots for needed mults
3. **Work all bands**: Maximize per-band multipliers
4. **Check Needs display**: See worked bands for each station

### Minimize Errors

1. **Review exchange before logging**: Tab through fields
2. **Listen carefully**: Exchange predictions may be wrong
3. **Use partial call search**: Confirm previous QSOs
4. **Rescore periodically**: Catch any scoring errors

### Multi-Op Operation

1. **Enable web server**: Tools → Start Web Server
2. **Share URL**: Other operators can view log
3. **Use UDP broadcast**: Real-time updates to all stations (Preferences → Advanced)
4. **Coordinate mults**: Multiplier window shows what's needed

## Editing/Deleting QSOs

### Edit Existing QSO

1. **Double-click** QSO in table
2. **Edit fields** in dialog
3. **Click Save**
4. **Score recalculates** automatically

### Delete QSO

1. **Right-click** QSO in table
2. **Select Delete**
3. **Confirm deletion**
4. **Score recalculates** automatically

**Keyboard Shortcut**: Select QSO, press Delete key

### Delete Last QSO (Quick)

- **Operating → Delete Last QSO** (or Ctrl+Backspace)
- Immediately removes most recent QSO
- Useful for correcting mistakes

## Backup and Recovery

### Auto-Backup

TR4QT automatically backs up your log:

- **Frequency**: Every 10 QSOs (configurable in Preferences)
- **Location**: `~/.tr4qt/backups/`
- **Retention**: Last 50 backups kept (configurable)

### Manual Backup

**Tools → Backup Log**

Creates timestamped backup immediately.

### Restore from Backup

1. **Close current contest**
2. **File → Open Contest**
3. **Navigate to**: `~/.tr4qt/backups/`
4. **Select backup file**: Named with timestamp
5. **Open**: Contest loads from backup

**Tip**: Backups are SQLite database files - fully portable

## Contest Completion

### Export Cabrillo

After contest ends:

1. **File → Export Cabrillo**
2. **Review contest info**: Callsign, operators, category
3. **Select file location**
4. **Click Export**
5. **Submit to contest sponsor**

**Format**: Standard Cabrillo format accepted by all major contests

### Export ADIF

For logging services (LoTW, QRZ, Clublog):

1. **File → Export ADIF**
2. **Select file location**
3. **Click Export**
4. **Upload to logging service**

**Format**: ADIF 3.1 with all contest fields

## Next Steps

- [ADIF Import & Export details](USER_GUIDE_ADIF.md)
- [Learn all keyboard shortcuts](USER_GUIDE_SHORTCUTS.md)
- [Troubleshooting common issues](USER_GUIDE_TROUBLESHOOTING.md)
