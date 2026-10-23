<#
.SYNOPSIS
    StargateTC: Redial. Installs the StargateTC bots (jk_botti SGTC port, loaded by Metamod-P)
    and the Half-Life 25th Anniversary fixes into an existing StargateTC 2.0 install.

.DESCRIPTION
    1. Finds Half-Life\stargatetc (-GameDir, else the Steam registry + libraryfolders.vdf).
    2. Backs up every file it changes to <file>.sgtcbots.bak (only the first time).
    3. config.cfg (+ autoexec.cfg, userconfig.cfg): removes any key bound to +portrait
       (HL25 crashes at startup on it). Line endings are kept.
    4. Titles.txt: CRLF -> LF (HL25 keeps the CR and asserts when you join a team).
    5. Liblist.gam: gamedll -> addons/metamod/dlls/metamod.dll; the original game DLL
       goes into addons/metamod/config.ini. plugins.ini loads jk_botti (an old HPB_bot
       line is commented out).
    6. Copies the payload: addons\jk_botti, addons\metamod\dlls\metamod.dll and the
       stamped menu background tile(s) in resource\background.
    7. commandmenu.txt: adds a "Bots" submenu to the "Server Admin" menu (before "Status")
       and to the "Rcon" menu (before "Set Tickets"). Each one is added only if missing.
    8. With -PackWeapons: mp_wepselmode 2 (class packs) in server.cfg and listenserver.cfg.

    Running it again is safe: what is already done is left alone, and the first backups
    (the originals) are never overwritten. uninstall.ps1 puts everything back.

.PARAMETER GameDir
    The stargatetc folder, or the Half-Life folder that contains it.
.PARAMETER PackWeapons
    Also set mp_wepselmode 2 (class weapon packs) in both server cfgs.
.PARAMETER MetamodZip
    metamod-p-1.19p32-windows.zip, if the package has no addons\metamod\dlls\metamod.dll.
.PARAMETER DownloadMetamod
    Download that zip from SourceForge. Its metamod.dll is checked against a known SHA-256.
.PARAMETER PackageDir
    Folder that holds the payload (addons\, resource\). Default: .\payload next to this script.
.PARAMETER DetectOnly
    Only print the StargateTC folder that would be used.
.PARAMETER Force
    Overwrite your jk_botti.cfg and waypoints, replace another Metamod build, run while the game runs.
#>
[CmdletBinding()]
param(
    [string]$GameDir,
    [switch]$PackWeapons,
    [string]$MetamodZip,
    [switch]$DownloadMetamod,
    [string]$PackageDir,
    [switch]$DetectOnly,
    [switch]$Force
)

Set-StrictMode -Version 2
$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot 'sgtcbots-common.ps1')

$MetamodDllSha256 = '382B8FCBEFFD556C34971E945857B3A88105C423FB9F2BE222E395E0491C478F'   # Metamod-P 1.19p32 win32
$MetamodZipUrl = 'https://sourceforge.net/projects/metamod-p/files/metamod-p-1.19p32-windows.zip/download'
$MetamodRel = 'addons\metamod\dlls\metamod.dll'
$MetamodGamedll = 'addons/metamod/dlls/metamod.dll'
$PluginLine = 'win32 addons/jk_botti/dlls/jk_botti_mm.dll'

# The in-game "Bots" submenu for commandmenu.txt. <rcon> becomes '' in "Server Admin" (listen
# server host) and 'rcon ' in "Rcon" (dedicated server). Indented with spaces like the menu around it.
$BotsMenu = @(
    '      " " "Bots"'
    '     {'
    '        " " "Add a bot" "<rcon>jk_botti addbot"'
    '        " " "Auto: 4 bots, room for humans (10 players)" "<rcon>jk_botti min_bots 4; <rcon>jk_botti max_bots 10"'
    '        " " "Auto: 8 bots, room for humans (12 players)" "<rcon>jk_botti min_bots 8; <rcon>jk_botti max_bots 12"'
    '        " " "Remove all bots" "<rcon>jk_botti min_bots 0; <rcon>jk_botti max_bots 0"'
    '        " " "Skill for new bots: Best" "<rcon>jk_botti botskill 1"'
    '        " " "Skill for new bots: Good" "<rcon>jk_botti botskill 2"'
    '        " " "Skill for new bots: Normal" "<rcon>jk_botti botskill 3"'
    '        " " "Skill for new bots: Easy" "<rcon>jk_botti botskill 4"'
    '        " " "Bot chat on" "<rcon>jk_botti bot_chat_percent 10; <rcon>jk_botti bot_taunt_percent 20; <rcon>jk_botti bot_whine_percent 10"'
    '        " " "Bot chat off" "<rcon>jk_botti bot_chat_percent 0; <rcon>jk_botti bot_taunt_percent 0; <rcon>jk_botti bot_whine_percent 0"'
    '     }'
)
# Where each copy goes: the submenu is inserted just before the first line equal to Anchor.
$BotsMenuSpots = @(
    @{ Menu = 'Server Admin'; Prefix = '';      Anchor = '" " "Status" "status"' },
    @{ Menu = 'Rcon';         Prefix = 'rcon '; Anchor = '" " "Set Tickets" "messagemode rcon mp_tickets"' }
)

