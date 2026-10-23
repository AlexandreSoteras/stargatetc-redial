<#
.SYNOPSIS
    Removes the StargateTC bots and puts back every file install.ps1 changed.

.DESCRIPTION
    Uses stargatetc\sgtcbots_manifest.json, written by install.ps1:
      - every backed-up file is restored from <file>.sgtcbots.bak. If the file changed since
        the install (the engine rewrites config.cfg on every exit), the current copy is kept
        as <file>.sgtcbots.prev first;
      - every file the installer created is deleted, then the folders it created if empty.
        addons\jk_botti is deleted whole when the installer created it (jk_botti writes its
        own waypoint and route files there).

    That includes commandmenu.txt, whose original (without the "Bots" submenu) is in
    commandmenu.txt.sgtcbots.bak.

    Restoring config.cfg and Titles.txt brings back the two HL25 problems (startup crash,
    assertion when joining a team). Use -KeepFixes to remove only the bots.

.PARAMETER GameDir
    The stargatetc folder, or the Half-Life folder that contains it.
.PARAMETER KeepFixes
    Keep the HL25 fixes (config.cfg, autoexec.cfg, userconfig.cfg, Titles.txt). Everything
    else, the Bots submenu in commandmenu.txt included, is still removed.
.PARAMETER Force
    Run even if the game or a server runs from this Half-Life folder.
#>
[CmdletBinding()]
param(
    [string]$GameDir,
    [switch]$KeepFixes,
    [switch]$Force
)

Set-StrictMode -Version 2
$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot 'sgtcbots-common.ps1')

$Sgtc = Resolve-SgtcDir $GameDir
Write-Host "Uninstalling StargateTC: Redial from: $Sgtc"
Assert-GameNotRunning $Sgtc -Force:$Force

$M = Read-Manifest $Sgtc
if (-not $M) {
    # No manifest: restore whatever backups exist, remove nothing else.
    Write-Warn "No $ManifestName found; restoring the *$BakExt files only."
    $M = New-Manifest 'unknown'
    foreach ($b in Get-ChildItem -LiteralPath $Sgtc -Recurse -File -Filter "*$BakExt") {
        $M.backups.Add($b.FullName.Substring($Sgtc.Length + 1, $b.FullName.Length - $Sgtc.Length - 1 - $BakExt.Length))
    }
}

$fixes = @('config.cfg', 'autoexec.cfg', 'userconfig.cfg', 'Titles.txt')
$n = 0
foreach ($rel in ($M.backups | Sort-Object)) {
    $full = Join-Path $Sgtc $rel
    $bak = $full + $BakExt
    if ($KeepFixes -and ($fixes -contains $rel)) { Write-Step "${rel}: kept (HL25 fix), original still in $rel$BakExt"; continue }
    if (-not (Test-Path -LiteralPath $bak)) { Write-Warn "$rel$BakExt is missing; $rel left as it is."; continue }
    if ((Test-Path -LiteralPath $full) -and $M.written.ContainsKey($rel) -and ((Get-Sha256 $full) -ne $M.written[$rel])) {
        Copy-Item -LiteralPath $full -Destination ($full + $PrevExt) -Force
        Write-Step "${rel}: changed since the install, current copy saved as $rel$PrevExt"
    }
    Move-Item -LiteralPath $bak -Destination $full -Force
    Write-Step "${rel}: restored"
    $n++
}

foreach ($rel in ($M.created | Sort-Object -Descending)) {
    $full = Join-Path $Sgtc $rel
    if (Test-Path -LiteralPath $full) { Remove-Item -LiteralPath $full -Force; Write-Step "${rel}: removed"; $n++ }
}

foreach ($rel in ($M.createdDirs | Sort-Object -Descending)) {
    $full = Join-Path $Sgtc $rel
    if (-not (Test-Path -LiteralPath $full)) { continue }
    if ($rel -ieq 'addons\jk_botti') {
        Remove-Item -LiteralPath $full -Recurse -Force
        Write-Step "${rel}\: removed (with the waypoint/route files jk_botti wrote)"
    } elseif (-not (Get-ChildItem -LiteralPath $full -Force | Select-Object -First 1)) {
        Remove-Item -LiteralPath $full -Force
    } else {
        Write-Step "${rel}\: not empty, left in place"
    }
}

$manifest = Join-Path $Sgtc $ManifestName
if (Test-Path -LiteralPath $manifest) { Remove-Item -LiteralPath $manifest -Force }
Write-Host "Done: $n file(s) restored or removed."
