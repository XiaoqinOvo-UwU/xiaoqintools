$ErrorActionPreference = "Stop"
$exe = Join-Path $PSScriptRoot "XiaoQinTools.exe"
$desktop = [Environment]::GetFolderPath("Desktop")
$lnk = Join-Path $desktop "小钦的工具.lnk"
$ws = New-Object -ComObject WScript.Shell
$s = $ws.CreateShortcut($lnk)
$s.TargetPath = $exe
$s.WorkingDirectory = $PSScriptRoot
$s.Description = "小钦的工具 v3.0.0"
$s.Save()
Write-Host "快捷方式已创建: $lnk"
