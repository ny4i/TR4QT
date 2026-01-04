# TR4QT Version Requirements

This document specifies the required versions of dependencies for TR4QT development and CI builds.

**CRITICAL**: These versions MUST be kept in sync across:
- Local development environment
- GitHub Actions CI (`.github/workflows/build.yml`)
- Build scripts
- Documentation

## Required Versions

### Qt
- **Version**: `6.10.1`
- **Rationale**:
  - Fixes critical issues with SQL drivers and theme/palette handling present in Qt 6.7.x
  - Required for consistent behavior across Windows and macOS builds
  - Local development uses Qt 6.10.1 - CI must match to ensure identical binaries

### Hamlib
- **Version**: `4.6.5`
- **Rationale**: Latest stable release with K4 support

## Platform-Specific Notes

### Windows
- Qt installed via `jurplel/install-qt-action@v4` in CI
- MinGW 13.1.0 toolchain (from Qt)
- Explicit Qt version controlled by `QT_VERSION` env var in build.yml

### macOS
- Qt installed via `jurplel/install-qt-action@v4` in CI (as of v3.31.11+)
- **Previous**: Used Homebrew `qt@6` which installed Qt 6.7.2 (inconsistent with Windows!)
- **Now**: Uses same install-qt-action as Windows for version parity

### Linux
- Builds currently disabled per user request (2026-01-01)
- If re-enabled: Should use install-qt-action for version consistency

## Version Validation

CI builds include automated version validation:
- Checks Qt version from CMakeCache.txt after configure
- Fails build if version doesn't match expected version
- Prevents version drift between platforms

Local validation script:
```bash
./scripts/check-versions.sh
```

## Updating Versions

When updating dependency versions:

1. Update `env.QT_VERSION` or `env.HAMLIB_VERSION` in `.github/workflows/build.yml`
2. Update this document
3. Test locally with new version BEFORE pushing
4. Verify CI builds succeed on all platforms
5. Document changes in CHANGELOG.md

## Version Compatibility Matrix

| TR4QT Version | Qt Version | Hamlib Version | Notes |
|---------------|------------|----------------|-------|
| 3.31.11+      | 6.10.1     | 4.6.5          | Fixed version parity between Windows/macOS |
| 3.31.0-3.31.10| 6.7.2 (macOS)<br>6.10.1 (Windows) | 4.6.5 | **BROKEN**: Version mismatch caused QSO grid issues |
| < 3.31.0      | 6.7.2      | 4.5.x          | Legacy versions |

## Common Issues

### QSO Grid Not Populating
- **Symptom**: QSO grid empty in log display, radio indicator flashes black instead of red
- **Cause**: Qt version mismatch between CI build and local development
- **Fix**: Ensure CI uses same Qt version as local dev (currently 6.10.1)

### TLS/HTTPS Downloads Fail
- **Symptom**: Country file download fails, "No functional TLS backend was found"
- **Cause**: Missing Qt TLS plugins in deployment
- **Fix**: Explicitly copy TLS plugins (see build.yml macOS bundle step)

## Enforcement

Pre-commit hooks check for:
- Hardcoded hex colors (should use ThemeManager)
- Dangerous Qt patterns (setParent(nullptr))
- **TODO**: Add version requirement validation

CI validation checks:
- Qt version matches `QT_VERSION` env var
- Build fails if mismatch detected
- Runs on every platform build

## Why This Matters

**Real example from v3.31.10:**
- Windows CI: Qt 6.7.2
- Local development: Qt 6.10.1
- Result: CI-built releases had broken QSO grid and theme issues
- Users downloaded broken installers from GitHub Releases
- Wasted hours debugging "works on my machine" issues

**Solution:** Enforce version parity with this document and CI validation.
