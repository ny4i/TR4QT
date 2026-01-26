# TR4QT Dependency Management

## Overview

TR4QT uses an automated validation system to ensure Qt modules and other dependencies are consistently deployed across all platforms.

## Automated Validation System

### Script: `scripts/validate-qt-modules.sh`

Runs in CI before builds and validates that Qt modules are consistently declared in:

1. **CMakeLists.txt** - Source of truth (what the app needs)
2. **.github/workflows/build.yml** - Windows deployment
3. **scripts/verify-deployment.sh** - Deployment verification

**Exit codes:**
- `0` = All checks passed
- `1` = Inconsistencies found (fails CI build)

### What It Checks

✅ Every Qt module in CMakeLists.txt is deployed (Windows)
✅ Every deployed module is verified
⚠️ Warns about stale deployments (deployed but not in CMakeLists.txt)

### Base vs. Additional Modules

**Base modules** (always installed by Qt, no explicit deployment needed):
- Core, Gui, Widgets, Network, Sql, PrintSupport, Concurrent, Test, OpenGL, Xml

**Additional modules** (MUST be explicitly deployed):
- HttpServer, WebSockets, SerialPort, Svg, etc.

## Adding New Qt Modules

### Step-by-Step Workflow

#### 1. Add to CMakeLists.txt (Source of Truth)

```cmake
find_package(Qt6 REQUIRED COMPONENTS
    Core
    Widgets
    # ... existing modules ...
    NewModule  # ← Add here
)

target_link_libraries(tr4qt PRIVATE
    Qt6::Core
    Qt6::Widgets
    Qt6::NewModule  # ← And here
)
```

#### 2. Update Windows CI (if not a base module)

**Check if it's a base module:** Core, Gui, Widgets, Network, Sql, PrintSupport, Concurrent, Test, OpenGL, Xml

**If NOT a base module**, add to `.github/workflows/build.yml`:

```yaml
- name: Install Qt
  uses: jurplel/install-qt-action@v4
  with:
    modules: 'qtwebsockets qthttpserver qtserialport qtnewmodule'  # ← Add here
```

#### 3. Update Windows Deployment

Add to `.github/workflows/build.yml` deployment section:

```bash
# Copy Qt DLLs (explicit list of what we need)
echo "Copying Qt DLLs..."
cp "$QT_BIN/Qt6Core.dll" "$DEST/"
# ... existing DLLs ...
cp "$QT_BIN/Qt6NewModule.dll" "$DEST/"  # ← Add here
```

#### 4. Update Verification Script

Add to `scripts/verify-deployment.sh`:

```bash
echo "=== Qt Additional DLLs ==="
check_file "Qt6HttpServer.dll" "Qt HTTP Server" true
# ... existing checks ...
check_file "Qt6NewModule.dll" "Qt New Module" true  # ← Add here
```

#### 5. Test Locally

```bash
# Build
cmake --build build

# Run module validation
bash scripts/validate-qt-modules.sh

# Run deployment verification
bash scripts/verify-deployment.sh build/src
```

#### 6. Commit All Together

```bash
git add CMakeLists.txt .github/workflows/build.yml scripts/verify-deployment.sh
git commit -m "Add Qt NewModule dependency"
```

### CI Will Catch Mistakes

If you forget step 3 or 4, CI will fail with clear instructions:

```
✗ ERROR: NewModule declared in CMakeLists.txt but NOT deployed in build.yml
  Fix: Add to .github/workflows/build.yml:
       cp "$QT_BIN/Qt6NewModule.dll" "$DEST/"
```

## Adding Non-Qt Dependencies

### Example: Adding a New Library

#### 1. Update CMakeLists.txt

Add library finding logic and linking.

#### 2. Update CI Build

Add download/install step in `.github/workflows/build.yml`:

```yaml
- name: Download NewLibrary
  run: |
    curl -L -o library.zip "https://example.com/library.zip"
    unzip library.zip -d C:/library
```

#### 3. Update Deployment

Copy DLLs/dylibs/so files:

```bash
echo "Copying NewLibrary..."
cp C:/library/bin/*.dll "$DEST/"
```

#### 4. Update Verification

Add to `scripts/verify-deployment.sh`:

```bash
echo "=== NewLibrary ==="
check_file "newlibrary.dll" "NewLibrary" true
```

## Quick Reference Checklist

When adding a **Qt module**:
- [ ] CMakeLists.txt: `find_package` + `target_link_libraries`
- [ ] CI (if not base): `.github/workflows/build.yml` → `modules:`
- [ ] CI deployment: `.github/workflows/build.yml` → `cp Qt6NewModule.dll`
- [ ] Verification: `scripts/verify-deployment.sh` → `check_file Qt6NewModule.dll`
- [ ] Test: `bash scripts/validate-qt-modules.sh`

When adding a **non-Qt library**:
- [ ] CMakeLists.txt: Library finding + linking
- [ ] CI: Download/install step
- [ ] CI deployment: Copy library files
- [ ] Verification: `scripts/verify-deployment.sh` → new check

When adding **Qt plugins**:
- [ ] CI deployment: Create plugin directory + copy files
- [ ] NSIS installer: Add plugin directory to `installer/tr4qt.nsi`
- [ ] Verification: `scripts/verify-deployment.sh` → `check_directory`

## Scripts Overview

### validate-qt-modules.sh
- **Purpose**: Ensure Qt modules are consistent across CMakeLists.txt, deployment, and verification
- **When**: Runs in CI before builds
- **Fails**: If any module is missing from deployment or verification

### verify-deployment.sh
- **Purpose**: Check that all required DLLs and plugins are present in build directory
- **When**: Runs in CI after deployment, before creating installer
- **Fails**: If any critical dependency is missing

## Preventing "Whack-a-Mole" Issues

The automated validation system prevents issues like:

1. **Missing DLLs** - Caught by validate-qt-modules.sh
2. **Missing plugins** - Caught by verify-deployment.sh
3. **Stale dependencies** - Warned by validate-qt-modules.sh
4. **Incomplete deployments** - Caught by verify-deployment.sh

Both scripts provide clear, actionable error messages with exact fix instructions.

## History

- **v3.38.74**: Added Qt6Svg.dll and Qt6Xml.dll (issue #63)
- **v3.38.75**: Embedded SVG resources, created verify-deployment.sh
- **v3.38.76**: Fixed verification script to complete all checks
- **v3.38.77**: Added validate-qt-modules.sh automated validation

## Related Files

- `CMakeLists.txt` - Source of truth for dependencies
- `.github/workflows/build.yml` - CI build and deployment
- `scripts/validate-qt-modules.sh` - Module consistency validation
- `scripts/verify-deployment.sh` - Deployment verification
- `installer/tr4qt.nsi` - Windows installer configuration
- `VERSION_REQUIREMENTS.md` - Required dependency versions
