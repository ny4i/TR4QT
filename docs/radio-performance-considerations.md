# Radio Performance Considerations

## Context

TR4QT is a **contest logging program**, not a full radio control application like wfview. The radio interface provides essential contest functionality (frequency, mode, band changes) but does not aim to be a "single pane of glass" radio emulation.

## Current Performance (v3.38.38)

After implementing transceive fixes:
- Frequency updates are "much faster" than before
- Display updates are "probably good enough" for contest use
- Slightly slower than wfview but acceptable for logging operations

## Architectural Differences: TR4QT vs wfview

### **wfview Architecture** (Full Radio Control)
```cpp
// Generic message queue with QVariant (copy-on-write)
struct cacheItem {
    funcs command;      // Enum for all radio functions
    QVariant value;     // Generic value container
    uchar receiver;     // VFO/receiver
};

// Single signal for all updates
connect(queue, SIGNAL(sendValue(cacheItem)), this, SLOT(receiveValue(cacheItem)));

// Centralized dispatch (big switch statement)
void wfmain::receiveValue(cacheItem val) {
    switch (val.command) {
        case funcFreq: ... break;
        case funcMode: ... break;
        // ... hundreds of cases
    }
}
```

**Advantages:**
- QVariant uses copy-on-write (very cheap to pass by value)
- Queue can batch/coalesce rapid updates
- Single signal path reduces Qt meta-object overhead
- Optimized for high-throughput radio control

**Disadvantages:**
- Type-unsafe (runtime errors if wrong type)
- Giant switch statement hard to maintain
- Less clear API (what fields exist?)

---

### **TR4QT Architecture** (Contest Logging Focus)

```cpp
// Typed signals per field
emit frequencyChanged(freq_t freq, VFO vfo);      // ~8 bytes copied
emit modeChanged(ModeType mode, VFO vfo);         // ~2 bytes copied
emit stateUpdated(RadioState state);              // ~200 bytes copied

// Multiple connections
connect(radio, &RadioController::frequencyChanged, ...);
connect(radio, &RadioController::modeChanged, ...);
connect(radio, &RadioController::stateUpdated, ...);
```

**Advantages:**
- Type-safe (compile-time checking)
- Clear API (explicit signal names)
- Easy to understand and maintain
- Good enough for contest logging needs

**Disadvantages:**
- Each signal goes through Qt event system separately
- More meta-object overhead per update
- No automatic batching/coalescing
- Slightly slower raw throughput than wfview

---

## Performance Bottlenecks Fixed (v3.38.38)

### **1. Not Accepting Broadcast Address**
**Problem:** CI-V transceive updates are sent to broadcast address `0x00`, but we only accepted controller address `0xE0/0xE1`.

**Fix:** Accept both controller and broadcast addresses.

**Impact:** This was the PRIMARY bottleneck - fixed slow updates entirely.

---

### **2. Disrupting Transceive Mode**
**Problem:** Sending transceive ON command (`0x1A 0x05 01 31 01`) when transceive was already ON in radio menu disrupted the radio's transceive mechanism.

**Fix:** Removed the command entirely - rely on radio's menu setting.

**Impact:** Significant - prevents disruption of radio's existing transceive stream.

---

### **3. VFO B Overwriting VFO A**
**Problem:** RadioManager forwarded ALL frequency changes (including VFO B) to main window, causing VFO A display to flicker/update incorrectly.

**Fix:** Filter VFO B updates in `RadioManager::onFrequencyChanged()`.

**Impact:** Eliminates display flickering and confusion.

---

### **4. Display Precision**
**Problem:** 5 decimal places (10 Hz resolution) meant small frequency changes (1-9 Hz) were invisible, making updates feel slower.

**Fix:** Increased Radio Control window to 6 decimals (1 Hz resolution). Kept radio grid at 3 decimals (compact display).

**Impact:** Every knob turn visible, improves perceived responsiveness.

---

## Potential Future Optimizations

### **Option A: Throttle Updates to Display Refresh Rate** ⭐ SIMPLE
**Rationale:** Display refresh is typically 60 Hz (16ms). Processing updates faster than this wastes CPU cycles.

**Implementation:**
```cpp
void RadioManager::onFrequencyChanged(freq_t freq, VFO vfo) {
    if (vfo != VFO::VFO_A) return;

    // Throttle to display refresh rate (60 Hz = 16ms)
    static QElapsedTimer throttle;
    if (throttle.isValid() && throttle.elapsed() < 16) {
        return;  // Skip update if less than 16ms since last
    }
    throttle.start();

    // ... rest of code
}
```

