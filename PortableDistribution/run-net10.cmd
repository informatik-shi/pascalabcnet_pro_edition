@echo off
setlocal
set "ROOT=%~dp0"
set "DOTNET_ROOT=%ROOT%dotnet"
set "DOTNET_ROOT_X64=%DOTNET_ROOT%"
set "PATH=%DOTNET_ROOT%;%PATH%"
if "%~1"=="" (
  echo Usage: run-net10.cmd program.exe [arguments]
  exit /b 2
)
"%DOTNET_ROOT%\dotnet.exe" %*
exit /b %ERRORLEVEL%
