@echo off
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0Portable\launch-ide.ps1" -Root "%~dp0." -Installed %*
exit /b %ERRORLEVEL%
