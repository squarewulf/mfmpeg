; MFmpeg v2 Installer Script for Inno Setup
; Requires Inno Setup 6.x  (https://jrsoftware.org/isinfo.php)
;
; Before compiling this script, place the following in the v2\ folder:
;   - MFmpegV2.exe   (build with build.bat)
;   - ffmpeg.exe     (from https://www.gyan.dev/ffmpeg/builds/ or BtbN releases)
;   - icon.ico

#define MyAppName "MFmpeg v2"
#define MyAppVersion "2.0.0"
#define MyAppPublisher "MFmpeg"
#define MyAppExeName "MFmpegV2.exe"

[Setup]
AppId={{A1B2C3D4-5E6F-7A8B-9C0D-E1F2A3B4C5D6}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppPublisher={#MyAppPublisher}
DefaultDirName={autopf}\MFmpeg
DefaultGroupName={#MyAppName}
AllowNoIcons=yes
OutputDir=.\installer
OutputBaseFilename=MFmpegV2_Setup_{#MyAppVersion}
SetupIconFile=icon.ico
Compression=lzma2/ultra64
SolidCompression=yes
WizardStyle=modern
PrivilegesRequired=admin
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
UninstallDisplayIcon={app}\{#MyAppExeName}
UninstallDisplayName={#MyAppName}
VersionInfoVersion={#MyAppVersion}

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked
Name: "contextmenu"; Description: "Add ""Open sequence in MFmpeg"" to image file context menus"; GroupDescription: "Shell Integration:"

[Files]
Source: "MFmpegV2.exe"; DestDir: "{app}"; Flags: ignoreversion
Source: "icon.ico"; DestDir: "{app}"; Flags: ignoreversion
; FFmpeg -- bundle if present, otherwise the app will prompt to download
#ifexist "ffmpeg.exe"
Source: "ffmpeg.exe"; DestDir: "{app}"; Flags: ignoreversion
#endif

[Dirs]
Name: "{userappdata}\MFmpeg"; Permissions: users-modify

[Icons]
Name: "{group}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"; IconFilename: "{app}\icon.ico"
Name: "{group}\Uninstall {#MyAppName}"; Filename: "{uninstallexe}"
Name: "{autodesktop}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"; IconFilename: "{app}\icon.ico"; Tasks: desktopicon

[Registry]
; Context menu for image file types
; .png
Root: HKCU; Subkey: "Software\Classes\SystemFileAssociations\.png\shell\MFmpeg"; ValueType: string; ValueName: ""; ValueData: "Open sequence in MFmpeg"; Flags: uninsdeletekey; Tasks: contextmenu
Root: HKCU; Subkey: "Software\Classes\SystemFileAssociations\.png\shell\MFmpeg"; ValueType: string; ValueName: "Icon"; ValueData: """{app}\{#MyAppExeName}"""; Tasks: contextmenu
Root: HKCU; Subkey: "Software\Classes\SystemFileAssociations\.png\shell\MFmpeg\command"; ValueType: string; ValueName: ""; ValueData: """{app}\{#MyAppExeName}"" ""%1"""; Tasks: contextmenu
; .jpg
Root: HKCU; Subkey: "Software\Classes\SystemFileAssociations\.jpg\shell\MFmpeg"; ValueType: string; ValueName: ""; ValueData: "Open sequence in MFmpeg"; Flags: uninsdeletekey; Tasks: contextmenu
Root: HKCU; Subkey: "Software\Classes\SystemFileAssociations\.jpg\shell\MFmpeg"; ValueType: string; ValueName: "Icon"; ValueData: """{app}\{#MyAppExeName}"""; Tasks: contextmenu
Root: HKCU; Subkey: "Software\Classes\SystemFileAssociations\.jpg\shell\MFmpeg\command"; ValueType: string; ValueName: ""; ValueData: """{app}\{#MyAppExeName}"" ""%1"""; Tasks: contextmenu
; .jpeg
Root: HKCU; Subkey: "Software\Classes\SystemFileAssociations\.jpeg\shell\MFmpeg"; ValueType: string; ValueName: ""; ValueData: "Open sequence in MFmpeg"; Flags: uninsdeletekey; Tasks: contextmenu
Root: HKCU; Subkey: "Software\Classes\SystemFileAssociations\.jpeg\shell\MFmpeg"; ValueType: string; ValueName: "Icon"; ValueData: """{app}\{#MyAppExeName}"""; Tasks: contextmenu
Root: HKCU; Subkey: "Software\Classes\SystemFileAssociations\.jpeg\shell\MFmpeg\command"; ValueType: string; ValueName: ""; ValueData: """{app}\{#MyAppExeName}"" ""%1"""; Tasks: contextmenu
; .bmp
Root: HKCU; Subkey: "Software\Classes\SystemFileAssociations\.bmp\shell\MFmpeg"; ValueType: string; ValueName: ""; ValueData: "Open sequence in MFmpeg"; Flags: uninsdeletekey; Tasks: contextmenu
Root: HKCU; Subkey: "Software\Classes\SystemFileAssociations\.bmp\shell\MFmpeg"; ValueType: string; ValueName: "Icon"; ValueData: """{app}\{#MyAppExeName}"""; Tasks: contextmenu
Root: HKCU; Subkey: "Software\Classes\SystemFileAssociations\.bmp\shell\MFmpeg\command"; ValueType: string; ValueName: ""; ValueData: """{app}\{#MyAppExeName}"" ""%1"""; Tasks: contextmenu
; .tif
Root: HKCU; Subkey: "Software\Classes\SystemFileAssociations\.tif\shell\MFmpeg"; ValueType: string; ValueName: ""; ValueData: "Open sequence in MFmpeg"; Flags: uninsdeletekey; Tasks: contextmenu
Root: HKCU; Subkey: "Software\Classes\SystemFileAssociations\.tif\shell\MFmpeg"; ValueType: string; ValueName: "Icon"; ValueData: """{app}\{#MyAppExeName}"""; Tasks: contextmenu
Root: HKCU; Subkey: "Software\Classes\SystemFileAssociations\.tif\shell\MFmpeg\command"; ValueType: string; ValueName: ""; ValueData: """{app}\{#MyAppExeName}"" ""%1"""; Tasks: contextmenu
; .tiff
Root: HKCU; Subkey: "Software\Classes\SystemFileAssociations\.tiff\shell\MFmpeg"; ValueType: string; ValueName: ""; ValueData: "Open sequence in MFmpeg"; Flags: uninsdeletekey; Tasks: contextmenu
Root: HKCU; Subkey: "Software\Classes\SystemFileAssociations\.tiff\shell\MFmpeg"; ValueType: string; ValueName: "Icon"; ValueData: """{app}\{#MyAppExeName}"""; Tasks: contextmenu
Root: HKCU; Subkey: "Software\Classes\SystemFileAssociations\.tiff\shell\MFmpeg\command"; ValueType: string; ValueName: ""; ValueData: """{app}\{#MyAppExeName}"" ""%1"""; Tasks: contextmenu
; .exr
Root: HKCU; Subkey: "Software\Classes\SystemFileAssociations\.exr\shell\MFmpeg"; ValueType: string; ValueName: ""; ValueData: "Open sequence in MFmpeg"; Flags: uninsdeletekey; Tasks: contextmenu
Root: HKCU; Subkey: "Software\Classes\SystemFileAssociations\.exr\shell\MFmpeg"; ValueType: string; ValueName: "Icon"; ValueData: """{app}\{#MyAppExeName}"""; Tasks: contextmenu
Root: HKCU; Subkey: "Software\Classes\SystemFileAssociations\.exr\shell\MFmpeg\command"; ValueType: string; ValueName: ""; ValueData: """{app}\{#MyAppExeName}"" ""%1"""; Tasks: contextmenu
; .tga
Root: HKCU; Subkey: "Software\Classes\SystemFileAssociations\.tga\shell\MFmpeg"; ValueType: string; ValueName: ""; ValueData: "Open sequence in MFmpeg"; Flags: uninsdeletekey; Tasks: contextmenu
Root: HKCU; Subkey: "Software\Classes\SystemFileAssociations\.tga\shell\MFmpeg"; ValueType: string; ValueName: "Icon"; ValueData: """{app}\{#MyAppExeName}"""; Tasks: contextmenu
Root: HKCU; Subkey: "Software\Classes\SystemFileAssociations\.tga\shell\MFmpeg\command"; ValueType: string; ValueName: ""; ValueData: """{app}\{#MyAppExeName}"" ""%1"""; Tasks: contextmenu

; Store install path
Root: HKCU; Subkey: "Software\MFmpeg"; ValueType: string; ValueName: "InstallPath"; ValueData: "{app}"; Flags: uninsdeletekey

[Run]
Filename: "{app}\{#MyAppExeName}"; Description: "Launch {#MyAppName}"; Flags: nowait postinstall skipifsilent
