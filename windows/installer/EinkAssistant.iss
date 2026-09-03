#ifndef AppVersion
  #define AppVersion "1.1"
#endif

#ifndef SourceDir
  #error SourceDir must point to the deployable Windows application directory.
#endif

#ifndef OutputDir
  #error OutputDir must point to the installer artifact directory.
#endif

#ifndef OutputBaseFilename
  #define OutputBaseFilename "E-Ink-Assistant-Windows-Setup"
#endif

#ifndef ChineseSimplifiedIsl
  #error ChineseSimplifiedIsl must point to the pinned Simplified Chinese Inno Setup translation.
#endif

#ifndef ChineseTraditionalIsl
  #error ChineseTraditionalIsl must point to the pinned Traditional Chinese Inno Setup translation.
#endif

#define AppName "E-Ink Assistant"
#define AppExeName "EinkAssistant.exe"
#define AppPublisher "Bozhen Peng"
#define AppUrl "https://github.com/kiteretsu903/eink-assistant"

[Setup]
AppId={{5E8B9170-5E8D-43DC-AB42-5EDDFE034E04}
AppName={#AppName}
AppVersion={#AppVersion}
AppVerName={#AppName} {#AppVersion}
AppPublisher={#AppPublisher}
AppPublisherURL={#AppUrl}
AppSupportURL={#AppUrl}/issues
AppUpdatesURL={#AppUrl}/releases
DefaultDirName={autopf}\E-Ink Assistant
DefaultGroupName=E-Ink Assistant
DisableProgramGroupPage=yes
LicenseFile=..\..\LICENSE
OutputDir={#OutputDir}
OutputBaseFilename={#OutputBaseFilename}
SetupIconFile=..\..\Resources\AppIcon.ico
UninstallDisplayIcon={app}\{#AppExeName}
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
MinVersion=6.1sp1
PrivilegesRequired=admin
CloseApplications=force
RestartApplications=no
Compression=lzma2/ultra64
SolidCompression=yes
WizardStyle=modern dynamic excludelightcontrols hidebevels
WizardSizePercent=115
WizardSmallImageFile=..\..\Resources\AppIcon.png
WizardSmallImageFileDynamicDark=..\..\Resources\AppIcon.png
SetupLogging=yes
VersionInfoVersion=1.1.0.0
VersionInfoProductName={#AppName}
VersionInfoProductVersion={#AppVersion}
VersionInfoCompany={#AppPublisher}
VersionInfoDescription={#AppName} {#AppVersion} Setup

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"
Name: "chinesesimplified"; MessagesFile: "{#ChineseSimplifiedIsl}"
Name: "chinesetraditional"; MessagesFile: "{#ChineseTraditionalIsl}"
Name: "japanese"; MessagesFile: "compiler:Languages\Japanese.isl"

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked

[Files]
Source: "{#SourceDir}\*"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs createallsubdirs

[Icons]
Name: "{autoprograms}\E-Ink Assistant"; Filename: "{app}\{#AppExeName}"; WorkingDir: "{app}"
Name: "{autodesktop}\E-Ink Assistant"; Filename: "{app}\{#AppExeName}"; WorkingDir: "{app}"; Tasks: desktopicon

[Run]
Filename: "{app}\{#AppExeName}"; Description: "{cm:LaunchProgram,{#StringChange(AppName, '&', '&&')}}"; WorkingDir: "{app}"; Flags: nowait postinstall skipifsilent runascurrentuser

[Code]
const
  EVENT_MODIFY_STATE = $0002;
  GracefulExitWaitIterations = 80;

function OpenEvent(DesiredAccess: DWORD; InheritHandle: BOOL; Name: String): THandle;
  external 'OpenEventW@kernel32.dll stdcall';
function SetEvent(EventHandle: THandle): BOOL;
  external 'SetEvent@kernel32.dll stdcall';
function CloseHandle(Handle: THandle): BOOL;
  external 'CloseHandle@kernel32.dll stdcall';

function RequestGracefulAppExit: Boolean;
var
  ExitEvent: THandle;
begin
  Result := False;
  ExitEvent := OpenEvent(EVENT_MODIFY_STATE, False, 'Local\EinkAssistant.QuitInstance.v1');
  if ExitEvent <> 0 then
  begin
    Result := SetEvent(ExitEvent);
    CloseHandle(ExitEvent);
    if Result then
      Log('Requested a graceful E-Ink Assistant shutdown before updating.');
  end;
end;

function PrepareToInstall(var NeedsRestart: Boolean): String;
var
  Attempt: Integer;
begin
  Result := '';
  if not RequestGracefulAppExit then
    Exit;

  for Attempt := 1 to GracefulExitWaitIterations do
  begin
    if not CheckForMutexes('Local\EinkAssistant.SingleInstance.v1') then
    begin
      Log('E-Ink Assistant completed its graceful shutdown.');
      Exit;
    end;
    Sleep(100);
  end;

  Log('Graceful shutdown timed out; Restart Manager will offer the force-close option.');
end;
