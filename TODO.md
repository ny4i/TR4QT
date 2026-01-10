# TR4QT TODO List

## High Priority

None currently.

## Medium Priority

### Implement Secure Credential Storage
**Added**: 2026-01-10 (v3.34.3)
**Priority**: Medium (Security improvement)

**Problem**:
Passwords are currently stored in plain text in QSettings (RadioConfig, etc.). This is a security risk if the settings file is compromised.

**Solution**:
Implement a `CredentialStore` class using QtKeychain library, similar to QLog's implementation in `../qlog/core/CredentialStore.cpp`.

**Implementation Details**:
- Use QtKeychain library (qt6keychain) for OS-native secure storage:
  - macOS: Keychain
  - Windows: Credential Manager
  - Linux: Secret Service API / KWallet / Gnome Keyring
- API:
  ```cpp
  int savePassword(const QString &storage_key, const QString &user, const QString &pass);
  QString getPassword(const QString &storage_key, const QString &user);
  int deletePassword(const QString &storage_key, const QString &user);
  ```
- Storage keys to migrate:
  - `RadioConfig::password` (Icom Direct password)
  - Future: Any other passwords added to the system

**Dependencies**:
- Add `qtkeychain` to CMakeLists.txt (available via Homebrew on macOS, vcpkg on Windows)
- Update GitHub Actions CI to install qtkeychain

**Migration Strategy**:
1. On first run with CredentialStore, migrate existing plain-text passwords to secure storage
2. Clear plain-text passwords from QSettings after successful migration
3. Add migration flag to prevent re-migration

**Reference Implementation**:
- QLog: `/Users/toms/projects/qlog/core/CredentialStore.{h,cpp}`
- QtKeychain docs: https://github.com/frankosterfeld/qtkeychain

**Affected Files**:
- New: `/src/utils/CredentialStore.{h,cpp}`
- Update: `/src/config/Settings.cpp` (RadioConfig load/save)
- Update: `/CMakeLists.txt` (add qtkeychain dependency)
- Update: `/.github/workflows/build.yml` (CI dependencies)

## Low Priority

None currently.

## Completed

### Fix Test Suite Linking Issues
**Added**: 2025-12-25 (v2.79.0)
**Completed**: 2025-12-29
**Status**: Already fixed - DXCCRepository.cpp was properly included in both test_countryfile and test_geographicutils targets

**Verification**:
- test_countryfile: 39 tests passed ✅
- test_geographicutils: 23 tests passed, 2 skipped ✅

See git commit history for other completed items.
