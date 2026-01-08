# CW Timing Algorithm

## Overview

TR4QT v3.31.23+ uses an accurate character-weighted algorithm to calculate CW transmission duration, based on standard Morse code timing units.

## Standard Morse Timing

### Basic Units
- **Dit**: 1 unit
- **Dah**: 3 units
- **Space between elements** (within a character): 1 unit
- **Space between characters**: 3 units
- **Space between words**: 7 units

### Reference Standard: "PARIS"

The word "PARIS" = 50 timing units, which defines the WPM standard:

- **P** (·--·): 1+1+3+1+3+1+1 = **11 units**
- **A** (·-): 1+1+3 = **5 units**
- **R** (·-·): 1+1+3+1+1 = **9 units**
- **I** (··): 1+1+1 = **3 units**
- **S** (···): 1+1+1+1+1 = **5 units**
- **Inter-character spacing**: 3×4 = **12 units**
- **Word spacing**: **5 units** (7 total - 2 already counted)

**Total**: 11+5+9+3+5+12+5 = **50 units**

At **1 WPM**: "PARIS" takes 60 seconds = 60,000ms
Therefore: **1 unit = 1200ms** at 1 WPM

At **N WPM**: **1 unit = 1200/N ms**

## Algorithm Implementation

### Character Lookup Table

Each character has a pre-calculated unit count including internal spacing:

```cpp
// Examples:
'E' = 1 unit       // .
'T' = 3 units      // -
'A' = 5 units      // .- (1+1+3)
'S' = 5 units      // ... (1+1+1+1+1)
'O' = 13 units     // --- (3+1+3+1+3)
'0' = 19 units     // ----- (3+1+3+1+3+1+3+1+3)
```

See `CWTiming.cpp` for the complete lookup table (A-Z, 0-9, punctuation).

### Duration Calculation

```cpp
int CWTiming::calculateDuration(const QString& text, int wpm) {
    const double msPerUnit = 1200.0 / wpm;
    int totalUnits = 0;

    for (each character) {
        totalUnits += characterUnits;
        if (not first character) {
            totalUnits += 3;  // Inter-character space
        }
    }

    for (each space) {
        totalUnits += 4;  // Word space (7 total - 3 already counted)
    }

    return totalUnits * msPerUnit;
}
```

## Example Calculations

### "CQ" at 20 WPM

1. **C** (–·–·): 3+1+1+1+3+1+1+1+3 = **15 units**
2. **Inter-char space**: **3 units**
3. **Q** (––·–): 3+1+3+1+1+1+3 = **13 units**

**Total**: 15+3+13 = **31 units**
**Duration**: 31 × (1200/20) = 31 × 60 = **1860ms**

### "TEST" at 30 WPM

1. **T** (–): **3 units**
2. **Inter-char**: **3 units**
3. **E** (·): **1 unit**
4. **Inter-char**: **3 units**
5. **S** (···): **5 units**
6. **Inter-char**: **3 units**
7. **T** (–): **3 units**

**Total**: 3+3+1+3+5+3+3 = **21 units**
**Duration**: 21 × (1200/30) = 21 × 40 = **840ms**

### "CQ W1AW" at 25 WPM

```
C (15) + space (3) + Q (13) + word_gap (4) +
W (9) + space (3) + 1 (17) + space (3) +
A (5) + space (3) + W (9) = 84 units
```

**Duration**: 84 × (1200/25) = 84 × 48 = **4032ms** (4.0 seconds)

## Comparison: Old vs New Algorithm

### Old Algorithm (v3.31.22 and earlier)
```cpp
// Simple character-count estimation
// Assumes 5 chars/word, adds 20% buffer
int estimatedMs = (text.length() * 1200) / (cwSpeed * 5) * 1.2;
```

**Problems:**
- Treats all characters equally (E = O = 1)
- Doesn't account for actual Morse patterns
- Inaccurate for character-heavy texts

**Example**: "TEST" at 30 WPM
- Old: `(4 * 1200) / (30 * 5) * 1.2 = 38.4ms` ❌ **WAY OFF!**
- Actual: **840ms**

### New Algorithm (v3.31.23+)
```cpp
// Character-weighted with accurate Morse timing
int accurateMs = CWTiming::calculateDuration(text, cwSpeed);
int estimatedMs = accurateMs * 1.1;  // Add 10% buffer
```

**Benefits:**
- Uses actual dit/dah patterns
- Accounts for character and word spacing
- Accurate to within ~5% (radio timing variations)

**Example**: "TEST" at 30 WPM
- New: **840ms** (accurate)
- With 10% buffer: **924ms** ✅

## Dual Completion Detection

TR4QT uses **two methods** to detect when CW transmission completes:

### 1. Radio Feedback (Primary)
In AI5 mode, the K4 radio sends an **RX command** when it stops transmitting. This is the most accurate signal:
- Radio sends "RX;" when keying stops
- K4Radio immediately stops timer and clears `m_cwInProgress`
- Logs: "CW transmission completed (radio confirmed with RX)"

### 2. Timer Fallback (Safety)
If the RX command is missed or delayed, the timer expires after the estimated duration:
- Timer set to calculated duration + 10% buffer
- Ensures function doesn't hang waiting for RX that never arrives
- Logs: "CW transmission estimated complete (timer expired, RX not received yet)"

**Why both?**
- **Accuracy**: Radio feedback is exact, not estimated
- **Responsiveness**: Completes immediately when radio says "RX"
- **Robustness**: Timer ensures completion even if RX is lost
- **Network reliability**: Handles packet loss gracefully

## Buffer Adjustment

The algorithm adds a **10% buffer** to account for:
- Radio internal processing delays (5-10ms)
- Keying relay time (1-5ms)
- Network latency variations (±5ms)
- Farnsworth spacing (if enabled on radio)

Since the RX command provides actual completion feedback, the buffer is primarily a safety margin for the timer fallback.

## Testing

To verify timing accuracy:

1. Send a known phrase (e.g., "PARIS" or "CQ TEST")
2. Check debug logs for calculated duration
3. Compare with stopwatch measurement
4. Expected accuracy: ±50ms for typical contest exchanges

## References

- **ARRL Operating Manual** - Morse Code Timing Standards
- **PARIS Standard**: 50 units = 1 minute at 1 WPM
- **ITU-R M.1677**: International Morse code specification
- **Farnsworth Timing**: Character speed vs word speed (not yet implemented)

## Future Enhancements

Possible improvements:
1. **Farnsworth spacing**: Support character speed ≠ word speed
2. **Weighting**: Adjust for extra-long dahs (some ops send 3.5:1 ratio)
3. **Prosigns**: Add support for SK, AR, BT sent as single characters
4. **Measurement**: Learn actual radio timing via TX monitoring
