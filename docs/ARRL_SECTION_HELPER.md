# ARRL Section Helper

Comprehensive helper for mapping US state and county to ARRL section abbreviations.

## Overview

The ARRL Section Helper provides accurate mapping from US state and county combinations to official ARRL section abbreviations. This is essential for contest logging, particularly for contests like Winter Field Day and ARRL Field Day where the exchange includes section information.

## Features

- **Complete Coverage**: All 71 ARRL sections (50 US states + DC/VI + subdivided states)
- **Subdivided States**: Accurate county-level mapping for 8 states with multiple sections:
  - Florida (3 sections: NFL, WCF, SFL)
  - California (10 sections: EB, LAX, ORG, SB, SCV, SDG, SF, SJV, SV, PAC)
  - Texas (3 sections: NTX, STX, WTX)
  - New York (4 sections: NLI, NNY, WNY, ENY)
  - New Jersey (2 sections: NNJ, SNJ)
  - Massachusetts (2 sections: EMA, WMA)
  - Pennsylvania (2 sections: EPA, WPA)
  - Washington (2 sections: EWA, WWA)
- **Smart Normalization**: Case-insensitive, whitespace-trimmed, handles common variations
- **Zero Dependencies**: Only requires Qt Core

## Usage

### Basic Usage

```cpp
#include "utils/ArrlSectionHelper.h"

using namespace TR4QT::Arrl;

// Simple 1:1 states
QString section = sectionForStateCounty("UT", "Salt Lake");
// Returns: "UT"

section = sectionForStateCounty("CO", "Denver");
// Returns: "CO"

// Subdivided states
section = sectionForStateCounty("FL", "Pinellas");
// Returns: "WCF" (West Central Florida)

section = sectionForStateCounty("CA", "Los Angeles");
// Returns: "LAX"

section = sectionForStateCounty("TX", "Harris");
// Returns: "STX" (South Texas)

section = sectionForStateCounty("NY", "Queens");
// Returns: "NLI" (New York City-Long Island)
```

### Integration with QRZ Lookup

```cpp
// Example: Get section from QRZ data
QString state = "FL";           // From QRZ
QString county = "Pinellas";    // From QRZ

QString section = sectionForStateCounty(state, county);
if (!section.isEmpty()) {
    qDebug() << "ARRL Section:" << section;  // "WCF"
}
```

### Handling User Input

```cpp
// The helper normalizes input automatically:

// Case variations - all return "WCF"
sectionForStateCounty("fl", "pinellas");
sectionForStateCounty("FL", "PINELLAS");
sectionForStateCounty("Fl", "Pinellas");

// County suffix variations - all return "WCF"
sectionForStateCounty("FL", "Pinellas County");
sectionForStateCounty("FL", "Pinellas county");
sectionForStateCounty("FL", "Pinellas");

// St./Saint normalization - all return "NFL"
sectionForStateCounty("FL", "St. Johns");
sectionForStateCounty("FL", "Saint Johns");
sectionForStateCounty("FL", "St Johns");

// Whitespace handling
sectionForStateCounty("  FL  ", "  Pinellas  ");  // Returns "WCF"
```

### Error Handling

```cpp
// Unknown state returns empty QString
QString section = sectionForStateCounty("ZZ", "Unknown");
if (section.isEmpty()) {
    qDebug() << "Unknown state";
}

// Known state but unknown county in subdivided state
section = sectionForStateCounty("FL", "NonexistentCounty");
if (section.isEmpty()) {
    qDebug() << "County not found in this state";
}

// Empty inputs
section = sectionForStateCounty("", "");
// Returns: empty QString
```

### Simple State Lookup

```cpp
// For states with no county subdivision, you can use the convenience function:
QString section = sectionForState("UT");
// Returns: "UT"

section = sectionForState("FL");
// Returns: empty QString (Florida is subdivided by county)
```

## ARRL Sections Reference

### Single-Section States (43 states + DC/VI)

State | Section | State | Section | State | Section
------|---------|-------|---------|-------|--------
AK | AK | AL | AL | AR | AR
AZ | AZ | CO | CO | CT | CT
DE | DE | GA | GA | HI | HI
IA | IA | ID | ID | IL | IL
IN | IN | KS | KS | KY | KY
LA | LA | MDC | MD/DC | ME | ME
MI | MI | MN | MN | MO | MO
MS | MS | MT | MT | NC | NC
ND | ND | NE | NE | NH | NH
NM | NM | NV | NV | OH | OH
OK | OK | OR | OR | RI | RI
SC | SC | SD | SD | TN | TN
UT | UT | VA | VA | VI | VI
VT | VT | WI | WI | WV | WV
WY | WY

### Multi-Section States (8 states, 28 sections)

