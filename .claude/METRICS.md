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
