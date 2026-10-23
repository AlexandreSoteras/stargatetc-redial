<#
.SYNOPSIS
    Tests install.ps1 / uninstall.ps1 on throwaway copies of a StargateTC folder. Never touches a real install.

.DESCRIPTION
    -StockDir     a folder laid out like stargatetc\ with the STOCK 2.0 files the installer edits:
                  Liblist.gam, config.cfg, Titles.txt, server.cfg, listenserver.cfg, commandmenu.txt
                  (+ autoexec.cfg, resource\background\800_3_c_loading.tga, dlls\hl.dll ...). It is
                  copied, never modified.
    -PackageDir   a complete payload (addons\jk_botti\dlls\jk_botti_mm.dll, addons\metamod\dlls\metamod.dll, ...).
    -ReferenceDir optional, read only: an install patched by hand. Liblist.gam, Titles.txt, server.cfg,
                  listenserver.cfg, commandmenu.txt and addons\metamod\config.ini must come out
                  byte-identical to it.
    -MetamodZip   optional: metamod-p-1.19p32-windows.zip, for the "Metamod from zip" scenario.
    -Shell        pwsh or powershell (Windows PowerShell 5.1, what install.bat uses).

    Scenarios: A clean install (+ second run must change nothing) then uninstall; B over the
    2012 "SGTC Bots 0.2" HPB setup; C Metamod taken from the zip; D config.cfg rewritten after
    the install; E uninstall -KeepFixes; F install fails halfway; G commandmenu.txt already
    patched, half patched, without the anchor lines, or missing. After every uninstall the folder
    must be byte-identical to what it was before the install (D: plus config.cfg.sgtcbots.prev;
    E: plus the kept fixes).
#>
param(
    [Parameter(Mandatory = $true)][string]$StockDir,
    [Parameter(Mandatory = $true)][string]$PackageDir,
    [Parameter(Mandatory = $true)][string]$Testbed,
    [string]$ReferenceDir,
    [string]$MetamodZip,
    [string]$Shell = 'pwsh'
)

Set-StrictMode -Version 2
$ErrorActionPreference = 'Stop'
$Root = Split-Path -Parent $PSScriptRoot
$Latin1 = [System.Text.Encoding]::GetEncoding(28591)
$script:Fails = 0

function Ok([bool]$Cond, [string]$What) {
    if ($Cond) { Write-Host "  PASS  $What" -ForegroundColor Green }
    else { Write-Host "  FAIL  $What" -ForegroundColor Red; $script:Fails++ }
}
function Txt([string]$Path) { return $Latin1.GetString([System.IO.File]::ReadAllBytes($Path)) }
function Sha([string]$Path) { return (Get-FileHash -LiteralPath $Path -Algorithm SHA256).Hash }
function SameBytes([string]$A, [string]$B) { return (Test-Path -LiteralPath $A) -and (Test-Path -LiteralPath $B) -and ((Sha $A) -eq (Sha $B)) }

function Snapshot([string]$Dir) {
    $h = @{}
    foreach ($f in Get-ChildItem -LiteralPath $Dir -Recurse -File -Force) { $h[$f.FullName.Substring($Dir.Length + 1)] = Sha $f.FullName }
    return $h
}
function Diff-Snapshot($A, $B) {
    $d = New-Object System.Collections.Generic.List[string]
    foreach ($k in $A.Keys) { if (-not $B.ContainsKey($k)) { $d.Add("- $k") } elseif ($A[$k] -ne $B[$k]) { $d.Add("~ $k") } }
    foreach ($k in $B.Keys) { if (-not $A.ContainsKey($k)) { $d.Add("+ $k") } }
    return ,$d
}

function New-Bed([string]$Name) {
    $game = Join-Path $Testbed "$Name\Program Files (x86)\Half-Life\stargatetc"
    if (Test-Path -LiteralPath (Join-Path $Testbed $Name)) { Remove-Item -LiteralPath (Join-Path $Testbed $Name) -Recurse -Force }
    New-Item -ItemType Directory -Path $game | Out-Null
    Copy-Item -Path (Join-Path $StockDir '*') -Destination $game -Recurse
    return (Resolve-Path -LiteralPath $game).Path
}

function Run([string]$Script, [string[]]$Arguments) {
    $out = & $Shell -NoProfile -ExecutionPolicy Bypass -File (Join-Path $Root $Script) @Arguments 2>&1 | Out-String
    $script:LastOut = $out
    return $LASTEXITCODE
}

