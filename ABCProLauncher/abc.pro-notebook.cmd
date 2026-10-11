@echo off
setlocal
set "ROOT=%~dp0"
set "DATA=%LOCALAPPDATA%\abc.pro"
set "DOTNET_ROOT=%ROOT%dotnet"
set "DOTNET_ROOT_X64=%DOTNET_ROOT%"
set "PABCNET_ROOT=%ROOT%"
set "PABC_PYTHON_EXE=%ROOT%python\python.exe"
set "MPLCONFIGDIR=%DATA%\Work\Matplotlib"
set "PATH=%DOTNET_ROOT%;%PATH%"
if not exist "%DATA%\Notebooks" mkdir "%DATA%\Notebooks"
if not exist "%MPLCONFIGDIR%" mkdir "%MPLCONFIGDIR%"
"%DOTNET_ROOT%\dotnet.exe" "%ROOT%notebook\PascalABCNotebook.dll" --root "%ROOT%." --workdir "%DATA%\Notebooks" %*
exit /b %ERRORLEVEL%
