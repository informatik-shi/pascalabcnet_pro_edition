@echo off
setlocal
set "ROOT=%~dp0"
set "DOTNET_ROOT=%ROOT%dotnet"
set "DOTNET_ROOT_X64=%DOTNET_ROOT%"
set "PATH=%DOTNET_ROOT%;%PATH%"
"%ROOT%bin\pabcnetc.exe" %*
exit /b %ERRORLEVEL%
