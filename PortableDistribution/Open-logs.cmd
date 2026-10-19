@echo off
setlocal
echo abc.pro diagnostic reports:
echo %LOCALAPPDATA%\abc.pro\Logs
echo %TEMP%\abc.pro\Logs
echo %APPDATA%\abc.pro\Logs
for %%D in ("%LOCALAPPDATA%\abc.pro\Logs" "%TEMP%\abc.pro\Logs" "%APPDATA%\abc.pro\Logs") do if exist "%%~D" start "" explorer.exe "%%~D"
