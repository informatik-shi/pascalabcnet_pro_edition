# Воспроизводимая сборка portable-версии с редактором SPython

Готовый архив доступен в [релизах GitHub](https://github.com/informatik-shi/pascalabcnet_pro_edition/releases):
[скачать portable-сборку для Windows x64](https://github.com/informatik-shi/pascalabcnet_pro_edition/releases/download/portable-2026-10-06.2/PascalABCNET-Portable-win-x64.zip).

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

В меню **Вид → Тёмная тема интерфейса** снимите галочку, чтобы
переключить все вкладки кода на светлую палитру; повторное нажатие возвращает
тёмную. Переключение сразу меняет фон и подсветку открытых редакторов, верхнее
меню, его выпадающие списки, панель инструментов, строку состояния, контекстные
меню, вкладки документов, строку «Глобальные имена», полосы прокрутки редактора,
заголовки и разделители служебных окон, окно вывода, поле ввода и консоль компилятора.
На Windows 11 меняется и заголовок главного окна. Выбор записывается при закрытии IDE в
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
через настоящий компонент редактора, обе палитры `.pys`, 20 тёмных режимов
подсветки для остальных файлов, переключение редактора, меню и цвета обеих панелей
вывода. Также он проверяет палитру вкладок и заголовков dock-панелей,
реально нарисованную горизонтальную полосу прокрутки, строку выбора класса и,
на Windows 11, применение цвета заголовка окна. Он должен
завершиться строкой `PASS`.

Для ручной проверки распакуйте ZIP в отдельную папку с правом записи:

```powershell
Expand-Archive -LiteralPath Release\PascalABCNET-Portable-win-x64.zip -DestinationPath .\portable-smoke
& '.\portable-smoke\PascalABCNET-Portable-win-x64\PascalABCNET.cmd'
powershell.exe -STA -NoProfile -ExecutionPolicy Bypass -File scripts\test-spython-editor.ps1 -BinDirectory '.\portable-smoke\PascalABCNET-Portable-win-x64\bin'
```

В интерактивном терминале проверьте оба компилятора из распакованной папки:

```powershell
$package = (Resolve-Path '.\portable-smoke\PascalABCNET-Portable-win-x64').Path
"begin writeln('classic ok'); end." | Set-Content -Encoding ascii (Join-Path $package 'smoke-classic.pas')
"begin writeln('net10 ok'); end." | Set-Content -Encoding ascii (Join-Path $package 'smoke-net10.pas')
& (Join-Path $package 'pabcnetc.cmd') (Join-Path $package 'smoke-classic.pas')
& (Join-Path $package 'pabcnetc-net10.cmd') (Join-Path $package 'smoke-net10.pas')
```

Обе команды должны закончиться сообщением `OK` и создать `.exe` рядом с
соответствующим `.pas`. Обычному компилятору нужен интерактивный терминал:
при перенаправленном консольном выводе он вызывает `Console.GetBufferInfo`.

Откройте или создайте файл `.pys`. После `for i in range(3):` и Enter курсор
должен оказаться на новой строке с отступом на один уровень глубже. Исходная
настройка редактора — два пробела; для отступа в четыре пробела измените
ширину отступа в настройках IDE. Повторите для `if ready:`, `def f():`, `try:` и `else:`.
После `values = {'a': 1}` дополнительного уровня отступа быть не должно.
Фон области кода и окна вывода должен быть `#1E1E1E`, текст и подсветка —
светлыми. Запустите программу с выводом (`print('test')`) и проверьте, что он
виден на тёмном фоне. Верхнее меню, выпадающие пункты, вкладки документов,
строка «Глобальные имена», горизонтальная и вертикальная полосы прокрутки,
заголовок окна вывода и разделители должны быть тёмными. На Windows 11
заголовок главного окна тоже должен быть тёмным. В меню **Вид → Тёмная тема
интерфейса** снимите галочку: все перечисленные элементы должны стать светлыми
без перезапуска. Также откройте
файл `.pas` и проверьте оба режима на нём. Закройте и запустите
IDE снова, проверьте сохранение выбранной темы, затем верните тёмную.

Если менялись только файлы палитр `bin\Highlighting\SPython.xshd` или
`bin\Highlighting\SPythonLight.xshd`, можно
повторно упаковать уже собранные бинарные файлы командой
`scripts\build-portable.ps1 -SkipBuild`. После изменения C# кода нужна
полная сборка без `-SkipBuild`.

## Публикация новой версии

Перед фиксацией замените тег в прямых ссылках на ZIP в `README.md` и в начале
этого документа. После успешной проверки зафиксируйте изменения в ветке
`portable` и отправьте её на GitHub. Для каждого нового архива создавайте
новый тег и релиз; уже опубликованный архив не заменяйте. Пример для версии
`portable-2026-10-06.2`:

```powershell
git push origin portable
git tag portable-2026-10-06.2
git push origin portable-2026-10-06.2
$zip = 'Release\PascalABCNET-Portable-win-x64.zip'
$hash = (Get-FileHash -LiteralPath $zip -Algorithm SHA256).Hash.ToLowerInvariant()
"$hash  PascalABCNET-Portable-win-x64.zip" | Set-Content -Encoding ascii Release\SHA256SUMS.txt
```

На странице [GitHub Releases](https://github.com/informatik-shi/pascalabcnet_pro_edition/releases)
выберите создание нового релиза с этим тегом, загрузите ZIP и `SHA256SUMS.txt`,
затем опубликуйте релиз. В `README.md` ветки `master` тоже обновите прямую
ссылку на ZIP. Проверьте скачивание по опубликованной ссылке.

## Где менять поведение

- `VisualPascalABCNET\DockContent\SPythonFormattingStrategy.cs` — правила
  отступа после Enter.
- `VisualPascalABCNET\DockContent\CodeFileDocument.cs` — подключение стратегии
  только для `.pys` и возврат обычной стратегии для других файлов.
- `VisualPascalABCNET\AutoInsertCode\AutoInsertCode.cs` — отключение
  автодополнения конструкций Pascal для `.pys`.
- `bin\Highlighting\SPython.xshd` — цвета редактора и синтаксиса SPython.
- `bin\Highlighting\SPythonLight.xshd` — светлая палитра SPython.
- `VisualPascalABCNET\DockContent\DarkSyntaxModeProvider.cs` — построение
  тёмной подсветки для остальных языков из имеющихся определений синтаксиса.
- `VisualPascalABCNET\DockContent\ToolStripThemeManager.cs` — цвета верхнего,
  вложенных и контекстных меню, панели инструментов и строки состояния.
- `Libraries\WeifenLuo\WinFormsUI\Docking\DockPanelTheme.cs` и классы
  `VS2005DockPaneStrip`, `VS2005DockPaneCaption`, `VS2005AutoHideStrip`,
  `DockPanel`, `DockWindow`, `DockPane` — цвета вкладок, заголовков,
  разделителей и область вокруг dock-панелей. `Form1.ApplyEditorTheme()`
  вызывает `MainDockPanel.ApplyTheme()` при каждом переключении.
- `VisualPascalABCNET\IB\CodeCompletion\QuickClassBrowserPanel.cs` — цвета
  строки «Глобальные имена» и выпадающих списков.
- `ICSharpCode.TextEditor\Src\Gui\ThemedScrollBars.cs` и `TextAreaControl.cs` —
  прорисовка полос прокрутки редактора в обеих темах с сохранением прокрутки.
  Проект `ICSharpCode.TextEditor.Portable.csproj` собирает изменённый компонент
  из исходников для .NET Framework 4.7.2. Ссылки на него в `PluginsSupport`,
  `VisualPascalABCNET` и `CodeTemplatesPlugin` не дают старой готовой DLL
  перезаписать новую при полной сборке.
- `VisualPascalABCNET\DockContent\WindowsCaptionTheme.cs` — заголовок главного
  окна через DWM на Windows 11; на предыдущих версиях Windows его цвет зависит
  от системной темы.
- `VisualPascalABCNET\Form1.cs` — пункт меню и применение темы к открытым
  редакторам и панелям вывода.
- `VisualPascalABCNET\Workbench\OptionsManager.cs` — сохранение и загрузка
  выбора темы.
- `scripts\build-portable.ps1` и `scripts\test-spython-editor.ps1` — упаковка
  и воспроизводимая проверка.

Папка `Release` не хранится в Git: итоговый ZIP нужно собрать локально после
клонирования ветки.
