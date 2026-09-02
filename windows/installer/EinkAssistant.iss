#ifndef AppVersion
  #define AppVersion "1.0"
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
CloseApplications=yes
RestartApplications=no
AppMutex=Local\EinkAssistant.SingleInstance.v1
Compression=lzma2/ultra64
SolidCompression=yes
WizardStyle=modern
SetupLogging=yes
VersionInfoVersion=1.0.0.0
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
