# Contest Development Guide

## Overview

TR4QT uses a modular, plugin-based contest system that makes it easy to add new contests without modifying core code. Each contest is a self-contained class that implements the `ContestBase` interface.

## Architecture

### Key Design Principles

1. **Contest classes are stateless** - All information needed for scoring/multipliers is passed as parameters
2. **Contest classes are queried** - The logging system asks the contest "what are the points?" and "is this a multiplier?"
3. **No core code changes needed** - Add a new contest by creating a new class and registering it
4. **Clean separation** - Scoring logic, multiplier logic, and exchange validation are isolated in the contest class

### ContestBase Interface

Every contest implements these methods:

#### Identity Methods
- `getContestId()` - Unique ID (e.g., "CQWW_CW")
- `getContestName()` - Display name for UI
- `getContestMode()` - Contest mode restriction (CW, SSB, Mixed)

#### Exchange Configuration
- `getReceivedExchangeFields()` - What fields to collect from other station
- `getSentExchangeFields()` - What fields we send (for settings dialog)
- `formatSentExchange()` - Format our exchange (with serial numbers if needed)
- `validateReceivedExchange()` - Check if received exchange is valid
- `parseReceivedExchange()` - Parse exchange into field components

#### Scoring
- `calculateQSOPoints(qso, myInfo...)` - **Core method**: Returns points for one QSO
- `calculateTotalScore(qsoPoints, multCounts)` - Compute final score from points and mults

#### Multipliers
- `getMultiplierTypes()` - List of multiplier types (Country, Zone, etc.) and their scope
- `getMultiplierValue(qso, multType, alreadyWorked)` - **Core method**: Returns mult value or empty if not a new mult

