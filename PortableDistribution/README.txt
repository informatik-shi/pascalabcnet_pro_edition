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

При ошибке сообщение предлагает открыть диагностический отчёт. Передайте
файлы bootstrap-<номер>.log, launcher-<номер>.log и ide-<номер>.log одного запуска
из %LOCALAPPDATA%\abc.pro\Logs. Open-logs.cmd открывает папки отчётов.
При недоступной записи резервные папки: %TEMP%\abc.pro\Logs, затем
%APPDATA%\abc.pro\Logs. В сообщении указан фактический путь к файлу.
Используйте PascalABCNET.cmd для диагностики до загрузки IDE/CLR.
Отчёт содержит версии, стадию, исключение и стек; учебные файлы и пароли
не собираются. Отключение питания и запрет записи во все папки перехватить
невозможно. Обычное закрытие IDE не считается аварией.

Если отчёт показывает ошибку загрузки языковой DLL 0x80131515, Windows
могла пометить файлы как скачанные из интернета. Для ZIP из доверенного
источника откройте свойства исходного архива, нажмите «Разблокировать»,
затем распакуйте его заново в новую папку. Программа не снимает такие
метки автоматически. Если это не помогло, передайте новый отчёт:
версия 4.0.0.3884 сохраняет первичную причину загрузчика вместо ошибки index.

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