if (-not $PackageDir) { $PackageDir = Join-Path $PSScriptRoot 'payload' }
if (-not (Test-Path -LiteralPath $PackageDir)) { throw "Payload folder not found: $PackageDir" }
$PackageDir = (Resolve-Path -LiteralPath $PackageDir).Path.TrimEnd('\')
$Version = Get-PackageVersion $PSScriptRoot

$Sgtc = Resolve-SgtcDir $GameDir
if ($DetectOnly) { Write-Output $Sgtc; return }

Write-Host "StargateTC: Redial $Version (HL25 fixes + bots)"
Write-Host "Installing into: $Sgtc"
Assert-GameNotRunning $Sgtc -Force:$Force

$M = Read-Manifest $Sgtc
if (-not $M) { $M = New-Manifest $Version }
$M.version = $Version
$script:Changed = 0

function Get-Full([string]$Rel) { return Join-Path $Sgtc $Rel }

# Directories are created one level at a time so uninstall knows which ones are ours.
function New-GameDir([string]$RelDir) {
    if (-not $RelDir) { return }
    if (Test-Path -LiteralPath (Get-Full $RelDir)) { return }
    New-GameDir (Split-Path -Parent $RelDir)
    New-Item -ItemType Directory -Path (Get-Full $RelDir) | Out-Null
    if (-not $M.createdDirs.Contains($RelDir)) { $M.createdDirs.Add($RelDir) }
}

# First backup of a file we are about to change (unless we created the file ourselves).
function Backup-GameFile([string]$Rel) {
    if ($M.created.Contains($Rel)) { return }
    $bak = (Get-Full $Rel) + $BakExt
    if (-not (Test-Path -LiteralPath $bak)) {
        Copy-Item -LiteralPath (Get-Full $Rel) -Destination $bak
        Write-Step "backup: $Rel$BakExt"
    }
    if (-not $M.backups.Contains($Rel)) { $M.backups.Add($Rel) }
}

function Set-GameBytes([string]$Rel, [byte[]]$Bytes, [string]$What) {
    $full = Get-Full $Rel
    if (Test-Path -LiteralPath $full) {
        Backup-GameFile $Rel
    } else {
        New-GameDir (Split-Path -Parent $Rel)
        if (-not $M.created.Contains($Rel)) { $M.created.Add($Rel) }
    }
    [System.IO.File]::WriteAllBytes($full, $Bytes)
    $M.written[$Rel] = Get-Sha256 $full
    $script:Changed++
    Write-Step "${Rel}: $What"
}

# Apply a text transform to an existing file; write only if it changes something.
function Edit-GameText([string]$Rel, [scriptblock]$Transform, [string]$What) {
    $full = Get-Full $Rel
    if (-not (Test-Path -LiteralPath $full)) { return $false }
    $old = Read-Text $full
    $new = & $Transform $old
    if ($new -ceq $old) { return $false }
    Set-GameBytes $Rel ($Latin1.GetBytes($new)) $What
    return $true
}

function Get-Eol([string]$Text) { if ($Text.Contains("`r`n") -or -not $Text.Contains("`n")) { return "`r`n" } return "`n" }

function Add-Line([string]$Text, [string]$Line) {
    $eol = Get-Eol $Text
    if ($Text.Length -gt 0 -and -not $Text.EndsWith("`n")) { $Text += $eol }
    return $Text + $Line + $eol
}

# commandmenu.txt with the "Bots" submenu in every menu that lacks it. A menu that already runs
# "<prefix>jk_botti addbot" is left alone; a menu whose anchor line is missing gets a warning.
function Add-BotsMenu([string]$Text) {
    $eol = Get-Eol $Text
    foreach ($spot in $BotsMenuSpots) {
        if ($Text.Contains('"' + $spot.Prefix + 'jk_botti addbot"')) { continue }
        $m = [regex]::Match($Text, '(?m)^[ \t]*' + [regex]::Escape($spot.Anchor) + '[ \t]*\r?$')
        if (-not $m.Success) {
            Write-Warn "commandmenu.txt: no '$($spot.Anchor)' line, so the $($spot.Menu) menu gets no Bots submenu."
            continue
        }
        $block = ($BotsMenu -join $eol).Replace('<rcon>', $spot.Prefix) + $eol
        $Text = $Text.Substring(0, $m.Index) + $block + $Text.Substring($m.Index)
    }
    return $Text
}

# ---- payload check -----------------------------------------------------------------------
$JkDll = Join-Path $PackageDir 'addons\jk_botti\dlls\jk_botti_mm.dll'
if (-not (Test-Path -LiteralPath $JkDll)) {
    throw "Missing $JkDll. Download the release zip (it contains the built DLLs), or build jk_botti_mm.dll (see README)."
}

$MetamodSrc = Join-Path $PackageDir $MetamodRel
$TempFiles = New-Object System.Collections.Generic.List[string]
if (-not (Test-Path -LiteralPath $MetamodSrc)) {
    $MetamodSrc = $null
    if ($DownloadMetamod -and -not $MetamodZip) {
        $MetamodZip = Join-Path ([System.IO.Path]::GetTempPath()) ('metamod-p-1.19p32-windows_' + [guid]::NewGuid().ToString('N') + '.zip')
        $TempFiles.Add($MetamodZip)
        Write-Step "downloading Metamod-P 1.19p32 from SourceForge ..."
        [Net.ServicePointManager]::SecurityProtocol = [Net.ServicePointManager]::SecurityProtocol -bor [Net.SecurityProtocolType]::Tls12
        Invoke-WebRequest -Uri $MetamodZipUrl -OutFile $MetamodZip -UserAgent 'Wget/1.21' -UseBasicParsing
    }
    if ($MetamodZip) {
        Add-Type -AssemblyName System.IO.Compression.FileSystem
        $zip = [System.IO.Compression.ZipFile]::OpenRead($MetamodZip)
        try {
            $entry = $zip.Entries | Where-Object { $_.Name -ieq 'metamod.dll' } | Select-Object -First 1
            if (-not $entry) { throw "No metamod.dll in $MetamodZip" }
            $MetamodSrc = Join-Path ([System.IO.Path]::GetTempPath()) ('sgtcbots_metamod_' + [guid]::NewGuid().ToString('N') + '.dll')
            $TempFiles.Add($MetamodSrc)
            [System.IO.Compression.ZipFileExtensions]::ExtractToFile($entry, $MetamodSrc, $true)
        } finally { $zip.Dispose() }
        if ((Get-Sha256 $MetamodSrc) -ne $MetamodDllSha256) {
            if (-not $Force) { throw "metamod.dll from $MetamodZip is not the Metamod-P 1.19p32 build this was tested with (SHA-256 differs). Use -Force to install it anyway." }
            Write-Warn 'metamod.dll is not the tested Metamod-P 1.19p32 build.'
        }
    } elseif (-not (Test-Path -LiteralPath (Get-Full $MetamodRel))) {
        throw "No Metamod in the package or the game. Use -DownloadMetamod, or -MetamodZip <metamod-p-1.19p32-windows.zip> (http://metamod-p.sourceforge.net/)."
    }
}

try {
    # ---- 1. HL25 fixes ----------------------------------------------------------------------
    foreach ($cfg in @('config.cfg', 'autoexec.cfg', 'userconfig.cfg')) {
        [void](Edit-GameText $cfg {
            param($t)
            [regex]::Replace($t, '(?im)^[ \t]*bind[ \t]+("[^"\r\n]*"|\S+)[ \t]+("\+portrait"|\+portrait)[ \t]*(\r\n|\n|$)', '')
        } 'removed the +portrait bind (HL25 startup crash)')
    }
    [void](Edit-GameText 'Titles.txt' { param($t) $t.Replace("`r`n", "`n") } 'converted to LF line endings (HL25 CR assertion)')

    # ---- 2. Metamod ---------------------------------------------------------------------------
    $lib = Read-Text (Get-Full 'Liblist.gam')
    # .NET's (?m)$ matches before \n only, hence the \r? lookahead for CRLF files
    $active = [regex]::Match($lib, '(?im)^[ \t]*gamedll[ \t]+"([^"\r\n]*)"(?=[ \t]*\r?$)')
    if (-not $active.Success) { throw 'Liblist.gam has no gamedll line.' }
    $origDll = $active.Groups[1].Value
    if ($origDll.Replace('\', '/') -ieq $MetamodGamedll) {
        # Already Metamod (e.g. the 2012 "SGTC Bots" pack): the game DLL is in config.ini or a commented line.
        $c = [regex]::Match($lib, '(?im)^[ \t]*//[ \t]*gamedll[ \t]+"([^"\r\n]*)"')
        $origDll = if ($c.Success) { $c.Groups[1].Value } else { 'dlls/hl.dll' }
    } else {
        [void](Edit-GameText 'Liblist.gam' {
            param($t)
            $eol = Get-Eol $t
            $t.Substring(0, $active.Index) + '//' + $active.Value.TrimStart() + $eol + 'gamedll "' + $MetamodGamedll + '"' + $t.Substring($active.Index + $active.Length)
        } 'gamedll -> Metamod')
    }

    $ini = 'addons\metamod\config.ini'
    if (Test-Path -LiteralPath (Get-Full $ini)) {
        $t = Read-Text (Get-Full $ini)
        if ($t -notmatch '(?im)^[ \t]*gamedll[ \t]+\S') {
            [void](Edit-GameText $ini { param($t) Add-Line $t "gamedll $origDll" } "gamedll $origDll")
        }
    } else {
        Set-GameBytes $ini ($Latin1.GetBytes("gamedll $origDll`r`n")) "created (gamedll $origDll)"
    }

    $plugins = 'addons\metamod\plugins.ini'
    if (Test-Path -LiteralPath (Get-Full $plugins)) {
        [void](Edit-GameText $plugins {
            param($t)
            $t = [regex]::Replace($t, '(?im)^([ \t]*win32[ \t]+\S*HPB_bot_mm\.dll)', ';$1')
            if ($t -notmatch '(?im)^[ \t]*win32[ \t]+addons[\\/]+jk_botti[\\/]+dlls[\\/]+jk_botti_mm\.dll[ \t]*\r?$') { $t = Add-Line $t $PluginLine }
            $t
        } 'loads jk_botti (any HPB_bot line commented out)')
    } else {
        Set-GameBytes $plugins ($Latin1.GetBytes("; Metamod plugins (StargateTC bots)`r`n$PluginLine`r`n")) 'created (loads jk_botti)'
    }

    # ---- 3. payload -----------------------------------------------------------------------
    $files = New-Object System.Collections.Generic.List[object]
    foreach ($f in Get-ChildItem -LiteralPath $PackageDir -Recurse -File) {
        $rel = $f.FullName.Substring($PackageDir.Length + 1)
        if ($f.Name.StartsWith('.')) { continue }                                 # .gitkeep and the like
        if ($rel -like '*.matrix') { continue }                                   # route caches, rebuilt by jk_botti
        if ($rel -ieq $MetamodRel) { continue }                                   # handled below
        $files.Add(@($f.FullName, $rel))
    }
    if ($MetamodSrc) { $files.Add(@($MetamodSrc, $MetamodRel)) }

    foreach ($pair in $files) {
        $src, $rel = $pair[0], $pair[1]
        $dst = Get-Full $rel
        if (Test-Path -LiteralPath $dst) {
            if ((Get-Sha256 $dst) -eq (Get-Sha256 $src)) { continue }
            $userFile = ($rel -like 'addons\jk_botti\*') -and ($rel -notlike 'addons\jk_botti\dlls\*') -and ($rel -notlike '*readme*')
            if ($userFile -and -not $Force) { Write-Step "${rel}: kept your version (-Force to replace)"; continue }
            if ($rel -ieq $MetamodRel -and -not $M.created.Contains($rel) -and -not $Force) {
                Write-Step "${rel}: kept the Metamod that was already installed (-Force to replace)"; continue
            }
        }
        Set-GameBytes $rel ([System.IO.File]::ReadAllBytes($src)) 'installed'
    }

    # ---- 4. in-game "Bots" submenu ---------------------------------------------------------
    [void](Edit-GameText 'commandmenu.txt' { param($t) Add-BotsMenu $t } 'added the in-game "Bots" submenu')

    # ---- 5. class weapon packs ------------------------------------------------------------
    if ($PackWeapons) {
        $M.packWeapons = $true
        foreach ($cfg in @('server.cfg', 'listenserver.cfg')) {
            [void](Edit-GameText $cfg {
                param($t)
                if ($t -match '(?im)^[ \t]*mp_wepselmode\b') {
                    [regex]::Replace($t, '(?im)^([ \t]*mp_wepselmode[ \t]+)"?\d+"?', '${1}2')
                } else { Add-Line $t 'mp_wepselmode 2' }
            } 'mp_wepselmode 2 (class packs)')
        }
    }

    Save-Manifest $Sgtc $M
} catch {
    # keep a record of what was already changed, so uninstall.ps1 can undo a half-done install
    if ($script:Changed -gt 0) { try { Save-Manifest $Sgtc $M } catch { } }
    for ($e = $_.Exception; $e; $e = $e.InnerException) {
        if ($e -is [System.UnauthorizedAccessException]) {
            throw "Access denied while writing to $Sgtc. Close the game, or run install.bat as administrator. ($($e.Message))"
        }
    }
    throw
} finally {
    foreach ($t in $TempFiles) { Remove-Item -LiteralPath $t -Force -ErrorAction SilentlyContinue }
}

if ($script:Changed -eq 0) { Write-Host 'Nothing to do: everything was already installed.' }
else { Write-Host "Done: $($script:Changed) file(s) written. Backups end in $BakExt; run uninstall.bat to undo." }
