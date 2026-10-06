# Воспроизводимая сборка portable-версии с редактором SPython

## Что находится в сборке

Архив `Release\PascalABCNET-Portable-win-x64.zip` содержит Windows IDE,
обычный компилятор, компилятор .NET 10 и .NET 10 Runtime. После распаковки
запускайте `PascalABCNET.cmd`. Установка PascalABC.NET и регистрация DLL в GAC
на целевом компьютере не требуются. Для IDE и обычного компилятора Windows
должна содержать .NET Framework 4.7.2 или новее.

Редактор файлов `.pys` по умолчанию использует тёмную палитру и добавляет отступ по Enter
после заголовков блоков: `if`, `elif`, `else`, `for`, `while`, `def`, `class`,
`try`, `except`, `finally`, `with`, `async def/for/with`, `match`, `case`.
Размер добавляемого отступа берётся из настройки редактора. Отступ уже
вложенной строки сохраняется. Двоеточия в строках, словарях, срезах и
однострочных блоках дополнительный отступ не создают.

В меню **Вид → Тёмная тема редактора и вывода** снимите галочку, чтобы
переключить SPython на светлую палитру; повторное нажатие возвращает тёмную.
Переключение сразу меняет фон и текст окна вывода, поля ввода и консоли
компилятора, включая уже открытые вкладки. Редакторы других языков сохраняют
свою стандартную палитру. Выбор записывается при закрытии IDE в
`Work\PascalABCNET.ini` внутри распакованной portable-папки. Запускайте
portable-версию через `PascalABCNET.cmd`: он указывает IDE путь к этой папке.

## Подготовка машины сборки

1. Установите на Windows x64 .NET 10 SDK и .NET 10 Windows Desktop Runtime.
   Проверенная конфигурация: SDK 10.0.401, Runtime 10.0.12. Для сборки IDE
   также нужны средства разработки .NET Framework 4.7.2 из Visual Studio
   2026 или Build Tools. Проверьте `dotnet --info` и `dotnet --list-runtimes`.
2. Получите ветку: `git clone https://github.com/informatik-shi/pascalabcnet_pro_edition.git`
   и затем `git switch portable` в каталоге репозитория.
3. Исходный проект требует HelixToolkit и NUnit при первой полной сборке
   .NET Framework. Если они ещё не подготовлены на **машине сборки**,
   выполните `_RegisterHelixNUnit.bat` от администратора, как описано в
   корневом `README.md`. На компьютере пользователя этот шаг не нужен.

## Полная сборка и проверка

Из корня репозитория выполните в PowerShell:

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File scripts\build-portable.ps1
powershell.exe -STA -NoProfile -ExecutionPolicy Bypass -File scripts\test-spython-editor.ps1
```

Если SDK установлен в нестандартный каталог, передайте `-DotnetRoot`,
например `-DotnetRoot C:\Users\PC\.dotnet`. Сценарий собирает IDE и оба
компилятора, пересоздаёт стандартные `.pcu`, копирует нужные ресурсы и
Runtime, затем создаёт ZIP. Файл `bin\Lib\PABCRtl.dll` входит в ветку как
готовая зависимость; сценарий не собирает эту DLL заново.

Тестовый сценарий запускайте после сборки. Он проверяет 24 сценария Enter
через настоящий компонент редактора, загрузку обеих палитр `.pys` и цвета
обеих панелей вывода в двух режимах. Он должен
завершиться строкой `PASS`.

Для ручной проверки распакуйте ZIP в отдельную папку с правом записи:

```powershell
Expand-Archive -LiteralPath Release\PascalABCNET-Portable-win-x64.zip -DestinationPath .\portable-smoke
& '.\portable-smoke\PascalABCNET-Portable-win-x64\PascalABCNET.cmd'
powershell.exe -STA -NoProfile -ExecutionPolicy Bypass -File scripts\test-spython-editor.ps1 -BinDirectory '.\portable-smoke\PascalABCNET-Portable-win-x64\bin'
```

Откройте или создайте файл `.pys`. После `for i in range(3):` и Enter курсор
должен оказаться на новой строке с отступом на один уровень глубже. Исходная
настройка редактора — два пробела; для отступа в четыре пробела измените
ширину отступа в настройках IDE. Повторите для `if ready:`, `def f():`, `try:` и `else:`.
После `values = {'a': 1}` дополнительного уровня отступа быть не должно.
Фон области кода и окна вывода должен быть `#1E1E1E`, текст и подсветка —
светлыми. Запустите программу с выводом (`print('test')`) и проверьте, что он
виден на тёмном фоне. В меню **Вид → Тёмная тема редактора и вывода** снимите
галочку: оба окна должны стать светлыми без перезапуска. Закройте и запустите
IDE снова, проверьте сохранение выбранной темы, затем верните тёмную.

Если менялись только файлы палитр `bin\Highlighting\SPython.xshd` или
`bin\Highlighting\SPythonLight.xshd`, можно
повторно упаковать уже собранные бинарные файлы командой
`scripts\build-portable.ps1 -SkipBuild`. После изменения C# кода нужна
полная сборка без `-SkipBuild`.

## Где менять поведение

- `VisualPascalABCNET\DockContent\SPythonFormattingStrategy.cs` — правила
  отступа после Enter.
- `VisualPascalABCNET\DockContent\CodeFileDocument.cs` — подключение стратегии
  только для `.pys` и возврат обычной стратегии для других файлов.
- `VisualPascalABCNET\AutoInsertCode\AutoInsertCode.cs` — отключение
  автодополнения конструкций Pascal для `.pys`.
- `bin\Highlighting\SPython.xshd` — цвета редактора и синтаксиса SPython.
- `bin\Highlighting\SPythonLight.xshd` — светлая палитра SPython.
- `VisualPascalABCNET\Form1.cs` — пункт меню и применение темы к открытым
  редакторам и панелям вывода.
- `VisualPascalABCNET\Workbench\OptionsManager.cs` — сохранение и загрузка
  выбора темы.
- `scripts\build-portable.ps1` и `scripts\test-spython-editor.ps1` — упаковка
  и воспроизводимая проверка.

Папка `Release` не хранится в Git: итоговый ZIP нужно собрать локально после
клонирования ветки.
