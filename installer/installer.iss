; Sekiro Mod Manager - Inno Setup Installer Script
; Defines can be passed from command line via /DMyAppVersion=v0.3.3 /DSourceDir=... /DOutputDir=...

#ifndef MyAppVersion
  #define MyAppVersion "v0.3.3"
#endif

#ifndef SourceDir
  #define SourceDir "dist\sekiro-mod-manager-gui"
#endif

#ifndef OutputDir
  #define OutputDir "dist"
#endif

[Setup]
AppId={{5C1D84B9-864B-4E38-A39B-89E62DF5BC17}
AppName=Sekiro Mod Manager
AppVersion={#MyAppVersion}
AppVerName=Sekiro Mod Manager {#MyAppVersion}
AppPublisher=RoL1n_SrP
AppPublisherURL=https://github.com/RolinShmily/sekiro-mod-manager
AppSupportURL=https://github.com/RolinShmily/sekiro-mod-manager/issues
AppUpdatesURL=https://github.com/RolinShmily/sekiro-mod-manager/releases
DefaultDirName={autopf}\SekiroModManager
DefaultGroupName=Sekiro Mod Manager
AllowNoIcons=yes
OutputDir={#OutputDir}
OutputBaseFilename=sekiro-mod-manager-{#MyAppVersion}-windows-x64-gui-setup
SetupIconFile=..\src\gui\resources\images\sekiro.ico
UninstallDisplayIcon={app}\SekiroModManager.exe
Compression=lzma2/max
SolidCompression=yes
WizardStyle=modern
ArchitecturesInstallIn64BitMode=x64
PrivilegesRequired=lowest
PrivilegesRequiredOverridesAllowed=dialog

[Languages]
Name: "chinesesimplified"; MessagesFile: "ChineseSimplified.isl"
Name: "english"; MessagesFile: "compiler:Default.isl"

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"

[Files]
Source: "{#SourceDir}\*"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs createallsubdirs

[Icons]
Name: "{group}\Sekiro Mod Manager"; Filename: "{app}\SekiroModManager.exe"
Name: "{group}\{cm:UninstallProgram,Sekiro Mod Manager}"; Filename: "{uninstallexe}"
Name: "{autodesktop}\Sekiro Mod Manager"; Filename: "{app}\SekiroModManager.exe"; Tasks: desktopicon

[Run]
Filename: "{app}\SekiroModManager.exe"; Description: "{cm:LaunchProgram,Sekiro Mod Manager}"; Flags: nowait postinstall skipifsilent
