# Windows App Deployment Guide

**CRITICAL**: This section documents the complete process for creating distributable Windows applications. These lessons apply to ANY Windows Qt app, not just TR4QT.

## The Problem: Missing DLL Hell

When you build a Qt app on Windows, the .exe works on YOUR machine but fails on other machines with "missing DLL" errors. Why?

1. **Qt DLLs not bundled**: Your dev machine has Qt in PATH, but users don't
2. **Silent omissions**: `windeployqt` doesn't copy everything (especially HttpServer, TLS plugins)
3. **Runtime dependencies**: MinGW runtime DLLs, Hamlib, libusb not included
4. **Plugin directories**: Qt requires plugins in specific subdirectories

## Complete Deployment Workflow

**Use the explicit deployment script:**

```batch
# From project root on Windows
.\scripts\windows-deploy.bat
```

**What the script does** (`scripts/windows-deploy.bat`):

1. **Copies Qt DLLs explicitly** (no windeployqt):
   - Qt6Core.dll, Qt6Gui.dll, Qt6Widgets.dll, Qt6Network.dll
   - Qt6Sql.dll, Qt6HttpServer.dll (windeployqt forgets this!)
   - Qt6PrintSupport.dll, Qt6Concurrent.dll, Qt6WebSockets.dll

2. **Copies MinGW runtime DLLs**:
   - libgcc_s_seh-1.dll
   - libstdc++-6.dll
   - libwinpthread-1.dll

3. **Copies Qt plugins** (CRITICAL - app won't run without these):
   - `platforms\qwindows.dll` - Windows GUI support
   - `sqldrivers\qsqlite.dll` - Database access
   - `tls\*.dll` - HTTPS connections (qopensslbackend, qschannelbackend, qcertonlybackend)
   - `styles\qwindowsvistastyle.dll` - Native Windows appearance

4. **Copies Hamlib and dependencies**:
   - libhamlib-4.dll (radio control)
   - libusb-1.0.dll (USB radio support)

5. **Creates qt.conf** - Tells Qt where to find plugins

6. **Verifies deployment** - Lists all deployed files and checks for missing critical components

## Manual Deployment (if script unavailable)

If you need to deploy manually:

```batch
cd build\src

# Qt Core DLLs
copy c:\Qt\6.10.1\mingw_64\bin\Qt6Core.dll .
copy c:\Qt\6.10.1\mingw_64\bin\Qt6Gui.dll .
copy c:\Qt\6.10.1\mingw_64\bin\Qt6Widgets.dll .
copy c:\Qt\6.10.1\mingw_64\bin\Qt6Network.dll .
copy c:\Qt\6.10.1\mingw_64\bin\Qt6Sql.dll .
copy c:\Qt\6.10.1\mingw_64\bin\Qt6HttpServer.dll .
copy c:\Qt\6.10.1\mingw_64\bin\Qt6PrintSupport.dll .
copy c:\Qt\6.10.1\mingw_64\bin\Qt6Concurrent.dll .
copy c:\Qt\6.10.1\mingw_64\bin\Qt6WebSockets.dll .

# MinGW runtime
copy c:\Qt\6.10.1\mingw_64\bin\libgcc_s_seh-1.dll .
copy c:\Qt\6.10.1\mingw_64\bin\libstdc++-6.dll .
copy c:\Qt\6.10.1\mingw_64\bin\libwinpthread-1.dll .

# Qt Plugins (CRITICAL!)
mkdir platforms
copy c:\Qt\6.10.1\mingw_64\plugins\platforms\qwindows.dll platforms\

mkdir sqldrivers
copy c:\Qt\6.10.1\mingw_64\plugins\sqldrivers\qsqlite.dll sqldrivers\

mkdir tls
copy c:\Qt\6.10.1\mingw_64\plugins\tls\*.dll tls\

# Hamlib
copy c:\projects\hamlib\bin\libhamlib-4.dll .
copy c:\projects\hamlib\bin\libusb-1.0.dll .

# Create qt.conf
echo [Paths] > qt.conf
echo Plugins = . >> qt.conf
```

## Common Windows Deployment Issues

### Issue 1: "Qt6HttpServer.dll is missing"
**Symptom**: App fails to launch with popup about missing Qt6HttpServer.dll

**Cause**: `windeployqt` doesn't know about HttpServer module (it's not a standard Qt module)

