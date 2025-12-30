# CountryFile Thread Safety Analysis

## Critical Issue: Race Condition in CountryFile Reload

**Status**: UNPROTECTED - No locking mechanism exists
**Severity**: HIGH - Can cause crashes or data corruption
**Affected Version**: All current versions

## The Problem

`CountryFile` has no mutex or thread safety protection, but can be reloaded while other operations are in progress.

### Race Condition in `loadFromFile()`

```cpp
// src/utils/CountryFile.cpp:20-22
bool CountryFile::loadFromFile(const QString& filePath) {
    // ...
    m_countries.clear();      // ⚠️ UNSAFE: Clears hash table immediately
    m_prefixMap.clear();      // ⚠️ UNSAFE: Other threads may be reading
    m_exactMatches.clear();   // ⚠️ UNSAFE: No locking
    // ... then repopulates
}
```

### Race Condition in `lookup()`

```cpp
// src/utils/CountryFile.cpp (lookup method)
CountryData CountryFile::lookup(const QString& callsign) const {
    // Reads from m_countries, m_prefixMap, m_exactMatches
    // ⚠️ UNSAFE: No lock, data structures may be cleared mid-read
}
```

## Scenarios Where This Can Fail

### Scenario 1: ADIF Import + Country File Download
1. User opens ADIF import dialog
2. Dialog receives pointer to `MainWindow::m_countryFile`
3. Import starts, calling `countryFile->lookup()` for each record
4. **Meanwhile**: User downloads new cty.dat
5. `MainWindow::m_countryFile.loadFromFile()` called
6. Hash tables cleared while ADIF mapper is reading them
7. **Result**: Crash or incorrect country data

**Code path**:
- `ADIFImportDialog::onImportClicked()` → Line 124
- `ADIFFieldMapper::mapToQSO()` → Line 180 (`m_countryFile->lookup()`)
- **Concurrent**: `MainWindow.cpp:4269` (`m_countryFile.loadFromFile()`)

### Scenario 2: DX Cluster Spot Processing + Country File Reload
1. DX Cluster spots arriving
2. `BandMapWidget` calling `m_countryFile.lookup()` for each spot
3. User downloads new cty.dat
4. `loadFromFile()` clears hash tables mid-lookup
5. **Result**: Crash or spots show wrong country

**Code path**:
- `BandMapWidget::addSpotToMap()` → Line 978 (`m_countryFile.lookup()`)
- **Concurrent**: `MainWindow.cpp:4269` reload

### Scenario 3: QSO Logging + Country File Update
1. User logging QSO, zone prediction in progress
2. `MainWindow::logQSO()` calls `m_countryFile.lookup()`
3. User (or automatic update) downloads new cty.dat
4. Reload happens during lookup
5. **Result**: Wrong zone assigned to QSO

**Code path**:
- `MainWindow::logQSO()` → Line 1838 (`m_countryFile.lookup()`)
- **Concurrent**: `MainWindow.cpp:4269` reload

## Current Reload Trigger Points

Country file can be reloaded from:

1. **Manual download completion** (MainWindow.cpp:4269)
   ```cpp
   if (m_countryFile.loadFromFile(filePath)) {
       m_countryFile.setVersion(version);
   }
   ```

2. **Headless mode auto-reload** (MainWindow.cpp:4285)
   ```cpp
   if (m_countryFile.loadFromFile(filePath)) {
       m_countryFile.setVersion(version);
   }
   ```

3. **Initial startup** (MainWindow.cpp:94)
   ```cpp
   if (!m_countryFile.loadFromFile(countryFilePath)) {
       LOG_WARN("MainWindow", "Failed to load country file");
   }
   ```

## Other Components with CountryFile

Multiple components have their own `CountryFile` instances:

1. **MainWindow** - `m_countryFile` (MainWindow.h:289)
2. **BandMapWidget** - `m_countryFile` (BandMapWidget.h:243)
3. **InitialExchangeManager** - `m_countryFile*` (InitialExchangeManager.h:112)

