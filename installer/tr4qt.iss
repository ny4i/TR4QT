; TR4QT Inno Setup Script
; Requires Inno Setup 6.x - https://jrsoftware.org/isinfo.php

#define MyAppName "TR4QT"
#define MyAppVersion "2.88.0"
#define MyAppPublisher "TR4QT Project"
#define MyAppURL "https://github.com/ny4i/TR4QT"
#define MyAppExeName "tr4qt.exe"

[Setup]
; NOTE: AppId uniquely identifies this application. Do not use the same AppId in other installers.
AppId={{B8F3E2A1-5C4D-4E6F-8A9B-1C2D3E4F5A6B}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppPublisher={#MyAppPublisher}
AppPublisherURL={#MyAppURL}
AppSupportURL={#MyAppURL}
AppUpdatesURL={#MyAppURL}/releases
DefaultDirName={autopf}\{#MyAppName}
DefaultGroupName={#MyAppName}
AllowNoIcons=yes
; Output settings
OutputDir=..\installer_output
OutputBaseFilename=TR4QT-{#MyAppVersion}-Setup
; Compression
Compression=lzma2
SolidCompression=yes
; Windows version requirements
MinVersion=10.0
; Privileges - install for current user by default, allow admin install
PrivilegesRequired=lowest
PrivilegesRequiredOverridesAllowed=dialog
; Modern wizard style
WizardStyle=modern
; License file (optional - uncomment if you have one)
; LicenseFile=..\LICENSE

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked

[Files]
; Main executable
Source: "..\build\src\tr4qt.exe"; DestDir: "{app}"; Flags: ignoreversion

; Qt DLLs
Source: "..\build\src\Qt6Core.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\build\src\Qt6Gui.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\build\src\Qt6Widgets.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\build\src\Qt6Network.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\build\src\Qt6Sql.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\build\src\Qt6Svg.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\build\src\Qt6PrintSupport.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\build\src\Qt6Concurrent.dll"; DestDir: "{app}"; Flags: ignoreversion

; MinGW runtime DLLs
Source: "..\build\src\libgcc_s_seh-1.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\build\src\libstdc++-6.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\build\src\libwinpthread-1.dll"; DestDir: "{app}"; Flags: ignoreversion

; Hamlib DLLs
Source: "..\build\src\libhamlib-4.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\build\src\libusb-1.0.dll"; DestDir: "{app}"; Flags: ignoreversion

; Graphics DLLs
Source: "..\build\src\D3Dcompiler_47.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\build\src\opengl32sw.dll"; DestDir: "{app}"; Flags: ignoreversion

; Qt plugins - platforms
Source: "..\build\src\platforms\*"; DestDir: "{app}\platforms"; Flags: ignoreversion recursesubdirs

; Qt plugins - imageformats
Source: "..\build\src\imageformats\*"; DestDir: "{app}\imageformats"; Flags: ignoreversion recursesubdirs

; Qt plugins - sqldrivers
Source: "..\build\src\sqldrivers\*"; DestDir: "{app}\sqldrivers"; Flags: ignoreversion recursesubdirs

; Qt plugins - styles
Source: "..\build\src\styles\*"; DestDir: "{app}\styles"; Flags: ignoreversion recursesubdirs

; Qt plugins - tls
Source: "..\build\src\tls\*"; DestDir: "{app}\tls"; Flags: ignoreversion recursesubdirs

; Qt plugins - generic
Source: "..\build\src\generic\*"; DestDir: "{app}\generic"; Flags: ignoreversion recursesubdirs

; Qt plugins - networkinformation
Source: "..\build\src\networkinformation\*"; DestDir: "{app}\networkinformation"; Flags: ignoreversion recursesubdirs

; Qt translations (optional - can be large)
Source: "..\build\src\translations\*"; DestDir: "{app}\translations"; Flags: ignoreversion recursesubdirs

[Icons]
Name: "{group}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"
Name: "{group}\{cm:UninstallProgram,{#MyAppName}}"; Filename: "{uninstallexe}"
Name: "{autodesktop}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"; Tasks: desktopicon

[Run]
Filename: "{app}\{#MyAppExeName}"; Description: "{cm:LaunchProgram,{#StringChange(MyAppName, '&', '&&')}}"; Flags: nowait postinstall skipifsilent
