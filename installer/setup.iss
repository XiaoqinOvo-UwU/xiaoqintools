; 小钦的工具 v3.1.3 安装脚本
#define MyAppName "小钦的工具"
#define MyAppVersion "3.1.3"
#define MyAppExeName "XiaoQinTools.exe"
#define MyAppPublisher "XiaoQinUwU"
#define MyAppURL "https://github.com/xiaoqinnb666/xiaoqintools"

[Setup]
AppId={{8F5C3E2A-1B4D-4E7C-9A2B-XIAOQIN001}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppPublisher={#MyAppPublisher}
AppPublisherURL={#MyAppURL}
DefaultDirName={autopf}\XiaoQinTools
DefaultGroupName={#MyAppName}
DisableProgramGroupPage=yes
OutputDir=C:\XiaoQinTools\installer
OutputBaseFilename=XiaoQinTools-{#MyAppVersion}-setup
Compression=lzma2
SolidCompression=yes
WizardStyle=modern
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
; 安装后可卸载，但保留 %APPDATA% 数据
UninstallDisplayIcon={app}\{#MyAppExeName}

[Languages]
Name: "chinesesimplified"; MessagesFile: "..\ChineseSimplified.isl"

[Tasks]
Name: "desktopicon"; Description: "创建桌面快捷方式"; GroupDescription: "附加任务:"; Flags: unchecked
Name: "startup"; Description: "开机自启动"; GroupDescription: "附加任务:"; Flags: unchecked

[Files]
Source: "C:\XiaoQinTools\dist\*"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs createallsubdirs

[Icons]
Name: "{group}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"
Name: "{autodesktop}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"; Tasks: desktopicon

[Run]
Filename: "{app}\{#MyAppExeName}"; Description: "启动 {#MyAppName}"; Flags: nowait postinstall skipifsilent