$expectedIni = "gamedll dlls/hl.dll`r`n"
$jkLine = 'win32 addons/jk_botti/dlls/jk_botti_mm.dll'
$stockLib = Txt (Join-Path $StockDir 'Liblist.gam')
$expectedLib = $stockLib.Replace('gamedll "dlls/hl.dll"', "//gamedll ""dlls/hl.dll""`r`ngamedll ""addons/metamod/dlls/metamod.dll""")
$stockMenu = Txt (Join-Path $StockDir 'commandmenu.txt')
# Each "Bots" submenu (13 lines) goes right before this line, with this command prefix.
$menuSpots = @(@('" " "Status" "status"', ''), @('" " "Set Tickets" "messagemode rcon mp_tickets"', 'rcon '))

# Problems in a patched commandmenu.txt: CRLF only, one well-formed Bots submenu before each anchor
# line, and with both submenus taken out, the stock file byte for byte.
function Get-MenuProblems([string]$Text) {
    $problems = New-Object System.Collections.Generic.List[string]
    if ($Text -match '(?<!\r)\n') { $problems.Add('a line ends in LF only') }
    $lines = New-Object System.Collections.Generic.List[string]
    $lines.AddRange([string[]]($Text -split "`r`n"))
    foreach ($s in $menuSpots) {
        $i = $lines.IndexOf($s[0])
        if ($i -lt 13) { $problems.Add("no '$($s[0])' line with room for a submenu before it"); continue }
        $blk = $lines.GetRange($i - 13, 13).ToArray()
        $cmds = [regex]::Matches(($blk -join "`n"), '(rcon )?jk_botti ')
        $wrongPrefix = @($cmds | Where-Object { $_.Value -ne "$($s[1])jk_botti " }).Count
        if (($blk[0] -ne '      " " "Bots"') -or ($blk[1] -ne '     {') -or ($blk[12] -ne '     }') -or
            ($blk[2] -ne "        "" "" ""Add a bot"" ""$($s[1])jk_botti addbot""") -or ($cmds.Count -ne 17) -or $wrongPrefix) {
            $problems.Add("no well-formed Bots submenu before '$($s[0])'"); continue
        }
        $lines.RemoveRange($i - 13, 13)
    }
    if (($lines.ToArray() -join "`r`n") -cne $stockMenu) { $problems.Add('lines other than the submenus differ from stock') }
    return ,$problems
}

Write-Host "Shell: $Shell ($(& $Shell -NoProfile -Command '$PSVersionTable.PSVersion.ToString()'))"

