"""Check the actual Windows ZIPs without Qt/Tcl/Python directories in PATH."""
import argparse
import json
import os
from pathlib import Path
import subprocess
import tempfile
import zipfile
import struct


def assert_native_binaries(folder):
    """Reject CLR metadata and runtime hosts in the actual extracted package."""
    checked = 0
    for binary in folder.rglob("*"):
        if not binary.is_file():
            continue
        name = binary.name.lower()
        if (name in {"dotnet", "dotnet.exe", "mscorlib.dll", "system.private.corelib.dll", "mono", "mono.exe"}
                or name.startswith(("coreclr.", "libcoreclr.", "clr.", "libmonosgen-", "mono-2.0."))
                or name.endswith(".runtimeconfig.json")):
            raise RuntimeError(f"Managed runtime is forbidden in the application package: {binary}")
        if binary.suffix.lower() not in {".exe", ".dll", ".netmodule"}:
            continue
        data = binary.read_bytes()
        if not data.startswith(b"MZ"):
            continue
        if len(data) < 64:
            raise RuntimeError(f"Truncated PE binary: {binary}")
        pe = struct.unpack_from("<I", data, 60)[0]
        if pe + 24 > len(data) or data[pe:pe + 4] != b"PE\0\0":
            raise RuntimeError(f"Invalid PE binary: {binary}")
        optional = pe + 24
        size = struct.unpack_from("<H", data, pe + 20)[0]
        if size < 2 or optional + size > len(data):
            raise RuntimeError(f"Truncated PE optional header: {binary}")
        magic = struct.unpack_from("<H", data, optional)[0]
        directories = {0x10b: 96, 0x20b: 112}.get(magic)
        if directories is None or size < directories:
            raise RuntimeError(f"Unsupported PE optional header: {binary}")
        count = struct.unpack_from("<I", data, optional + directories - 4)[0]
        if count > 14:
            if size < directories + 15 * 8:
                raise RuntimeError(f"Truncated CLR directory: {binary}")
            address, length = struct.unpack_from("<II", data, optional + directories + 14 * 8)
            if address or length:
                raise RuntimeError(f"CLR-managed binary is forbidden in the application package: {binary}")
        checked += 1
    return checked


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("dist", type=Path)
    parser.add_argument("--variant", choices=["Qt", "TclTk", "both"], default="Qt")
    args = parser.parse_args()
    results = []
    for name, editor in [("Qt", "abc-ide.exe"), ("TclTk", "abc-tk.exe")]:
        if args.variant not in (name, "both"):
            continue
        archive = args.dist / f"ABC-Native-{name}-win-x64.zip"
        with tempfile.TemporaryDirectory(prefix="ABC Native Проверка ") as temporary:
            folder = Path(temporary)
            with zipfile.ZipFile(archive) as package:
                package.extractall(folder)
            native_binaries = assert_native_binaries(folder)
            if not (folder / "licenses" / "PCRE2-LICENCE.md").is_file():
                raise RuntimeError(f"{name} PCRE2 license is missing")
            env = os.environ.copy()
            windows = env.get("SystemRoot", r"C:\Windows")
            env["PATH"] = str(Path(windows) / "System32") + os.pathsep + windows
            for key in ["TCL_LIBRARY", "TK_LIBRARY", "TCLLIBPATH", "QT_PLUGIN_PATH", "QT_QPA_PLATFORM_PLUGIN_PATH", "QT_QPA_PLATFORM", "PYTHONPATH"]:
                env.pop(key, None)
            startup = {}
            if os.name == "nt":
                info = subprocess.STARTUPINFO()
                info.dwFlags |= subprocess.STARTF_USESHOWWINDOW
                info.wShowWindow = 0
                startup["startupinfo"] = info
            modes = [[]] if name == "TclTk" else [[], ["-platform", "offscreen"]]
            for mode in modes:
                gui_args = [str(folder / "bin" / editor), "--smoke-test", *mode]
                gui = subprocess.run(gui_args, cwd=folder, env=env, capture_output=True, timeout=20, **startup)
                if gui.returncode:
                    raise RuntimeError(f"{name} editor failed: {gui.returncode}, {gui.stderr!r}")
            if name == "Qt":
                gui = subprocess.run([str(folder / "bin" / editor), "--smoke-runtime-error"],
                                     cwd=folder, env=env, capture_output=True, timeout=20, **startup)
                if gui.returncode:
                    raise RuntimeError(f"Qt bilingual error GUI failed: {gui.returncode}, {gui.stderr!r}")
            runner = str(folder / "bin" / "abc.exe")
            console = subprocess.run([runner, "run", str(folder / "examples" / "console-input.pas")],
                                     input="7 8 tail\n1 2 3 rest\n", text=True, encoding="utf-8",
                                     capture_output=True, cwd=folder, env=env, timeout=10)
            if console.returncode or console.stdout != "Два целых: Сумма: 15\nТри числа: 1, 2, 3\n":
                raise RuntimeError(f"{name} shared console input failed: {console.stdout!r}, {console.stderr!r}")
            console_source = folder / "Консольный ввод.pas"
            for supplied, expected in [
                ("7\n", "Введите целое число: Прочитано: 7\n"),
                ("bad\n", "Введите целое число: Некорректный ввод. Значение: 0\n"),
            ]:
                attempted = subprocess.run([runner, "run", str(folder / "examples" / "try-read.pas")],
                                           input=supplied, text=True, encoding="utf-8", capture_output=True,
                                           cwd=folder, env=env, timeout=10)
                if attempted.returncode or attempted.stdout != expected:
                    raise RuntimeError(f"{name} TryRead failed: {attempted.stdout!r}, {attempted.stderr!r}")
            console = subprocess.run([runner, "run", str(folder / "examples" / "console-functions.pas")],
                                     input="42 tail\nАнна\n", text=True, encoding="utf-8",
                                     capture_output=True, cwd=folder, env=env, timeout=10)
            if console.returncode or console.stdout != "Введите число: Введите имя: Здравствуйте, Анна! Число: 42\n":
                raise RuntimeError(f"{name} typed console functions/prompts failed: {console.stdout!r}, {console.stderr!r}")
            console_source.write_text("begin var c:char; Readln(c); Writeln(Ord(c)); end.", encoding="utf-8")
            console = subprocess.run([runner, "run", str(console_source)], input=b"\r\n",
                                     capture_output=True, cwd=folder, env=env, timeout=10)
            if console.returncode or console.stdout != b"13\n":
                raise RuntimeError(f"{name} CRLF input was changed by the host runtime: {console.stdout!r}, {console.stderr!r}")
            console_source.write_text("begin\n var n:integer;\n Read(n);\nend.", encoding="utf-8")
            console = subprocess.run([runner, "run", str(console_source)], input="bad\n", text=True, encoding="utf-8",
                                     capture_output=True, cwd=folder, env=env, timeout=10)
            if (console.returncode != 1 or "E_INPUT" not in console.stderr or f"{console_source}:3:" not in console.stderr
                    or "EN:" not in console.stderr or "RU:" not in console.stderr):
                raise RuntimeError(f"{name} bilingual console error failed: {console.stderr!r}")
            example = subprocess.run([runner, "run", str(folder / "examples" / "sum.pas")],
                                     input="10\n", text=True, encoding="utf-8", capture_output=True,
                                     cwd=folder, env=env, timeout=10)
            if example.returncode or "Сумма: 55" not in example.stdout:
                raise RuntimeError(f"{name} sum failed: {example.stdout!r}, {example.stderr!r}")
            library = subprocess.run([runner, "run", str(folder / "examples" / "standard-library.pas")],
                                     text=True, encoding="utf-8", capture_output=True,
                                     cwd=folder, env=env, timeout=10)
            if (library.returncode or "2500000000" not in library.stdout
                    or "Площадь круга: 78.54" not in library.stdout or "После Inc и Dec: 14" not in library.stdout):
                raise RuntimeError(f"{name} standard library failed: {library.stdout!r}, {library.stderr!r}")
            text_numbers = subprocess.run([runner, "run", str(folder / "examples" / "text-numbers.pas")],
                                          text=True, encoding="utf-8", capture_output=True,
                                          cwd=folder, env=env, timeout=10)
            if (text_numbers.returncode or "Цена: 123.45" not in text_numbers.stdout
                    or "Большое целое: 9223372036854775807" not in text_numbers.stdout
                    or "Наоборот: lacsaP" not in text_numbers.stdout):
                raise RuntimeError(f"{name} text conversions failed: {text_numbers.stdout!r}, {text_numbers.stderr!r}")
            mutations = subprocess.run([runner, "run", str(folder / "examples" / "string-editing.pas")],
                                       text=True, encoding="utf-8", capture_output=True,
                                       cwd=folder, env=env, timeout=10)
            if (mutations.returncode or "После Insert: Привет, новый мир" not in mutations.stdout
                    or "После Delete: Привет, мир" not in mutations.stdout
                    or "После дополнения пробелами: [Привет   ]" not in mutations.stdout):
                raise RuntimeError(f"{name} string mutations failed: {mutations.stdout!r}, {mutations.stderr!r}")
            for source, expected in [
                ("reverse-shuffle.pas", "Shuffle: 4 3 5 1 2 \nReverse: 4 1 5 3 2 \nьлаксаП\nSwap: 2 4\n"),
                ("extended-math.pas", "НОД: 6\nНОК: 36\nГипотенуза: 5\nИнтерполяция: 12.5\nНовая шкала: 150\nУгол: True\n"),
                ("array-output.pas", "Числа: 10, 20, 30\nИмена: Анна / Борис / Вера\nПустой: []\n"),
                ("bitwise.pas", "AND: 8\nOR: 14\nXOR: 6\nБеззнаковый сдвиг: 1\nЗнаковый сдвиг: -4\nИнверсия byte: 255\nЛогический XOR: True\n"),
                ("unicode-output.pas", "По символам UTF-16: 🌍\nОтдельные Write: 🌍\nМассив: 🌍\n"),
                ("matrices.pas", "Матрица:\n  25   11   47 \n  77   66   43 \nРазмер копии: 3x2\nИсходный элемент: 25\n"),
                ("object-output.pas", "Записи: [(Анна,[5,4]),(Борис,[4,5])]\n(Анна,[5,4]) / (Борис,[4,5])\nМатрица: [[0,0],[0,0]]\n"),
                ("formatted-output.pas", "Число: [    42]\nДробь: [   3.142]\nМассив: [     [1,2,3]]\n"),
                ("print-settings.pas", "Обычный: 1 2 3\nС разделителем | [1,2,3] | True\nБез разделителя:123\n"),
                ("string-conversions.pas", "Форматирование: [   3.142]\nОшибка: 1; прежнее значение: 10\nПрочитано: 42\n"),
                ("string-cursor.pas", "Целое: 12\nДробь: 3.5\nСлово: слово\nКонец: True\n"),
                ("string-members.pas", "Целое: 12\nДробь: 3.5\nСлово: слово\nПо умолчанию: 7\nС запятой: 1.5\n"),
                ("string-arrays.pas", "Слова: [Анна,Борис,Вера]\nЦелые: [10,-3,7]\nДроби: [1.5,2.25]\nСтрок: 3\n"),
                ("school-bases.pas", "Двоичная: 101010\nШестнадцатеричная: FF\nИз HEX: 255\nБайты: 00000001 00000010\nКорень: 9\nСинус 30: 0.5\n"),
                ("school-primes.pas", "7: True\n9: False\nБольшое простое: True\nБольшое составное: False\n"),
                ("school-lists.pas", "Цифры: [2,0,2,6]\nОбратно: 2026\nПростые до 30: [2,3,5,7,11,13,17,19,23,29]\nПервые 5: [2,3,5,7,11]\n"),
                ("school-factors.pas", "Множители 360: [2,2,2,3,3,5]\nПростые делители: [2,3,5]\nДелители 12: [1,2,3,4,6,12]\nЧисло делителей 360: 24\nБольшое число: [71,839,1471,6857]\n"),
                ("school-text.pas", "Последовательная замена: cc\nПосимвольный перевод: bc\nПоследнее вхождение: раз-два-три\nПодстроки: кот пёс\nЧисла int64: [2147483648,-7]\nИмпликация: False\n"),
                ("school-swap.pas", "Обмен: пёс кот пёс\nОбратно: кот пёс кот\nПустые совпадения: aaa\n"),
                ("regex-search.pas", "Есть число: True\nПервое число: 123\nБез числа: []\nПроверка: True\n"),
                ("sequence-lists.pas", "Массив: [1,2]\nСписок: [9,2,3]\nКопия: [9,2,3]\nПосле изменения: [9,8,3]\n"),
                ("sequence-factories.pas", "Снимок: [1,2]\nИсходный массив: [9,2]\nПовторения: [да,да,да]\nПервый большого повтора: 7\nДиапазон: [2,3,4,5]\nСимволы: abc\n"),
                ("assertions.pas", "Проверки выполнены\n"),
                ("params-parameters.pas", "Без аргументов: 0\nТри числа: 6\nГотовый массив: 60\n"),
                ("array-initializers.pas", "Числа: [10,20,30]\nМатрица: 2 3\nТочки: 1 4\n"),
                ("const-out-parameters.pas", "Сумма: 42\nПосле изменения: 43\n"),
                ("sequence-flatmap.pas", "Все элементы: [1,1,2,1,2,3]\nПервые четыре: [1,1,2,1]\nСумма: 10\n"),
                ("sequence-predicates.pas", "Есть чётные: True\nЧётных чисел: 5\nПервое чётное: 2\nСумма квадратов: 385\n"),
                ("sequence-callbacks.pas", "Квадраты чётных: [4,16,36,64,100]\nПервые два: [4,16]\nСумма: 220\n"),
                ("sequence-tools.pas", "В обратном порядке: [3,2,1]\nИсходный массив: [1,2,3]\nЗначение для пустого источника: 42\nСовпадают: True\n"),
                ("string-sequences.pas", "Символов: 6\nПервые три: [П,р,и]\nПоследние два: [е,т]\nОбъединение: !\n"),
                ("sequence-concat.pas", "Последовательность: [0,1,2,3,4,5,6]\nПовторный обход: [0,1,2,3,4,5,6]\nПервые два: [0,1]\nСумма: 21\n"),
                ("default-arguments.pas", "Первый вызов: 11\nВторой вызов: 12\nЯвный аргумент: 17\nВычислений по умолчанию: 2\nСтроковый default равен nil: True\n"),
                ("sequence-slices.pas", "Первые: [1,2]\nПропустить: [3,4,5]\nПоследние: [4,5]\nБез последних: [1,2,3]\nБез одного последнего: [1,2,3,4]\nРанняя остановка: 2147483647\n"),
                ("sequence-aggregates.pas", "Сумма: 10\nСреднее: 2.5\nПроизведение: 24\nЧастичные суммы: [1,3,6,10]\nBigInteger: 24\nСреднее пустой BigInteger: 0\n"),
                ("real-ranges.pas", "Точек: 11\nКонец точно 1: True\nЧетверти: [0,0.25,0.5,0.75,1]\nОбратно: [1,0.75,0.5,0.25,0]\nШаг 0.3: 4 точки\n"),
                ("partition-points.pas", "Разбиение на 4 части: [0,0.25,0.5,0.75,1]\nПять точек: [0,0.25,0.5,0.75,1]\nОбратный порядок: [1,0.75,0.5,0.25,0]\nПервый большого разбиения: 0\n"),
                ("big-integer-ranges.pas", "Большие числа: [1000000000000000000000000000000,1000000000000000000000000000002,1000000000000000000000000000004,1000000000000000000000000000006]\nОбратный порядок: [7,5,3,1]\nПервый огромного диапазона: 0\nРанний выход: 0\n"),
                ("stepped-ranges.pas", "Вверх: [1,4,7,10]\nВниз: [10,7,4,1]\nСимволы: авдж\nНачало большого диапазона: -2147483648\nПоследнее значение перед break: 2147483647\n"),
                ("sequence-queries.pas", "Непустая: True\nПервый: 4\nПоследний: 9\nПо индексу 1: 7\nСодержит 7: True\nПустая: False\nПо умолчанию: 0\nЕдинственный: 42\nПустой SingleOrDefault: 0\n"),
                ("russian-identifiers.pas", "Ответ: 42\nНОД: 6\nНОК: 90\n"),
                ("big-integers.pas", "100! = 93326215443944152681699238856266700490715968264381621468592963895217599993229915608941463976156518286253697920827223758251185210916864000000000000000000000000\nЦелый корень квадрата: 123456789012345678901234567890\nНОД: 6\nСдвиги: 16\n"),
                ("big-integer-files.pas", "Из файла: 246913578024691357802469135780\n"),
                ("school-big-integers.pas", "Число: 79228162514264337593543950335\nОснование 16: FFFFFFFFFFFFFFFFFFFFFFFF\nЦифры: [15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15]\nОбратное преобразование: 79228162514264337593543950335\nБайты: 00000000:00000001:00000000\nНОК массива: 55340232221128654848\nМинимум и максимум: (-79228162514264337593543950335,79228162514264337593543950335)\n"),
                ("tuples.pas", "ответ: 42\nНОД и НОК: (6,90)\nНОД: 6; НОК: 90\n"),
                ("school-minmax.pas", "Минимум: -7\nМаксимум: 42\nПара: (-7,42)\n"),
                ("truth-tables.pas", "Исключающее ИЛИ:\n a b F\n -----\n 0 0 0\n 0 1 1\n 1 0 1\n 1 1 0\nТри переменные, только истинные строки:\n x y z F\n -------\n 1 1 1 1\n"),
                ("string-methods.pas", "Имя: Анна....\nНомер: 00042\nПодстрока: cde\nУдаление: ab\nВставка: aXbcde\nИсходная строка: abcde\n"),
                ("school-ip.pas", "Адрес: 192.168.1.129\nЧисло: 3232235905\nБиты: 11000000.10101000.00000001.10000001\nМаска: 255.255.255.0\nСеть: 192.168.1.0\nBroadcast: 192.168.1.255\nХостов: 254\nДиапазон: 192.168.1.1 - 192.168.1.254\nХост: 10.0.0.1\nХост: 10.0.0.2\n"),
                ("school-ip-sequences.pas", "Первый хост большой сети: 128.0.0.1\nКоличество хостов: 2\nМассив: 10.0.0.1 10.0.0.2\nПовторный перебор: 10.0.0.1\nПовторный перебор: 10.0.0.2\n"),
                ("sequence-views.pas", "Представление: [2,-3,9]\nМинимум и максимум: (-3,9)\nКоличество: 2\nКопия списка: [4,5]\nЭлемент: 4\nЭлемент: 5\n"),
                ("unicode-characters.pas", "Нижний регистр: привет, ёжик! 𐐨\nВерхний регистр: ПРИВЕТ, ЁЖИК! 𐐀\nБуква: True; код: 1071\nАрабская цифра: True\n"),
                ("structures.pas", "Точка: 3 4\nФакториал 10: 3628800\n1\n2\n5\n8\nИмен: 2\nПервое имя: Анна\n"),
                ("files.pas", "Привет из C++ интерпретатора!\nПрочитано строк: 2\nВторая строка\n"),
                ("collections.pas", "Стек: 30 20 10 \nОчередь: Анна Борис Вера \nПустая очередь: True\n"),
                ("buffers.pas", "Массив: 10 20 30 \nРазмер: 3; ёмкость: 4\nПосле удаления: 20 30 \nБез повторов: Анна Борис \nЕсть Анна: False\n"),
                ("associations.pas", "Цена яблок: 125\nНет цены: 0\nВсе цены:\nЯблоки 125\nГруши 150\n"),
                ("linked-lists.pas", "Вперёд: Анна Борис Вера \nНазад после удаления: Вера Анна \n"),
                ("single-nodes.pas", "Цепочка: Анна Борис Вера \nПосле изменения: Анна Виктория\n"),
                ("nil-strings.pas", "Нет значения: nil\nЭто nil: True\nПустая строка — nil: False\nДлина nil: 0\nСоединение: [текст]\nПосле Insert: Новое имя\n"),
                ("directories.pas", "Файл в выбранной папке\nНовое имя: заметка.bak\nФайл удалён: True\nВложенная папка удалена: True\nПапка примера удалена: True\n"),
                ("renames.pas", "Файл переименован: True\nКаталог переименован: True\nТекст заметки\nЗанятое имя: False\nСуществующий текст\n"),
                ("text-files.pas", "Сумма: 30\nСтрока из файла\nКонец файла: True\nДобавленная строка\n"),
                ("text-methods.pas", "Файл: Заметка.txt\nПервое слово: Цена:\nЧисло: 125\nОстаток: Количество: 3\nКонец файла: True\n"),
                ("text-file-changes.pas", "Файл переименован: True\nИмя в переменной: Исходная заметка.txt\nЗаписано на C++\nФайл удалён: True\n"),
                ("loops.pas", "Сумма: 55\nОбратный отсчёт: 3 2 1 \nПовторений: 3\nНечётные: 1 3 5 \n"),
                ("foreach.pas", "Сумма: 14\nИмена: Анна Борис \nНечётные: 3 1 1 5 \n"),
                ("characters.pas", "Буква: Б\nКод UTF-16: 1041\nИмя: Инна\nАлфавит: абвгде\nСимволы: И|н|н|а|\n"),
                ("file-operations.pas", "Прочитано:\nПервая строка\nВторая строка\nФайл удалён: True\n"),
                ("small-integers.pas", "Значения: 255 -128 -32768\nСумма с расширением: 256\nКвадрат: 65025\nГраница byte: 254|255|\nМассив smallint: 1 -2\n"),
                ("unsigned-integers.pas", "Граница 32 бит: 4294967295\nС расширением: 4294967296\nГраница 64 бит: 18446744073709551615\nКвадрат word: 4294836225\nПредыдущее: 18446744073709551614\nПереключение: True False\n"),
                ("random-arrays.pas", "Arrays, 0..99: 24|11|46|\nPABCSystem, 0..100: 25|11|47|\nПосле заполнения: 10|10|11|\nВещественное: 0.25\n"),
            ]:
                example = subprocess.run([runner, "run", str(folder / "examples" / source)],
                                         text=True, encoding="utf-8", capture_output=True,
                                         cwd=folder, env=env, timeout=10)
                if example.returncode or example.stdout != expected:
                    raise RuntimeError(f"{name} {source} failed: {example.stdout!r}, {example.stderr!r}")
            example = subprocess.run([runner, "run", str(folder / "examples" / "school-folds.pas")], text=True, encoding="utf-8",
                                     capture_output=True, cwd=folder, env=env, timeout=10)
            if example.returncode or example.stdout != "НОД массива: 6\nНОК массива: 630\nНОД нескольких чисел: 6\nНОК нескольких чисел: 60\n":
                raise RuntimeError(f"{name} school-folds.pas failed: {example.stdout!r}, {example.stderr!r}")
            character = folder / "Ошибки символов.pas"
            school_error = folder / "Ошибки School.pas"
            for expression, code, english, russian in [
                ("School.НОК(0,0)", "E_DIV_ZERO", "Division by zero", "Деление на ноль"),
                ("var s:string:='[x'; SwapSubstr(s,'[','x')", "E_REGEX", "Invalid regular expression", "Некорректное регулярное выражение"),
                ("Writeln(IsMatch('abc','['))", "E_REGEX", "Invalid regular expression", "Некорректное регулярное выражение"),
                ("Writeln(MatchValue('abc','['))", "E_REGEX", "Invalid regular expression", "Некорректное регулярное выражение"),
                ("var s:sequence of integer:=nil; s.ToList()", "E_NIL", "Cannot access a nil reference", "Нельзя обратиться к пустой ссылке nil"),
                ("var a:=new integer[0]; a.First", "E_EMPTY", "Sequence contains no elements", "Последовательность не содержит элементов"),
                ("var a:=new integer[0]; a.Last", "E_EMPTY", "Sequence contains no elements", "Последовательность не содержит элементов"),
                ("var a:=new integer[0]; a.Single", "E_EMPTY", "Sequence contains no elements", "Последовательность не содержит элементов"),
                ("var a:=Arr(1,2); a.Single", "E_MANY", "Sequence contains more than one element", "Последовательность содержит более одного элемента"),
                ("var a:=Arr(1,2); a.SingleOrDefault", "E_MANY", "Sequence contains more than one element", "Последовательность содержит более одного элемента"),
                ("SeqFill(-1,7)", "E_RANGE", "Sequence count cannot be negative", "Число элементов последовательности не может быть отрицательным"),
                ("var a:array of integer; Seq(a)", "E_NIL", "Cannot access a nil reference", "Нельзя обратиться к пустой ссылке nil"),
                ("Range(not(MaxInt),MaxInt)", "E_OVERFLOW", "Integer overflow", "Переполнение целого числа"),
                ("Range(1,2,0).Any", "E_RANGE", "Sequence step cannot be zero", "Шаг последовательности не может быть равен нулю"),
                ("var s:=Seq(1); for var i:=1 to 129 do s:=s.PartialSum", "E_LIMIT", "Sequence nesting limit exceeded", "Превышена допустимая глубина последовательностей"),
                ("var s:sequence of integer; s.Take(0)", "E_NIL", "Cannot access a nil reference", "Нельзя обратиться к пустой ссылке nil"),
                ("Assert(False)", "E_ASSERT", "Assertion failed", "Проверка Assert не пройдена"),
                ("Assert(False,'причина')", "E_ASSERT", "Assertion failed: причина", "Проверка Assert не пройдена: причина"),
                ("var s:sequence of integer; var reversed:=s.Reverse", "E_NIL", "Cannot access a nil reference", "Нельзя обратиться к пустой ссылке nil"),
                ("var s:sequence of integer; var filled:=s.DefaultIfEmpty", "E_NIL", "Cannot access a nil reference", "Нельзя обратиться к пустой ссылке nil"),
                ("var s:string:=nil; s.ToArray", "E_NIL", "Cannot access a nil reference", "Нельзя обратиться к пустой ссылке nil"),
                ("var s:sequence of integer; var joined:=Seq(1).Concat(s)", "E_NIL", "Cannot access a nil reference", "Нельзя обратиться к пустой ссылке nil"),
                ("var s:sequence of integer; s.TakeLast(1).First", "E_NIL", "Cannot access a nil reference", "Нельзя обратиться к пустой ссылке nil"),
                ("Range(MaxInt,MaxInt,1).Skip(1).Any", "E_OVERFLOW", "Integer overflow", "Переполнение целого числа"),
                ("Arr(MaxInt,1).Sum", "E_OVERFLOW", "Integer overflow", "Переполнение целого числа"),
                ("Writeln((new integer[0]).Average)", "E_EMPTY", "Sequence contains no elements", "Последовательность не содержит элементов"),
                ("Arr(MaxInt,MaxInt,MaxInt).Product", "E_OVERFLOW", "Integer overflow", "Переполнение целого числа"),
                ("Arr(MaxInt,1).PartialSum.Count", "E_OVERFLOW", "Integer overflow", "Переполнение целого числа"),
                ("var a:array of integer; a.PartialSum.Any", "E_NIL", "Cannot access a nil reference", "Нельзя обратиться к пустой ссылке nil"),
                ("Range(0,1,0.0).Any", "E_RANGE", "Sequence step cannot be zero", "Шаг последовательности не может быть равен нулю"),
                ("Range(0,1e29,1.0).First", "E_OVERFLOW", "Decimal overflow", "Переполнение десятичного числа"),
                ("Range(0,1,1e-29).Any", "E_DIV_ZERO", "Division by zero", "Деление на ноль"),
                ("PartitionPoints(0,1,0).Any", "E_RANGE", "Partition count must be positive", "Число частей разбиения должно быть положительным"),
                ("Linspace(0,1,1).Any", "E_RANGE", "Partition count must be positive", "Число частей разбиения должно быть положительным"),
                ("Linspace(0,1,not(MaxInt))", "E_OVERFLOW", "Integer overflow", "Переполнение целого числа"),
                ("Range(1bi,2bi,0bi).Any", "E_RANGE", "Sequence step cannot be zero", "Шаг последовательности не может быть равен нулю"),
                ("var p:=1bi shl 65535; Range(p,p,p).Last", "E_LIMIT", "BigInteger size limit exceeded", "Превышен допустимый размер BigInteger"),
                ("Range(MaxInt,MaxInt,1).Count", "E_OVERFLOW", "Integer overflow", "Переполнение целого числа"),
                ("var s:string:='a'; SwapSubstr(s,'a','a')", "E_KEY", "Duplicate swap key", "Повторяющийся ключ замены"),
                ("var s:string:='a.b acb z'; SwapSubstr(s,'a.b','z')", "E_KEY", "Matched text is not a swap key", "Совпавший текст не является ключом замены"),
                ("var a:=new Addr32('256.0.0.1')", "E_FORMAT", "Invalid IPv4 address", "Некорректная запись адреса IPv4"),
                ("var c:=new CalcIP('10.0.0.1',32)", "E_RANGE", "IPv4 mask suffix must be between 1 and 31", "Суффикс маски IPv4 должен быть от 1 до 31"),
                ("var c:=new CalcIP('10.0.0.1','255.0.255.0')", "E_FORMAT", "Invalid contiguous IPv4 netmask", "Некорректная непрерывная маска сети IPv4"),
                ("var l:=new List<integer>; l.Add(1); var s:sequence of integer:=l; foreach var x in s do l.Clear", "E_MODIFIED", "Collection was modified during enumeration", "Коллекция изменена во время перебора"),
                ("TrueTablePrint(nil)", "E_NIL", "Cannot access a nil reference", "Нельзя обратиться к пустой ссылке nil"),
                ("School.НОДНОК(0,0)", "E_DIV_ZERO", "Division by zero", "Деление на ноль"),
                ("var t:(integer,integer); Writeln(t.Item1)", "E_NIL", "Cannot access a nil reference", "Нельзя обратиться к пустой ссылке nil"),
                ("School.GCD()", "E_RANGE", "Argument list cannot be empty", "Список аргументов не может быть пустым"),
                ("School.Dec('2',2)", "E_FORMAT", "Invalid digit in base notation", "Недопустимая цифра в записи числа"),
                ("School.ToBase(1,1)", "E_RANGE", "Base must be between 2 and 36", "Основание системы счисления должно быть от 2 до 36"),
                ("DigitsToInt64(Digits(15,16),10)", "E_RANGE", "A digit must be less than the base", "Цифра должна быть меньше основания системы счисления"),
                ("Factorize(4611686022722355201)", "E_FACTOR", "Prime factorization failed", "Ошибка разложения на простые множители"),
                ("ReplaceMany('',Arr(''),Arr('target'))", "E_RANGE", "Replacement source cannot be empty", "Заменяемая подстрока не может быть пустой"),
            ]:
                school_error.write_text(f"uses School;\nbegin\n {expression};\nend.", encoding="utf-8")
                invalid = subprocess.run([runner, "run", str(school_error)], text=True, encoding="utf-8",
                                         capture_output=True, cwd=folder, env=env, timeout=10)
                if (invalid.returncode != 1 or invalid.stdout or code not in invalid.stderr
                        or f"EN: {english}" not in invalid.stderr or f"RU: {russian}" not in invalid.stderr
                        or f"{school_error}:3:" not in invalid.stderr):
                    raise RuntimeError(f"{name} bilingual School diagnostic failed: {invalid.stderr!r}")
            school_error.write_text("function F(a:integer:=1 div 0):integer; begin Result:=a; end;\nbegin Writeln(F); end.", encoding="utf-8")
            invalid = subprocess.run([runner, "run", str(school_error)], text=True, encoding="utf-8",
                                     capture_output=True, cwd=folder, env=env, timeout=10)
            if (invalid.returncode != 1 or invalid.stdout or "E_DIV_ZERO" not in invalid.stderr
                    or "EN: Division by zero" not in invalid.stderr or "RU: Деление на ноль" not in invalid.stderr
                    or f"{school_error}:1:" not in invalid.stderr):
                raise RuntimeError(f"{name} bilingual optional-argument error failed: {invalid.stderr!r}")
            school_error.write_text("function F(value:integer):integer;\nbegin Result:=1 div value; end;\nbegin Seq(0).Select(F).First; end.", encoding="utf-8")
            invalid = subprocess.run([runner, "run", str(school_error)], text=True, encoding="utf-8",
                                     capture_output=True, cwd=folder, env=env, timeout=10)
            if (invalid.returncode != 1 or invalid.stdout or "E_DIV_ZERO" not in invalid.stderr
                    or "EN: Division by zero" not in invalid.stderr or "RU: Деление на ноль" not in invalid.stderr
                    or f"{school_error}:2:" not in invalid.stderr):
                raise RuntimeError(f"{name} bilingual callback error failed: {invalid.stderr!r}")
            for operation, predicate, error_code, english, russian in [
                ("First", "False", "E_EMPTY", "Sequence contains no matching element", "Последовательность не содержит подходящих элементов"),
                ("Single", "True", "E_MANY", "Sequence contains more than one matching element", "Последовательность содержит более одного подходящего элемента"),
            ]:
                school_error.write_text(f"function P(value:integer):boolean; begin Result:={predicate}; end;\nbegin Seq(1,2).{operation}(P); end.", encoding="utf-8")
                invalid = subprocess.run([runner, "run", str(school_error)], text=True, encoding="utf-8",
                                         capture_output=True, cwd=folder, env=env, timeout=10)
                if (invalid.returncode != 1 or invalid.stdout or error_code not in invalid.stderr
                        or f"EN: {english}" not in invalid.stderr or f"RU: {russian}" not in invalid.stderr
                        or f"{school_error}:2:" not in invalid.stderr):
                    raise RuntimeError(f"{name} bilingual predicate query failed: {invalid.stderr!r}")
            school_error.write_text("uses School;\nfunction F(a:array of boolean):boolean; begin Result:=True; end;\nbegin\n TrueTable(-1,F);\nend.", encoding="utf-8")
            invalid = subprocess.run([runner, "run", str(school_error)], text=True, encoding="utf-8",
                                     capture_output=True, cwd=folder, env=env, timeout=10)
            if (invalid.returncode != 1 or invalid.stdout or "E_RANGE" not in invalid.stderr
                    or "EN: Truth-table variable count must be between 0 and 30" not in invalid.stderr
                    or "RU: Число переменных таблицы истинности должно быть от 0 до 30" not in invalid.stderr
                    or f"{school_error}:4:" not in invalid.stderr):
                raise RuntimeError(f"{name} bilingual truth-table count error failed: {invalid.stderr!r}")
            school_error.write_text("uses School;\nfunction F(a,b:boolean):integer; begin Result:=1; end;\nbegin Writeln('must never run'); TrueTable(F); end.", encoding="utf-8")
            invalid = subprocess.run([runner, "run", str(school_error)], text=True, encoding="utf-8",
                                     capture_output=True, cwd=folder, env=env, timeout=10)
            if invalid.returncode != 1 or invalid.stdout or "E_TYPE" not in invalid.stderr:
                raise RuntimeError(f"{name} truth-table function types were not checked before output")
            school_error.write_text("begin Writeln('must never run'); Bin(7); end.", encoding="utf-8")
            invalid = subprocess.run([runner, "run", str(school_error)], text=True, encoding="utf-8",
                                     capture_output=True, cwd=folder, env=env, timeout=10)
            if invalid.returncode != 1 or invalid.stdout or "E_NAME" not in invalid.stderr:
                raise RuntimeError(f"{name} School unit import was not checked before execution")
            if not (folder / "licenses" / "Unicode-License.txt").is_file():
                raise RuntimeError(f"{name} missing Unicode table license notice")
            if not (folder / "licenses" / "DotNet-Runtime-MIT.txt").is_file():
                raise RuntimeError(f"{name} missing random algorithm license notice")
            character.write_text("begin Writeln('must never run'); WriteStringToFile('absent-unit.txt','bad'); end.", encoding="utf-8")
            invalid = subprocess.run([runner, "run", str(character)], text=True, encoding="utf-8",
                                     capture_output=True, cwd=folder, env=env, timeout=10)
            if invalid.returncode != 1 or invalid.stdout or "E_NAME" not in invalid.stderr or (folder / "absent-unit.txt").exists():
                raise RuntimeError(f"{name} unit import was not checked before execution")
            character.write_text("uses FilesOperations;\nbegin\n ReadFileToEnd('absent-unit.txt');\nend.", encoding="utf-8")
            invalid = subprocess.run([runner, "run", str(character)], text=True, encoding="utf-8",
                                     capture_output=True, cwd=folder, env=env, timeout=10)
            if (invalid.returncode != 1 or "E_IO" not in invalid.stderr or f"{character}:3:" not in invalid.stderr
                    or "EN: Cannot open, read or write the file" not in invalid.stderr
                    or "RU: Не удалось открыть, прочитать или записать файл" not in invalid.stderr):
                raise RuntimeError(f"{name} bilingual FilesOperations error failed: {invalid.stderr!r}")
            for source in [
                "begin Writeln('must never run'); var c:='a'; c:='ab'; end.",
                "begin Writeln('must never run'); var s:='ab'; s[1]:='XY'; end.",
                "begin Writeln('must never run'); var i:integer:=2; var b:byte:=i; end.",
            ]:
                character.write_text(source, encoding="utf-8")
                invalid = subprocess.run([runner, "run", str(character)], text=True, encoding="utf-8",
                                         capture_output=True, cwd=folder, env=env, timeout=10)
                if invalid.returncode != 1 or invalid.stdout or "E_TYPE" not in invalid.stderr:
                    raise RuntimeError(f"{name} character types were not checked before execution")
            for source, code, english, russian in [
                ("var c:=#65535;\n Inc(c);", "E_OVERFLOW", "Integer overflow", "Переполнение целого числа"),
                ("var b:byte:=255;\n Inc(b);", "E_OVERFLOW", "Integer overflow", "Переполнение целого числа"),
                ("var u:=MaxUInt64;\n Inc(u);", "E_OVERFLOW", "Integer overflow", "Переполнение целого числа"),
                ("Randomize(1);\n Random(-1);", "E_RANGE", "Random bounds are invalid", "Некорректные границы генератора случайных чисел"),
                ("var a:=Arr(1,2);\n Reverse(a,1,2);", "E_BOUNDS", "Index is outside sequence bounds", "Индекс находится за границами строки, массива или списка"),
                ("var m:=new integer[1,2];\n m[1,0]:=1;", "E_BOUNDS", "Index is outside sequence bounds", "Индекс находится за границами строки, массива или списка"),
                ("var n:=1;\n FormatValue(n,-1);", "E_RANGE", "Field width cannot be negative", "Ширина поля не может быть отрицательной"),
                ("var n:=MaxInt64;\n LCM(n,2);", "E_OVERFLOW", "Integer overflow", "Переполнение целого числа"),
                ("var n:=1bi;\n Writeln(n shl 65536);", "E_LIMIT", "BigInteger size limit exceeded", "Превышен допустимый размер BigInteger"),
                ("var c:='Я';\n Writeln(c.ToDigit);", "E_FORMAT", "Character is not a decimal digit from 0 to 9", "Символ не является десятичной цифрой от 0 до 9"),
                ("var s:='ab';\n s[0]:='X';", "E_BOUNDS", "Index is outside sequence bounds", "Индекс находится за границами строки, массива или списка"),
                ("var s:='ab';\n s.Substring(3);", "E_BOUNDS", "Index is outside sequence bounds", "Индекс находится за границами строки, массива или списка"),
                ("var s:string:=nil;\n s.PadLeft(3);", "E_NIL", "Cannot access a nil reference", "Нельзя обратиться к пустой ссылке nil"),
            ]:
                character.write_text(f"begin\n {source}\nend.", encoding="utf-8")
                invalid = subprocess.run([runner, "run", str(character)], text=True, encoding="utf-8",
                                         capture_output=True, cwd=folder, env=env, timeout=10)
                if (invalid.returncode != 1 or code not in invalid.stderr
                        or f"EN: {english}" not in invalid.stderr or f"RU: {russian}" not in invalid.stderr
                        or f"{character}:3:" not in invalid.stderr):
                    raise RuntimeError(f"{name} bilingual character diagnostic failed: {invalid.stderr!r}")
            file_error = folder / "Ошибка текстового файла.pas"
            for body, code, english, russian in [
                ("var f:Text;\n Writeln(f,1);", "E_IO", "Text file has not been assigned a filename", "Текстовому файлу не назначено имя"),
                ("var f:=OpenWrite('empty.txt'); Close(f); Reset(f);\n ReadInteger(f);", "E_EOF", "Cannot read a token past the end of the text file", "Нельзя прочитать значение после конца текстового файла"),
            ]:
                file_error.write_text(f"begin\n {body}\nend.", encoding="utf-8")
                invalid = subprocess.run([runner, "run", str(file_error)], text=True, encoding="utf-8",
                                         capture_output=True, cwd=folder, env=env, timeout=10)
                if (invalid.returncode != 1 or code not in invalid.stderr
                        or f"EN: {english}" not in invalid.stderr or f"RU: {russian}" not in invalid.stderr
                        or f"{file_error}:3:" not in invalid.stderr):
                    raise RuntimeError(f"{name} bilingual text file error failed: {invalid.stderr!r}")
            empty = folder / "Пустая коллекция.pas"
            file_error.write_text("begin\n var f:Text;\n Writeln(f.ReadToEnd);\nend.", encoding="utf-8")
            invalid = subprocess.run([runner, "run", str(file_error)], text=True, encoding="utf-8",
                                     capture_output=True, cwd=folder, env=env, timeout=10)
            if (invalid.returncode != 1 or "E_NIL" not in invalid.stderr
                    or "EN: Cannot access a nil reference" not in invalid.stderr
                    or "RU: Нельзя обратиться к пустой ссылке nil" not in invalid.stderr
                    or f"{file_error}:3:" not in invalid.stderr):
                raise RuntimeError(f"{name} bilingual Text member error failed: {invalid.stderr!r}")
            for container, method, russian in [("Stack", "Pop", "Стек пуст"), ("Queue", "Dequeue", "Очередь пуста")]:
                empty.write_text(f"uses Collections;\nbegin\n  var c:=new {container}<integer>;\n  c.{method};\nend.", encoding="utf-8")
                invalid = subprocess.run([runner, "run", str(empty)], text=True, encoding="utf-8",
                                         capture_output=True, cwd=folder, env=env, timeout=10)
                if (invalid.returncode != 1 or "E_EMPTY" not in invalid.stderr
                        or f"EN: {container} is empty" not in invalid.stderr
                        or f"RU: {russian}" not in invalid.stderr or f"{empty}:4:" not in invalid.stderr):
                    raise RuntimeError(f"{name} bilingual {container} diagnostic failed: {invalid.stderr!r}")
            for body, english, russian in [
                ("var f:Text; Assign(f,'.');\n f.Erase;", "Cannot erase text file", "Не удалось удалить текстовый файл"),
                ("var f:Text; Assign(f,'rename-source.txt'); WriteAllText('rename-source.txt','source'); WriteAllText('rename-target.txt','occupied');\n f.Rename('rename-target.txt');", "Cannot rename text file", "Не удалось переименовать текстовый файл"),
                ("var f:=OpenWrite('locked-text.txt'); f.Close; f.Reset;\n f.Erase;", "Cannot erase text file", "Не удалось удалить текстовый файл"),
            ]:
                file_error.write_text(f"begin\n {body}\nend.", encoding="utf-8")
                invalid = subprocess.run([runner, "run", str(file_error)], text=True, encoding="utf-8",
                                         capture_output=True, cwd=folder, env=env, timeout=10)
                if (invalid.returncode != 1 or "E_IO" not in invalid.stderr
                        or f"EN: {english}" not in invalid.stderr or f"RU: {russian}" not in invalid.stderr
                        or f"{file_error}:3:" not in invalid.stderr):
                    raise RuntimeError(f"{name} bilingual file change error failed: {invalid.stderr!r}")
            if ((folder / "rename-source.txt").read_text(encoding="utf-8") != "source"
                    or (folder / "rename-target.txt").read_text(encoding="utf-8") != "occupied"):
                raise RuntimeError(f"{name} Text Rename replaced an occupied destination")
            bounds = folder / "Границы массива.pas"
            bounds.write_text("begin\n  var a := Arr(1);\n  Writeln(a[1]);\nend.", encoding="utf-8")
            invalid = subprocess.run([runner, "run", str(bounds)], text=True, encoding="utf-8",
                                     capture_output=True, cwd=folder, env=env, timeout=10)
            if (invalid.returncode != 1 or "E_BOUNDS" not in invalid.stderr
                    or "EN:" not in invalid.stderr or "RU:" not in invalid.stderr
                    or f"{bounds}:3:" not in invalid.stderr):
                raise RuntimeError(f"{name} bilingual array diagnostic failed: {invalid.stderr!r}")
            bad = folder / "Ошибка типов.pas"
            bad.write_text("begin writeln('must never run'); var n := 1; n := 'bad'; end.", encoding="utf-8")
            rejected = subprocess.run([runner, "run", str(bad)], text=True, encoding="utf-8",
                                      capture_output=True, cwd=folder, env=env, timeout=10)
            if rejected.returncode != 1 or rejected.stdout or "E_TYPE" not in rejected.stderr:
                raise RuntimeError(f"{name} did not reject invalid program before execution")
            zero = folder / "Деление на ноль.pas"
            zero.write_text("begin\n  var zero := 0;\n  Writeln(10 div zero);\nend.", encoding="utf-8")
            failed = subprocess.run([runner, "run", str(zero)], text=True, encoding="utf-8",
                                    capture_output=True, cwd=folder, env=env, timeout=10)
            if (failed.returncode != 1 or "EN: Division by zero" not in failed.stderr
                    or "RU: Деление на ноль" not in failed.stderr or f"{zero}:3:" not in failed.stderr):
                raise RuntimeError(f"{name} bilingual runtime diagnostic failed: {failed.stderr!r}")
            domain = folder / "Область определения.pas"
            domain.write_text("begin\n  Writeln(Sqrt(-1));\nend.", encoding="utf-8")
            invalid = subprocess.run([runner, "run", str(domain)], text=True, encoding="utf-8",
                                     capture_output=True, cwd=folder, env=env, timeout=10)
            if (invalid.returncode != 1 or "E_DOMAIN" not in invalid.stderr
                    or "EN: Argument is outside the function domain" not in invalid.stderr
                    or "RU: Аргумент находится вне области определения функции" not in invalid.stderr
                    or f"{domain}:2:" not in invalid.stderr):
                raise RuntimeError(f"{name} standard library runtime error failed: {invalid.stderr!r}")
            conversion = folder / "Преобразование.pas"
            conversion.write_text("begin\n  Writeln(StrToInt('bad'));\nend.", encoding="utf-8")
            invalid = subprocess.run([runner, "run", str(conversion)], text=True, encoding="utf-8",
                                     capture_output=True, cwd=folder, env=env, timeout=10)
            if (invalid.returncode != 1 or "E_FORMAT" not in invalid.stderr
                    or "EN: Invalid number text for integer" not in invalid.stderr
                    or "RU: Некорректная запись числа для типа integer" not in invalid.stderr
                    or f"{conversion}:2:" not in invalid.stderr):
                raise RuntimeError(f"{name} bilingual conversion error failed: {invalid.stderr!r}")
            resize = folder / "Длина строки.pas"
            resize.write_text("begin\n  var s := 'abc';\n  SetLength(s,-1);\nend.", encoding="utf-8")
            invalid = subprocess.run([runner, "run", str(resize)], text=True, encoding="utf-8",
                                     capture_output=True, cwd=folder, env=env, timeout=10)
            if (invalid.returncode != 1 or "E_RANGE" not in invalid.stderr
                    or "EN: String length must not be negative" not in invalid.stderr
                    or "RU: Длина строки не должна быть отрицательной" not in invalid.stderr
                    or f"{resize}:3:" not in invalid.stderr):
                raise RuntimeError(f"{name} bilingual string mutation error failed: {invalid.stderr!r}")
            negative = folder / "Размер DynArray.pas"
            negative.write_text("uses Collections;\nbegin\n  var d:=new DynArray<integer>;\n  d.Count:=-1;\nend.", encoding="utf-8")
            invalid = subprocess.run([runner, "run", str(negative)], text=True, encoding="utf-8",
                                     capture_output=True, cwd=folder, env=env, timeout=10)
            if (invalid.returncode != 1 or "E_RANGE" not in invalid.stderr
                    or "EN: Array length must not be negative" not in invalid.stderr
                    or "RU: Длина массива не должна быть отрицательной" not in invalid.stderr
                    or f"{negative}:4:" not in invalid.stderr):
                raise RuntimeError(f"{name} bilingual DynArray diagnostic failed: {invalid.stderr!r}")
            invalid_map = folder / "Ошибка ключа.pas"
            invalid_map.write_text("uses Collections; begin Writeln('must never run'); var m:=new AssocArray<string,integer>; m[1]:=2; end.", encoding="utf-8")
            invalid = subprocess.run([runner, "run", str(invalid_map)], text=True, encoding="utf-8",
                                     capture_output=True, cwd=folder, env=env, timeout=10)
            if invalid.returncode != 1 or invalid.stdout or "E_TYPE" not in invalid.stderr:
                raise RuntimeError(f"{name} AssocArray key was not checked before execution")
            invalid_map.write_text("uses Collections;\nbegin\n  var m:AssocArray<string,integer>;\n  Writeln(m['x']);\nend.", encoding="utf-8")
            invalid = subprocess.run([runner, "run", str(invalid_map)], text=True, encoding="utf-8",
                                     capture_output=True, cwd=folder, env=env, timeout=10)
            if (invalid.returncode != 1 or "E_NIL" not in invalid.stderr
                    or "EN: Cannot access a nil reference" not in invalid.stderr
                    or "RU: Нельзя обратиться к пустой ссылке nil" not in invalid.stderr
                    or f"{invalid_map}:4:" not in invalid.stderr):
                raise RuntimeError(f"{name} bilingual AssocArray diagnostic failed: {invalid.stderr!r}")
            linked = folder / "Связанный список.pas"
            linked.write_text("uses Collections;\nbegin\n  var l:=new LinkedList<integer>;\n  l.RemoveFirst;\nend.", encoding="utf-8")
            invalid = subprocess.run([runner, "run", str(linked)], text=True, encoding="utf-8",
                                     capture_output=True, cwd=folder, env=env, timeout=10)
            if (invalid.returncode != 1 or "E_EMPTY" not in invalid.stderr
                    or "EN: Linked list is empty" not in invalid.stderr or "RU: Связанный список пуст" not in invalid.stderr
                    or f"{linked}:4:" not in invalid.stderr):
                raise RuntimeError(f"{name} bilingual linked-list error failed: {invalid.stderr!r}")
            linked.write_text("uses Collections;\nbegin\n var l:=new LinkedList<integer>;\n l.AddLast(1); l.AddLast(2);\n var n:=new LinkedListNode<integer>(4,l.Last,l.First);\n l.AddBefore(n,3);\n l.ToString;\nend.", encoding="utf-8")
            invalid = subprocess.run([runner, "run", str(linked), "--max-instructions=200"], text=True, encoding="utf-8",
                                     capture_output=True, cwd=folder, env=env, timeout=10)
            if (invalid.returncode != 1 or "E_LIMIT" not in invalid.stderr
                    or "EN: Instruction limit exceeded" not in invalid.stderr
                    or "RU: Превышен предел числа выполняемых инструкций" not in invalid.stderr
                    or f"{linked}:7:" not in invalid.stderr):
                raise RuntimeError(f"{name} cyclic linked-list limit failed: {invalid.stderr!r}")
            directory_error = folder / "Ошибка каталога.pas"
            for operation in ["ChDir(nil)", "MkDir(nil)", "RmDir(nil)"]:
                directory_error.write_text(f"begin\n {operation};\nend.", encoding="utf-8")
                invalid = subprocess.run([runner, "run", str(directory_error)], text=True, encoding="utf-8",
                                         capture_output=True, cwd=folder, env=env, timeout=10)
                if (invalid.returncode != 1 or "E_IO" not in invalid.stderr
                        or "EN: Directory operation failed" not in invalid.stderr
                        or "RU: Не удалось выполнить операцию с каталогом" not in invalid.stderr
                        or f"{directory_error}:2:" not in invalid.stderr):
                    raise RuntimeError(f"{name} bilingual directory error failed: {invalid.stderr!r}")
            nil_string = folder / "Строковый nil.pas"
            nil_string.write_text("begin\n var s:string:=nil;\n Writeln(Trim(s));\nend.", encoding="utf-8")
            invalid = subprocess.run([runner, "run", str(nil_string)], text=True, encoding="utf-8",
                                     capture_output=True, cwd=folder, env=env, timeout=10)
            if (invalid.returncode != 1 or "E_NIL" not in invalid.stderr
                    or "EN: Cannot access a nil reference" not in invalid.stderr
                    or "RU: Нельзя обратиться к пустой ссылке nil" not in invalid.stderr or f"{nil_string}:3:" not in invalid.stderr):
                raise RuntimeError(f"{name} bilingual nil-string error failed: {invalid.stderr!r}")
            single = folder / "Односвязный узел.pas"
            single.write_text("uses Collections;\nbegin\n var n:SingleNode<integer>;\n n.Next:=nil;\nend.", encoding="utf-8")
            invalid = subprocess.run([runner, "run", str(single)], text=True, encoding="utf-8",
                                     capture_output=True, cwd=folder, env=env, timeout=10)
            if (invalid.returncode != 1 or "E_NIL" not in invalid.stderr
                    or "EN: Cannot access a nil reference" not in invalid.stderr
                    or "RU: Нельзя обратиться к пустой ссылке nil" not in invalid.stderr or f"{single}:4:" not in invalid.stderr):
                raise RuntimeError(f"{name} bilingual SingleNode error failed: {invalid.stderr!r}")
            single.write_text("uses Collections;\nbegin\n var n:=new SingleNode<integer>(1,nil);\n n.Next:=n;\n while n<>nil do n:=n.Next;\nend.", encoding="utf-8")
            invalid = subprocess.run([runner, "run", str(single), "--max-instructions=200"], text=True, encoding="utf-8",
                                     capture_output=True, cwd=folder, env=env, timeout=10)
            if (invalid.returncode != 1 or "E_LIMIT" not in invalid.stderr
                    or "EN: Instruction limit exceeded" not in invalid.stderr
                    or "RU: Превышен предел числа выполняемых инструкций" not in invalid.stderr or f"{single}:5:" not in invalid.stderr):
                raise RuntimeError(f"{name} SingleNode cycle limit failed: {invalid.stderr!r}")
            results.append({"native_pe_binaries": native_binaries, "no_managed_runtime": "passed", "variant": name, "gui": "passed", "sum": 55, "type_error_before_output": "passed", "bilingual_runtime_error": "passed", "standard_library": "passed", "text_conversions": "passed", "string_mutations": "passed", "arrays_records_routines_lists": "passed", "text_files": "passed", "bilingual_array_error": "passed", "stacks_queues": "passed", "bilingual_empty_collections": "passed", "dynarray_simpleset": "passed", "bilingual_dynarray_error": "passed", "assocarray": "passed", "typed_keys_before_output": "passed", "bilingual_assocarray_error": "passed", "linked_lists": "passed", "bilingual_linked_error": "passed", "cyclic_linked_limit": "passed", "single_nodes": "passed", "bilingual_single_error": "passed", "single_cycle_limit": "passed"})
            loop = folder / "Циклы.pas"
            loop.write_text("begin\n repeat until false;\nend.", encoding="utf-8")
            invalid = subprocess.run([runner, "run", str(loop), "--max-instructions=50"], text=True, encoding="utf-8",
                                     capture_output=True, cwd=folder, env=env, timeout=10)
            if (invalid.returncode != 1 or "E_LIMIT" not in invalid.stderr
                    or "EN: Instruction limit exceeded" not in invalid.stderr
                    or "RU: Превышен предел числа выполняемых инструкций" not in invalid.stderr
                    or f"{loop}:2:" not in invalid.stderr):
                raise RuntimeError(f"{name} bilingual loop limit failed: {invalid.stderr!r}")
            loop.write_text("begin Writeln('must never run'); for var i:=1 to 2 do Inc(i); end.", encoding="utf-8")
            invalid = subprocess.run([runner, "run", str(loop)], text=True, encoding="utf-8",
                                     capture_output=True, cwd=folder, env=env, timeout=10)
            if invalid.returncode != 1 or invalid.stdout or "E_TYPE" not in invalid.stderr:
                raise RuntimeError(f"{name} for control mutation was not rejected before output")
            loop.write_text("begin\n var l:=new List<integer>; l.Add(1);\n foreach var x in l do l.Add(2);\nend.", encoding="utf-8")
            invalid = subprocess.run([runner, "run", str(loop)], text=True, encoding="utf-8",
                                     capture_output=True, cwd=folder, env=env, timeout=10)
            if (invalid.returncode != 1 or "E_MODIFIED" not in invalid.stderr
                    or "EN: Collection was modified during enumeration" not in invalid.stderr
                    or "RU: Коллекция изменена во время перебора" not in invalid.stderr
                    or f"{loop}:3:" not in invalid.stderr):
                raise RuntimeError(f"{name} bilingual foreach mutation error failed: {invalid.stderr!r}")
            loop.write_text("begin Writeln('must never run'); foreach var x in Arr(1) do Inc(x); end.", encoding="utf-8")
            invalid = subprocess.run([runner, "run", str(loop)], text=True, encoding="utf-8",
                                     capture_output=True, cwd=folder, env=env, timeout=10)
            if invalid.returncode != 1 or invalid.stdout or "E_TYPE" not in invalid.stderr:
                raise RuntimeError(f"{name} foreach variable mutation was not checked before output")
            results[-1].update({"directories": "passed", "bilingual_directory_error": "passed", "nil_strings": "passed", "bilingual_nil_string": "passed"})
            results[-1]["renames_and_no_overwrite"] = "passed"
            results[-1].update({"classic_text_files": "passed", "bilingual_text_file_error": "passed"})
            results[-1].update({"text_methods": "passed", "bilingual_text_method_error": "passed"})
            results[-1].update({"text_erase_rename": "passed", "bilingual_text_change_error": "passed", "open_text_lock": "passed"})
            results[-1].update({"structured_loops": "passed", "bilingual_loop_limit": "passed", "readonly_for_control": "passed"})
            results[-1].update({"typed_foreach": "passed", "bilingual_modified_list": "passed", "readonly_foreach": "passed"})
            results[-1].update({"characters_and_word": "passed", "string_character_assignment": "passed", "bilingual_character_errors": "passed", "character_types_before_output": "passed"})
            results[-1]["files_operations_unit"] = "passed"
            results[-1]["unit_import_before_output"] = "passed"
            results[-1]["small_integer_types"] = "passed"
            results[-1]["wide_unsigned_and_boolean_ordinals"] = "passed"
            results[-1]["seeded_random_and_arrays"] = "passed"
            results[-1]["reverse_shuffle_swap"] = "passed"
            results[-1]["extended_math"] = "passed"
            results[-1]["arrays_output_and_bitwise"] = "passed"
            results[-1]["shared_console_and_readarray"] = "passed"
            results[-1]["typed_console_functions_and_prompts"] = "passed"
            results[-1]["streaming_unicode_output"] = "passed"
            results[-1]["rectangular_matrices_and_formatting"] = "passed"
            results[-1]["structural_object_output"] = "passed"
            results[-1]["try_read_and_preserved_values"] = "passed"
            results[-1]["str_val_and_try_parse"] = "passed"
            results[-1]["string_cursor_input"] = "passed"
            results[-1]["string_numeric_members"] = "passed"
            results[-1]["string_array_conversions"] = "passed"
            results[-1]["school_bases_and_math"] = "passed"
            results[-1]["bilingual_school_errors_and_unit_scope"] = "passed"
            results[-1]["school_prime_integer_and_int64"] = "passed"
            results[-1]["school_digits_and_prime_lists"] = "passed"
            results[-1]["school_factors_and_divisors"] = "passed"
            results[-1]["school_text_and_implication"] = "passed"
            results[-1]["school_int64_folds"] = "passed"
            results[-1]["unicode_identifiers_and_school_aliases"] = "passed"
            results[-1]["unicode_character_case_and_categories"] = "passed"
            results[-1]["typed_immutable_tuples_and_school_pairs"] = "passed"
            results[-1]["school_minmax_arrays_and_lists"] = "passed"
            results[-1]["biginteger_language_arithmetic_and_io"] = "passed"
            results[-1]["school_biginteger_overloads"] = "passed"
            results[-1]["school_typed_truth_tables_and_printing"] = "passed"
            results[-1]["string_padding_substring_insert_remove"] = "passed"
            results[-1]["school_ipv4_classes_and_generators"] = "passed"
            results[-1]["lazy_typed_ip_sequences"] = "passed"
            results[-1]["array_list_sequence_views"] = "passed"
            results[-1]["school_swap_substrings_and_bilingual_regex_errors"] = "passed"
            results[-1]["regex_is_match_and_first_value"] = "passed"
            results[-1]["sequence_to_list_record_copy_and_ownership"] = "passed"
            results[-1]["typed_sequence_queries_and_empty_diagnostics"] = "passed"
            results[-1]["typed_single_queries_and_multiple_diagnostics"] = "passed"
            results[-1]["typed_seq_snapshot_and_lazy_seqfill"] = "passed"
            results[-1]["lazy_integer_character_range"] = "passed"
            results[-1]["arbitrary_span_biginteger_range"] = "passed"
            results[-1]["partition_points_and_linspace"] = "passed"
            results[-1]["real_range_decimal_scaling"] = "passed"
            results[-1]["typed_numeric_aggregates_and_partial_sums"] = "passed"
            results[-1]["typed_streaming_and_buffered_sequence_slices"] = "passed"
            results[-1]["runtime_optional_arguments_and_typed_default"] = "passed"
            results[-1]["typed_lazy_concat_and_sequence_addition"] = "passed"
            results[-1]["utf16_string_sequence_adapters"] = "passed"
            results[-1]["typed_reverse_default_if_empty_sequence_equal_and_views"] = "passed"
            results[-1]["typed_lazy_select_where_and_shared_vm_callbacks"] = "passed"
            results[-1]["typed_predicate_queries_and_numeric_selectors"] = "passed"
            results[-1]["typed_lazy_select_many"] = "passed"
            results[-1]["typed_const_and_out_parameters"] = "passed"
            results[-1]["typed_user_params_arrays"] = "passed"
            results[-1]["native_assertions_and_bilingual_failures"] = "passed"
    print(json.dumps(results, ensure_ascii=False, indent=2))


if __name__ == "__main__":
    main()
