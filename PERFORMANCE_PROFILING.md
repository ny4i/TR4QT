# K4 Direct Performance Profiling

## Summary

This document describes the performance profiling instrumentation added to compare K4 Direct vs Hamlib radio interfaces.

## Critical Bug Fix: Recursive Mutex Deadlock

**Bug**: Worker thread froze after processing exactly 4 onReadyRead events
**Root Cause**: `K4Radio::processMessage()` locks `m_stateMutex`, then calls `parseIFCommand()` which tried to lock the SAME mutex again. QMutex is non-recursive by default, causing deadlock.
**Fix**: Removed mutex lock from `parseIFCommand()` since caller already holds it
**Result**: Worker thread now processes events continuously (1000+ onReadyRead calls)

## Performance Profiling System

### Architecture

**PerformanceProfiler** - Singleton class that collects timing statistics
- Thread-safe (QMutex protected)
- Tracks min/max/avg/count for each function
- Separate stats for "K4Direct" vs "Hamlib" radio types

**ScopedTimer** - RAII timer for automatic function profiling
- Starts timer on construction
- Records timing on destruction
- Zero overhead when profiling is disabled

**PROFILE_FUNCTION(radioType)** - Convenience macro
- Automatically times the entire function scope
- Example: `PROFILE_FUNCTION("K4Direct");`

### Instrumented Functions

Both `K4Radio` and `HamlibRadio` have profiling in:
- `setFrequency()` - Change radio frequency
- `setMode()` - Change operating mode (CW/SSB/etc)
- `setPTT()` - Toggle transmit
- `sendCW()` - Send Morse code

### Usage

1. **Run with both radio types** - Connect using K4 Direct, collect data, then switch to Hamlib
2. **Perform operations** - Click band buttons, change modes, etc.
3. **View report** - Help menu → "Show Performance Report..."

### Report Format

```
================================================================================
                    RADIO PERFORMANCE COMPARISON REPORT
================================================================================

Function: setFrequency
--------------------------------------------------------------------------------
  K4 Direct:      45 calls, avg:     1.23 ms, min:      0 ms, max:      5 ms
  Hamlib:         42 calls, avg:    12.45 ms, min:      8 ms, max:     18 ms
  Speedup:    10.12x faster with K4 Direct

Function: setMode
--------------------------------------------------------------------------------
  K4 Direct:      12 calls, avg:     0.89 ms, min:      0 ms, max:      2 ms
  Hamlib:         10 calls, avg:    11.23 ms, min:      9 ms, max:     15 ms
  Speedup:    12.62x faster with K4 Direct
```

## Implementation Files

- `src/utils/PerformanceProfiler.h` - Header with profiler class and RAII timer
- `src/utils/PerformanceProfiler.cpp` - Implementation with report generation
- `src/radio/K4Radio.cpp` - Instrumented with `PROFILE_FUNCTION("K4Direct")`
- `src/radio/HamlibRadio.cpp` - Instrumented with `PROFILE_FUNCTION("Hamlib")`
- `src/ui/MainWindow.cpp` - Added "Show Performance Report..." menu item

## Expected Results

K4 Direct should show significant speedup over Hamlib because:
1. **No polling** - K4 pushes status updates (AI5 mode) vs Hamlib polling every 200ms
2. **Direct TCP** - Raw socket communication vs Hamlib library overhead
3. **Async I/O** - Non-blocking socket operations vs synchronous Hamlib calls

## Testing Notes

The profiler captures **wall-clock time** (QElapsedTimer), which includes:
- Actual radio command execution time
- Network latency (for K4 Direct TCP)
- Serial port latency (for Hamlib USB/serial)
- Queue processing delays

For meaningful comparisons:
- Test with same radio hardware
- Use same connection (e.g., both via TCP or both via USB)
- Perform same operations in similar sequence

## Future Enhancements

- Add profiling for more functions (getFrequency, getMode, etc.)
- Add percentile statistics (p50, p95, p99)
- Export report to CSV for analysis
- Graph performance over time
- Track memory usage
