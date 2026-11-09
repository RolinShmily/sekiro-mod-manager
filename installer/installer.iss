; Sekiro Mod Manager - Inno Setup Installer Script
; Defines can be passed from command line via /DMyAppVersion=v0.4.5 /DSourceDir=... /DOutputDir=...

#ifndef MyAppVersion
  #define MyAppVersion "v0.4.5"
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
OutputBaseFilename=sekiro-mod-manager-{#MyAppVersion}-windows-x64-setup
SetupIconFile=..\src\gui\resources\images\sekiro.ico
UninstallDisplayIcon={app}\SekiroModManager.exe
Compression=lzma2/max
SolidCompression=yes
WizardStyle=modern
ArchitecturesInstallIn64BitMode=x64
PrivilegesRequired=lowest
PrivilegesRequiredOverridesAllowed=dialog
; The CLI is installed next to the GUI; announce PATH edits to the shell.
ChangesEnvironment=yes

[Languages]
Name: "chinesesimplified"; MessagesFile: "ChineseSimplified.isl"
Name: "english"; MessagesFile: "compiler:Default.isl"

[CustomMessages]
AddToPath=Add SMM to the user PATH (run smm from any terminal)
CommandLineGroup=Command line
chinesesimplified.AddToPath=将 SMM 加入当前用户的 PATH（在任意终端直接运行 smm）
chinesesimplified.CommandLineGroup=命令行

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"
Name: "addtopath"; Description: "{cm:AddToPath}"; GroupDescription: "{cm:CommandLineGroup}"; Flags: checkedonce

[Files]
Source: "{#SourceDir}\*"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs createallsubdirs

[Icons]
Name: "{group}\Sekiro Mod Manager"; Filename: "{app}\SekiroModManager.exe"
Name: "{group}\{cm:UninstallProgram,Sekiro Mod Manager}"; Filename: "{uninstallexe}"
Name: "{autodesktop}\Sekiro Mod Manager"; Filename: "{app}\SekiroModManager.exe"; Tasks: desktopicon

[Run]
Filename: "{app}\SekiroModManager.exe"; Description: "{cm:LaunchProgram,Sekiro Mod Manager}"; Flags: nowait postinstall skipifsilent

[Code]
// The installer may add the install directory to the *user* PATH so `smm` works from a
// terminal. Both directions are handled here rather than with [Registry] entries: Inno would
// restore the value it saved at install time, which silently discards PATH entries the user
// adds afterwards. This only ever appends (install) or drops (uninstall) our own directory and
// leaves every other entry, and its order, untouched.
const
  UserEnvironmentKey = 'Environment';

function ReadUserPath(): string;
begin
  if not RegQueryStringValue(HKEY_CURRENT_USER, UserEnvironmentKey, 'Path', Result) then
    Result := '';
end;

function PathHasDir(const Paths, Dir: string): Boolean;
begin
  Result := Pos(';' + Uppercase(Dir) + ';', ';' + Uppercase(Paths) + ';') > 0;
end;

procedure WriteUserPath(const Value: string);
begin
  if Value = '' then
    RegDeleteValue(HKEY_CURRENT_USER, UserEnvironmentKey, 'Path')
  else
    RegWriteExpandStringValue(HKEY_CURRENT_USER, UserEnvironmentKey, 'Path', Value);
end;

procedure AddToUserPath(const Dir: string);
var
  Current: string;
begin
  Current := ReadUserPath();
  if PathHasDir(Current, Dir) then
    Exit;
  if (Current <> '') and (Current[Length(Current)] <> ';') then
    Current := Current + ';';
  WriteUserPath(Current + Dir);
end;

procedure RemoveFromUserPath(const Dir: string);
var
  Current, Updated, Part: string;
  I, Start: Integer;
begin
  Current := ReadUserPath();
  if not PathHasDir(Current, Dir) then
    Exit;

  Updated := '';
  Start := 1;
  for I := 1 to Length(Current) + 1 do
  begin
    if (I > Length(Current)) or (Current[I] = ';') then
    begin
      Part := Trim(Copy(Current, Start, I - Start));
      Start := I + 1;
      if (Part <> '') and (Uppercase(Part) <> Uppercase(Dir)) then
      begin
        if Updated <> '' then
          Updated := Updated + ';';
        Updated := Updated + Part;
      end;
    end;
  end;
  WriteUserPath(Updated);
end;

procedure CurStepChanged(CurStep: TSetupStep);
begin
  if (CurStep = ssPostInstall) and WizardIsTaskSelected('addtopath') then
    AddToUserPath(ExpandConstant('{app}'));
end;

procedure CurUninstallStepChanged(CurUninstallStep: TUninstallStep);
begin
  if CurUninstallStep = usPostUninstall then
    RemoveFromUserPath(ExpandConstant('{app}'));
end;