**Pros:**
- 5-line change
- Easy to back out if no improvement
- Reduces CPU usage on rapid updates

**Cons:**
- Adds 16ms latency (imperceptible at human timescales)
- May not provide noticeable speedup if radio already sends at ~60 Hz

**Status:** Implemented for testing (v3.38.39)

---

### **Option B: Custom Painted Frequency Display** ❌ COMPLEX
**Rationale:** QLabel's `setText()` triggers full text layout and repaint. Custom widget could do incremental updates.

**Implementation:**
- Create custom `FrequencyDisplayWidget` similar to wfview's `freqctrl`
- Paint digits directly to framebuffer
- Only repaint changed digits

**Pros:**
- Potentially faster rendering
- More control over appearance

**Cons:**
- High complexity (hundreds of lines)
- Not worth it for contest logging use case
- QLabel performance is already good enough

**Status:** NOT RECOMMENDED - TR4QT is not a radio control program

---

### **Option C: Message Queue Architecture** ❌ MAJOR REFACTOR
**Rationale:** Adopt wfview's generic message queue with QVariant for lower overhead.

**Implementation:**
- Replace all typed signals with generic `RadioMessage` signal
- Implement centralized dispatch in MainWindow
- Add queue for batching/coalescing

**Pros:**
- Matches wfview's throughput
- Lower per-message overhead

**Cons:**
- Major refactoring (days of work)
- Loses type safety
- Adds complexity
- Not justified for contest logging needs

**Status:** NOT RECOMMENDED - current architecture is appropriate for TR4QT's purpose

---

### **Option D: Shared Pointer to RadioState** ⚠️ MODERATE COMPLEXITY
**Rationale:** Avoid copying entire RadioState struct (~200 bytes) on every update.

**Implementation:**
```cpp
using RadioStatePtr = QSharedPointer<RadioState>;

// Emit shared pointer instead of value
emit stateUpdated(RadioStatePtr state);

// Receivers hold shared pointer (no copy)
void onRadioStateUpdated(RadioStatePtr state) {
    m_currentState = *state;  // Only copy when needed
}
```

**Pros:**
- Reduces copying overhead
- Minimal code changes

**Cons:**
- Adds shared pointer overhead
- Thread safety considerations (immutability required)
- Not a bottleneck (stateUpdated only emitted every 5 seconds, not on transceive)

**Status:** NOT RECOMMENDED - frequency/mode signals already optimized (8 bytes copied)

---

## Recommendations

### **For Contest Logging (Current Use Case)**
✅ **Keep current architecture** - it's clean, maintainable, and fast enough

✅ **Try Option A (throttling)** - simple optimization, easy to back out

❌ **Don't do Options B/C/D** - too complex for diminishing returns

### **If TR4QT Becomes Full Radio Control**
If TR4QT evolves into a general-purpose radio control application:
- Consider Option C (message queue) for feature parity with wfview
- Custom widgets (Option B) for polish
- But only if user demand justifies the complexity

---

## Measurement Baseline (v3.38.38)

From timing logs:
- **Radio thread parsing**: 5-56 μs (excellent)
- **RadioManager forwarding**: 37-201 μs (good)
- **MainWindow UI update**: 1-154 μs (good)

**Total latency**: ~200-400 μs from radio packet to display update

This is **more than fast enough** for human perception (humans perceive ~100ms delays).

---

## Philosophy: Simplicity Over Performance

TR4QT is a **contest logging program**, not a radio emulator:
- Prefer simple, maintainable code over micro-optimizations
- Optimize only when users complain about slowness
- Current performance is "probably good enough" for contest use

**When to optimize:**
- If users report delays affecting contesting operations
- If profiling shows clear bottleneck
- If optimization is simple (like Option A)

**When NOT to optimize:**
- Chasing wfview's speed for speed's sake
- Complex refactoring with marginal gains
- Optimizing already-fast code paths

---

## Related Files

- `src/radio/IcomRadio.cpp` - CI-V transceive handling
- `src/controllers/RadioManager.cpp` - Signal forwarding and VFO filtering
- `src/ui/widgets/RadioControlWidget.cpp` - VFO display (6 decimals)
- `src/ui/MainWindow.cpp` - Radio grid display (3 decimals)

---

## Version History

- **v3.38.38** (2026-01-19) - Fixed transceive bottlenecks, polymorphic Icom architecture
- **v3.38.39** (pending) - Added throttling optimization (Option A)
