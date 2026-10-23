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
                ("structures.pas", "Точка: 3 4\nФакториал 10: 3628800\n1\n2\n5\n8\nИмен: 2\nПервое имя: Анна\n"),
                ("files.pas", "Привет из C++ интерпретатора!\nПрочитано строк: 2\nВторая строка\n"),
                ("collections.pas", "Стек: 30 20 10 \nОчередь: Анна Борис Вера \nПустая очередь: True\n"),
                ("buffers.pas", "Массив: 10 20 30 \nРазмер: 3; ёмкость: 4\nПосле удаления: 20 30 \nБез повторов: Анна Борис \nЕсть Анна: False\n"),
                ("associations.pas", "Цена яблок: 125\nНет цены: 0\nВсе цены:\nЯблоки 125\nГруши 150\n"),
                ("linked-lists.pas", "Вперёд: Анна Борис Вера \nНазад после удаления: Вера Анна \n"),
                ("single-nodes.pas", "Цепочка: Анна Борис Вера \nПосле изменения: Анна Виктория\n"),
                ("nil-strings.pas", "Нет значения: nil\nЭто nil: True\nПустая строка — nil: False\nДлина nil: 0\nСоединение: [текст]\nПосле Insert: Новое имя\n"),
                ("directories.pas", "Файл в выбранной папке\nНовое имя: заметка.bak\nФайл удалён: True\nВложенная папка удалена: True\nПапка примера удалена: True\n"),
            ]:
                example = subprocess.run([runner, "run", str(folder / "examples" / source)],
                                         text=True, encoding="utf-8", capture_output=True,
                                         cwd=folder, env=env, timeout=10)
                if example.returncode or example.stdout != expected:
                    raise RuntimeError(f"{name} {source} failed: {example.stdout!r}, {example.stderr!r}")
            empty = folder / "Пустая коллекция.pas"
            for container, method, russian in [("Stack", "Pop", "Стек пуст"), ("Queue", "Dequeue", "Очередь пуста")]:
                empty.write_text(f"uses Collections;\nbegin\n  var c:=new {container}<integer>;\n  c.{method};\nend.", encoding="utf-8")
                invalid = subprocess.run([runner, "run", str(empty)], text=True, encoding="utf-8",
                                         capture_output=True, cwd=folder, env=env, timeout=10)
                if (invalid.returncode != 1 or "E_EMPTY" not in invalid.stderr
                        or f"EN: {container} is empty" not in invalid.stderr
                        or f"RU: {russian}" not in invalid.stderr or f"{empty}:4:" not in invalid.stderr):
                    raise RuntimeError(f"{name} bilingual {container} diagnostic failed: {invalid.stderr!r}")
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
            results[-1].update({"directories": "passed", "bilingual_directory_error": "passed", "nil_strings": "passed", "bilingual_nil_string": "passed"})
    print(json.dumps(results, ensure_ascii=False, indent=2))


if __name__ == "__main__":
    main()