**Fix**: Manually copy `Qt6HttpServer.dll` from Qt bin directory

### Issue 2: "The application failed to start because no Qt platform plugin could be initialized"
**Symptom**: Black screen or immediate crash with this error message

**Diagnosis**:
```batch
# Check if platforms directory exists
dir platforms

# Check if qwindows.dll exists
dir platforms\qwindows.dll
```

**Cause**: Missing `platforms\qwindows.dll` plugin or missing qt.conf

**Fix**:
1. Copy `qwindows.dll` to `platforms\` subdirectory
2. Create qt.conf with `[Paths]` and `Plugins = .`

### Issue 3: Database doesn't work (can't create/open logs)
**Symptom**: Application runs but can't create contests or open logs

**Diagnosis**: Check for SQL driver plugin
```batch
dir sqldrivers\qsqlite.dll
```

**Cause**: Missing `sqldrivers\qsqlite.dll` plugin

**Fix**: Copy qsqlite.dll to `sqldrivers\` subdirectory

### Issue 4: HTTPS downloads fail (CTY.DAT, LOTW updates)
**Symptom**: Application runs but downloading country file or LOTW data fails with TLS errors

**Diagnosis**: Check for TLS plugins
```batch
dir tls\*.dll
```

**Cause**: Missing TLS backend plugins (windeployqt **always** forgets these!)

**Fix**: Copy all TLS plugins from `Qt\plugins\tls\` to `tls\` subdirectory:
- qopensslbackend.dll (OpenSSL TLS)
- qschannelbackend.dll (Windows native TLS - recommended)
- qcertonlybackend.dll (certificate-only)

## Why Each Step Matters

1. **Explicit DLL copying**: Know exactly what's deployed, no surprises
2. **Qt plugins in subdirectories**: Qt requires plugins in specific locations relative to .exe
3. **qt.conf file**: Tells Qt to look in current directory for plugins (not Qt installation)
4. **Verification**: Catch missing files before distribution, not after users report issues

## Testing a Deployment Before Distribution

```batch
# 1. Build the application
cmake --build build --config Release

# 2. Run deployment script
.\scripts\windows-deploy.bat

# 3. Test the deployed exe
cd build\src
tr4qt.exe --version

# 4. Verify all DLLs are present
dir *.dll
dir platforms\*.dll
dir sqldrivers\*.dll
dir tls\*.dll

# 5. Test on a clean Windows VM (best practice)
# Copy build\src\ folder to a machine WITHOUT Qt or dev tools installed
# Should run without any DLL errors
```

## Automation in CI/CD

The complete workflow is in `.github/workflows/build.yml` under the `build-windows` job.

**Critical notes for CI**:
- Build on `windows-latest` runner
- Use MinGW or MSVC (specify in CMAKE_PREFIX_PATH)
- Run `windows-deploy.bat` after build
- Create ZIP or installer with all files from deploy directory
- Test the packaged app on the runner before creating release

## Quick Reference: Windows Deployment Checklist

Before distributing a Windows build:

- [ ] All Qt6*.dll files present in exe directory
- [ ] All MinGW runtime DLLs present (libgcc, libstdc++, libwinpthread)
- [ ] `platforms\qwindows.dll` exists
- [ ] `sqldrivers\qsqlite.dll` exists
- [ ] `tls\*.dll` plugins exist (at least one TLS backend)
- [ ] `libhamlib-4.dll` and `libusb-1.0.dll` present
- [ ] `qt.conf` file created
- [ ] Tested `tr4qt.exe --version` from deployed directory
- [ ] Tested full GUI launch from deployed directory
- [ ] (Optional) Tested on clean Windows machine without Qt/dev tools

## Common Windows Build Errors

### Error: "interface" is a reserved keyword
**Symptom**:
```
error: expected ',' or '...' before 'struct'
void sendDiscoveryMessage(const QNetworkInterface& interface);
                                                   ^~~~~~~~~
```

**Cause**: Windows COM headers define `#define interface struct`, which conflicts with parameter names

**Fix**: Rename the parameter to something else (e.g., `netInterface`, `iface`, `networkInterface`)

**Files to check**: Any file that includes Windows headers (directly or through Hamlib) and uses "interface" as an identifier