**Note**: Each component loads its own copy, so they don't interfere with each other. The race condition is within a SINGLE CountryFile instance being accessed concurrently.

## Recommended Solutions

### Solution 1: Add QReadWriteLock (Preferred)

**Benefits**:
- Multiple concurrent readers (lookup operations)
- Exclusive writer (reload operation)
- Minimal performance impact for common case (reads)

**Implementation**:

```cpp
// src/utils/CountryFile.h
#include <QReadWriteLock>

class CountryFile {
public:
    // ... existing methods ...

private:
    // ... existing members ...
    mutable QReadWriteLock m_lock;  // Protects all data structures
};
```

```cpp
// src/utils/CountryFile.cpp

bool CountryFile::loadFromFile(const QString& filePath) {
    QWriteLocker locker(&m_lock);  // Exclusive lock for writing

    // ... existing load code ...
    m_countries.clear();
    m_prefixMap.clear();
    m_exactMatches.clear();
    // ... repopulate ...

    return true;
}

CountryData CountryFile::lookup(const QString& callsign) const {
    QReadLocker locker(&m_lock);  // Shared lock for reading

    // ... existing lookup code ...
}

QVector<CountryData> CountryFile::getAllCountries() const {
    QReadLocker locker(&m_lock);  // Shared lock for reading

    // ... existing code ...
}
```

**Impact**:
- ✅ Thread-safe
- ✅ Minimal performance overhead (readers don't block each other)
- ✅ Simple to implement
- ❌ Reload blocks all lookups until complete

### Solution 2: Copy-on-Write with Atomic Pointer Swap

**Benefits**:
- Zero lock contention for readers
- Readers never block
- More complex but optimal for high-read scenarios

**Implementation**:

```cpp
// More complex - use shared_ptr and atomic swap
// Load into new instance, then atomically swap pointer
// Old instance stays valid until all readers release it
```

**Complexity**: Higher, probably overkill for this use case.

### Solution 3: Disable Reload During Operations

**Benefits**:
- Simple flag-based protection
- No performance impact

**Drawbacks**:
- User must wait for operations to complete
- Requires tracking all in-progress operations

**Not recommended** - Too complex to track all uses.

## Recommended Immediate Action

1. **Add QReadWriteLock to CountryFile** (Solution 1)
   - Protect `loadFromFile()` with QWriteLocker
   - Protect `lookup()` with QReadLocker
   - Protect `getAllCountries()` with QReadLocker

2. **Test race condition**:
   - Start ADIF import with large file
   - Download/reload cty.dat while import is running
   - Verify no crashes or data corruption

3. **Add to CLAUDE.md**:
   - Document that CountryFile is thread-safe
   - Explain when to use locks in similar situations

## Testing Approach

```cpp
// Test concurrent reload + lookup
void testConcurrentReloadAndLookup() {
    CountryFile cf;
    cf.loadFromFile("cty.dat");

    // Thread 1: Continuous lookups
    QFuture<void> lookupThread = QtConcurrent::run([&cf]() {
        for (int i = 0; i < 10000; i++) {
            CountryData data = cf.lookup("W1AW");
            QVERIFY(data.isValid());
        }
    });

    // Thread 2: Reload country file
    QFuture<void> reloadThread = QtConcurrent::run([&cf]() {
        for (int i = 0; i < 100; i++) {
            cf.loadFromFile("cty.dat");
        }
    });

    lookupThread.waitForFinished();
    reloadThread.waitForFinished();
}
```

## Additional Notes

- Qt containers (QHash, QVector) are **not thread-safe** for concurrent read/write
- Qt containers **are safe** for concurrent reads (but not during write)
- The `const` qualifier on `lookup()` does NOT provide thread safety
- Need explicit synchronization primitives (mutex/lock)

## References

- Qt Thread Safety: https://doc.qt.io/qt-6/threads-reentrancy.html
- QReadWriteLock: https://doc.qt.io/qt-6/qreadwritelock.html
- Qt Container Thread Safety: https://doc.qt.io/qt-6/threads-modules.html#container-classes
