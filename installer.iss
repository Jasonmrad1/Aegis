[Setup]
AppId={{B9A62B1E-3C6F-4D35-9C3C-BC2C2A819E2A}}
AppName=Aegis
#define MyAppVersion GetFileVersion("build\\Release\\Aegis.exe")
AppVersion={#MyAppVersion}
AppPublisher=Aegis
DefaultDirName={autopf}\Aegis
DefaultGroupName=Aegis
DisableProgramGroupPage=no
OutputDir=dist
OutputBaseFilename=AegisSetup
Compression=lzma
SolidCompression=yes
PrivilegesRequired=admin
UninstallDisplayIcon={app}\Aegis.exe
ChangesEnvironment=yes

[Files]
Source: "build\Release\Aegis.exe"; DestDir: "{app}"; Flags: ignoreversion
Source: "build\Release\*.dll"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs createallsubdirs skipifsourcedoesntexist

[Icons]
Name: "{group}\Aegis"; Filename: "{app}\Aegis.exe"

[Registry]
Root: HKLM; Subkey: "SYSTEM\CurrentControlSet\Control\Session Manager\Environment"; ValueType: expandsz; ValueName: "Path"; ValueData: "{olddata};{app}"; Check: NeedsAddPath; Flags: preservestringtype

[Run]
Filename: "{app}\Aegis.exe"; Description: "Launch Aegis"; Flags: nowait postinstall skipifsilent

[Code]
function NeedsAddPath(): Boolean;
var
  Path: string;
begin
  if not RegQueryStringValue(HKLM, 'SYSTEM\CurrentControlSet\Control\Session Manager\Environment', 'Path', Path) then
  begin
    Result := true;
  end
  else
  begin
    Result := Pos(Uppercase(ExpandConstant('{app}')), Uppercase(Path)) = 0;
  end;
end;

procedure RemovePath();
var
  Path: string;
  AppPath: string;
begin
  AppPath := ExpandConstant('{app}');
  if not RegQueryStringValue(HKLM, 'SYSTEM\CurrentControlSet\Control\Session Manager\Environment', 'Path', Path) then
  begin
    exit;
  end;

  StringChangeEx(Path, AppPath + ';', '', True);
  StringChangeEx(Path, ';' + AppPath, '', True);
  StringChangeEx(Path, AppPath, '', True);

  RegWriteExpandStringValue(HKLM, 'SYSTEM\CurrentControlSet\Control\Session Manager\Environment', 'Path', Path);
end;

procedure CurUninstallStepChanged(CurUninstallStep: TUninstallStep);
begin
  if CurUninstallStep = usUninstall then
  begin
    RemovePath();
  end;
end;