# ---------------------------------------------------------------- A: clean install
Write-Host "`n[A] clean stock install, -PackWeapons"
$g = New-Bed 'A'
$before = Snapshot $g
$rc = Run 'install.ps1' @('-GameDir', $g, '-PackageDir', $PackageDir, '-PackWeapons')
Ok ($rc -eq 0) "install exit code 0"
if ($rc -ne 0) { Write-Host $LastOut }
Ok ((Txt "$g\Liblist.gam") -ceq $expectedLib) 'Liblist.gam: gamedll -> metamod, original commented'
Ok ((Txt "$g\config.cfg") -ceq (Txt "$StockDir\config.cfg").Replace("bind ""KP_LEFTARROW"" ""+portrait""`r`n", '')) 'config.cfg: only the +portrait line removed, CRLF kept'
Ok ((Txt "$g\Titles.txt") -ceq (Txt "$StockDir\Titles.txt").Replace("`r`n", "`n")) 'Titles.txt: LF'
foreach ($c in 'server.cfg', 'listenserver.cfg') {
    Ok ((Txt "$g\$c") -ceq (Txt "$StockDir\$c").Replace('mp_wepselmode 1', 'mp_wepselmode 2')) "${c}: mp_wepselmode 2, nothing else"
}
Ok ((Txt "$g\addons\metamod\config.ini") -ceq $expectedIni) 'metamod config.ini: gamedll dlls/hl.dll'
Ok ((Txt "$g\addons\metamod\plugins.ini") -match [regex]::Escape($jkLine)) 'plugins.ini loads jk_botti'
$patchedMenu = Txt "$g\commandmenu.txt"
$mp = Get-MenuProblems $patchedMenu
Ok ($mp.Count -eq 0) ("commandmenu.txt: Bots submenu before 'Status' and 'Set Tickets', CRLF, nothing else changed" + $(if ($mp.Count) { ' (' + ($mp -join '; ') + ')' }))
Ok (((Txt "$g\sgtcbots_manifest.json") | ConvertFrom-Json).backups -contains 'commandmenu.txt') 'commandmenu.txt recorded in the manifest'
foreach ($rel in 'Liblist.gam', 'config.cfg', 'Titles.txt', 'server.cfg', 'listenserver.cfg', 'commandmenu.txt', 'resource\background\800_3_c_loading.tga') {
    Ok (SameBytes "$g\$rel.sgtcbots.bak" "$StockDir\$rel") "backup $rel.sgtcbots.bak = stock"
}
$payloadOk = $true
foreach ($f in Get-ChildItem -LiteralPath $PackageDir -Recurse -File) {
    $rel = $f.FullName.Substring((Resolve-Path -LiteralPath $PackageDir).Path.TrimEnd('\').Length + 1)
    if (-not (SameBytes $f.FullName "$g\$rel")) { $payloadOk = $false; Write-Host "    payload mismatch: $rel" }
}
Ok $payloadOk 'every payload file installed byte for byte'
if ($ReferenceDir) {
    foreach ($rel in 'Liblist.gam', 'Titles.txt', 'server.cfg', 'listenserver.cfg', 'commandmenu.txt', 'addons\metamod\config.ini') {
        Ok (SameBytes "$g\$rel" "$ReferenceDir\$rel") "$rel identical to the hand-patched reference install"
    }
}
$after1 = Snapshot $g
$rc = Run 'install.ps1' @('-GameDir', $g, '-PackageDir', $PackageDir, '-PackWeapons')
$after2 = Snapshot $g
Ok (($rc -eq 0) -and ((Diff-Snapshot $after1 $after2).Count -eq 0) -and ($LastOut -match 'Nothing to do')) 'second install: no file changed (idempotent)'
$rc = Run 'uninstall.ps1' @('-GameDir', $g)
Ok ($rc -eq 0) 'uninstall exit code 0'
$d = Diff-Snapshot $before (Snapshot $g)
Ok ($d.Count -eq 0) "after uninstall: byte-identical to before the install ($($d.Count) differences)"
if ($d.Count) { $d | ForEach-Object { Write-Host "    $_" } }

# ---------------------------------------------------------------- B: over the 2012 HPB pack
Write-Host "`n[B] over the 2012 'SGTC Bots 0.2' setup (Metamod + HPB_bot already there)"
$g = New-Bed 'B'
[System.IO.File]::WriteAllBytes("$g\Liblist.gam", $Latin1.GetBytes($expectedLib))
New-Item -ItemType Directory -Path "$g\addons\metamod\dlls", "$g\addons\bots" | Out-Null
[System.IO.File]::WriteAllBytes("$g\addons\metamod\config.ini", $Latin1.GetBytes($expectedIni))
$hpb = ";; Metamod plugins.ini`r`n; AMX Mod X 1.8.1`r`n`r`n;win32   addons\amxmodx\dlls\amxmodx_mm.dll`r`n; Enable this instead for binary logging`r`n; win32   addons\amxmodx\dlls\amxmodx_bl_mm.dll`r`nwin32   addons\bots\HPB_bot_mm.dll"
[System.IO.File]::WriteAllBytes("$g\addons\metamod\plugins.ini", $Latin1.GetBytes($hpb))
Copy-Item "$PackageDir\addons\metamod\dlls\metamod.dll" "$g\addons\metamod\dlls\metamod.dll"
[System.IO.File]::WriteAllBytes("$g\addons\bots\HPB_bot_mm.dll", $Latin1.GetBytes('dummy HPB'))
$before = Snapshot $g
$rc = Run 'install.ps1' @('-GameDir', $g, '-PackageDir', $PackageDir)
Ok ($rc -eq 0) 'install exit code 0'
if ($rc -ne 0) { Write-Host $LastOut }
Ok ((Txt "$g\Liblist.gam") -ceq $expectedLib) 'Liblist.gam left alone (already Metamod)'
$p = Txt "$g\addons\metamod\plugins.ini"
Ok (($p -match '(?m)^;win32   addons\\bots\\HPB_bot_mm\.dll') -and ($p.EndsWith("$jkLine`r`n"))) 'plugins.ini: HPB_bot commented out, jk_botti added'
Ok (SameBytes "$g\server.cfg" "$StockDir\server.cfg") 'server.cfg untouched without -PackWeapons'
$rc = Run 'uninstall.ps1' @('-GameDir', $g)
$d = Diff-Snapshot $before (Snapshot $g)
Ok (($rc -eq 0) -and $d.Count -eq 0) "after uninstall: byte-identical to before ($($d.Count) differences)"
if ($d.Count) { $d | ForEach-Object { Write-Host "    $_" } }

# ---------------------------------------------------------------- C: Metamod from the zip
if ($MetamodZip) {
    Write-Host "`n[C] package without metamod.dll, -MetamodZip"
    $pkg = Join-Path $Testbed 'package_no_metamod'
    if (Test-Path -LiteralPath $pkg) { Remove-Item -LiteralPath $pkg -Recurse -Force }
    Copy-Item -LiteralPath $PackageDir -Destination $pkg -Recurse
    Remove-Item -LiteralPath "$pkg\addons\metamod" -Recurse -Force
    $g = New-Bed 'C'
    $before = Snapshot $g
    $rc = Run 'install.ps1' @('-GameDir', $g, '-PackageDir', $pkg)
    Ok (($rc -ne 0) -and ($LastOut -match 'No Metamod') -and (Diff-Snapshot $before (Snapshot $g)).Count -eq 0) 'no Metamod anywhere: clear error, nothing changed'
    $rc = Run 'install.ps1' @('-GameDir', $g, '-PackageDir', $pkg, '-MetamodZip', $MetamodZip)
    Ok ($rc -eq 0) 'install exit code 0'
    if ($rc -ne 0) { Write-Host $LastOut }
    Ok ((Sha "$g\addons\metamod\dlls\metamod.dll") -eq '382B8FCBEFFD556C34971E945857B3A88105C423FB9F2BE222E395E0491C478F') 'metamod.dll extracted from the zip, SHA-256 checked'
    $rc = Run 'install.ps1' @('-GameDir', (Join-Path $Testbed 'C\Program Files (x86)\Half-Life'), '-PackageDir', $pkg)
    Ok (($rc -eq 0) -and ($LastOut -match 'Nothing to do')) 'second run, -GameDir given as the Half-Life folder: nothing to do'
    $rc = Run 'uninstall.ps1' @('-GameDir', $g)
    $d = Diff-Snapshot $before (Snapshot $g)
    Ok (($rc -eq 0) -and $d.Count -eq 0) "after uninstall: byte-identical to before ($($d.Count) differences)"
    $rc = Run 'install.ps1' @('-GameDir', $g, '-PackageDir', $pkg, '-MetamodZip', "$StockDir\Liblist.gam")
    Ok (($rc -ne 0) -and (Diff-Snapshot $before (Snapshot $g)).Count -eq 0) 'a bad -MetamodZip fails before changing anything'
}

# ---------------------------------------------------------------- D: config.cfg rewritten after the install
Write-Host "`n[D] config.cfg rewritten by the engine after the install"
$g = New-Bed 'D'
$before = Snapshot $g
[void](Run 'install.ps1' @('-GameDir', $g, '-PackageDir', $PackageDir))
$edited = (Txt "$g\config.cfg") + "bind ""F5"" ""snapshot""`r`n"
[System.IO.File]::WriteAllBytes("$g\config.cfg", $Latin1.GetBytes($edited))
$rc = Run 'uninstall.ps1' @('-GameDir', $g)
$d = Diff-Snapshot $before (Snapshot $g)
Ok (($rc -eq 0) -and (SameBytes "$g\config.cfg" "$StockDir\config.cfg") -and ((Txt "$g\config.cfg.sgtcbots.prev") -ceq $edited)) 'config.cfg restored, the newer copy kept as config.cfg.sgtcbots.prev'
Ok (($d.Count -eq 1) -and ($d[0] -eq '+ config.cfg.sgtcbots.prev')) 'nothing else differs'

# ---------------------------------------------------------------- E: uninstall -KeepFixes
Write-Host "`n[E] uninstall -KeepFixes"
$g = New-Bed 'E'
$before = Snapshot $g
[void](Run 'install.ps1' @('-GameDir', $g, '-PackageDir', $PackageDir))
$fixedCfg = Sha "$g\config.cfg"; $fixedTitles = Sha "$g\Titles.txt"
$rc = Run 'uninstall.ps1' @('-GameDir', $g, '-KeepFixes')
$d = @((Diff-Snapshot $before (Snapshot $g)).ToArray() | Sort-Object)
Ok (($rc -eq 0) -and ((Sha "$g\config.cfg") -eq $fixedCfg) -and ((Sha "$g\Titles.txt") -eq $fixedTitles)) 'config.cfg and Titles.txt keep the fixes'
Ok ((($d -join ',') -eq '+ config.cfg.sgtcbots.bak,+ Titles.txt.sgtcbots.bak,~ config.cfg,~ Titles.txt')) "only those two (and their backups) differ: $($d -join ', ')"

# ---------------------------------------------------------------- F: install fails halfway
Write-Host "`n[F] install fails halfway (Titles.txt read-only), then uninstall"
$g = New-Bed 'F'
$before = Snapshot $g
Set-ItemProperty -LiteralPath "$g\Titles.txt" -Name IsReadOnly -Value $true
$rc = Run 'install.ps1' @('-GameDir', $g, '-PackageDir', $PackageDir)
Ok (($rc -ne 0) -and ($LastOut -match 'Access denied') -and (Test-Path -LiteralPath "$g\sgtcbots_manifest.json")) 'fails with "Access denied", manifest of the partial install saved'
$rc = Run 'uninstall.ps1' @('-GameDir', $g)
Set-ItemProperty -LiteralPath "$g\Titles.txt" -Name IsReadOnly -Value $false
$d = Diff-Snapshot $before (Snapshot $g)
Ok (($rc -eq 0) -and $d.Count -eq 0) "uninstall undoes the partial install ($($d.Count) differences)"
if ($d.Count) { $d | ForEach-Object { Write-Host "    $_" } }

# ---------------------------------------------------------------- G: commandmenu.txt variants
# Each case: install, check commandmenu.txt, uninstall, and the folder must be as before.
# $Menu is untyped on purpose: a [string] parameter would turn $null (no file) into ''.
function Test-MenuCase([string]$Name, $Menu, [scriptblock]$Check, [string]$What) {
    $g = New-Bed $Name
    if ($null -eq $Menu) { Remove-Item -LiteralPath "$g\commandmenu.txt" }
    else { [System.IO.File]::WriteAllBytes("$g\commandmenu.txt", $Latin1.GetBytes($Menu)) }
    $before = Snapshot $g
    $rc = Run 'install.ps1' @('-GameDir', $g, '-PackageDir', $PackageDir)
    $ok = ($rc -eq 0) -and [bool](& $Check $g)
    Ok $ok $What
    if (-not $ok) { Write-Host $LastOut }
    $rc = Run 'uninstall.ps1' @('-GameDir', $g)
    $d = Diff-Snapshot $before (Snapshot $g)
    Ok (($rc -eq 0) -and $d.Count -eq 0) "  then uninstall: byte-identical to before ($($d.Count) differences)"
    if ($d.Count) { $d | ForEach-Object { Write-Host "    $_" } }
}
function Test-NotInManifest([string]$Dir) {
    $j = (Txt "$Dir\sgtcbots_manifest.json") | ConvertFrom-Json
    return (@($j.backups) -notcontains 'commandmenu.txt') -and (@($j.created) -notcontains 'commandmenu.txt')
}

Write-Host "`n[G] commandmenu.txt variants"
# The patched menu with the Rcon submenu taken out: only the Server Admin one is there.
$halfLines = New-Object System.Collections.Generic.List[string]
$halfLines.AddRange([string[]]($patchedMenu -split "`r`n"))
$halfLines.RemoveRange($halfLines.IndexOf($menuSpots[1][0]) - 13, 13)
$halfMenu = $halfLines.ToArray() -join "`r`n"
$customMenu = "`"1`" `"Say hello`" `"say hello`"`r`n`"2`" `"Status`" `"status`"`r`n"

Test-MenuCase 'G1' $patchedMenu {
    param($g) ((Txt "$g\commandmenu.txt") -ceq $patchedMenu) -and -not (Test-Path -LiteralPath "$g\commandmenu.txt.sgtcbots.bak") -and (Test-NotInManifest $g)
} 'already patched by hand: left alone, no backup, not in the manifest'
Test-MenuCase 'G2' $halfMenu {
    param($g) ((Txt "$g\commandmenu.txt") -ceq $patchedMenu) -and ((Txt "$g\commandmenu.txt.sgtcbots.bak") -ceq $halfMenu)
} 'only the Server Admin submenu there: the Rcon one is added, same result as a stock install'
Test-MenuCase 'G3' $customMenu {
    param($g) ((Txt "$g\commandmenu.txt") -ceq $customMenu) -and ($LastOut -match 'WARNING: commandmenu\.txt: no') -and (Test-NotInManifest $g)
} 'custom menu without the anchor lines: warning, file left alone, install goes on'
Test-MenuCase 'G4' $null {
    param($g) -not (Test-Path -LiteralPath "$g\commandmenu.txt")
} 'no commandmenu.txt: none created'

Write-Host ''
if ($script:Fails) { Write-Host "$($script:Fails) check(s) FAILED" -ForegroundColor Red; exit 1 }
Write-Host 'All checks passed.' -ForegroundColor Green
