param(
    [Parameter(Mandatory = $true)][string]$QtRoot,
    [string]$CMake = 'cmake'
)

$ErrorActionPreference = 'Stop'
$nativeRoot = Split-Path -Parent $PSScriptRoot
$cmakeCommand = (Get-Command $CMake -ErrorAction Stop).Source
$ctestCommand = Join-Path (Split-Path -Parent $cmakeCommand) 'ctest.exe'
$buildDir = Join-Path $nativeRoot 'build/qt-editor'
$packageDir = Join-Path $nativeRoot 'dist/Qt'
$zipPath = Join-Path $nativeRoot 'dist/ABC-Native-Qt-win-x64.zip'

function Invoke-BuildTool([string]$Tool, [string[]]$Arguments) {
    & $Tool @Arguments | Where-Object { $_ -notmatch '^-- (Installing|Up-to-date):' }
    if ($LASTEXITCODE -ne 0) { throw "$Tool failed with exit code $LASTEXITCODE" }
}

Invoke-BuildTool $cmakeCommand @('-S', $nativeRoot, '-B', $buildDir,
    '-G', 'Visual Studio 17 2022', '-A', 'x64', '-DABC_BUILD_IDE=ON', '-DABC_BUILD_TK=OFF',
    '-DBUILD_TESTING=ON', "-DCMAKE_PREFIX_PATH=$QtRoot")
Invoke-BuildTool $cmakeCommand @('--build', $buildDir, '--config', 'Release', '--parallel', '6')
Invoke-BuildTool $ctestCommand @('--test-dir', $buildDir, '-C', 'Release', '--output-on-failure')
Invoke-BuildTool $cmakeCommand @('--install', $buildDir, '--config', 'Release', '--prefix', $packageDir)
Set-Content -LiteralPath (Join-Path $packageDir 'Launch.cmd') -Encoding ascii -Value "@echo off`r`nstart `"`" `"%~dp0bin\abc-ide.exe`"`r`n"
$startText = @"
ABC Native — Qt — экспериментальная версия

Распакуйте архив целиком и запустите Launch.cmd.
Ночная тема включена по умолчанию.
F8 — проверить программу. F9 — запустить.
Когда программа ждёт Read/Readln, введите значение в строке под консолью и нажмите Enter.
Кнопка «Остановить» завершает выполняемую программу.
Ошибки выполнения показываются на английском (EN) и русском (RU),
с кодом ошибки, строкой и столбцом исходника.

