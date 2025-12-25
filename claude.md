# Claude Development Notes for TR4QT

This file contains important reminders and conventions for Claude when working on TR4QT.

## Version Management

**CRITICAL**: The version number must be updated with every release commit.

Location: `/src/core/Constants.h`
```cpp
constexpr const char* APP_VERSION = "X.Y.Z";
```

Convention:
- **Major (X)**: Major features or breaking changes
- **Minor (Y)**: New features, enhancements
- **Patch (Z)**: Bug fixes, small improvements

Update process:
1. Increment version in Constants.h
2. Update comment to describe the change
3. Include version in commit message: "Feature description - vX.Y.Z"
4. Rebuild: `cmake --build build`

Example:
```cpp
constexpr const char* APP_VERSION = "2.58.0";  // Zone lookup from cty.dat, manual band selection fixes
```

## Build Process

Standard build:
```bash
cmake --build build
```

Clean rebuild:
```bash
rm -rf build && cmake -B build && cmake --build build
```

Run after build:
```bash
./build/src/tr4qt.app/Contents/MacOS/tr4qt
```

## Testing

Run tests:
```bash
cd build && ctest --output-on-failure
```

## Git Workflow

Commit message format:
```
Brief description of change - vX.Y.Z

Detailed explanation of what changed and why.

Changes:
- Bullet point list of specific changes
- Include file locations if helpful

🤖 Generated with [Claude Code](https://claude.com/claude-code)

Co-Authored-By: Claude Sonnet 4.5 <noreply@anthropic.com>
```

## Common Patterns

### Radio State
Radio frequency/band/mode is stored in `m_currentState` (RadioState struct):
- `frequencyA`: freq_t (Hz)
- `bandA`: BandType enum
- `modeA`: ModeType enum

### Country Lookup
```cpp
CountryData countryData = m_countryFile.lookup(callsign);
if (countryData.isValid()) {
    int zone = countryData.cqZone;
    QString country = countryData.name;
    QString continent = continentToString(countryData.continent);
}
```

### Exchange Prediction
Uses InitialExchangeManager singleton:
```cpp
QString prediction = InitialExchangeManager::instance().predictExchange(
    callsign,
    m_activeContest,
    m_currentState.modeA
);
```

Strategy priority:
1. Exchange memory (exact match)
2. CTY.DAT lookup (zone/country)
3. Contest defaults (RST)

### Database Operations
Always check for active contest:
```cpp
if (m_hasActiveContest) {
    QSORepository repo;
    if (!repo.saveQSO(qso, m_currentContestDbId)) {
        LOG_WARN("Tag", QString("Failed: %1").arg(repo.lastError()));
    }
}
```

## Architecture Notes

### Contest System
- Base class: `ContestBase` (abstract)
- Contest implementations: `CQWWContest`, `CQWPXContest`, `WinterFieldDayContest`
- Auto-registration: `REGISTER_CONTEST(ClassName, "ID")`
- Factory access: `ContestRegistry::instance().createContest("ID")`

### Exchange Fields
Contests define received/sent exchange fields:
```cpp
QList<ExchangeField> getReceivedExchangeFields() const override;
QList<ExchangeField> getSentExchangeFields() const override;
```

QSOTableModel adapts columns based on contest exchange fields.

### Duplicate Checking
Each contest defines duplicate rule:
```cpp
DuplicateCheckingRule getDuplicateCheckingRule() const override {
    return DuplicateCheckingRule::PerBandMode;
}
```

Options:
- `PerBandMode`: Same call/band/mode is dupe
- `AllBandMode`: Same call/mode across all bands
- `PerBand`: Same call/band (any mode)
- `AllBand`: Same call (once per contest)

## Key Files

### Core
- `/src/core/Constants.h` - **VERSION HERE**
- `/src/core/Types.h` - Enums and type definitions
- `/src/models/QSO.h` - QSO record structure

### UI
- `/src/ui/MainWindow.cpp` - Main application logic
- `/src/ui/models/QSOTableModel.cpp` - Log display

### Contests
- `/src/contests/ContestBase.h` - Contest interface
- `/src/contests/CQWWContest.cpp` - CQ WW implementation
- `/src/contests/CQWPXContest.cpp` - CQ WPX implementation

### Exchange System
- `/src/exchanges/InitialExchangeManager.cpp` - Exchange prediction
- `/src/data/ExchangeMemoryRepository.cpp` - Exchange memory

### Country Data
- `/src/utils/CountryFile.cpp` - CTY.DAT parser
- `~/.tr4qt/cty.dat` - Country file data

## Recent Changes (Session Summary)

### v2.57.x - Manual Band Selection
- Fixed focus issues (callsign field gets focus on startup)
- Fixed duplicate checking (band/mode stored as TEXT not enums)
- Implemented manual band selection when radio disconnected
- Added ALT-B/ALT-V (Option-B/Option-V on Mac) for band up/down
- Set frequency to band edge for manual selection (visual indicator)
- Fixed frequency display without radio connection

### v2.58.0 - Zone Lookup
- Implemented country/zone lookup from cty.dat when logging QSO
- Zones now auto-fill in exchange field (via InitialExchangeManager)
- Zones display in "Zn" column of QSO table
- Geographic fields populated: CQ Zone, ITU Zone, Country, Continent

## Known Issues / TODOs

From code comments:
- [ ] Calculate QSO points via contest (line 1385)
- [ ] Check for new multipliers (line 1386)
- [ ] Improve exchange prediction architecture (per plan file)

## Mac-Specific Notes

Keyboard shortcuts:
- Qt::ALT maps to Option (⌥) on Mac
- Qt::CTRL maps to Command (⌘) on Mac
- Displayed automatically in menus with correct symbols

Path handling:
- Use QDir::homePath() for cross-platform home directory
- Config: `~/.tr4qt/`
- Logs: `~/.tr4qt/logs/`
- Backups: `~/.tr4qt/backups/`
