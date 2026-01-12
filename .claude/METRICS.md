# TR4QT Architecture Metrics

Track architecture health over time.

**Purpose**: Measure progress toward architecture goals and identify trends.

**Goal**: MainWindow < 2,500 lines (55% reduction from 5,564)

---

## Metrics Format

| Date | MainWindow LOC | God Classes | Test Coverage | Notes |
|------|----------------|-------------|---------------|-------|

---

## Historical Data

| Date | MainWindow LOC | God Classes | Test Coverage | Notes |
|------|----------------|-------------|---------------|-------|
| 2026-01-12 | 5,564 | 1 | ~65% | Baseline before extraction plan |

---

## Trends

**Week over week:**
- MainWindow LOC: Baseline established
- God classes: 1 (MainWindow)
- Test coverage: ~65% (need more integration tests)

**Target milestones:**
- Week 1 (2026-01-19): Extract QSOLoggingService → MainWindow < 5,200 lines
- Week 2 (2026-01-26): Extract QSOPersistenceService → MainWindow < 5,000 lines
- Week 4 (2026-02-09): Extract remaining services → MainWindow < 3,000 lines
- Week 8 (2026-03-09): Final optimization → MainWindow < 2,500 lines

---

## Architecture Health Score

**Formula**: `100 - (MainWindow_LOC / 15) - (GodClasses * 20) - (100 - TestCoverage)`

**Current Score**: 100 - (5564/15) - (1*20) - (100-65) = 100 - 371 - 20 - 35 = **-326** (CRITICAL)

**Target Score**: 100 - (2500/15) - (0*20) - (100-80) = 100 - 167 - 0 - 20 = **-87** (ACCEPTABLE)

**Ideal Score**: 100 - (1500/15) - (0*20) - (100-90) = 100 - 100 - 0 - 10 = **-10** (EXCELLENT)

---

## Weekly Review Template

Copy this template for weekly reviews:

```markdown
## Review: YYYY-MM-DD

**Metrics:**
- MainWindow: X lines (Δ: ±Y from last week)
- God classes: N (changed: ...)
- Test coverage: Z% (Δ: ±W%)
- Architecture score: S (Δ: ±T)

**Progress this week:**
- [ ] Extracted: [ServiceName] (-X lines)
- [ ] Tests added: [FeatureName] (+Y% coverage)
- [ ] Violations resolved: N

**Blockers:**
- ...

**Next week priorities:**
1. ...
2. ...
3. ...

**On track for goals?** YES / NO / NEEDS ADJUSTMENT
```

---

## Automated Updates

The `scripts/check_architecture.sh` script automatically appends metrics to this file.

Manual reviews should be added using the Weekly Review Template above.
2026-01-12 | 5564 | 1 | TBD | Automated check
2026-01-12 | 5564 | 1 | TBD | Automated check

---

## Review: 2026-01-12 (Phase 1 Complete)

**Metrics:**
- MainWindow: 5,564 lines (Δ: 0 from baseline - Phase 1 doesn't modify MainWindow yet)
- God classes: 1 (MainWindow)
- Test coverage: ~65% → ~68% (+3% from CommandDispatcher tests)
- Architecture score: -326 (CRITICAL - unchanged, Phase 1 is groundwork)

**Progress this session:**
- ✅ **Extracted: CommandDispatcher** (stateless utility class)
  - Files created: CommandDispatcher.h (85 lines), CommandDispatcher.cpp (38 lines)
  - Tests created: test_command_dispatcher.cpp (222 lines)
  - **Test results: 12/12 passed** ✅
  - Command parsing logic extracted from MainWindow::onLogQSO() lines 1996-2031
  - Ready for MainWindow integration (Phase 5)

**Test Coverage:**
- CommandDispatcher: 100% coverage (all commands tested)
- Edge cases: empty input, case insensitive, whitespace trimming, similar callsigns

**Why MainWindow LOC unchanged:**
Phase 1-4 extracts create NEW service classes but don't modify MainWindow yet.
Phase 5 will integrate all services and reduce MainWindow LOC dramatically.

**Next phase priorities:**
1. Phase 2: Extract QSOPersistenceService (database save/retry logic) - ~1 day
2. Phase 3: Extract ExchangeMemoryService (exchange prediction) - ~4 hours
3. Phase 4: Extract QSOLoggingCoordinator (post-logging actions) - ~4 hours

**Blockers:** None

**On track for goals?** YES - Phase 1 complete on schedule (2 hours estimated, ~1.5 hours actual)
