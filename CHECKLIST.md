# TR4QT Development Checklist

Quick reference for common operations. Use this to prompt Claude before key steps.

## Before Committing to GitHub

**Prompt Claude:** *"Ready to commit - run through the commit checklist"*

- [ ] Version updated in **all 4 places** (see CLAUDE.md):
  - [ ] `src/core/Constants.h` - APP_VERSION
  - [ ] `installer/tr4qt.nsi` - APPVERSION
  - [ ] `src/CMakeLists.txt` - MACOSX_BUNDLE_*_VERSION (2 lines)
  - [ ] `resources/tr4qt.rc` - VER_FILEVERSION/VER_PRODUCTVERSION (4 defines)
- [ ] Code compiles without errors
- [ ] Relevant tests pass (if applicable)
- [ ] Version in commit message matches code version
- [ ] No debug/temporary code left in files

**Verification command:**
```bash
grep APP_VERSION src/core/Constants.h
grep APPVERSION installer/tr4qt.nsi | head -1
grep BUNDLE_VERSION src/CMakeLists.txt
grep "VER_.*VERSION" resources/tr4qt.rc | head -4
```

---

## Before Creating a Release Tag

**Prompt Claude:** *"Ready to tag release - run through pre-release checklist"*

- [ ] All commit checklist items above completed
- [ ] Last CI build passed (check with `gh run list --limit 3`)
- [ ] All tests pass locally (`cd build && ctest --output-on-failure`)
- [ ] No uncommitted changes (`git status` clean)
- [ ] Qt modules in CMakeLists.txt match Windows CI configuration
- [ ] Version number matches intended release

**Then:**
```bash
git tag -a vX.Y.Z -m "Release vX.Y.Z - Brief description"
git push origin vX.Y.Z
gh run watch
```

---

## Before Building Locally

**Prompt Claude:** *"Build the project"* (Claude should do this automatically)

- [ ] Kill running instances first: `pkill -9 tr4qt`
- [ ] Clean build if major changes: `rm -rf build && cmake -B build`
- [ ] Standard build: `cmake --build build --target tr4qt -j4`

---

## When Adding New Qt Modules

**Prompt Claude:** *"Added Qt module - update CI config"*

- [ ] Module added to `CMakeLists.txt` (find_package and target_link_libraries)
- [ ] Module added to `.github/workflows/build.yml` Windows CI (modules: line)
- [ ] Verify: `grep "find_package(Qt6" CMakeLists.txt` matches `grep "modules:" .github/workflows/build.yml`

---

## When Adding New Features

**Prompt Claude:** *"Feature complete - review checklist"*

- [ ] Code follows existing patterns (check similar features)
- [ ] Error handling in place (DialogHelper for user messages)
- [ ] Logging added for debugging (LOG_DEBUG/INFO/WARN)
- [ ] No hardcoded values (use constants or settings)
- [ ] Database changes include schema updates if needed
- [ ] Consider: Does this need a test?

---

## When Fixing Bugs

**Prompt Claude:** *"Bug fix complete - verify fix"*

- [ ] Root cause identified and documented
- [ ] Fix tested with reproduction case
- [ ] Similar code checked for same issue
- [ ] Related code reviewed for impact
- [ ] Commit message explains the bug and fix

---

## Common Commands Reference

### Git Operations
```bash
git status                          # Check for uncommitted changes
git add <files>                     # Stage specific files
git commit -m "message"             # Commit with message
git push origin master              # Push to GitHub
gh run list --limit 3               # Check recent CI runs
```

### Build & Test
```bash
cmake --build build -j4             # Build project
cd build && ctest --output-on-failure  # Run tests
pkill -9 tr4qt                      # Kill running app
./build/src/tr4qt.app/Contents/MacOS/tr4qt  # Run app (macOS)
```

### Database Inspection
```bash
sqlite3 ~/.tr4qt/logs/CONTEST_NAME.db "SELECT ..."  # Query database
sqlite3 ~/.tr4qt/logs/CONTEST_NAME.db ".schema qsos"  # Show schema
```

---

## Quick Prompts for Common Tasks

Copy/paste these when needed:

- **"commit to gh - check claude.md first"**
- **"run through the commit checklist"**
- **"run through pre-release checklist"**
- **"verify all 4 version locations match"**
- **"check if tests need updating"**
- **"review code for hardcoded values"**

---

## Notes

- This checklist supplements CLAUDE.md (the detailed reference)
- When in doubt, prompt Claude to check the relevant section
- Add new items as you discover common oversights
- Keep this file updated as processes evolve