#### Florida (3 sections)
- **NFL** (Northern Florida): 36 counties including Duval, Escambia, Leon
- **WCF** (West Central Florida): 17 counties including Pinellas, Hillsborough, Orange
- **SFL** (South Florida): 18 counties including Broward, Miami-Dade, Palm Beach

#### California (10 sections)
- **EB** (East Bay): Alameda, Contra Costa
- **LAX** (Los Angeles): Los Angeles
- **ORG** (Orange): Orange
- **SB** (Santa Barbara): San Luis Obispo, Santa Barbara, Ventura
- **SCV** (Santa Clara Valley): Santa Clara
- **SDG** (San Diego): Imperial, San Diego
- **SF** (San Francisco): Marin, San Francisco, San Mateo
- **SJV** (San Joaquin Valley): 9 counties including Fresno, Kern, Tulare
- **SV** (Sacramento Valley): 36 counties including Sacramento, Placer, Riverside
- **PAC** (Pacific): Santa Cruz

#### Texas (3 sections)
- **NTX** (North Texas): 35 counties including Dallas, Tarrant, Collin
- **STX** (South Texas): 160+ counties including Harris, Travis, Bexar
- **WTX** (West Texas): 58 counties including El Paso, Midland, Lubbock

#### New York (4 sections)
- **NLI** (New York City-Long Island): 7 counties (5 NYC boroughs + Nassau, Suffolk)
- **NNY** (Northern New York): 11 counties including Jefferson, St. Lawrence
- **WNY** (Western New York): 15 counties including Erie, Monroe, Niagara
- **ENY** (Eastern New York): 29 counties including Albany, Westchester, Orange

#### New Jersey (2 sections)
- **NNJ** (Northern New Jersey): 11 counties including Bergen, Essex, Morris
- **SNJ** (Southern New Jersey): 10 counties including Atlantic, Camden, Ocean

#### Massachusetts (2 sections)
- **EMA** (Eastern Massachusetts): 9 counties including Suffolk, Middlesex, Plymouth
- **WMA** (Western Massachusetts): 5 counties including Berkshire, Hampshire, Worcester

#### Pennsylvania (2 sections)
- **EPA** (Eastern Pennsylvania): 17 counties including Philadelphia, Bucks, Lehigh
- **WPA** (Western Pennsylvania): 50 counties including Allegheny, Erie, Lancaster

#### Washington (2 sections)
- **EWA** (Eastern Washington): 20 counties east of Cascades including Spokane, Yakima
- **WWA** (Western Washington): 19 counties west of Cascades including King, Pierce

## Implementation Details

### Data Sources

The county→section mappings are derived from two authoritative sources:

1. **ARRL Section Boundaries**: http://www.arrl.org/section-boundaries
2. **Ham::Reference::QRZ Perl Module**: https://metacpan.org/pod/Ham::Reference::QRZ

### Normalization

The helper performs the following normalizations:

1. **Case**: All inputs converted to uppercase for comparison
2. **Whitespace**: Leading/trailing whitespace trimmed
3. **County suffix**: "County" word removed (e.g., "Pinellas County" → "Pinellas")
4. **Abbreviations**: "St." and "Saint" normalized to "ST" (e.g., "St. Johns" → "ST JOHNS")
5. **Punctuation**: Hyphens and spaces in multi-word counties handled correctly

### Lookup Algorithm

1. Normalize state abbreviation and county name
2. Check if state is subdivided (FL, CA, TX, NY, NJ, MA, PA, WA)
3. If subdivided, perform county lookup in state-specific table
4. If not subdivided or county not found, fall back to simple state→section table
5. Return section abbreviation or empty QString if not found

## Testing

Comprehensive test coverage includes:

- All 8 subdivided states with representative counties from each section
- Simple 1:1 state mappings
- Case insensitivity (uppercase, lowercase, mixed)
- County normalization ("County" suffix, "St."/"Saint", whitespace)
- Edge cases (unknown states, unknown counties, empty inputs)

Run tests:
```bash
cd build
ctest -R test_arrlsection
```

## Maintenance

### Updating Section Mappings

If ARRL changes section boundaries:

1. Update the appropriate county hash table in `ArrlSectionHelper.cpp`
2. Verify against official ARRL section boundaries page
3. Add test cases for changed mappings
4. Update version comment at top of file

### Adding New Subdivided States

If ARRL creates new subdivisions:

1. Create a new `QHash<QString, QString>` for the state's counties
2. Add county→section mappings
3. Add state check in `sectionForStateCounty()` function
4. Add comprehensive test cases

## Examples

See `tests/test_arrlsection.cpp` for comprehensive examples covering:
- All subdivided states
- Normalization variations
- Edge cases
- Integration patterns

## License

Part of TR4QT - Qt-based ham radio contest logging software.
