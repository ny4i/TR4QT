# ADIF Import & Export Guide

TR4QT supports ADIF (Amateur Data Interchange Format) for exchanging logs with other logging software and services like LoTW, QRZ, and Clublog.

## What is ADIF?

**ADIF** (Amateur Data Interchange Format) is the standard format for amateur radio logging data.

**Version supported**: ADIF 3.1.4

**Use cases**:
- Import QSOs from other logging software
- Export for LoTW, QRZ, Clublog confirmation
- Backup and archive logs
- Share logs with other stations

## Exporting ADIF

### Export Entire Log

1. **File → Export ADIF**
2. **Select destination**: Choose filename and location
3. **Click Save**
4. **Success message**: Shows number of QSOs exported

**Filename suggestion**: `MYCALL_CONTEST_2025.adi`

### What Gets Exported

Every QSO includes:
- **Required**: Call, QSO date/time, band, mode, frequency
- **Contest fields**: Exchange sent/received, contest ID
- **Station info**: My callsign, grid square
- **Geographic**: Country, CQ zone, ITU zone, continent
- **Custom fields**: ARRL section, operator, notes

**Example ADIF record**:
```
<CALL:5>W1AW <QSO_DATE:8>20251225 <TIME_ON:6>143000 <BAND:3>20M <MODE:2>CW <FREQ:8>14.02500 <RST_SENT:3>599 <RST_RCVD:3>599 <CQZ:1>5 <STX_STRING:1>5 <SRX_STRING:1>5 <CONTEST_ID:4>CQWW <STATION_CALLSIGN:4>K4NY <MY_GRIDSQUARE:6>EM95wm <DXCC:3>291 <COUNTRY:24>United States of America <CONT:2>NA <EOR>
```

### Export for Specific Services

**LoTW (Logbook of the World)**:
- Export ADIF from TR4QT
- Sign with TQSL tool
- Upload to LoTW

**QRZ Logbook**:
- Export ADIF from TR4QT
- Upload at https://logbook.qrz.com/import

**Clublog**:
- Export ADIF from TR4QT
- Upload at https://clublog.org/importadif.php

**eQSL**:
- Export ADIF from TR4QT
- Upload at https://www.eqsl.cc/qslcard/ImportADIF.cfm

## Importing ADIF

### Import into Existing Contest

**Important**: You must have an active contest open before importing.

1. **Open or create a contest**: File → New/Open Contest
2. **File → Import ADIF**
3. **Import dialog appears** with options
4. **Select ADIF file**: Click "Select File" button
5. **Choose file**: Navigate to .adi or .adif file
6. **Configure options** (see below)
7. **Click Import**
8. **View results**: Summary shows imported vs failed QSOs

### Import Options

**Auto-correct common errors** (recommended: ✓ enabled)
- Fixes common ADIF formatting issues
- Corrects date/time formats
- Normalizes band/mode values
- Enables automatic import without prompts

**Rescore contest after import** (recommended: ✓ enabled)
- Recalculates QSO points for active contest
- Detects and marks multipliers
- Finds duplicates
- Ensures imported QSOs scored correctly

