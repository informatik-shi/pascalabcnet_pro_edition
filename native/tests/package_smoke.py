"""Check the actual Windows ZIPs without Qt/Tcl/Python directories in PATH."""
import argparse
import json
import os
from pathlib import Path
import subprocess
import tempfile
import zipfile


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
                ("russian-identifiers.pas", "Ответ: 42\nНОД: 6\nНОК: 90\n"),
                ("big-integers.pas", "100! = 93326215443944152681699238856266700490715968264381621468592963895217599993229915608941463976156518286253697920827223758251185210916864000000000000000000000000\nЦелый корень квадрата: 123456789012345678901234567890\nНОД: 6\nСдвиги: 16\n"),
                ("big-integer-files.pas", "Из файла: 246913578024691357802469135780\n"),
                ("school-big-integers.pas", "Число: 79228162514264337593543950335\nОснование 16: FFFFFFFFFFFFFFFFFFFFFFFF\nЦифры: [15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15]\nОбратное преобразование: 79228162514264337593543950335\nБайты: 00000000:00000001:00000000\nНОК массива: 55340232221128654848\nМинимум и максимум: (-79228162514264337593543950335,79228162514264337593543950335)\n"),
                ("tuples.pas", "ответ: 42\nНОД и НОК: (6,90)\nНОД: 6; НОК: 90\n"),
                ("school-minmax.pas", "Минимум: -7\nМаксимум: 42\nПара: (-7,42)\n"),
                ("truth-tables.pas", "Исключающее ИЛИ:\n a b F\n -----\n 0 0 0\n 0 1 1\n 1 0 1\n 1 1 0\nТри переменные, только истинные строки:\n x y z F\n -------\n 1 1 1 1\n"),
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
            results.append({"variant": name, "gui": "passed", "sum": 55, "type_error_before_output": "passed", "bilingual_runtime_error": "passed", "standard_library": "passed", "text_conversions": "passed", "string_mutations": "passed", "arrays_records_routines_lists": "passed", "text_files": "passed", "bilingual_array_error": "passed", "stacks_queues": "passed", "bilingual_empty_collections": "passed", "dynarray_simpleset": "passed", "bilingual_dynarray_error": "passed", "assocarray": "passed", "typed_keys_before_output": "passed", "bilingual_assocarray_error": "passed", "linked_lists": "passed", "bilingual_linked_error": "passed", "cyclic_linked_limit": "passed", "single_nodes": "passed", "bilingual_single_error": "passed", "single_cycle_limit": "passed"})
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
    print(json.dumps(results, ensure_ascii=False, indent=2))


if __name__ == "__main__":
    main()
