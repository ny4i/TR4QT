; TR4QT NSIS Installer Script
; Requires NSIS 3.x - https://nsis.sourceforge.io/

!include "MUI2.nsh"
!include "FileFunc.nsh"

; Application metadata
!define APPNAME "TR4QT"
!define APPVERSION "2.89.0"
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
    File "..\dist\TR4QT\tr4qt.exe"

    ; Qt DLLs
    File "..\dist\TR4QT\Qt6Core.dll"
    File "..\dist\TR4QT\Qt6Gui.dll"
    File "..\dist\TR4QT\Qt6Widgets.dll"
    File "..\dist\TR4QT\Qt6Network.dll"
    File "..\dist\TR4QT\Qt6Sql.dll"
    File "..\dist\TR4QT\Qt6Svg.dll"
    File "..\dist\TR4QT\Qt6PrintSupport.dll"

    ; MinGW runtime DLLs
    File "..\dist\TR4QT\libgcc_s_seh-1.dll"
    File "..\dist\TR4QT\libstdc++-6.dll"
    File "..\dist\TR4QT\libwinpthread-1.dll"

    ; Hamlib DLLs
    File "..\dist\TR4QT\libhamlib-4.dll"
    File "..\dist\TR4QT\libusb-1.0.dll"

    ; Graphics DLLs
    File "..\dist\TR4QT\D3Dcompiler_47.dll"
    File "..\dist\TR4QT\opengl32sw.dll"

    ; Qt plugins - platforms
    SetOutPath "$INSTDIR\platforms"
    File "..\dist\TR4QT\platforms\*.*"

    ; Qt plugins - imageformats
    SetOutPath "$INSTDIR\imageformats"
    File "..\dist\TR4QT\imageformats\*.*"

    ; Qt plugins - sqldrivers
    SetOutPath "$INSTDIR\sqldrivers"
    File "..\dist\TR4QT\sqldrivers\*.*"

    ; Qt plugins - styles
    SetOutPath "$INSTDIR\styles"
    File "..\dist\TR4QT\styles\*.*"

    ; Qt plugins - tls
    SetOutPath "$INSTDIR\tls"
    File "..\dist\TR4QT\tls\*.*"

    ; Qt plugins - generic
    SetOutPath "$INSTDIR\generic"
    File "..\dist\TR4QT\generic\*.*"

    ; Qt plugins - networkinformation
    SetOutPath "$INSTDIR\networkinformation"
    File "..\dist\TR4QT\networkinformation\*.*"

    ; Qt plugins - iconengines
    SetOutPath "$INSTDIR\iconengines"
    File "..\dist\TR4QT\iconengines\*.*"

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
    RMDir /r "$INSTDIR\iconengines"
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
