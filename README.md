# PascalABC.NET

PascalABC.NET is a modern Pascal programming language and an integrated
development environment for Microsoft .NET. The repository currently supports
two compiler targets:

- the complete system for .NET Framework 4.0/4.7.2;
- the console compiler for .NET 10.

## Downloads

The [GitHub Releases page](https://github.com/informatik-shi/pascalabcnet_pro_edition/releases)
contains ready-to-use builds. Download the
[Windows x64 portable ZIP](https://github.com/informatik-shi/pascalabcnet_pro_edition/releases/download/portable-2026-10-06.1/PascalABCNET-Portable-win-x64.zip)
for the IDE and both compilers. Extract it to a writable folder and run
`PascalABCNET.cmd`. Installation is not required.

## Building on Windows

PascalABC.NET is developed with Visual Studio 2026. Install the .NET desktop
development tools and the .NET 10 SDK.

Before the first complete .NET Framework build, run `_RegisterHelixNUnit.bat` as
Administrator. It installs the required HelixToolkit and NUnit assemblies into
the GAC.

### .NET Framework version

```bat
_ReBuildDebug.bat
_ReBuildRelease.bat
```

Build the Release configuration, rebuild Pascal units, and run the complete
.NET Framework test suite (Administrator privileges are required):

```bat
_RebuildReleaseAndRunTests.bat
```

Build and test the complete system, then create the Windows installers:

```bat
_GenerateAllSetups.bat
```

The generated installers are written to the `Release` directory.

### Portable Windows package (IDE and both compilers)

On the build machine, install the .NET 10 SDK and the Windows Desktop runtime.
Build the self-contained ZIP with:

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File scripts\build-portable.ps1
```

The output is `Release\PascalABCNET-Portable-win-x64.zip`. Extract it to a
writable folder and run `PascalABCNET.cmd`; the command-line launchers are
`pabcnetc.cmd` and `pabcnetc-net10.cmd`. The package contains the .NET 10
runtime and an app-local `PABCRtl.dll`, so it does not need a system-wide
PascalABC.NET installation or GAC registration. Windows still needs its
.NET Framework 4.7.2 or newer component for the IDE and classic compiler.

The prebuilt `bin\Lib\PABCRtl.dll` is included in this branch because the
Pascal RTL is a required app-local runtime dependency. `-SkipBuild` repackages
existing outputs after a full build.

The complete Windows build and SPython editor verification steps are in
[`docs/build-portable-spython.md`](docs/build-portable-spython.md).

### .NET 10 console compiler

Build the console compiler and rebuild its standard units:

```bat
_RebuildStandartModules_net10.bat Release
```

Run the console test suites independently:

```bat
_RunCrossTargetTests_net472.bat
_RunCrossTargetTests_net10_clean.bat
```

Create the ready-to-publish .NET 10 console distribution:

```bat
_BuildConsoleNet10Distribution.bat
```

The resulting archive is written to
`Release\PascalABCNET-Console-net10.zip`.

## Linux and Mono

The existing Linux IDE and .NET Framework compiler run on Mono. A short source-build
command is:

```bash
git clone https://github.com/pascalabcnet/pascalabcnet
cd pascalabcnet
sh _RebuildReleaseAndRunTests.sh
```

Run the command-line compiler with:

```bash
cd bin
mono pabcnetc.exe
```

The complete installation guide, including distribution-specific Mono setup,
CP1251 support, and Mono registry configuration, is currently available in
Russian: [Installing PascalABC.NET on Linux](https://pascalabcnet.github.io/mydoc_linux1.html).

---

# PascalABC.NET: сборка проекта

PascalABC.NET — современный язык программирования Паскаль и интегрированная
среда разработки для платформы Microsoft .NET.

Репозиторий поддерживает две цели:

- полную систему для .NET Framework 4.0/4.7.2;
- консольный компилятор для .NET 10.

## Сборка в Windows

Разработка ведётся в Visual Studio 2026. Необходимо установить средства
разработки классических приложений .NET и .NET 10 SDK.

Перед первой полной сборкой версии для .NET Framework запустите `_RegisterHelixNUnit.bat` с правами
администратора. Сценарий устанавливает необходимые сборки HelixToolkit и NUnit
в GAC.

### Версия для .NET Framework

```bat
_ReBuildDebug.bat
_ReBuildRelease.bat
```

Сборка Release, перекомпиляция Pascal-модулей и полный прогон старых тестов
(требуются права администратора):

```bat
_RebuildReleaseAndRunTests.bat
```

Полная сборка, тестирование и создание Windows-установщиков:

```bat
_GenerateAllSetups.bat
```

Готовые установщики сохраняются в каталоге `Release`.

### Консольный компилятор .NET 10

Сборка компилятора и перекомпиляция стандартных модулей:

```bat
_RebuildStandartModules_net10.bat Release
```

Раздельный запуск консольных тестов:

```bat
_RunCrossTargetTests_net472.bat
_RunCrossTargetTests_net10_clean.bat
```

Создание готового дистрибутива консольного компилятора .NET 10:

```bat
_BuildConsoleNet10Distribution.bat
```

Результат сохраняется в
`Release\PascalABCNET-Console-net10.zip`.

## Linux и Mono

Существующая Linux IDE и legacy-компилятор работают под Mono. Полная актуальная
инструкция по установке Mono, поддержке кодировки CP1251 и настройке Mono
находится в документе
[«Установка PascalABC.NET под Linux»](https://pascalabcnet.github.io/mydoc_linux1.html).

Сборка исходного проекта и запуск тестов:

```bash
git clone https://github.com/pascalabcnet/pascalabcnet
cd pascalabcnet
sh _RebuildReleaseAndRunTests.sh
```

Запуск компилятора из командной строки:

```bash
cd bin
mono pabcnetc.exe
```

## Тесты

Основной набор тестов находится в каталоге `TestSuite`. Полный старый TestRunner
используется сценариями сборки .NET Framework. Для независимой проверки консольных
компиляторов используйте `_RunCrossTargetTests_net472.bat` и
`_RunCrossTargetTests_net10_clean.bat`.
