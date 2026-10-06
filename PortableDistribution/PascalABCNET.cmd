@echo off
setlocal
set "ROOT=%~dp0"
set "DOTNET_ROOT=%ROOT%dotnet"
set "DOTNET_ROOT_X64=%DOTNET_ROOT%"
set "PATH=%DOTNET_ROOT%;%PATH%"
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%ROOT%Portable\set-workdir.ps1" -Root "%ROOT%."
if errorlevel 1 exit /b %ERRORLEVEL%
start "" /D "%ROOT%bin" "%ROOT%bin\PascalABCNET.exe" %*
exit /b %ERRORLEVEL%
