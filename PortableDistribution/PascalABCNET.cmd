@echo off
setlocal
set "ROOT=%~dp0"
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%ROOT%Portable\launch-ide.ps1" -Root "%ROOT%." %*
exit /b %ERRORLEVEL%
