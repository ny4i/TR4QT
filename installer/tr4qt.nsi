; TR4QT NSIS Installer Script
; Requires NSIS 3.x - https://nsis.sourceforge.io/

!include "MUI2.nsh"
!include "FileFunc.nsh"

; Application metadata
!define APPNAME "TR4QT"
!define APPVERSION "2.88.0"
!define APPURL "https://github.com/ny4i/TR4QT"
!define APPPUBLISHER "TR4QT Project"

; Installer attributes
Name "${APPNAME} ${APPVERSION}"
OutFile "..\dist\TR4QT-${APPVERSION}-Setup.exe"
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

    ; Main executable
    File "..\build\src\tr4qt.exe"

    ; Qt DLLs
    File "..\build\src\Qt6Core.dll"
    File "..\build\src\Qt6Gui.dll"
    File "..\build\src\Qt6Widgets.dll"
    File "..\build\src\Qt6Network.dll"
    File "..\build\src\Qt6Sql.dll"
    File "..\build\src\Qt6Svg.dll"
    File "..\build\src\Qt6PrintSupport.dll"
    File "..\build\src\Qt6Concurrent.dll"

    ; MinGW runtime DLLs
    File "..\build\src\libgcc_s_seh-1.dll"
    File "..\build\src\libstdc++-6.dll"
    File "..\build\src\libwinpthread-1.dll"

    ; Hamlib DLLs
    File "..\build\src\libhamlib-4.dll"
    File "..\build\src\libusb-1.0.dll"

    ; Graphics DLLs
    File "..\build\src\D3Dcompiler_47.dll"
    File "..\build\src\opengl32sw.dll"

    ; Qt plugins - platforms
    SetOutPath "$INSTDIR\platforms"
    File "..\build\src\platforms\*.*"

    ; Qt plugins - imageformats
    SetOutPath "$INSTDIR\imageformats"
    File "..\build\src\imageformats\*.*"

    ; Qt plugins - sqldrivers
    SetOutPath "$INSTDIR\sqldrivers"
    File "..\build\src\sqldrivers\*.*"

    ; Qt plugins - styles
    SetOutPath "$INSTDIR\styles"
    File "..\build\src\styles\*.*"

    ; Qt plugins - tls
    SetOutPath "$INSTDIR\tls"
    File "..\build\src\tls\*.*"

    ; Qt plugins - generic
    SetOutPath "$INSTDIR\generic"
    File "..\build\src\generic\*.*"

    ; Qt plugins - networkinformation
    SetOutPath "$INSTDIR\networkinformation"
    File "..\build\src\networkinformation\*.*"

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
    RMDir /r "$INSTDIR\imageformats"
    RMDir /r "$INSTDIR\sqldrivers"
    RMDir /r "$INSTDIR\styles"
    RMDir /r "$INSTDIR\tls"
    RMDir /r "$INSTDIR\generic"
    RMDir /r "$INSTDIR\networkinformation"
    RMDir /r "$INSTDIR\translations"

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
