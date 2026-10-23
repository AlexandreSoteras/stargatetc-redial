@echo off
rem StargateTC bots: runs uninstall.ps1 with Windows PowerShell.
rem Option: uninstall.bat -KeepFixes   (remove the bots, keep the HL25 crash fixes)
setlocal
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0uninstall.ps1" %*
set RC=%ERRORLEVEL%
if not "%RC%"=="0" echo. & echo Uninstall FAILED (exit code %RC%).
if "%~1"=="" pause
exit /b %RC%
