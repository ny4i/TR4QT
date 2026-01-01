@echo off
REM Windows Deployment Script - EXPLICIT DEPLOYMENT (no windeployqt)
REM Copies all required DLLs, plugins, and dependencies explicitly

setlocal enabledelayedexpansion

REM Configuration - adjust these paths for your system
set QT_DIR=c:\Qt\6.10.1\mingw_64
set HAMLIB_DIR=c:\projects\hamlib
set BUILD_DIR=.\build\src
set DEPLOY_DIR=%BUILD_DIR%

echo =^> Explicit Windows Deployment (no windeployqt)
echo =^> Deploy directory: %DEPLOY_DIR%
echo.

REM Step 1: Copy Qt Core DLLs explicitly
echo =^> Step 1: Copying Qt core DLLs...
set QT_DLLS=Qt6Core Qt6Gui Qt6Widgets Qt6Network Qt6Sql Qt6HttpServer Qt6PrintSupport Qt6Concurrent Qt6WebSockets Qt6SerialPort

for %%D in (%QT_DLLS%) do (
    echo     Copying %%D.dll...
    copy /Y "%QT_DIR%\bin\%%D.dll" "%DEPLOY_DIR%\" >nul
    if !errorlevel! neq 0 (
        echo     ERROR: Failed to copy %%D.dll from %QT_DIR%\bin
        exit /b 1
    )
)

REM Step 2: Copy MinGW runtime DLLs
echo =^> Step 2: Copying MinGW runtime DLLs...
set MINGW_DLLS=libgcc_s_seh-1 libstdc++-6 libwinpthread-1

for %%D in (%MINGW_DLLS%) do (
    echo     Copying %%D.dll...
    copy /Y "%QT_DIR%\bin\%%D.dll" "%DEPLOY_DIR%\" >nul
    if !errorlevel! neq 0 (
        echo     ERROR: Failed to copy %%D.dll from %QT_DIR%\bin
        exit /b 1
    )
)