Поддерживаются integer, int64, single, real/double, boolean, char, byte, shortint, smallint, word, longword, uint64, string,
динамические и двумерные прямоугольные массивы, записи, пользовательские функции и процедуры с var-параметрами,
условия if, циклы while/for/repeat и foreach по строкам/массивам/List<T>, break/continue, базовая математика, строки, List<T>, Stack<T>, Queue<T>,
DynArray<T>, SimpleSet<T>, AssocArray<K,V>, LinkedList<T>, LinkedListNode<T>, SingleNode<T> и текстовые файлы с выбором кодировки.
Для классов Collections добавьте uses Collections; Для ReadFileToEnd/WriteStringToFile — uses FilesOperations;
Примеры: examples/structures.pas, examples/files.pas, examples/collections.pas, examples/buffers.pas,
examples/associations.pas, examples/linked-lists.pas, examples/single-nodes.pas, examples/standard-library.pas,
examples/text-numbers.pas, examples/string-editing.pas, examples/nil-strings.pas, examples/directories.pas, examples/renames.pas, examples/text-files.pas, examples/text-methods.pas, examples/text-file-changes.pas, examples/loops.pas, examples/foreach.pas, examples/characters.pas, examples/file-operations.pas, examples/small-integers.pas, examples/unsigned-integers.pas, examples/random-arrays.pas, examples/reverse-shuffle.pas, examples/extended-math.pas, examples/array-output.pas, examples/bitwise.pas, examples/console-input.pas, examples/console-functions.pas, examples/unicode-output.pas, examples/matrices.pas.
Text/TextFile: Assign, Reset, Rewrite, Append, Read/Readln, Write/Writeln, Eof, Flush и Close.
Методы Text: ReadWord, ReadToEnd, Name/FullName, Write/Writeln, Print/Println, Erase/Rename и чтение значений.
Добавлены создание, удаление и переименование файлов и каталогов, работа с текущей папкой программы.
Строковый nil отличается от пустой строки; стандартные строковые операции учитывают это различие.
Процедурные значения: function/procedure, Func, Action, Predicate и короткие типы T->U; пример examples/callable-values.pas.
Типы параметров и результата проверяются до запуска; вызов nil даёт двуязычную ошибку.
Сохранённые функции принимают Select/Where/Zip и TrueTable; пример examples/library-callables.pas.
Лямбды и локальные захваты: examples/lambdas.pas.
Генераторы SeqGen/ArrGen/MatrGen/Iterate/SeqWhile: examples/generators.pas.
Декартово произведение и степени Cartesian/CartesianPower: examples/cartesian.pas.
Сочетания, перестановки и NextPermutation: examples/combinatorics.pas.
Индексы массивов, списков и матриц: examples/array-views.pas.
Строки и столбцы матриц, замена и обмен: examples/matrix-lines.pas.
Заполнение, форма, транспонирование и сравнение массивов: examples/array-operations.pas.
ConvertAll, Transform, Fill и ForEach: examples/collection-callbacks.pas.
Срезы матриц по индексам и включительным границам: examples/matrix-slices.pas.
Конструкторы Matr, MatrByRow и MatrByCol: examples/matrix-construction.pas.
Условные выражения if/then/else и ?: examples/conditional-expressions.pas.
Замена элементов и независимые копии массивов/List: examples/replace-copy.pas.
Поиск, вставка, удаление и диапазоны List: examples/list-ranges.pas.
Добавление и вставка диапазонов AddRange/InsertRange: examples/list-bulk-ranges.pas.
JoinToString, JoinIntoString и Flatten: examples/sequence-text-flatten.pas.
Sleep, Milliseconds и MillisecondsDelta: examples/timing.pas.
Конструкторы List, Capacity, EnsureCapacity и TrimExcess: examples/list-capacity.pas.
Фабрики Lst, LstInt и LstStr: examples/list-factories.pas.
Явные типы и пустой Lst: examples/explicit-list-types.pas.
Копирование списка в массив: examples/list-copy-to.pas.
Базовые классы и взаимные ссылки: examples/class-fields.pas.
Методы класса, Self и аргументы по умолчанию: examples/class-methods.pas.
Свойства класса с read/write: examples/class-properties.pas.
Конструкторы с параметрами: examples/class-constructors.pas.
Секции доступа и внешние тела методов: examples/class-external-methods.pas.
Начальные значения полей: examples/class-field-initializers.pas.
Короткие тела функций и методов: examples/short-class-bodies.pas.
Шестнадцатеричные числа `$FF, `$FF_FF и разделители 1_000_000: examples/numeric-literals.pas.
Циклы for ... to ... step с положительным и отрицательным шагом: examples/for-step.pas.
Перечисления, флаги, Ord/Inc/Low/High и типизированные коллекции: examples/enumerations.pas.
Перечисления проверяются до запуска; HasFlag и форматы G/D/X/F имеют двуязычные ошибки.
System.Enum как объект, reflection и поддиапазоны ещё не перенесены.
InRange, Between, Clamp и методы ClampTop/ClampBottom: examples/comparable-ranges.pas.
Нативный HashSet<T>, операции множеств и проверки включения: examples/hash-sets.pas.
CopyTo, TryGetValue, RemoveWhere и TrimExcess: examples/hash-set-extra.pas.
HSet/HSetInt/HSetStr, Copy и явные преобразования: examples/hash-set-factories.pas.
Записи IntRange/CharRange, 1..5, CopyTo и преобразования: examples/ordinal-ranges.pas.
RealRange, Random2/Random3 и случайные массивы по диапазону: examples/real-range-random.pas.
Ленивые SeqRandom/SeqRandomInteger/SeqRandomReal: examples/random-sequences.pas.
Групповой ввод ReadInteger/Real/Char/String2..4: examples/console-tuples.pas.
Потоковый обход каталогов: examples/directory-sequences.pas.
Выделение частей и полного имени файла: examples/file-names.pas.
Параметры программы и свободное место на диске: examples/system-services.pas.
Аргументы CLI: abc run файл.pas -- "два слова" аргумент.
Halt завершает программу с кодом 0; Halt(integer) задаёт код завершения: examples/halt.pas.
Стандартные функции как типизированные значения: examples/builtin-callables.pas.
Имена типов, typeof и GetType без .NET: examples/type-names.pas.
Свойства Name/FullName/Namespace/AssemblyQualifiedName: examples/type-properties.pas. Полные имена пользовательских типов сохраняют имя исходного файла при запуске из редактора.
Числа single с округлением до 32 бит, вводом и коллекциями: examples/single-numbers.pas.
Словари Dictionary, Dict, Keys/Values, EachCount и ToDictionary: examples/dictionaries.pas.
Перечислители словаря с независимым копированием: examples/dictionary-enumerators.pas.
Общие интерфейсы IEnumerable<T>/IEnumerator<T>: examples/enumerator-interfaces.pas.
Перечислители List<T> и HashSet<T> с копированием и интерфейсами: examples/collection-enumerators.pas.
Типизированный NewSet<T>, SetOf, копирование и процедуры: examples/new-sets.pas.
Литералы [1..4,9], EmptyCollection, ToSet и ToHashSet: examples/set-literals.pas.
Print/Println, PrintLines(map), матрицы и Include/Exclude: examples/print-extensions.pas.
PrintMatrixWithFormat управляет выравниванием матриц; PrintLines возвращает исходную последовательность.
SortedSet, TypedSet, числовые преобразования множеств и остальные API ещё переносятся.
Проверка принадлежности in и not in, поиск подстроки Contains: examples/membership.pas.
Работают строки, одномерные массивы, матрицы, List и последовательности.
Объединение массивов через + и повторение через *: examples/array-operators.pas.
Результат — новый массив; записи копируются, вложенные ссылки сохраняются.
Составные присваивания +=, -=, *=, /=: examples/compound-assignments.pas.
Целочисленное /= запрещено; string/BigInteger и коллекции используют исходные перегрузки.
Пользовательские операторы, multicast-делегаты и полный набор типов ещё переносятся.
Пользовательские interface, наследуемые контракты, методы, свойства и var/params: examples/user-interfaces.pas.
Классы могут реализовывать IEnumerable<T>/IEnumerator<T>: examples/user-enumerables.pas.
IComparable<T>, IComparer<T>, Comparer<T>.Default/Create: examples/comparers.pas.
Сортировка массивов записей/классов по сравнению и ключу: examples/sorting.pas.
Сортировки List<T>, IComparer<T>, диапазоны и ключи: examples/list-sorting.pas.
Статическая сортировка System.Array и диапазоны: examples/array-sorting.pas.
Парная сортировка ключей и элементов: examples/paired-sorting.pas.
Типизированное равенство и хеши: examples/equality.pas.
HashSet с пользовательским IEqualityComparer<T> и сохранением сравнителя в Copy: examples/hashset-comparers.pas.
Dictionary со сравнителем ключей, Copy и равенством значений: examples/dictionary-comparers.pas.
Поиск Array.IndexOf/LastIndexOf, List и Contains со сравнителем: examples/collection-search.pas.
Сравнение последовательностей по типу элемента или пользовательскому сравнителю: examples/sequence-equality.pas.
Отложенные Distinct, Union, Intersect и Except со сравнителем: examples/sequence-sets.pas.
Неполная сигнатура файла и выбор кодировки: examples/file-streams.pas.
Явные обобщённые вызовы и значения процедур Arrays: examples/array-generics.pas.
Типизированный ICollection<T> для нативных контейнеров: examples/collection-interfaces.pas.
Типизированный IList<T>, доступ по индексу и School.ReplaceMany: examples/list-interfaces.pas.
Типизированный IDictionary<K,V>, перегрузки Add/Remove и представления ключей: examples/dictionary-interfaces.pas.
Типизированный ISet<T> и отдельный Add через ICollection<T>: examples/set-interfaces.pas.
Явные реализации методов интерфейсов: examples/explicit-interfaces.pas.
Явные свойства, выражения чтения и операторы записи: examples/property-interfaces.pas.
Диапазоны через ICollection<T>, IEnumerable<T> и IEquatable<T>: examples/range-interfaces.pas.
Равенство и упаковка RealRange через IEquatable<T>: examples/real-range-interfaces.pas.
Методы, свойства и интерфейсные упаковки пользовательских записей: examples/record-methods.pas.
Начальные значения полей записей, копирование и новые элементы массивов: examples/record-initialization.pas.
AddRange/InsertRange с пользовательским ICollection: examples/list-user-bulk.pas.
Разные типы Predicate/Func и сохранённые цели преобразований: examples/callable-families.pas.
Именованные типы function/procedure, короткая запись со стрелкой и var-параметры: examples/named-callables.pas.
Собственные типы Predicate2/Predicate3 и преобразования в Func: examples/predicate-generics.pas.
Однобайтовые кодировки CP866, KOI8-R, Windows-1252, Latin-1 и EBCDIC: examples/single-byte-encodings.pas. Кодировки встроены, установка провайдеров не нужна.
Двухбайтовые японская, китайские и корейская кодировки: examples/double-byte-encodings.pas. Поддержаны все 140 числовых кодировок исходной поставки; полный перенос библиотечных API продолжается.
GB18030 с китайским текстом и всеми символами Unicode, включая эмодзи: examples/gb18030-encoding.pas.
Десять кодировок ISCII с индийскими алфавитами и их смешением в одном тексте: examples/iscii-encodings.pas.
ISO-2022-JP/KR и HZ-GB2312 с переключением режимов: examples/iso2022-encodings.pas.
UTF-7 с исходными правилами UTF-16 и переключения режимов: examples/utf7-encoding.pas.
Свойства кодировки и расчёт максимального размера без создания буфера: examples/encoding-metadata.pas.
Порядок вычисления объекта и ключа при составной записи в коллекцию: examples/collection-property-writes.pas.
Перегрузки конструкторов, статические операции записей и оставшиеся возможности коллекций ещё переносятся.
Contains без сравнителя сохраняет правила исходной коллекции; явный nil выбирает стандартное сравнение элементов.
Обход автоматически вызывает Dispose при завершении, break, Exit и ошибке; общие пользовательские обобщения ещё переносятся.
Нативный IniFile.TIniFile: uses IniFile; пример examples/ini-files.pas.
WriteString/WriteInteger/WriteBoolean меняют память, Save записывает файл. Наследование TIniFile ещё не перенесено.
Обобщённые строковые проверки используют en-US, строковые методы InRange/Between — порядок UTF-16.
Для явного шага используйте to; downto step и нулевой шаг дают двуязычные ошибки.
Текстовые файлы с BOM читаются как UTF-8, UTF-16LE/BE или UTF-32LE/BE.
Символы CP1251 и ANSI-алиасы: examples/windows-characters.pas.
Encoding, CP1251/ASCII/Unicode и выбор кодировки ReadAllText/WriteAllText: examples/text-encodings.pas.
DefaultEncoding изначально CP1251. Его смена и явные Encoding-перегрузки Text: examples/default-encoding.pas.
Двоичные file и file of T, числа, строки, перечисления и вложенные записи: examples/binary-files.pas.
Вложенные процедуры/функции, рекурсия и сохранение локального состояния: examples/nested-routines.pas.
Статические массивы, независимые копии, двумерные индексы и file of массив: examples/fixed-arrays.pas.
Инициализаторы динамических массивов, записи и более семи элементов: examples/array-literals.pas.
Константы-массивы, известные границы типов и постоянные локальные таблицы: examples/array-constants.pas.
Статические поля классов, однократная инициализация и общие данные: examples/static-fields.pas.
Статические методы, типизированные функции и замыкания: examples/static-methods.pas.
Статические свойства, индексы и нулевые значения записей: examples/static-properties.pas.
Статические конструкторы классов: static constructor и class constructor.
Типизированные файлы открываются только в однобайтовых кодировках; фиксированные массивы, короткие строки и сериализация объектов ещё переносятся.
Для сохранения всех Unicode-символов выберите Encoding.UTF8/Unicode; полный API Encoding ещё переносится.
ReadAllLines/WriteAllLines с Encoding и WriteLines для последовательностей: examples/file-lines.pas.
Перед ReadAllText/ReadAllLines закройте писатель того же файла.
ReadLines с поточным чтением, повторным обходом и break: examples/streaming-lines.pas.
Одновременные обходы одного ReadLines разделяют читателя, как в исходной библиотеке; закрытие одного закрывает поток другого.
OpenRead/Reset и Read/Readln/ReadToEnd читают Text по блокам: examples/streaming-text.pas.
ReadString(f) переходит на новую строку; Read(f,s) оставляет перевод строки.
Нативная запись Text, Rewrite с сохранением буфера и Append: examples/text-writer.pas.
Rewrite после Append непустого файла даёт E_IO; закройте файл и откройте заново.
Text.Lines сохраняет буфер при перемещении потока в начало: examples/text-lines.pas. Для чистого повторного чтения используйте Reset.
Полный интерфейс перечислителя ещё не перенесён.
Прямой захват управляющей переменной цикла пока не перенесён; используйте отдельную переменную-копию.
Совместимость с PascalABC.NET частичная.
Русские имена и НОД/НОК: examples/russian-identifiers.pas. Массивные GCD/LCM: examples/school-folds.pas.
Регистр и категории Unicode: examples/unicode-characters.pas. Правила регистра фиксированы таблицами Unicode 15.1.
Кортежи и НОДНОК: examples/tuples.pas. Минимум и максимум: examples/school-minmax.pas.
BigInteger: examples/big-integers.pas и examples/big-integer-files.pas. Числа ограничены 65536 битами.
Установка .NET, Python или Qt для запуска этого пакета не требуется.
"@
Set-Content -LiteralPath (Join-Path $packageDir 'START-HERE.txt') -Value $startText -Encoding utf8
Set-Content -LiteralPath (Join-Path $packageDir 'README.md') -Value $startText -Encoding utf8
Invoke-BuildTool $cmakeCommand @("-DABC_PACKAGE_DIR=$packageDir", '-P', (Join-Path $nativeRoot 'cmake/VerifyNoPython.cmake'))
Compress-Archive -Path (Join-Path $packageDir '*') -DestinationPath $zipPath -Force
[pscustomobject]@{
    variant = 'Qt'
    runtimeBytes = (Get-ChildItem -LiteralPath $packageDir -File -Recurse | Measure-Object -Property Length -Sum).Sum
    archiveBytes = (Get-Item -LiteralPath $zipPath).Length
} | ConvertTo-Json | Set-Content -LiteralPath (Join-Path $nativeRoot 'dist/qt-package.json') -Encoding utf8
Write-Output $zipPath
