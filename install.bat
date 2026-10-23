@echo off
rem StargateTC bots + HL25 fixes: runs install.ps1 with Windows PowerShell.
rem Double-click it, or pass options, e.g.:  install.bat -PackWeapons -GameDir "D:\Games\Half-Life\stargatetc"
setlocal
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0install.ps1" %*
set RC=%ERRORLEVEL%
if not "%RC%"=="0" echo. & echo Installation FAILED (exit code %RC%). Run uninstall.bat to undo any change listed above.
if "%~1"=="" pause
exit /b %RC%
