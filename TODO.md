# TR4QT TODO List

## High Priority

None currently.

## Medium Priority

### Fix Test Suite Linking Issues
**Added**: 2025-12-25 (v2.79.0)
**Issue**: Test suite (test_countryfile, test_geographicutils) fails to link with DXCCRepository
**Impact**: Main application works fine, but tests can't run
**Root Cause**: Tests don't include DXCCRepository.cpp in their link targets
**Solution**: Update tests/CMakeLists.txt to link DXCCRepository object files for affected tests

**Affected Tests**:
- test_countryfile - Uses CountryFile which depends on DXCCRepository
- test_geographicutils - May use CountryFile indirectly

**Technical Details**:
```
Undefined symbols for architecture arm64:
  "TR4QT::DXCCRepository::DXCCRepository()", referenced from:
      TR4QT::CountryFile::getDXCCEntityCode(QString const&) in CountryFile.cpp.o
  "TR4QT::DXCCRepository::getEntityCode(QString const&) const", referenced from:
      TR4QT::CountryFile::getDXCCEntityCode(QString const&) in CountryFile.cpp.o
```

**Files to Modify**:
- `/Users/toms/projects/TR4QT/tests/CMakeLists.txt`
  - Add DXCCRepository.cpp to test_countryfile target
  - Add DXCCRepository.cpp to test_geographicutils target if needed

## Low Priority

None currently.

## Completed

See git commit history for completed items.
