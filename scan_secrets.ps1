$files = Get-ChildItem "C:\XiaoQinTools\src","C:\XiaoQinTools\qml","C:\XiaoQinTools\installer" -Recurse -Include *.cpp,*.h,*.qml,*.iss
"总文件数: $($files.Count)"

$patterns = @(
    'sk-[a-zA-Z0-9]{20,}',
    'ghp_[a-zA-Z0-9]{30,}',
    'github_pat_[a-zA-Z0-9_]{30,}',
    'gitee.*token.*=.*[''"]\S{10,}',
    'api.?key.*=.*[''"]\S{20,}'
)

$found = @()
foreach ($f in $files) {
    $c = [System.IO.File]::ReadAllText($f.FullName, [System.Text.Encoding]::UTF8)
    foreach ($p in $patterns) {
        if ($c -match $p) {
            $m = [regex]::Match($c, $p)
            $found += "$($f.FullName.Replace('C:\XiaoQinTools\','')) :: $($m.Value.Substring(0, [Math]::Min(40, $m.Value.Length)))"
            break
        }
    }
}
if ($found.Count -eq 0) { "OK: 无任何硬编码密钥" } else { $found }
