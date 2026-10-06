@echo off
setlocal
set "ROOT=%~dp0"
set "DOTNET_ROOT=%ROOT%dotnet"
set "DOTNET_ROOT_X64=%DOTNET_ROOT%"
set "PATH=%DOTNET_ROOT%;%PATH%"
"%DOTNET_ROOT%\dotnet.exe" "%ROOT%bin-net10\pabcnetc.dll" %*
exit /b %ERRORLEVEL%
