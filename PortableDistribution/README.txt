PascalABC.NET Portable (Windows x64)
====================================

Распакуйте архив целиком и запустите PascalABCNET.cmd. Права администратора
не нужны. Если папка сборки доступна только для чтения, рабочие файлы
сохраняются в %LOCALAPPDATA%\abc.pro\Work. Примеры находятся в Work\Samples
внутри архива. Установка и регистрация в GAC не нужны.

Компиляторы: pabcnetc.cmd программа.pas (обычный) и
pabcnetc-net10.cmd программа.pas (.NET 10). Для запуска программы
.NET 10 без системного Runtime используйте run-net10.cmd программа.exe.
Windows должна содержать .NET Framework 4.7.2 или новее для IDE и
обычного компилятора. .NET 10 Runtime включён в папку dotnet.

Тетрадки: запустите PascalABCNotebook.cmd. Приложение откроет браузер,
где можно выбрать PascalABC.NET или SPython. Графика показывается
под ячейкой кода. Тетрадки сохраняются в Work\Notebooks.

Extract the entire ZIP and run PascalABCNET.cmd for the IDE. No administrator
rights are required. The work folder is Work inside the package when writable;
otherwise it is %LOCALAPPDATA%\abc.pro\Work. The launcher does not modify bin.
No installer or GAC registration is needed.

If the IDE closes while loading, the launcher shows an error and writes logs to
%LOCALAPPDATA%\abc.pro\Logs\portable-launch.log and ide-startup.log.

Command-line compilers:
  pabcnetc.cmd program.pas
  pabcnetc-net10.cmd program.pas

For a generated .NET 10 program on a computer without a global .NET 10 install:
  run-net10.cmd Work\Output\program.exe

Windows must already provide .NET Framework 4.7.2 or newer for the IDE and
classic compiler. Windows 10/11 commonly provide this as an OS component.
The .NET 10 runtime for the modern compiler is included under dotnet\.
No SDK is needed to use the extracted package.

Run PascalABCNotebook.cmd for local browser notebooks with inline graphics.
Notebook files are saved under Work\Notebooks.

Keep the bin, bin-net10, dotnet, Portable, and Work folders together. Some
examples require optional third-party hardware or software, as in the upstream
PascalABC.NET distribution.
