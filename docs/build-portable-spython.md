# Воспроизводимая сборка portable-версии с редактором SPython

Готовый архив доступен в [релизах GitHub](https://github.com/informatik-shi/pascalabcnet_pro_edition/releases):
[скачать portable-сборку для Windows x64](https://github.com/informatik-shi/pascalabcnet_pro_edition/releases/download/portable-2026-10-08.2/PascalABCNET-Portable-win-x64.zip).
Ссылка ведёт на предыдущий опубликованный релиз; поддержка Matplotlib пока
есть в ветке `portable` и в архиве, собранном по инструкции ниже.
О запуске тетрадок и inline-графике читайте в
[отдельном руководстве](notebook.md).

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

Команда **Файл → Новый**, кнопка на панели и соответствующее сочетание клавиш
создают новый `.pys`, если активна вкладка `.pys`. Номер в имени `ProgramN.pys`
выбирается среди уже открытых и существующих файлов. Для активной вкладки Pascal
сохраняется обычное имя `ProgramN.pas`.

В меню **Вид → Тёмная тема интерфейса** снимите галочку, чтобы
переключить все вкладки кода на светлую палитру; повторное нажатие возвращает
тёмную. Переключение сразу меняет фон и подсветку открытых редакторов, верхнее
меню, его выпадающие списки, панель инструментов, строку состояния, контекстные
меню (включая полосу значков и выделенный пункт), вкладки документов,
строку «Глобальные имена», левое поле значков и полосы прокрутки редактора,
заголовки и разделители служебных окон, окно вывода, поле ввода и консоль компилятора.
На Windows 11 меняется и заголовок главного окна. Выбор записывается при закрытии IDE в
`Work\PascalABCNET.ini` внутри распакованной portable-папки. Запускайте
portable-версию через `PascalABCNET.cmd`: он указывает IDE путь к этой папке.

## Чтение файлов в SPython

Для текстовых файлов доступны `open(path)`, режимы `'r'`, `'rt'`, `'tr'`,
аргументы `encoding`, `errors`, `newline`, методы `read`, `readline`,
`readlines`, `seek`, `tell`, `close` и перебор строк в `for`. При `newline=None`
окончания строк CRLF и CR преобразуются в LF. Пример:

```python
f = open('notes.txt', encoding='utf-8')
for line in f:
    print(line)
f.close()
```

Для бинарного чтения укажите режим `'rb'` или `'br'` непосредственно либо через
строковую переменную. При буквальном режиме `read()` имеет тип `bytes`; при
режиме из переменной тип результата выбирается во время выполнения. В обоих
случаях доступны `len(data)`, индексирование (включая отрицательные индексы),
`data.hex()`, `data.decode('utf-8')` и `struct.unpack()`.

```python
import struct

mode = 'rb'
f = open('header.bin', mode)
header = f.read(6)
number, offset = struct.unpack('<Ih', header)
print(number + 1, offset * 2)
print(header[0] + 1)
f.close()
```

Модуль `struct` предоставляет `calcsize`, `unpack`, `unpack_from`,
`iter_unpack` и `Struct(format)`. Поддержаны префиксы порядка байтов
`@ = < > !` и коды чтения `x c b B ? h H i I l L q Q n N P e f d s p`.
`unpack` возвращает последовательность значений `StructValue`. Числовые поля
поддерживают арифметику `+`, `-`, `*`, `/`, `//`, `%`, сравнения и большие целые
значения без промежуточного `int()` или `float()`. Для полей `s`, `p`, `c`
доступны `hex()` и `decode()`.

Для `with open(path, 'rb') as source:` файл закрывается при обычном выходе
из блока, `return` и исключении. Работают `try/except Exception as err`
и параметры функций со значением по умолчанию. Функции без аннотации
возвращаемого типа получают динамическую оболочку `PyValue`: её можно
перебирать в `for`, передавать в `len()` и выводить через `print()`.
`struct.iter_unpack()` возвращает вложенные списки SPython, поэтому работает
генератор `[value[0] for value in struct.iter_unpack(...)]`. Сейчас это
материализованный список, тогда как CPython возвращает ленивый итератор
кортежей.

Исполняются `lambda` с нулём, одним или двумя параметрами. Сейчас оболочка
`PyValue` в теле `lambda` поддерживает сложение и конкатенацию строк, вычитание,
умножение, индексирование и `len()`; например, `lambda x: x + 1` и
`lambda text: text + '!'`. Замыкания на переменные внешней функции также
работают. Остальные операции и три и более параметров пока требуют дальнейшей
реализации.

Пример из проверки находится в
`TestSuiteAdditionalLanguages\SPythonTests\CompilationSamples\read_sbl_lambda.pys`.
В исходном примере пользователя проверка
`len(data) % sampling_quant == 0` возвращала пустой результат для корректного
файла. В проверяемом варианте используется `!= 0`.

Запись (`w`, `a`, `+`), срезы `bytes` и
полное поведение `tell`/`seek` для UTF-16 и дополнительных кодировок пока не
реализованы. Смещения для UTF-8 и однобайтового текста проверены тестами.
`with` пока поддерживает один контекстный менеджер с `as`; `try` поддерживает
`except` и `finally`, но не `else`. Типы и операции внутри `PyValue` пока
не охватывают всю динамику CPython.
Эти ограничения связаны со статической типизацией и текущим парсером SPython.
Объект чтения при режиме из переменной и значения `StructValue` представляют
собой обёртки: например, проверка типа через `type()` ещё не эквивалентна CPython.
Код работает без установки Python.

Для повторения этого изменения нужны три части. `OpenModeVisitor.cs` оставляет
буквальные режимы с конкретными типами файла, а режим из выражения направляет
в `PythonFile` из `SPythonSystem.pas`; этот класс выбирает текстовый или
бинарный поток при выполнении. `PythonReadData` передаёт прочитанные байты в
`struct` и поддерживает индексирование. `struct1.pas` возвращает `StructValue`
с числовыми операторами. После изменения этих файлов выполните полную сборку
ниже, чтобы обновились оба компилятора и обе версии `SPythonSystem.pcu` и
`struct1.pcu`, затем запустите 64 проверки для каждого компилятора.
`scripts\test-spython-read-sbl-lambda.ps1` создаёт бинарные файлы с полями
`h`, `i`, `q` и некратной длиной, компилирует пользовательский алгоритм,
проверяет 14 строк вывода, обработку исключения внутри `with` и работу
`lambda` на обоих компиляторах.

### Аннотации типов и неизменный тип переменной

SPython использует привычный синтаксис Python для аннотаций переменных,
аргументов и результата функции. Проверены `int`, `float`, `str`, `bool`,
`list[T]`, `dict[K, V]`, `set[T]`, `tuple[T1, T2]` и вложенные типы. Например:

```python
values: dict[str, int] = {'a': 1}
def label(x: str) -> str:
    return x
print(label('z'))
```

В отличие от CPython, SPython сохраняет статический тип имени внутри области
видимости. `a = lambda x: x + 1; a: int = 10` не компилируется из-за
несовместимых типов; обычное `a = 10` тоже отклоняется. Повторное
присваивание и повторная аннотация того же типа разрешены. Это проверяет
`NameCorrectVisitor.cs` при повторной аннотации и семантический анализатор
`spython_syntax_tree_visitor.cs` на этапе компиляции. Локальная переменная
функции может затенять внешнюю переменную с другим типом. Отдельные выражения
в программе, например `a + 4`, допустимы: результат вычисляется и
отбрасывается.

Односимвольные литералы SPython теперь всегда преобразует в `str`; без этого
`{'a': 1}` выводилось как словарь с ключом `char`, а аннотация
`dict[str, int]` давала ложную ошибку. Это реализовано в
`AssignmentCharAsStringVisitor.cs`. Отдельные выражения преобразуются через
`!discard<T>` из `SPythonSystem.pas`, поскольку прямой вывод такого узла
создавал некорректный исполняемый файл.

Это ещё не вся система аннотаций CPython. В частности, `int | None`,
`typing.Optional`, `typing.Callable` и `typing.Any` пока не поддерживаются.
Кроме того, обычный Python
хранит аннотации как подсказки и не запрещает смену типа при выполнении;
запрет в SPython — намеренное правило проекта. Справочник по синтаксису:
[Python typing](https://docs.python.org/3/library/typing.html).

После изменения грамматики, преобразователя и стандартного модуля повторите
генерацию парсера и полную сборку, как показано ниже, затем выполните:

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File scripts\test-spython-type-annotations.ps1
powershell.exe -NoProfile -ExecutionPolicy Bypass -File scripts\test-spython-type-annotations.ps1 -Runtime net10
```

Для изменений грамматики обновите сгенерированные файлы парсера до сборки:

```powershell
Push-Location AdditionalLanguages\SPython\SPythonParserKrylovMovchan
..\..\..\Utils\GPLex_GPPG\Gppg.exe /no-lines /gplex SPythonParser.y
..\..\..\Utils\GPLex_GPPG\Gplex.exe /unicode SPythonLexer.lex
Pop-Location
```

Изменения новой проверки затрагивают правила `try`, `except`, `finally`,
`with`, параметры функций и `lambda` в `SPythonParser.y` и `.lex`, обработку
имён и локальных переменных в `SPythonStandardTreeConverter`, тип `PyValue`
и файловые функции в `SPythonSystem.pas`, а также результат `iter_unpack`
в `struct1.pas`. `HoistFunctionLocalsVisitor.cs` переносит объявление
переменной на уровень функции, когда значение присвоено внутри блока,
используется после блока и тип известен или выводится для бинарного `read()`.
Исходное присваивание остаётся на месте, чтобы сохранить порядок выполнения.

## Путь к совместимости с Python

Текущая цель проекта — постепенно расширять SPython до совместимости с CPython,
сохраняя компиляцию в .NET. Для каждого следующего шага добавляйте один и тот
же исполняемый пример в тесты SPython и CPython и сравнивайте значения,
исключения, область видимости и закрытие ресурсов. В первую очередь нужны
общие правила динамических операций для `PyValue`, полноценные области
видимости функций и замыканий, остальные формы `lambda`, `with` и `try`, а
затем покрытие стандартной библиотеки. Эталонные правила для этих конструкций
описаны в [спецификации Python](https://docs.python.org/3/reference/compound_stmts.html)
и [разделе о `lambda`](https://docs.python.org/3/reference/expressions.html#lambda).

Компиляция в .NET сама по себе не гарантирует ускорение программ с динамическими
операциями. После достижения одинакового поведения измеряйте время и память
на одинаковых входных данных и фиксируйте версии обоих компиляторов.

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
powershell.exe -NoProfile -ExecutionPolicy Bypass -File scripts\test-spython-file-io.ps1
powershell.exe -NoProfile -ExecutionPolicy Bypass -File scripts\test-spython-file-io.ps1 -Runtime net10
powershell.exe -NoProfile -ExecutionPolicy Bypass -File scripts\test-spython-read-sbl-lambda.ps1
powershell.exe -NoProfile -ExecutionPolicy Bypass -File scripts\test-spython-read-sbl-lambda.ps1 -Runtime net10
powershell.exe -NoProfile -ExecutionPolicy Bypass -File scripts\test-spython-type-annotations.ps1
powershell.exe -NoProfile -ExecutionPolicy Bypass -File scripts\test-spython-type-annotations.ps1 -Runtime net10
```

Если установлен CPython, после сценариев сравните его вывод на тех же файлах:

```powershell
python TestSuiteAdditionalLanguages\SPythonTests\CompilationSamples\read_sbl_lambda.pys
```

Ожидаются те же 14 строк, что у обоих компиляторов SPython.

Если SDK установлен в нестандартный каталог, передайте `-DotnetRoot`,
например `-DotnetRoot C:\Users\PC\.dotnet`. Сценарий собирает IDE и оба
компилятора, пересоздаёт стандартные `.pcu`, копирует нужные ресурсы и
Runtime, затем создаёт ZIP. Файл `bin\Lib\PABCRtl.dll` входит в ветку как
готовая зависимость; сценарий не собирает эту DLL заново.
Если старая portable-версия запущена из `Release`, укажите
`-OutputRoot '.codex-build\release-next'`, чтобы не удалять используемую папку.

Тестовый сценарий запускайте после сборки. Он проверяет 24 сценария Enter
через настоящий компонент редактора, обе палитры `.pys`, 20 тёмных режимов
подсветки для остальных файлов, переключение редактора, меню и цвета обеих панелей
вывода. Также он проверяет палитру вкладок и заголовков dock-панелей,
реально нарисованные горизонтальную полосу прокрутки и левое поле значков,
полосу значков выпадающего меню, выделенный верхний пункт, строку выбора класса и,
на Windows 11, применение цвета заголовка окна. Проверяется и выбор `.pys` или
`.pas` для нового файла, включая занятые имена. Он должен
завершиться строкой `PASS`.

Для ручной проверки распакуйте ZIP в отдельную папку с правом записи:

```powershell
Expand-Archive -LiteralPath Release\PascalABCNET-Portable-win-x64.zip -DestinationPath .\portable-smoke
& '.\portable-smoke\PascalABCNET-Portable-win-x64\PascalABCNET.cmd'
powershell.exe -STA -NoProfile -ExecutionPolicy Bypass -File scripts\test-spython-editor.ps1 -BinDirectory '.\portable-smoke\PascalABCNET-Portable-win-x64\bin'
powershell.exe -NoProfile -ExecutionPolicy Bypass -File scripts\test-spython-file-io.ps1 -Runtime classic -PackageRoot '.\portable-smoke\PascalABCNET-Portable-win-x64'
powershell.exe -NoProfile -ExecutionPolicy Bypass -File scripts\test-spython-file-io.ps1 -Runtime net10 -PackageRoot '.\portable-smoke\PascalABCNET-Portable-win-x64'
powershell.exe -NoProfile -ExecutionPolicy Bypass -File scripts\test-spython-read-sbl-lambda.ps1 -Runtime classic -PackageRoot '.\portable-smoke\PascalABCNET-Portable-win-x64'
powershell.exe -NoProfile -ExecutionPolicy Bypass -File scripts\test-spython-read-sbl-lambda.ps1 -Runtime net10 -PackageRoot '.\portable-smoke\PascalABCNET-Portable-win-x64'
powershell.exe -NoProfile -ExecutionPolicy Bypass -File scripts\test-spython-type-annotations.ps1 -Runtime classic -PackageRoot '.\portable-smoke\PascalABCNET-Portable-win-x64'
powershell.exe -NoProfile -ExecutionPolicy Bypass -File scripts\test-spython-type-annotations.ps1 -Runtime net10 -PackageRoot '.\portable-smoke\PascalABCNET-Portable-win-x64'
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
Оставьте активным `.pys` и нажмите **Файл → Новый**: новая вкладка должна иметь
расширение `.pys` и подсветку SPython. Повторите с пустой новой вкладкой и с
вкладкой, где уже есть текст; проверьте, что имя не совпадает с открытым файлом.
Затем активируйте `.pas` и повторите команду: новая вкладка должна быть `.pas`.
Фон области кода и окна вывода должен быть `#1E1E1E`, текст и подсветка —
светлыми. Запустите программу с выводом (`print('test')`) и проверьте, что он
виден на тёмном фоне. Верхнее меню, выпадающие пункты, вкладки документов,
строка «Глобальные имена», левое поле значков редактора, горизонтальная и
вертикальная полосы прокрутки, область значков меню и выбранный верхний пункт,
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
`portable-2026-10-08.1`:

```powershell
git push origin portable
git tag portable-2026-10-08.1
git push origin portable-2026-10-08.1
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
  вложенных и контекстных меню, панели инструментов и строки состояния. Рендерер
  явно закрашивает полосу значков и выделенный пункт: одной таблицы цветов
  WinForms для этих областей оказалось недостаточно.
- `Libraries\WeifenLuo\WinFormsUI\Docking\DockPanelTheme.cs` и классы
  `VS2005DockPaneStrip`, `VS2005DockPaneCaption`, `VS2005AutoHideStrip`,
  `DockPanel`, `DockWindow`, `DockPane` — цвета вкладок, заголовков,
  разделителей и область вокруг dock-панелей. `Form1.ApplyEditorTheme()`
  вызывает `MainDockPanel.ApplyTheme()` при каждом переключении.
- `VisualPascalABCNET\IB\CodeCompletion\QuickClassBrowserPanel.cs` — цвета
  строки «Глобальные имена» и выпадающих списков.
- `ICSharpCode.TextEditor\Src\Gui\ThemedScrollBars.cs` и `TextAreaControl.cs` —
  прорисовка полос прокрутки редактора в обеих темах с сохранением прокрутки.
  `IconBarMargin.cs` рисует крайнее левое поле для закладок и точек останова;
  его фон переключается вместе с полосами прокрутки.
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
- `bin\Lib\SPython\SPythonSystem.pas` — текстовые и бинарные файлы, тип `bytes`.
- `bin\Lib\SPython\struct1.pas` — чтение форматов Python `struct`.
- `AdditionalLanguages\SPython\SyntaxTreeConverters\SPythonStandardTreeConverter\OpenModeVisitor.cs`
  — выбор конкретного типа файла для буквального режима и динамического файла
  для режима из переменной. `FunctionsWithNamedParametersDesugarVisitor.cs`
  сохраняет именованные аргументы `open()`.
- `AdditionalLanguages\SPython\SPythonLanguageInfo\SPythonLanguageInformation.cs`
  — соответствие имени Python `struct` модулю Pascal `struct1`.
- `ReleaseGenerators\RebuildStandartModulesSPython.pas` — пересборка `struct1.pcu`.
- `scripts\test-spython-file-io.ps1` — создание тестовых файлов и 64 проверки
  чтения, динамического режима и арифметики с обычным и .NET 10 компилятором.
- `bin\Lib\SPython\itertools1.pas` — итераторы `product`, `permutations`,
  `combinations`, а также `chain` и `repeat`.
- `AdditionalLanguages\SPython\SyntaxTreeConverters\SPythonStandardTreeConverter\FunctionsWithNamedParametersDesugarVisitor.cs`
  — перевод `product(..., repeat=n)` и `repeat(..., times=n)` в вызовы
  библиотеки без потери именованных параметров.
- `scripts\test-spython-itertools.ps1` и
  `TestSuiteAdditionalLanguages\SPythonTests\CompilationSamples\itertools_core.pys`
  — сравнение результатов с обычным Python 3.

## Проверка itertools

В SPython доступны основные комбинаторные функции:

```python
import itertools

for number, letter in itertools.product([1, 2], 'AB'):
    print(number, letter)

for pair in itertools.permutations('ABC', 2):
    print(pair)

for pair in itertools.combinations('ABC', 2):
    print(pair)
```

`product(..., repeat=2)` принимает именованный аргумент `repeat`.
`permutations(iterable)` по умолчанию использует полную длину входа.
Результаты можно перебирать только один раз, как итераторы Python. Элементы
результирующих кортежей можно индексировать и распаковывать, а длину кортежа
получать через `len()`.

После полной сборки проверьте обе версии компилятора на одном примере.
Python 3 нужен только для сравнения в тесте; portable-комплект не запускает
Python при работе программ.

```powershell
scripts\test-spython-itertools.ps1 -Runtime classic -PythonExe 'C:\Path\To\python.exe'
scripts\test-spython-itertools.ps1 -Runtime net10 -PythonExe 'C:\Path\To\python.exe'
```

Тест вызывает `product`, `permutations`, `combinations` на строках и списках,
проверяет `repeat=`, пустые результаты, одинаковые значения на разных
позициях, распаковку кортежей, порядок результатов и продолжение итератора
после `break`. Он также проверяет ранее существовавшие `chain` и `repeat`.
Каждая строка вывода SPython сравнивается со строкой, которую выдаёт Python 3.

Реализация `product`, `permutations` и `combinations` хранит входные
последовательности в памяти, как Python `itertools`, и выдаёт комбинации по
одной. `product` принимает однородные и смешанные типы входных элементов;
для смешанных типов доступ к элементам кортежа возвращает `PyValue`.
Остальные функции стандартного `itertools` ещё не поддержаны. Это не полная
совместимость с Python, но тестируемая основа для её расширения.

Папка `Release` не хранится в Git: итоговый ZIP нужно собрать локально после
клонирования ветки.

## Модуль `ipaddress`

Реализация находится в `bin\Lib\SPython\ipaddress1.pas`. Для программы SPython
она доступна под обычным именем `ipaddress`: соответствие задаётся в
`AdditionalLanguages\SPython\SPythonLanguageInfo\SPythonLanguageInformation.cs`.
`ReleaseGenerators\RebuildStandartModulesSPython.pas` включает модуль в
пересборку стандартных PCU. Добавленные в `SPythonSystem.pas` типы исключений
`ValueError` и `TypeError` позволяют перехватывать соответствующие ошибки.
Исправление в `Compiler\PCU\PCUReader.cs` позволяет загрузчику PCU восстанавливать
методы, которые он обнаруживает при восстановлении других методов; без этого
операторы сравнения новых типов вызывали внутренний сбой компилятора.

Поддержаны фабрики `ip_address`, `ip_network`, `ip_interface`, функции и
конструкторы `IPv4Address`, `IPv6Address`, `IPv4Network`, `IPv6Network`,
`IPv4Interface`, `IPv6Interface`, операции `summarize_address_range`,
`collapse_addresses`, `get_mixed_type_key`, `v4_int_to_packed`,
`v6_int_to_packed`. Адреса дают
`packed`, `compressed`, `exploded`, `reverse_pointer`, признаки частных,
глобальных, зарезервированных, multicast, loopback, link-local и unspecified
адресов. Для IPv6 доступны `scope_id`, `ipv4_mapped`, `sixtofour`, `teredo`;
для IPv4 — `ipv6_mapped`. Сети дают маски, границы, число адресов, перебор,
`hosts`, `subnets`, `supernet`, `subnet_of`, `supernet_of`, `overlaps`,
`address_exclude` и `compare_networks`. Поддержаны префиксы и маски IPv4,
`strict=False`, адрес с префиксом в виде пары, а также сетевые зоны IPv6.
Проверенные таблицы `is_private` соответствуют Python 3.13, включая его
исключения для IPv4 `192.0.0.9`, `192.0.0.10` и отдельных IPv6 адресов.

Для воспроизводимой сборки на Windows нужны .NET 10 SDK и установленный
Python 3.13 для сравнения результатов. Из корня ветки `portable` выполните:

```powershell
scripts\build-portable.ps1 -DotnetRoot "$env:USERPROFILE\.dotnet"
scripts\test-spython-ipaddress.ps1 -Runtime classic -PythonExe 'C:\Path\To\python.exe'
scripts\test-spython-ipaddress.ps1 -Runtime net10 -PythonExe 'C:\Path\To\python.exe'
```

Тест сверяет вывод SPython с Python построчно. Образцы находятся в
`TestSuiteAdditionalLanguages\SPythonTests\CompilationSamples\ipaddress_*.pys`:
проверяются адреса, сети, интерфейсы, классификация, преобразования в целое
и байты, подсети, суммирование, ошибки входных данных и сортировка. Чтобы
проверить готовый комплект, добавьте к тестовой команде
`-PackageRoot 'Release\PascalABCNET-Portable-win-x64'`.

Полная совместимость со стандартным `ipaddress` пока не достигнута. SPython
предоставляет именованные функции вместо классов Python, поэтому `type()`,
`isinstance()` и `repr()` отличаются. Смешанный литерал списка адресов и
сетей не компилируется из-за статической типизации SPython, хотя
`collapse_addresses` принимает перечисляемые объекты обоих видов.
`get_mixed_type_key` возвращает сравнимый ключ, но передача его как `key=` в
`sorted()` пока не компилируется. Форматирование адресов через Python
`format()`, целые литералы больше диапазона встроенного парсера SPython и
точные тексты
всех сообщений об ошибках ещё не перенесены. Эти ограничения требуют
изменений в языке SPython, а не только добавления модуля.

## Модуль `re`

SPython импортирует `re` как стандартный модуль. Реализация находится в
`bin\Lib\SPython\re1.pas`, сопоставление имён — в
`AdditionalLanguages\SPython\SPythonLanguageInfo\SPythonLanguageInformation.cs`.
`ReleaseGenerators\RebuildStandartModulesSPython.pas` пересобирает его PCU
для классического компилятора и .NET 10. Сторонних пакетов не нужно:
поиск выполняет `System.Text.RegularExpressions` из .NET. Обёртка переводит
синтаксис именованных групп Python и сохраняет сквозную нумерацию групп.

Доступны `compile`, `search`, `match`, `fullmatch`, `findall`, `finditer`,
`split`, `sub`, `subn`, `escape`, `purge`, основные флаги и их короткие имена,
`PatternError`/`error`. Объект шаблона содержит `pattern`, `flags`, `groups`,
`groupindex` и соответствующие методы. Объект совпадения содержит `group`,
`groups`, `groupdict`, `start`, `end`, `span`, `expand`, `string`, `pos`,
`endpos`, `re`, `lastindex`, `lastgroup`, `regs`; индекс `m[1]` и несколько
аргументов у `m.group(1, 2)` тоже работают. Строковые и байтовые шаблоны
раздельны; байтовые группы, части и результаты замены возвращают `bytes`.
`finditer` расходуется за один проход, как итератор Python. Поддержаны
позиционные и именованные аргументы основных функций. Для замены можно
передать строку, байты или функцию, включая `lambda m: m.group()`.

Лексер SPython в `SPythonLexer.lex`, сгенерированный `SPythonLexer.cs` и
`SPythonParserTools.cs` теперь распознают `r'...'`, `b'...'`, `rb'...'` и
`br'...'` (также двойные кавычки и заглавные префиксы). Побайтовый литерал
создаётся через `!bytes_literal` в `SPythonSystem.pas`. Для изменения правил
литералов нужно править `.lex`, запустить генератор лексера и затем
пересобрать компилятор:

```powershell
Push-Location AdditionalLanguages\SPython\SPythonParserKrylovMovchan
& ..\..\..\Utils\GPLex_GPPG\Gplex.exe /unicode SPythonLexer.lex
Pop-Location
scripts\build-portable.ps1 -DotnetRoot "$env:USERPROFILE\.dotnet"
```

Для проверки нужна установленная Python 3.13. Из корня ветки `portable`:

```powershell
scripts\test-spython-re.ps1 -Runtime classic -PythonExe 'C:\Path\To\python.exe'
scripts\test-spython-re.ps1 -Runtime net10 -PythonExe 'C:\Path\To\python.exe'
scripts\test-spython-re.ps1 -Runtime net10 -PackageRoot 'Release\PascalABCNET-Portable-win-x64' -PythonExe 'C:\Path\To\python.exe'
```

Скрипт компилирует четыре файла `re_*.pys` из
`TestSuiteAdditionalLanguages\SPythonTests\CompilationSamples`, выполняет их
на SPython и CPython и сравнивает каждую строку. Проверяются поиск,
позиции, группы и обратные ссылки, замены, разбиение, пустые совпадения,
флаги, байты, именованные аргументы, один проход итератора и исключения.
Единственная нормализация вывода — существующее отличие консоли SPython:
нулевое значение она печатает как `nil`, тогда как CPython печатает `None`.

Полное соответствие `re` пока недостижимо из-за различий движков и
статической типизации SPython:

- .NET считает позиции в кодовых единицах UTF-16, а Python — в символах
  Unicode. Для текста с символами вне базовой многоязычной плоскости
  `span()`, `start()` и `end()` могут отличаться.
- Наборы Unicode для `\w`, границ слова и регистронезависимого сопоставления
  отличаются; особенно это заметно на комбинируемых символах и отдельных
  буквах. Флаг `ASCII` переводит наиболее частые классы `\w`, `\d`, `\s` и
  границы слова в ASCII, но сложные вложенные символьные классы могут вести
  себя иначе.
- Синтаксис условных выражений, некоторые вложенные флаги (например,
  `(?a:...)`), правила допустимости lookbehind и отдельные редкие случаи
  обратных ссылок у Python и .NET различаются. Некоторые шаблоны, которые
  Python отвергает, .NET принимает, и наоборот. Сообщения об ошибках и поля
  объекта `PatternError` не повторяют CPython дословно.
- `LOCALE` для байтового шаблона не реализован; `DEBUG` не печатает разбор
  выражения; `TEMPLATE` не меняет семантику движка. `purge()` не управляет
  кэшем: сейчас шаблоны не кэшируются. Методы `Pattern.scanner()` и тип
  `re.Match` как отдельное имя модуля отсутствуют.
- Для `findall()` и `split()` тип результата в SPython — `list<object>`,
  поскольку число захватывающих групп известно только во время выполнения.
  Элементы содержат строки, байты или кортежи, как в Python, но статический
  компилятор не разрешает напрямую вызвать строковый метод у такого
  элемента без явного преобразования. Смешивание строкового и байтового
  шаблона обычно выявляется уже при компиляции SPython.
- Байтовые литералы пока поддерживают однострочную запись. Полные правила
  Python для тройных кавычек, всех префиксов и диагностики неверных escape
  последовательностей в исходном коде SPython ещё не перенесены.

Эти ограничения перечислены для проверки переносимых программ; для
обычного поиска, извлечения групп и замен используйте тот же код, что в
Python.

## Matplotlib в SPython

Графики строит настоящий Matplotlib 3.11.2 в отдельном процессе Python 3.13.16.
Portable-архив содержит этот Python, Matplotlib 3.11.2, NumPy 2.5.4 и
необходимые зависимости; на компьютере пользователя ничего устанавливать не
нужно. Программа SPython по-прежнему компилируется в .NET; время построения
графика определяется Python/Matplotlib. NumPy и pandas как модули языка
SPython пока не подключены.

```python
import matplotlib.pyplot as plt

x = [0, 1, 2, 3]
y = [0, 1, 4, 9]
plt.plot(x, y, color='red', label='квадрат')
plt.xlabel('x')
plt.ylabel('y')
plt.legend()
plt.savefig('graph.png')
```

Работают также `from matplotlib import pyplot as plt`,
`from matplotlib.pyplot import savefig`, распаковка `fig, ax = plt.subplots()`
и вызовы `ax.plot`, `ax.scatter`, `fig.savefig`. Доступны обычные функции
`plot`, `scatter`, `bar`, `hist`, `imshow`, `figure`, `subplots`, `subplot`,
`gca`, `gcf`, `title`, `xlabel`, `ylabel`, `legend`, `grid`, `xlim`, `ylim`,
`tight_layout`, `savefig`, `show`, `close`, `clf`, `cla` и основные методы
`Axes`/`Figure`, перечисленные в `bin\Lib\SPython\pyplot1.pas`.
Параметры линий `color`, `label`, `linewidth`, `linestyle`, `marker`, `alpha`
можно передавать по имени. Массивы и списки SPython преобразуются в Python
последовательности; объекты Matplotlib сохраняют идентичность внутри одного
процесса Python.

В тетрадке Matplotlib показывается под ячейкой как PNG. `plt.show()` в
обычной программе открывает изображение в системном просмотрщике Windows;
это статический снимок без интерактивных инструментов Matplotlib. Для
экспорта используйте `savefig`, как в Python. Для нескольких фигур в одной
ячейке тетрадка сейчас показывает последнюю активную фигуру.

Парсер `SPythonParser.y` разрешает точки в имени импортируемого модуля.
После изменения грамматики пересоздайте `SPythonParserYacc.cs` теми же
параметрами, что использует проект:

```powershell
Push-Location AdditionalLanguages\SPython\SPythonParserKrylovMovchan
& ..\..\..\Utils\GPLex_GPPG\Gppg.exe /no-lines /gplex SPythonParser.y
Pop-Location
```

Мост находится в `bin\Lib\SPython\matplotlib_bridge.py`, а оболочка SPython —
в `bin\Lib\SPython\pyplot1.pas`. Запросы передают числа, строки,
последовательности, словари и ссылки на Python-объекты; ссылки остаются
действительными между вызовами. `PythonCall`, `PythonMethod`,
`PythonAttribute`, `PythonIndex` в Pascal-модуле используют тот же процесс и
служат точкой расширения для будущих NumPy и pandas. Большие массивы NumPy
сейчас не создаются из SPython без копирования: отдельные оболочки для них
ещё предстоит сделать.

Полной совместимости с Python API пока нет. SPython имеет статическую
типизацию и фиксированные сигнатуры функций. `import matplotlib` с дальнейшим
`matplotlib.pyplot`, `plt.style.use`, `rcParams`, произвольные имена функций,
произвольные `**kwargs`, интерактивные backend-окна и динамические атрибуты
Artist пока не поддерживаются. При неподдерживаемом аргументе или методе
нужна дополнительная оболочка в `pyplot1.pas`; сам графический движок
Matplotlib при этом менять не требуется. Поведение конкретных поддержанных
вызовов определяется закреплённой версией Matplotlib, а не самостоятельной
реализацией графики.

Для повторения сборки на Windows x64 требуются .NET 10 SDK, установленный
Python 3.13 с `pip` **только на машине сборки** и доступ к python.org/PyPI
при первом запуске. Из корня ветки `portable`:

```powershell
scripts\build-portable.ps1 -DotnetRoot "$env:USERPROFILE\.dotnet" -PythonExe 'C:\Path\To\python.exe'
scripts\test-spython-matplotlib.ps1 -Runtime classic -PackageRoot 'Release\PascalABCNET-Portable-win-x64'
scripts\test-spython-matplotlib.ps1 -Runtime net10 -PackageRoot 'Release\PascalABCNET-Portable-win-x64'
```

`scripts\prepare-python-matplotlib.ps1` загружает официальный встраиваемый
Python 3.13.16, проверяет SHA-256 и устанавливает версии из
`scripts\matplotlib-requirements.txt` в `.codex-build\python-matplotlib`.
Следующие сборки используют кэш. `build-portable.ps1` кладёт его в папку
`python` архива и проверяет запуск Matplotlib до упаковки. Три примера
`matplotlib_*.pys` проверяются против CPython: совпадают SHA-256 сохранённых
PNG, а также проходят обычный и .NET 10 компиляторы.