REM Step 3: Copy Qt Plugins (CRITICAL - app won't run without these)
echo =^> Step 3: Copying Qt plugins...

REM Platforms plugin (REQUIRED for Windows GUI)
echo     Creating platforms directory...
if not exist "%DEPLOY_DIR%\platforms" mkdir "%DEPLOY_DIR%\platforms"
echo     Copying platforms\qwindows.dll...
copy /Y "%QT_DIR%\plugins\platforms\qwindows.dll" "%DEPLOY_DIR%\platforms\" >nul
if !errorlevel! neq 0 (
    echo     ERROR: Failed to copy platforms\qwindows.dll
    exit /b 1
)

REM Styles plugin (for native Windows look)
echo     Creating styles directory...
if not exist "%DEPLOY_DIR%\styles" mkdir "%DEPLOY_DIR%\styles"
echo     Copying styles\qwindowsvistastyle.dll...
copy /Y "%QT_DIR%\plugins\styles\qwindowsvistastyle.dll" "%DEPLOY_DIR%\styles\" >nul
if !errorlevel! neq 0 (
    echo     WARNING: styles\qwindowsvistastyle.dll not found (optional)
)

REM SQL drivers plugin (CRITICAL - for database access)
echo     Creating sqldrivers directory...
if not exist "%DEPLOY_DIR%\sqldrivers" mkdir "%DEPLOY_DIR%\sqldrivers"
echo     Copying sqldrivers\qsqlite.dll...
copy /Y "%QT_DIR%\plugins\sqldrivers\qsqlite.dll" "%DEPLOY_DIR%\sqldrivers\" >nul
if !errorlevel! neq 0 (
    echo     ERROR: Failed to copy sqldrivers\qsqlite.dll - database won't work!
    exit /b 1
)

REM TLS plugins (CRITICAL - for HTTPS downloads)
echo     Creating tls directory...
if not exist "%DEPLOY_DIR%\tls" mkdir "%DEPLOY_DIR%\tls"
echo     Copying TLS plugins...
if exist "%QT_DIR%\plugins\tls\qopensslbackend.dll" (
    echo       Copying tls\qopensslbackend.dll...
    copy /Y "%QT_DIR%\plugins\tls\qopensslbackend.dll" "%DEPLOY_DIR%\tls\" >nul
)
if exist "%QT_DIR%\plugins\tls\qschannelbackend.dll" (
    echo       Copying tls\qschannelbackend.dll...
    copy /Y "%QT_DIR%\plugins\tls\qschannelbackend.dll" "%DEPLOY_DIR%\tls\" >nul
)
if exist "%QT_DIR%\plugins\tls\qcertonlybackend.dll" (
    echo       Copying tls\qcertonlybackend.dll...
    copy /Y "%QT_DIR%\plugins\tls\qcertonlybackend.dll" "%DEPLOY_DIR%\tls\" >nul
)

REM Step 4: Copy Hamlib and dependencies
echo =^> Step 4: Copying Hamlib and dependencies...

echo     Copying libhamlib-4.dll...
copy /Y "%HAMLIB_DIR%\bin\libhamlib-4.dll" "%DEPLOY_DIR%\" >nul
if !errorlevel! neq 0 (
    echo     ERROR: Failed to copy libhamlib-4.dll from %HAMLIB_DIR%\bin
    exit /b 1
)

echo     Copying libusb-1.0.dll...
copy /Y "%HAMLIB_DIR%\bin\libusb-1.0.dll" "%DEPLOY_DIR%\" >nul
if !errorlevel! neq 0 (
    echo     ERROR: Failed to copy libusb-1.0.dll from %HAMLIB_DIR%\bin
    exit /b 1
)

REM Step 5: Create qt.conf (tells Qt where to find plugins)
echo =^> Step 5: Creating qt.conf...
echo [Paths] > "%DEPLOY_DIR%\qt.conf"
echo Plugins = . >> "%DEPLOY_DIR%\qt.conf"
echo     Created qt.conf

REM Step 6: Verification
echo.
echo =^> Step 6: Deployment verification...
echo Qt DLLs in %DEPLOY_DIR%:
dir /b "%DEPLOY_DIR%\Qt6*.dll" 2>nul | findstr /v /c:":" >nul
if !errorlevel! equ 0 (
    for /f %%F in ('dir /b "%DEPLOY_DIR%\Qt6*.dll"') do echo   %%F
) else (
    echo   ERROR: No Qt DLLs found!
    exit /b 1
)

echo.
echo Qt Plugins:
if exist "%DEPLOY_DIR%\platforms\qwindows.dll" (
    echo   platforms\qwindows.dll [OK]
) else (
    echo   platforms\qwindows.dll [MISSING - CRITICAL!]
    exit /b 1
)

if exist "%DEPLOY_DIR%\sqldrivers\qsqlite.dll" (
    echo   sqldrivers\qsqlite.dll [OK]
) else (
    echo   sqldrivers\qsqlite.dll [MISSING - CRITICAL!]
    exit /b 1
)

if exist "%DEPLOY_DIR%\tls" (
    for /f %%F in ('dir /b "%DEPLOY_DIR%\tls\*.dll" 2^>nul') do echo   tls\%%F [OK]
) else (
    echo   tls\ [MISSING - HTTPS won't work!]
)

echo.
echo Other Libraries:
if exist "%DEPLOY_DIR%\libhamlib-4.dll" (
    echo   libhamlib-4.dll [OK]
) else (
    echo   libhamlib-4.dll [MISSING - Radio control won't work!]
)

if exist "%DEPLOY_DIR%\libusb-1.0.dll" (
    echo   libusb-1.0.dll [OK]
) else (
    echo   libusb-1.0.dll [MISSING - USB radios won't work!]
)

echo.
echo =^> Done! All files deployed to: %DEPLOY_DIR%
echo.
echo To run the app:
echo   cd %DEPLOY_DIR%
echo   tr4qt.exe
echo.
echo To test version:
echo   cd %DEPLOY_DIR%
echo   tr4qt.exe --version
echo.

endlocal
