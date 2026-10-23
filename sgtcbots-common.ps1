# Shared helpers for install.ps1 / uninstall.ps1 (StargateTC bots + HL25 fixes).
# Works with Windows PowerShell 5.1 and PowerShell 7. Keep this file ASCII-only.

$script:BakExt = '.sgtcbots.bak'
$script:PrevExt = '.sgtcbots.prev'
$script:ManifestName = 'sgtcbots_manifest.json'
# ISO-8859-1 maps every byte to one char and back, so text edits keep all other bytes as they are.
$script:Latin1 = [System.Text.Encoding]::GetEncoding(28591)

function Write-Step([string]$Message) { Write-Host "  $Message" }
function Write-Warn([string]$Message) { Write-Host "  WARNING: $Message" -ForegroundColor Yellow }

function Get-PackageVersion([string]$Root) {
    $f = Join-Path $Root 'VERSION'
    if (Test-Path -LiteralPath $f) { return ([System.IO.File]::ReadAllText($f)).Trim() }
    return 'unknown'
}

function Read-Text([string]$Path) { return $script:Latin1.GetString([System.IO.File]::ReadAllBytes($Path)) }
function Write-Text([string]$Path, [string]$Text) { [System.IO.File]::WriteAllBytes($Path, $script:Latin1.GetBytes($Text)) }
# Not Get-FileHash: in Windows PowerShell 5.1 it is a script function that fails to autoload
# when powershell.exe inherits PowerShell 7's PSModulePath (install.bat run from a pwsh prompt).
function Get-Sha256([string]$Path) {
    $sha = [System.Security.Cryptography.SHA256]::Create()
    $fs = [System.IO.File]::OpenRead($Path)
    try { return [System.BitConverter]::ToString($sha.ComputeHash($fs)).Replace('-', '') }
    finally { $fs.Dispose(); $sha.Dispose() }
}

