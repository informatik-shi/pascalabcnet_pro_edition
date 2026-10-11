# Установщик abc.pro для текущего пользователя

`abc.pro` — отдельное приложение на базе этой ветки PascalABC.NET. Установщик
содержит IDE, классический компилятор, компилятор .NET 10, SPython, встроенный
Python для Matplotlib и браузерные тетрадки. Для установки права администратора
не нужны: по умолчанию файлы попадают в
`%LOCALAPPDATA%\Programs\abc.pro`, ярлыки создаются в меню «Пуск» текущего
пользователя, запись об удалении программы хранится в `HKCU`. Установщик не
вызывает GAC и не записывает файлы в `Program Files`.

На компьютере пользователя должна быть доступна .NET Framework 4.7.2 или
новее для IDE и классического компилятора. .NET 10 и Python уже включены в
пакет. Запуск происходит через `abc.pro.exe`; ярлык «abc.pro Notebook»
запускает локальный сервер тетрадок и браузер.

## Воспроизводимая сборка

Нужны Windows x64, исходники ветки `portable`, .NET 10 SDK, средства сборки
.NET Framework 4.7.2 и PowerShell. NSIS 3.10 уже включён в репозиторий:
`Utils\NSIS2\makensis.exe`. Вся подготовка и регистрация зависимостей,
необходимая для **сборки из исходников**, описана в
[руководстве по portable-сборке](build-portable-spython.md). На **установку**
готового `abc.pro` она не влияет.

Из корня репозитория:

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File scripts\build-portable.ps1
powershell.exe -NoProfile -ExecutionPolicy Bypass -File scripts\build-abc-pro-installer.ps1
```

Если .NET SDK лежит в другом месте, обоим сценариям можно передать
`-DotnetRoot "C:\путь\к\dotnet"`. Второй сценарий также принимает `-PackageRoot`
для уже собранной portable-папки и `-OutputRoot` для результата. Он пересобирает
IDE с именем `abc.pro`, собирает запускатель и пакует portable-компоненты
установщиком NSIS. Номер версии берётся из
`ReleaseGenerators\PascalABCNET_version.nsh`. Результат:

```text
Release-abc-pro\abc.pro-Setup-<версия>-win-x64.exe
```

Не используйте `-SkipBuild` в первом сценарии после изменения исходного кода:
этот режим только перепаковывает прежние выходные файлы.

## Установка, данные и удаление

Запустите `abc.pro-Setup-<версия>-win-x64.exe` обычным пользователем. Папку по
умолчанию можно менять, если у пользователя есть право записи в выбранное
место. Приложение не требует изменения системного `PATH` и не затрагивает
другую установку PascalABC.NET.

При первом запуске `abc.pro.exe` создаёт рабочую папку
`%LOCALAPPDATA%\abc.pro\Work`, копирует туда образцы и подключает её к IDE.
Там же сохраняются настройки, результаты компиляции и графика Matplotlib.
Тетрадки сохраняются в `%LOCALAPPDATA%\abc.pro\Notebooks`. При обновлении или
удалении программы эти пользовательские файлы сохраняются. Для полного
удаления данных их можно отдельно удалить вручную после закрытия программы.

Удаление доступно в «Установленные приложения» Windows и через ярлык
«Удалить abc.pro» в меню «Пуск». Выполняется от текущего пользователя.

## Проверка установленного пакета

На чистой установке проверьте наличие файлов
`abc.pro.exe`, `bin\PascalABCNET.exe`, `bin\pabcnetc.exe`,
`bin-net10\pabcnetcclear.exe`, `dotnet\dotnet.exe`, `python\python.exe` и
`notebook\PascalABCNotebook.dll`.

```powershell
$check = Start-Process -FilePath "$env:LOCALAPPDATA\Programs\abc.pro\abc.pro.exe" `
    -ArgumentList '--self-test' -WindowStyle Hidden -Wait -PassThru
if ($check.ExitCode -ne 0) { throw 'Пакет abc.pro неполон' }
```

Откройте IDE через ярлык «abc.pro», создайте и выполните небольшую программу.
Для проверки тетрадок запустите ярлык «abc.pro Notebook» и выполните ячейку
Pascal или SPython. Команды `PascalABCNET.cmd` и `PascalABCNotebook.cmd` в
установленной папке направляют запуск через соответствующие запускатели
`abc.pro` и также используют папки текущего пользователя.
