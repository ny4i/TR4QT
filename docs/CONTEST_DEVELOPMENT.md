# Contest Development Guide

This guide explains how to create a new contest implementation in TR4QT.

## Overview

TR4QT uses a **strategy pattern** for contest-specific logic. Each contest is implemented as a subclass of `ContestBase`, which defines the interface for:

- Exchange field definitions (sent and received)
- Scoring rules (QSO points calculation)
- Multiplier definitions and tracking
- Exchange validation and parsing
- Cabrillo/ADIF export metadata

## Contest Identifiers

Each contest class **must** define three public static identifiers:

### 1. WA7BNM Contest Calendar ID

The WA7BNM Contest Calendar (https://www.contestcalendar.com/) assigns a unique numeric ID to each contest. This ID is used for contest lookups and references.

```cpp
// Single-mode contest (e.g., Winter Field Day)
static constexpr int WA7BNM_ID = 421;

// Multi-mode contest (e.g., CQ WW)
static constexpr int WA7BNM_ID_CW = 3;
static constexpr int WA7BNM_ID_SSB = 4;
```

**How to find WA7BNM ID:**
1. Visit https://www.contestcalendar.com/weeklycont.php
2. Click on the contest name
3. Look at the URL: `https://www.contestcalendar.com/contestdetails.php?ref=421`
4. The `ref=` parameter is the WA7BNM ID

### 2. Cabrillo Contest Name

The Cabrillo file format requires a standardized contest name in the header (`CONTEST:` field). This must match the official Cabrillo specification.

```cpp
// Single-mode contest
static inline const QString CABRILLO_NAME = "WINTER-FIELD-DAY";

// Multi-mode contest
static inline const QString CABRILLO_NAME_CW = "CQ-WW-CW";
static inline const QString CABRILLO_NAME_SSB = "CQ-WW-SSB";
```

**How to find Cabrillo name:**
1. Check the contest's official rules page
2. Look for Cabrillo log submission requirements
3. Or visit: https://wwrof.org/cabrillo/ for standardized names

### 3. ADIF Contest-ID

The ADIF (Amateur Data Interchange Format) specification defines standardized contest identifiers for use in the `CONTEST_ID` field when exporting ADIF files.

```cpp
// Single-mode contest
static inline const QString ADIF_CONTEST_ID = "WINTER-FIELD-DAY";

// Multi-mode contest
static inline const QString ADIF_CONTEST_ID_CW = "CQ-WW-CW";
static inline const QString ADIF_CONTEST_ID_SSB = "CQ-WW-SSB";
```

**How to find ADIF Contest-ID:**
1. Visit: https://adif.org.uk/316/ADIF_316.htm#Contest_ID_Enumeration
2. Search for your contest name
3. Use the exact string from the specification (case-sensitive)

**Note:** In most cases, the Cabrillo name and ADIF Contest-ID are identical, but always verify both specifications.

## Creating a New Contest Class

### Step 1: Create Header File

Create `src/contests/YourContestName.h`:

```cpp
#ifndef YOURCONTESTNAME_H
#define YOURCONTESTNAME_H

#include "ContestBase.h"

namespace TR4QT {

/**
 * Your Contest Name
 *
 * Exchange: Brief description (e.g., "RST + Serial Number")
 * Modes: CW/SSB/Mixed/Digital
 * Multipliers:
 *   - List of multiplier types (e.g., "DXCC Countries (per band)")
 * Scoring:
 *   - Brief description of points per QSO
 * Total Score: Formula (e.g., "QSO points × Total multipliers")
 *
 * Contest website: https://...
 */
class YourContestName : public ContestBase {
public:
    explicit YourContestName(ModeType mode = ModeType::CW);
    ~YourContestName() override = default;

    // ===== Contest Identifiers =====
    // WA7BNM Contest Calendar ID
    static constexpr int WA7BNM_ID_CW = ???;  // Find at contestcalendar.com
    static constexpr int WA7BNM_ID_SSB = ???;

    // Cabrillo contest names
    static inline const QString CABRILLO_NAME_CW = "YOUR-CONTEST-CW";
    static inline const QString CABRILLO_NAME_SSB = "YOUR-CONTEST-SSB";

    // ADIF Contest-ID values
    static inline const QString ADIF_CONTEST_ID_CW = "YOUR-CONTEST-CW";
    static inline const QString ADIF_CONTEST_ID_SSB = "YOUR-CONTEST-SSB";

    // ===== Contest Identity =====
    QString getContestId() const override;
    QString getContestName() const override;
    ModeType getContestMode() const override { return m_mode; }

    // ===== Exchange Configuration =====
    QList<ExchangeField> getReceivedExchangeFields() const override;
    QList<ExchangeField> getSentExchangeFields() const override;
    QString formatSentExchange(int serialNumber, const QString& rst = "599") const override;
    bool validateReceivedExchange(const QString& exchange, QString& errorMsg) const override;
    QMap<QString, QString> parseReceivedExchange(const QString& exchange) const override;

    // ===== Scoring =====
    int calculateQSOPoints(
        const QSO& qso,
        const StationInfo& myStation) const override;

    int calculateTotalScore(
        int totalQSOPoints,
        const QMap<MultiplierType, int>& multiplierCounts) const override;

    // ===== Multipliers =====
    QList<MultiplierDefinition> getMultiplierTypes() const override;

    QString getMultiplierValue(
        const QSO& qso,
        MultiplierType multType,
        const QStringList& alreadyWorkedValues) const override;

    // ===== Special Rules =====
    bool usesSerialNumbers() const override { return true; }  // or false

    QMap<QString, QString> getCabrilloHeaders() const override;

private:
    ModeType m_mode;  // If contest is mode-specific
};

} // namespace TR4QT

#endif // YOURCONTESTNAME_H
```

### Step 2: Implement Contest Methods

Create `src/contests/YourContestName.cpp` and implement all virtual methods.

**Key Implementation Notes:**

1. **getReceivedExchangeFields()**: Define what you expect to receive from other stations
   - Example: RST, Serial Number, Section, etc.

2. **getSentExchangeFields()**: Define what you send to other stations
   - Often simpler than received (e.g., just RST + your section)

3. **validateReceivedExchange()**: Parse and validate incoming exchange
   - Use CTY.DAT for country/prefix lookups
   - Validate section codes against ARRL/RAC lists
   - Check serial number format, zone ranges, etc.

4. **calculateQSOPoints()**: Implement contest-specific scoring
   - Consider: same/different continent, band, mode
   - Use `myStation.continent`, `qso.dxccPrefix`, etc.

5. **getMultiplierTypes()**: Define multiplier categories
   - Per-band or all-band
   - Example: Countries, Zones, States, Sections

6. **getCabrilloHeaders()**: Return contest-specific Cabrillo headers
   - Use the `CABRILLO_NAME_*` constants defined in the header

### Step 3: Add to Build System

Edit `src/CMakeLists.txt` and add your new files:

```cmake
set(CONTEST_SOURCES
    contests/ContestBase.cpp
    contests/CQWWContest.cpp
    contests/CQWPXContest.cpp
    contests/WinterFieldDayContest.cpp
    contests/YourContestName.cpp       # ADD THIS
)
```

### Step 4: Register Contest

Add your contest to the contest selection system (location TBD - likely in a ContestRegistry or MainWindow).

## Exchange Field Types

The `ExchangeField` struct defines each component of the exchange:

```cpp
struct ExchangeField {
    QString name;           // "RST", "Serial", "Section", "Zone", etc.
    QString description;    // Human-readable description
    bool required;          // Is this field mandatory?
    QString regex;          // Validation regex (optional)
};
```

## Common Exchange Patterns

### RST + Serial Number (e.g., CQ WPX)
```cpp
QList<ExchangeField> getReceivedExchangeFields() const override {
    return {
        {"RST", "Signal Report", true, "^[1-5][1-9][1-9]$"},
        {"Serial", "Serial Number", true, "^[0-9]{1,4}$"}
    };
}
```

### RST + Zone (e.g., CQ WW)
```cpp
QList<ExchangeField> getReceivedExchangeFields() const override {
    return {
        {"RST", "Signal Report", true, "^[1-5][1-9][1-9]$"},
        {"Zone", "CQ Zone", true, "^[1-9]|[1-3][0-9]|40$"}  // 1-40
    };
}
```

### Class + Section (e.g., Field Day)
```cpp
QList<ExchangeField> getReceivedExchangeFields() const override {
    return {
        {"Class", "Station Class", true, "^[1-9][0-9]?[ABCDEF]$"},
        {"Section", "ARRL/RAC Section", true, ""}  // Validate via section list
    };
}
```

### Multi-Field Exchange (e.g., Sweepstakes)
```cpp
QList<ExchangeField> getReceivedExchangeFields() const override {
    return {
        {"Serial", "Serial Number", true, "^[0-9]{1,4}$"},
        {"Precedence", "Precedence", true, "^[QABUMS]$"},
        {"Check", "Check (Year Licensed)", true, "^[0-9]{2}$"},
        {"Section", "ARRL/RAC Section", true, ""},
        {"YearLicensed", "Year Licensed", true, "^[0-9]{2}$"}
    };
}
```

## Using CTY.DAT for Country Lookups

For contests that depend on country/continent (e.g., CQ WW, WPX), use the CTY.DAT parser:

```cpp
#include "../utils/CTYParser.h"

// In your scoring or multiplier method:
CTYParser& ctyParser = CTYParser::instance();
CountryInfo dxCountry = ctyParser.lookup(qso.callsign);

if (dxCountry.isValid) {
    QString continent = dxCountry.continent;  // "NA", "EU", "AS", etc.
    int cqZone = dxCountry.cqZone;
    int ituZone = dxCountry.ituZone;
    // ... use for scoring or multiplier logic
}
```

## Multiplier Definitions

Define multipliers using the `MultiplierDefinition` struct:

```cpp
QList<MultiplierDefinition> getMultiplierTypes() const override {
    return {
        {
            MultiplierType::Country,
            "DXCC Country",
            true,  // per-band = true
            false  // all-time = false
        },
        {
            MultiplierType::Zone,
            "CQ Zone",
            true,  // per-band = true
            false
        }
    };
}
```

## Testing Your Contest

1. **Build**: `cmake --build build`
2. **Run**: `./build/src/tr4qt.app/Contents/MacOS/tr4qt`
3. **Select Contest**: File → New Contest → Your Contest
4. **Test Exchange Validation**: Log some QSOs with valid/invalid exchanges
5. **Check Scoring**: Verify QSO points calculated correctly
6. **Check Multipliers**: Verify multiplier detection and counting
7. **Export Cabrillo**: Verify CONTEST: header has correct name
8. **Export ADIF**: Verify CONTEST_ID field has correct value

## Examples

See existing contest implementations for reference:

- **src/contests/CQWWContest.cpp** - RST + Zone exchange, per-band multipliers
- **src/contests/CQWPXContest.cpp** - RST + Serial, prefix extraction logic
- **src/contests/WinterFieldDayContest.cpp** - Class + Section, all-band multipliers

## Best Practices

1. **Always define all three identifiers** (WA7BNM ID, Cabrillo name, ADIF Contest-ID)
2. **Use static constexpr for integer constants** (compile-time)
3. **Use static inline const QString for string constants** (C++17 inline variables)
4. **Document exchange format in header comment block**
5. **Validate exchange fields thoroughly** (prevents bad data in log)
6. **Test with real contest data** if possible
7. **Check official contest rules** for scoring edge cases
8. **Use CTY.DAT for all country/prefix lookups** (don't hardcode)

## Troubleshooting

**Build errors with static QString?**
- Use `static inline const QString` instead of just `static const QString`
- C++17 inline variables are required for non-integral static const members

**Contest not appearing in selection menu?**
- Ensure you registered the contest in the contest registry
- Check that CMakeLists.txt includes your .cpp file

**Exchange validation always failing?**
- Print debug output in validateReceivedExchange()
- Check regex patterns are correct (test with regex101.com)
- Verify section/zone lists are loaded

**Multipliers not counting?**
- Check getMultiplierValue() returns non-empty string for new multipliers
- Verify alreadyWorkedValues list is passed correctly
- Test per-band vs all-band logic

## Future Enhancements

- Contest wizard for auto-generating boilerplate
- Exchange field auto-complete based on callsign lookup
- Real-time exchange validation UI feedback
- Contest rule conflict detection (e.g., W/VE special scoring in CQ WW)