**When to disable rescore**:
- Importing into different contest type (scoring rules don't match)
- Very large imports (10,000+ QSOs) where speed matters
- You plan to rescore manually later

### What Gets Imported

TR4QT maps ADIF fields to QSO records:

**Required fields**:
- `<CALL>` → Callsign
- `<QSO_DATE>` + `<TIME_ON>` → Timestamp
- `<BAND>` or `<FREQ>` → Band and frequency
- `<MODE>` → Mode (CW, SSB, FT8, etc.)

**Exchange fields** (contest-specific):
- `<RST_SENT>` / `<RST_RCVD>` → RST
- `<STX_STRING>` / `<SRX_STRING>` → Exchange sent/received
- `<CQZ>` / `<ITUZ>` → Zones
- `<ARRL_SECT>` → ARRL Section
- `<STATE>` → US State
- `<GRIDSQUARE>` → Grid square

**Geographic data** (if available):
- `<COUNTRY>` → Country
- `<CONT>` → Continent
- `<DXCC>` → DXCC entity

**Optional fields**:
- `<OPERATOR>` → Operator name
- `<NOTES>` → QSO comments
- `<TX_PWR>` → Transmit power

### Field Mapping Examples

**CQ WW DX Contest**:
- `<RST_SENT>` → Sent RST
- `<STX_STRING>` → Sent CQ Zone
- `<RST_RCVD>` → Received RST
- `<SRX_STRING>` → Received CQ Zone

**ARRL Field Day**:
- `<RST_SENT>` → Sent RST
- `<STX_STRING>` → Sent class+section (e.g., "2A ORG")
- `<RST_RCVD>` → Received RST
- `<SRX_STRING>` → Received class+section

**CQ WPX**:
- `<RST_SENT>` → Sent RST
- `<STX_STRING>` → Sent serial number
- `<RST_RCVD>` → Received RST
- `<SRX_STRING>` → Received serial number

### Error Handling

**Common import errors**:

1. **Invalid date format**
   - Auto-correct: Attempts to parse various formats
   - Manual: Shows error, lets you skip or fix

2. **Missing required field** (callsign, date, time)
   - Error reported, QSO skipped
   - Check source ADIF file

3. **Invalid band/mode**
   - Auto-correct: Normalizes to standard values
   - Manual: Shows error with suggested fix

4. **Duplicate QSO**
   - Warning shown but imports anyway
   - Rescore marks as duplicate (0 points)

### Import Results

After import completes:

**Success message**:
```
Successfully imported 150 QSOs
Failed: 2 QSOs
```

**Results panel** shows:
- Each imported QSO with status (✓ or ✗)
- Error details for failed imports
- Total counts

**If rescore enabled**, status bar shows:
```
Imported 150 QSOs, rescored: 150 updated, 45 mults, 3 dupes
```

## Common Workflows

### Import from Other Logging Software

**From N1MM+**:
1. N1MM: File → Export → ADIF
2. Select all QSOs for contest
3. Export to .adi file
4. TR4QT: Import ADIF with rescore enabled

**From Win-Test**:
1. Win-Test: File → Export ADIF
2. Save to .adi file
3. TR4QT: Import ADIF with rescore enabled

**From Log4OM**:
1. Log4OM: Export → ADIF
2. Filter by contest if needed
3. TR4QT: Import ADIF with rescore enabled

### Merge Logs from Multiple Stations

**Scenario**: Multi-op contest, each station logged independently

**Workflow**:
1. **Create master contest** in TR4QT
2. **Export ADIF** from each station's software
3. **Import each ADIF file** sequentially into master contest
4. **Rescore** after all imports complete
5. **Review duplicates**: Multi-op may have same QSOs logged twice
6. **Delete duplicates** or mark as such

### Paper Log Entry

**Scenario**: Operated portable, logged on paper, entering later

**Workflow**:
1. **Enter QSOs manually** into any logging software that exports ADIF
2. Or use ADIF editor (e.g., ADIFMaster, ADIF Editor)
3. **Export ADIF**
4. **Import into TR4QT** with rescore enabled
5. **Verify all QSOs** imported correctly

### Migrate from Another Logging Program

**Scenario**: Switching to TR4QT from another logger

**Workflow**:
1. **Export all contests** from old logger as ADIF
2. For each contest:
   - Create matching contest in TR4QT
   - Import ADIF file
   - Verify scoring matches
3. **Archive old logs** as ADIF backups

## Advanced: ADIF Field Details

### TR4QT Custom Fields

TR4QT uses some ADIF fields in specific ways:

**Contest Identification**:
- `<CONTEST_ID>`: Contest type (e.g., "CQ-WW-CW")
- Helps TR4QT validate imported QSOs match active contest

**Multiplier Tracking**:
- `<COUNTRY>`, `<CQZ>`, `<ITUZ>`, `<ARRL_SECT>`, `<STATE>`
- Used for multiplier detection during rescore

**Station Info**:
- `<STATION_CALLSIGN>`: Your callsign
- `<MY_GRIDSQUARE>`: Your grid square
- `<MY_CQ_ZONE>`: Your CQ zone

### ADIF Date/Time Formats

TR4QT supports:

**Date** (`<QSO_DATE>`):
- YYYYMMDD (e.g., "20251225")
- YYYY-MM-DD (auto-corrected)

**Time** (`<TIME_ON>`):
- HHMMSS (e.g., "143000")
- HHMM (auto-corrected to HHMMSS)
- HH:MM:SS (auto-corrected)

**Time zone**: Always UTC (ADIF standard)

### Band vs Frequency

ADIF supports both `<BAND>` and `<FREQ>`:

**If both present**: `<FREQ>` takes precedence
**If only `<BAND>`**: TR4QT assigns default frequency for band
**If only `<FREQ>`**: TR4QT derives band from frequency

**Band names supported**:
- 160M, 80M, 60M, 40M, 30M, 20M, 17M, 15M, 12M, 10M, 6M, 2M

**Frequency format**: MHz with decimals (e.g., "14.025")

## Troubleshooting

### "No active contest - please create a contest first"

**Cause**: Tried to import without opening/creating contest

**Solution**: File → New/Open Contest, then import

### "Failed to parse ADIF file"

**Cause**: Malformed ADIF file

**Solutions**:
1. Open ADIF file in text editor, check for corruption
2. Re-export from source software
3. Use ADIF validator tool (e.g., ADIFMaster)

### Import succeeds but QSOs have 0 points

**Cause**: Contest scoring rules don't match imported QSOs

**Solutions**:
1. Verify contest type matches source log
2. Run manual rescore: Operating → Rescore Contest
3. Check exchange fields mapped correctly

### Dates show as 1970 or very old

**Cause**: Date format not recognized

**Solution**:
- Enable "Auto-correct common errors"
- Or manually edit ADIF file to use YYYYMMDD format

### Import shows "150 failed, 0 succeeded"

**Cause**: Missing required fields (usually `<CALL>` or `<QSO_DATE>`)

**Solution**:
1. Check ADIF file in text editor
2. Ensure every record has callsign and date
3. Re-export from source with all fields

## Best Practices

### Before Importing

1. ✅ **Create matching contest** in TR4QT first
2. ✅ **Backup current log** if appending to existing contest
3. ✅ **Validate ADIF file** with external tool (optional)
4. ✅ **Enable auto-correct** for cleaner import

### After Importing

1. ✅ **Review import results** panel for errors
2. ✅ **Check score** matches expected value
3. ✅ **Verify multipliers** detected correctly
4. ✅ **Scan for duplicates** if merging multiple sources
5. ✅ **Rescore if needed**: Operating → Rescore Contest

### For Exports

1. ✅ **Export early and often** (backup your work)
2. ✅ **Use descriptive filenames** (callsign_contest_date.adi)
3. ✅ **Test import** into LoTW/QRZ before contest ends
4. ✅ **Keep ADIF backups** in multiple locations

## ADIF vs Cabrillo

**When to use ADIF**:
- Backup and archival
- Importing to other logging software
- LoTW, QRZ, Clublog submission
- Data interchange between programs

**When to use Cabrillo**:
- Contest log submission to sponsors
- Required format for contest entry
- Standardized contest exchange format

**Key difference**:
- ADIF: General-purpose log format
- Cabrillo: Contest-specific submission format

**Tip**: Export both from TR4QT after contest!

## Next Steps

- [Learn keyboard shortcuts for faster logging](USER_GUIDE_SHORTCUTS.md)
- [Troubleshoot common issues](USER_GUIDE_TROUBLESHOOTING.md)