function Get-SteamLibraries {
    $roots = New-Object System.Collections.Generic.List[string]
    $keys = @(
        @('HKLM:\SOFTWARE\WOW6432Node\Valve\Steam', 'InstallPath'),
        @('HKLM:\SOFTWARE\Valve\Steam', 'InstallPath'),
        @('HKCU:\Software\Valve\Steam', 'SteamPath')      # lower-cased by Steam, so last
    )
    foreach ($k in $keys) {
        try {
            $item = Get-ItemProperty -Path $k[0] -ErrorAction Stop
            $prop = $item.PSObject.Properties[$k[1]]
            if ($prop -and $prop.Value) { $roots.Add(([string]$prop.Value).Replace('/', '\')) }
        } catch { }
    }
    $libs = New-Object System.Collections.Generic.List[string]
    foreach ($r in $roots) {
        $libs.Add($r)
        foreach ($vdf in @((Join-Path $r 'steamapps\libraryfolders.vdf'), (Join-Path $r 'config\libraryfolders.vdf'))) {
            if (-not (Test-Path -LiteralPath $vdf)) { continue }
            foreach ($m in [regex]::Matches((Read-Text $vdf), '"path"\s+"([^"]+)"')) {
                $libs.Add($m.Groups[1].Value.Replace('\\', '\'))
            }
        }
    }
    $seen = @{}
    $out = New-Object System.Collections.Generic.List[string]
    foreach ($l in $libs) {
        $key = $l.TrimEnd('\').ToLowerInvariant()
        if (-not $seen.ContainsKey($key)) { $seen[$key] = $true; $out.Add($l.TrimEnd('\')) }
    }
    return ,$out
}

function Test-SgtcDir([string]$Dir) {
    if (-not $Dir) { return $false }
    $lib = Join-Path $Dir 'Liblist.gam'
    if (-not (Test-Path -LiteralPath $lib)) { return $false }
    return [bool]((Read-Text $lib) -match '(?im)^\s*game\s+"StargateTC')
}

# The stargatetc folder: -GameDir (the folder itself or Half-Life), else every Steam library.
function Resolve-SgtcDir([string]$GameDir) {
    $cands = New-Object System.Collections.Generic.List[string]
    if ($GameDir) {
        $cands.Add($GameDir)
        $cands.Add((Join-Path $GameDir 'stargatetc'))
    } else {
        foreach ($lib in (Get-SteamLibraries)) {
            $cands.Add([System.IO.Path]::Combine($lib, 'steamapps', 'common', 'Half-Life', 'stargatetc'))
        }
    }
    foreach ($c in $cands) {
        if (Test-SgtcDir $c) { return (Resolve-Path -LiteralPath $c).Path }
    }
    if ($GameDir) { throw "No StargateTC install found in '$GameDir' (looked for Liblist.gam with game ""StargateTC..."")." }
    throw 'StargateTC was not found in any Steam library. Run again with -GameDir "<...>\Half-Life\stargatetc".'
}

# Refuse to touch files while this Half-Life copy runs (the DLLs would be locked).
function Assert-GameNotRunning([string]$Sgtc, [switch]$Force) {
    $hlRoot = (Split-Path -Parent $Sgtc).TrimEnd('\').ToLowerInvariant()
    foreach ($p in @(Get-Process -Name 'hl', 'hlds' -ErrorAction SilentlyContinue)) {
        $exe = $null
        try { $exe = $p.Path } catch { }
        if ($exe -and ((Split-Path -Parent $exe).TrimEnd('\').ToLowerInvariant() -ne $hlRoot)) { continue }
        $msg = "$($p.ProcessName).exe (pid $($p.Id)) is running from this Half-Life folder. Quit the game/server first."
        if ($Force) { Write-Warn "$msg Continuing because of -Force." } else { throw $msg }
    }
}

function New-Manifest([string]$Version) {
    return @{
        tool        = 'sgtc-bots'
        version     = $Version
        installedAt = (Get-Date).ToUniversalTime().ToString('yyyy-MM-ddTHH:mm:ssZ')
        packWeapons = $false
        created     = New-Object System.Collections.Generic.List[string]
        createdDirs = New-Object System.Collections.Generic.List[string]
        backups     = New-Object System.Collections.Generic.List[string]
        written     = @{}
    }
}

function Read-Manifest([string]$Sgtc) {
    $path = Join-Path $Sgtc $script:ManifestName
    if (-not (Test-Path -LiteralPath $path)) { return $null }
    $j = [System.IO.File]::ReadAllText($path) | ConvertFrom-Json
    $m = New-Manifest $j.version
    $m.installedAt = $j.installedAt
    $m.packWeapons = [bool]$j.packWeapons
    foreach ($x in @($j.created)) { if ($x) { $m.created.Add($x) } }
    foreach ($x in @($j.createdDirs)) { if ($x) { $m.createdDirs.Add($x) } }
    foreach ($x in @($j.backups)) { if ($x) { $m.backups.Add($x) } }
    if ($j.written) { foreach ($p in $j.written.PSObject.Properties) { $m.written[$p.Name] = $p.Value } }
    return $m
}

function Save-Manifest([string]$Sgtc, $M) {
    $written = [ordered]@{}
    foreach ($k in ($M.written.Keys | Sort-Object)) { $written[$k] = $M.written[$k] }
    $obj = [ordered]@{
        tool        = $M.tool
        version     = $M.version
        installedAt = $M.installedAt
        packWeapons = $M.packWeapons
        created     = @($M.created | Sort-Object)
        createdDirs = @($M.createdDirs | Sort-Object)
        backups     = @($M.backups | Sort-Object)
        written     = $written
    }
    $json = ConvertTo-Json -InputObject $obj -Depth 5
    [System.IO.File]::WriteAllText((Join-Path $Sgtc $script:ManifestName), $json + "`r`n", (New-Object System.Text.UTF8Encoding $false))
}