#### Special Rules
- `isValidQSO()` - Apply contest-specific rules (e.g., can't work own country)
- `usesSerialNumbers()` - Does contest use auto-increment serial numbers?
- `getCabrilloHeaders()` - Cabrillo export header fields

## Adding a New Contest

### Step 1: Create Contest Class

Create `src/contests/YourContest.h`:

```cpp
#ifndef YOURCONTEST_H
#define YOURCONTEST_H

#include "ContestBase.h"

namespace TR4QT {

class YourContest : public ContestBase {
public:
    explicit YourContest(ModeType mode = ModeType::Mixed);
    ~YourContest() override = default;

    // Implement all pure virtual methods from ContestBase
    QString getContestId() const override;
    QString getContestName() const override;
    ModeType getContestMode() const override;

    // ... etc (see ContestBase.h for full list)

private:
    ModeType m_mode;
};

} // namespace TR4QT

#endif
```

### Step 2: Implement Scoring Logic

The most important method is `calculateQSOPoints()`. This is where your contest's unique scoring rules go.

Example from CQ WW Contest:

```cpp
int CQWWContest::calculateQSOPoints(
    const QSO& qso,
    const QString& myCountry,
    const QString& myContinent,
    int myCQZone,
    const QString& myState) const
{
    const QString& theirCountry = qso.dxccEntity;
    const QString& theirContinent = qso.continent;

    // Special W/VE rule
    bool imWVE = (myCountry == "United States" || myCountry == "Canada");
    bool theyWVE = (theirCountry == "United States" || theirCountry == "Canada");
    if (imWVE && theyWVE) {
        return 2;  // W/VE working each other
    }

    // Same continent, different country
    if (myContinent == theirContinent && myCountry != theirCountry) {
        return 1;
    }

    // Different continent (DX)
    if (myContinent != theirContinent) {
        return (m_mode == ModeType::CW) ? 3 : 2;
    }

    return 0;
}
```

### Step 3: Implement Multiplier Logic

Define what multipliers your contest uses:

```cpp
QList<MultiplierDefinition> CQWWContest::getMultiplierTypes() const {
    QList<MultiplierDefinition> mults;

    // DXCC Countries (per band)
    MultiplierDefinition country;
    country.type = MultiplierType::Country;
    country.scope = MultiplierScope::PerBand;  // Or AllBands
    country.displayName = "Countries";
    mults.append(country);

    // CQ Zones (per band)
    MultiplierDefinition zone;
    zone.type = MultiplierType::CQZone;
    zone.scope = MultiplierScope::PerBand;
    zone.displayName = "CQ Zones";
    mults.append(zone);

    return mults;
}
```

Then implement the multiplier checking:

```cpp
QString CQWWContest::getMultiplierValue(
    const QSO& qso,
    MultiplierType multType,
    const QStringList& alreadyWorkedValues) const
{
    QString value;

    switch (multType) {
    case MultiplierType::Country:
        value = qso.dxccPrefix;  // e.g., "K", "JA", "G"
        break;

    case MultiplierType::CQZone:
        value = QString::number(qso.cqZone);
        break;

    default:
        return QString();  // Not a multiplier for this contest
    }

    // Check if already worked
    if (alreadyWorkedValues.contains(value)) {
        return QString();  // Already worked, not new
    }

    return value;
}
```

### Step 4: Implement Exchange Handling

Define what exchange fields you need:

```cpp
QList<ExchangeField> YourContest::getReceivedExchangeFields() const {
    QList<ExchangeField> fields;

    // RST
    ExchangeField rst;
    rst.name = "RST";
    rst.hint = "599";
    rst.autoFill = true;  // Auto-populate
    rst.maxLength = 3;
    fields.append(rst);

    // Serial Number
    ExchangeField serial;
    serial.name = "Serial";
    serial.hint = "001";
    serial.autoFill = false;
    serial.maxLength = 4;
    fields.append(serial);

    return fields;
}
```

Validate the exchange:

```cpp
bool YourContest::validateReceivedExchange(const QString& exchange, QString& errorMsg) const {
    QStringList parts = exchange.trimmed().split(QRegularExpression("\\s+"));

    if (parts.size() < 2) {
        errorMsg = "Exchange must be RST + Serial (e.g., '599 042')";
        return false;
    }

    // Validate RST format
    // Validate serial number format
    // etc.

    return true;
}
```

### Step 5: Add to Build System

Edit `src/CMakeLists.txt`:

```cmake
set(SOURCES
    # ... existing sources ...
    contests/YourContest.cpp
)

set(HEADERS
    # ... existing headers ...
    contests/YourContest.h
)
```

### Step 6: Register in Contest Factory

(To be implemented - will allow UI to discover available contests)

```cpp
// Future: ContestFactory will auto-register contests
ContestFactory::registerContest("YOUR_CONTEST_ID",
    []() { return new YourContest(); });
```

## QSO Data Structure

Contest methods receive QSO objects with all available information:

```cpp
struct QSO {
    QString callsign;        // Callsign worked
    freq_t frequency;        // Frequency in Hz
    ModeType mode;           // Mode (CW, SSB, etc.)
    BandType band;           // Band (160M, 80M, etc.)
    QDateTime timestamp;     // When contact was made

    // Exchange
    QString rstSent;
    QString rstReceived;
    QString exchangeSent;
    QString exchangeReceived;
    QMap<QString, QString> parsedExchange;  // Parsed fields

    // Geographic info (from cty.dat)
    QString dxccEntity;      // Country name
    QString dxccPrefix;      // DXCC prefix
    int cqZone;              // CQ Zone
    int ituZone;             // ITU Zone
    QString continent;       // Continent code
    QString state;           // US/VE state

    // Scoring (populated by contest)
    int qsoPoints;
    bool isDupe;
    bool isMultiplier;
    QStringList multipliers;
};
```

## Multiplier Types

Available multiplier types (from `ContestBase.h`):

- `MultiplierType::Country` - DXCC countries
- `MultiplierType::CQZone` - CQ zones (1-40)
- `MultiplierType::ITUZone` - ITU zones
- `MultiplierType::State` - US/Canadian states/provinces
- `MultiplierType::Section` - ARRL/RAC sections
- `MultiplierType::Prefix` - Callsign prefix (WPX)
- `MultiplierType::Grid` - Maidenhead grid squares
- `MultiplierType::Custom` - Contest-specific

Each multiplier has a scope:
- `MultiplierScope::PerBand` - Counts separately on each band
- `MultiplierScope::AllBands` - Counts once across all bands

## Example Contests

### CQ WW DX Contest

- **File:** `src/contests/CQWWContest.h/cpp`
- **Exchange:** RST + CQ Zone
- **Multipliers:** Countries (per-band), CQ Zones (per-band)
- **Scoring:** 1-3 points based on continent, multiply by (countries + zones)
- **Special rules:** W/VE working each other = 2 points

### CQ WPX Contest (To be implemented)

- **Exchange:** RST + Serial Number
- **Multipliers:** Callsign prefixes (all-band)
- **Scoring:** Variable points, double on 160m/10m, multiply by prefixes
- **Serial numbers:** Auto-increment

### Winter Field Day (To be implemented)

- **Exchange:** Class + Section
- **Multipliers:** Sections (all-band)
- **Scoring:** 2pts CW/Digital, 1pt Phone, no multiplier in formula
- **Bonus points:** Checkbox UI on Cabrillo export

## Best Practices

1. **Keep contest classes stateless** - Don't store QSO counts or scores in the contest object
2. **Return empty string for non-multipliers** - `getMultiplierValue()` should return "" if not a new mult
3. **Use QSO geographic fields** - Leverage `dxccPrefix`, `continent`, `cqZone` populated by cty.dat lookup
4. **Validate early** - Check exchange format in `validateReceivedExchange()` before accepting QSO
5. **Document your scoring** - Add comments explaining point values and special rules
6. **Test with real data** - Use actual contest logs to verify scoring matches official results

## Testing Your Contest

```cpp
// Create contest instance
CQWWContest contest(ModeType::CW);

// Create test QSO
QSO qso;
qso.callsign = "JA1ABC";
qso.mode = ModeType::CW;
qso.band = BandType::Band20M;
qso.dxccEntity = "Japan";
qso.dxccPrefix = "JA";
qso.continent = "AS";
qso.cqZone = 25;
qso.exchangeReceived = "599 25";

// Test scoring
int points = contest.calculateQSOPoints(qso, "United States", "NA", 5);
// Should return 3 points (DX, CW mode)

// Test multiplier
QStringList worked;
QString mult = contest.getMultiplierValue(qso, MultiplierType::Country, worked);
// Should return "JA"

worked.append("JA");
mult = contest.getMultiplierValue(qso, MultiplierType::Country, worked);
// Should return "" (already worked)
```

## Future Enhancements

- **Contest Factory** - Auto-registration and discovery
- **Contest Selector Dialog** - UI to choose contest at startup
- **Contest Validation** - Check against official contest rules
- **Cabrillo Export** - Per-contest Cabrillo formatting
- **Contest-specific UI** - Custom widgets for special contests (e.g., WFD bonus points)

## Questions?

This modular system makes TR4QT highly extensible. To add support for your favorite contest, just implement the `ContestBase` interface and add your class to the build!
