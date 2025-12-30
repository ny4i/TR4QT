; TR4QT NSIS Installer Script
; Requires NSIS 3.x - https://nsis.sourceforge.io/

!include "MUI2.nsh"
!include "FileFunc.nsh"

; Application metadata
!define APPNAME "TR4QT"
!define APPVERSION "3.8.0"
!define APPURL "https://github.com/ny4i/TR4QT"
!define APPPUBLISHER "TR4QT Project"

; Installer attributes
Name "${APPNAME} ${APPVERSION}"
OutFile "..\build\src\TR4QT-${APPVERSION}-Setup.exe"
InstallDir "$PROGRAMFILES\${APPNAME}"
InstallDirRegKey HKLM "Software\${APPNAME}" "InstallDir"
RequestExecutionLevel admin

; Modern UI settings
!define MUI_ABORTWARNING
!define MUI_ICON "${NSISDIR}\Contrib\Graphics\Icons\modern-install.ico"
!define MUI_UNICON "${NSISDIR}\Contrib\Graphics\Icons\modern-uninstall.ico"

; Pages
!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES
!define MUI_FINISHPAGE_RUN "$INSTDIR\tr4qt.exe"
!define MUI_FINISHPAGE_RUN_TEXT "Launch ${APPNAME}"
!insertmacro MUI_PAGE_FINISH

; Uninstaller pages
!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES

; Language
!insertmacro MUI_LANGUAGE "English"

; Installation section
Section "Install"
    SetOutPath "$INSTDIR"

    ; Main executable and all DLLs from build/src
    File "..\build\src\tr4qt.exe"
    File "..\build\src\*.dll"

    ; Qt plugins - platforms
    SetOutPath "$INSTDIR\platforms"
    File "..\build\src\platforms\*.*"

    ; Qt plugins - sqldrivers (CRITICAL - required for database access)
    SetOutPath "$INSTDIR\sqldrivers"
    File "..\build\src\sqldrivers\*.*"

    ; Qt plugins - imageformats
    SetOutPath "$INSTDIR\imageformats"
    File /nonfatal "..\build\src\imageformats\*.*"

    ; Qt plugins - styles
    SetOutPath "$INSTDIR\styles"
    File /nonfatal "..\build\src\styles\*.*"

    ; Qt plugins - tls
    SetOutPath "$INSTDIR\tls"
    File /nonfatal "..\build\src\tls\*.*"

    ; Reset output path
    SetOutPath "$INSTDIR"

    ; Create Start Menu shortcuts
    CreateDirectory "$SMPROGRAMS\${APPNAME}"
    CreateShortcut "$SMPROGRAMS\${APPNAME}\${APPNAME}.lnk" "$INSTDIR\tr4qt.exe"
    CreateShortcut "$SMPROGRAMS\${APPNAME}\Uninstall.lnk" "$INSTDIR\uninstall.exe"

    ; Create Desktop shortcut
    CreateShortcut "$DESKTOP\${APPNAME}.lnk" "$INSTDIR\tr4qt.exe"

    ; Write registry keys for uninstaller
    WriteRegStr HKLM "Software\${APPNAME}" "InstallDir" "$INSTDIR"
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APPNAME}" "DisplayName" "${APPNAME}"
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APPNAME}" "DisplayVersion" "${APPVERSION}"
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APPNAME}" "Publisher" "${APPPUBLISHER}"
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APPNAME}" "URLInfoAbout" "${APPURL}"
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APPNAME}" "UninstallString" "$INSTDIR\uninstall.exe"
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APPNAME}" "InstallLocation" "$INSTDIR"
    WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APPNAME}" "NoModify" 1
    WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APPNAME}" "NoRepair" 1

    ; Calculate installed size
    ${GetSize} "$INSTDIR" "/S=0K" $0 $1 $2
    IntFmt $0 "0x%08X" $0
    WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APPNAME}" "EstimatedSize" "$0"

    ; Create uninstaller
    WriteUninstaller "$INSTDIR\uninstall.exe"
SectionEnd

; Uninstaller section
Section "Uninstall"
    ; Remove files
    Delete "$INSTDIR\tr4qt.exe"
    Delete "$INSTDIR\*.dll"
    Delete "$INSTDIR\uninstall.exe"

    ; Remove plugin directories
    RMDir /r "$INSTDIR\platforms"
    RMDir /r "$INSTDIR\sqldrivers"
    RMDir /r "$INSTDIR\imageformats"
    RMDir /r "$INSTDIR\styles"
    RMDir /r "$INSTDIR\tls"

    ; Remove install directory (if empty)
    RMDir "$INSTDIR"

    ; Remove shortcuts
    Delete "$SMPROGRAMS\${APPNAME}\${APPNAME}.lnk"
    Delete "$SMPROGRAMS\${APPNAME}\Uninstall.lnk"
    RMDir "$SMPROGRAMS\${APPNAME}"
    Delete "$DESKTOP\${APPNAME}.lnk"

    ; Remove registry keys
    DeleteRegKey HKLM "Software\${APPNAME}"
    DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APPNAME}"
SectionEnd
