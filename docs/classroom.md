# Класс abc.pro: локальный сервер, задания и проверка

Сервер работает **на компьютере преподавателя**. Ученические компьютеры в той
же локальной сети подключаются к его IPv4-адресу и порту. Интернет, внешний
хостинг и регистрация учеников на стороннем сервисе не нужны. Преподаватель
создаёт логины и выдаёт каждому ученику адрес, порт и пароль.

## Что происходит при решении

1. Преподаватель хранит на сервере группы, задания и тесты. У каждого задания
   есть исходный `.pas` файл с условием в первых `{фигурных скобках}`.
2. Ученик входит через меню IDE **Класс → Войти на сервер…**, скачивает ZIP в
   **Класс → Скачать задания…**, распаковывает его и открывает нужный `.pas`.
3. При запуске файла IDE помечает задание как «Задача запущена», проверяет
   решение на **открытых тестах на компьютере ученика** и отправляет исходный
   текст на сервер. Пароль не записывается в файл настроек IDE.
4. Сервер повторно компилирует и запускает решение на всех тестах, включая
   скрытые, в отдельных контейнерах Docker без доступа к сети. Только итог
   этой проверки даёт статусы «Задача решена» или «Решена неверно». Если Docker
   недоступен, показывается «Ошибка проверки»; решение не засчитывается.
   После первого правильного решения задача остаётся засчитанной, даже если
   ученик позже отправит неверную редакцию.
5. На странице преподавателя есть сводная таблица по ученикам: сколько всего
   задач выдано, решено, решено неверно и находится в работе. Ниже показана
   таблица по каждой задаче. Изменения передаются странице сразу после записи
   результата; дополнительное обновление каждые 10 секунд восстанавливает
   таблицу после разрыва соединения.

Индикатор «В сети» основан на сообщениях IDE каждые 30 секунд и считается
актуальным 90 секунд после последнего сообщения.

## Требования к компьютеру преподавателя

- Windows с Python **3.10 или новее**, Docker Desktop с запущенным **Linux
  engine** и доступ к этому репозиторию.
- Собранный portable-пакет с `bin-net10` (см.
  [инструкцию по сборке](build-portable-spython.md)). Сервер Flask и Python
  устанавливаются только на компьютере преподавателя; ученикам нужен abc.pro.
- Для доступа с других компьютеров разрешите входящие TCP-соединения на
  выбранный порт для **частной** сети в брандмауэре Windows. Настройка
  брандмауэра может требовать прав администратора компьютера преподавателя.

Пример команд PowerShell из корня репозитория:

```powershell
py -3 -m venv .codex-build\classroom-venv
.\.codex-build\classroom-venv\Scripts\python.exe -m pip install -r ClassroomServer\requirements.txt
powershell.exe -NoProfile -ExecutionPolicy Bypass -File scripts\build-portable.ps1
powershell.exe -NoProfile -ExecutionPolicy Bypass -File scripts\build-classroom-judge.ps1 -PackageRoot 'Release\PascalABCNET-Portable-win-x64'
```

Команда сборки Docker-образа создаёт `abc-pro-judge:latest` из компилятора
`.NET 10` внутри portable-пакета. При новом релизе компилятора повторите
сборку образа. Скрипт сразу компилирует и запускает простую Pascal-программу
в Linux-контейнере и завершится ошибкой, если проверка не работает. Убедитесь,
что `docker version` показывает раздел **Server**.

### Если Docker Desktop не запускается из-за `dockerInference` или `engine.sock`

На Windows Docker Desktop может завершаться при инициализации сервисов с
сообщением `The file cannot be accessed by the system` для
`%LOCALAPPDATA%\Docker\run\dockerInference` или
`%LOCALAPPDATA%\docker-secrets-engine\engine.sock`. Это ошибка доступа к
временным Unix-сокетам Docker Desktop. Если сначала убрать только один сокет,
при следующем запуске ошибка может появиться для второго. В [отчёте
Docker](https://github.com/docker/desktop-feedback/issues/554) описано
восстановление переносом **обоих каталогов до одного запуска**.

Полностью закройте Docker Desktop. В PowerShell проверьте, что его процессы
отсутствуют, а служба остановлена:

```powershell
Get-Process -Name 'Docker Desktop','com.docker.backend' -ErrorAction SilentlyContinue
Get-Service -Name 'com.docker.service'
```

Следующие команды выполняйте только если первая команда ничего не вывела, а
статус службы — `Stopped`. Они сохраняют каталоги с сокетами как резервные
копии и создают пустые. Перед переносом пути проверяются относительно
`%LOCALAPPDATA%`; образы и тома Docker в `Docker\wsl` не затрагиваются.

```powershell
$localData = [IO.Path]::GetFullPath([Environment]::GetFolderPath('LocalApplicationData')).TrimEnd('\')
$dockerRoot = [IO.Path]::GetFullPath((Join-Path $localData 'Docker')).TrimEnd('\')
$run = [IO.Path]::GetFullPath((Join-Path $dockerRoot 'run'))
$secrets = [IO.Path]::GetFullPath((Join-Path $localData 'docker-secrets-engine'))
$stamp = Get-Date -Format 'yyyyMMddHHmmss'
$runBackup = [IO.Path]::GetFullPath((Join-Path $dockerRoot "run-broken-$stamp"))
$secretsBackup = [IO.Path]::GetFullPath((Join-Path $localData "docker-secrets-engine-broken-$stamp"))
if (-not $run.StartsWith($dockerRoot + '\', [StringComparison]::OrdinalIgnoreCase) -or
    -not $runBackup.StartsWith($dockerRoot + '\', [StringComparison]::OrdinalIgnoreCase) -or
    -not $secrets.StartsWith($localData + '\', [StringComparison]::OrdinalIgnoreCase) -or
    -not $secretsBackup.StartsWith($localData + '\', [StringComparison]::OrdinalIgnoreCase)) {
    throw 'Unexpected Docker socket paths'
}
if (Get-Process -Name 'Docker Desktop','com.docker.backend' -ErrorAction SilentlyContinue) {
    throw 'Docker Desktop is still running'
}
if ((Get-Service -Name 'com.docker.service').Status -ne 'Stopped') {
    throw 'Docker service is still running'
}
if (Test-Path -LiteralPath $runBackup -PathType Any) { throw 'Run backup already exists' }
if (Test-Path -LiteralPath $secretsBackup -PathType Any) { throw 'Secrets backup already exists' }
if (Test-Path -LiteralPath $run -PathType Container) {
    Move-Item -LiteralPath $run -Destination $runBackup -ErrorAction Stop
}
if (Test-Path -LiteralPath $secrets -PathType Container) {
    Move-Item -LiteralPath $secrets -Destination $secretsBackup -ErrorAction Stop
}
New-Item -ItemType Directory -Path $run -Force | Out-Null
New-Item -ItemType Directory -Path $secrets -Force | Out-Null
docker desktop start
docker version
```

В выводе `docker version` должен появиться раздел **Server**. Если ошибка
повторяется, сохраните резервные каталоги и диагностический отчёт Docker
Desktop для поиска причины; сброс настроек к заводским для этой процедуры не
нужен. Восстановление временных сокетов не гарантирует, что ошибка не
повторится после очередного завершения Docker Desktop.

## Данные преподавателя

По умолчанию база SQLite, закрытые тесты, учётные записи и ключ сессии
хранятся в `ClassroomServer\data`. Папка исключена из Git. Делайте резервную
копию этой папки при остановленном сервере. Можно задать отдельное место через
`--data C:\abc-pro-classroom-data` **перед** названием каждой команды.

Создайте преподавателя и группу:

```powershell
$python = '.\.codex-build\classroom-venv\Scripts\python.exe'
& $python -m ClassroomServer init-admin teacher
& $python -m ClassroomServer create-group '7-А'
& $python -m ClassroomServer create-student '7-А' student01
```

`init-admin` запросит пароль (минимум 10 символов). `create-student` выведет
случайный пароль один раз: передайте его ученику вместе с логином. Для каждого
ученика создайте отдельную учётную запись.

Подготовьте `sum.pas` в UTF-8. Первым содержимым файла должно быть условие:

```pascal
{Считайте два целых числа и выведите их сумму.}
var a, b: integer;
begin
  Readln(a);
  Readln(b);
  Writeln(a + b)
end.
```

Остаток файла служит стартовым кодом. При выдаче сервер добавляет в начало
комментария идентификатор `abc.pro-task`; его нельзя удалять из ученической
копии, так как по нему IDE и сервер находят задание.

Импортируйте задачу, запишите напечатанный UUID и добавьте открытый и скрытый
тесты. В каждом входном файле находится ровно то, что программа получает на
стандартный ввод; в файле ответа — ожидаемый стандартный вывод.

```powershell
& $python -m ClassroomServer add-task '7-А' 'Сумма двух чисел' .\sum.pas
# Пример: ID задания: 11111111-2222-4333-8444-555555555555
& $python -m ClassroomServer add-test 11111111-2222-4333-8444-555555555555 .\open.in .\open.out --public
& $python -m ClassroomServer add-test 11111111-2222-4333-8444-555555555555 .\hidden.in .\hidden.out
```

Содержимое `open.in`: `2` и `3` на отдельных строках. Содержимое
`open.out`: `5`. Закрытый тест не выдаётся ученической IDE. Команда
`add-test` без `--public` делает тест скрытым.

## Запуск и подключение по локальной сети

```powershell
& $python -m ClassroomServer serve --host 0.0.0.0 --port 8080
```

`0.0.0.0` означает прослушивание сетевых интерфейсов преподавательского
компьютера. Это **не адрес для учеников**. Узнайте IPv4-адрес преподавателя
через `ipconfig`; например, если он равен `192.168.1.20`, выдайте ученикам
`192.168.1.20`, порт `8080`, их логины и пароли. С компьютера преподавателя
откройте `http://127.0.0.1:8080/admin`, войдите как преподаватель. Ученик
указывает выданный IP и порт в диалоге IDE. Подключение проверяется через
`http://192.168.1.20:8080/api/health`; ожидается `{"status":"ok"}`.

HTTPS-флажок в IDE предназначен для случая, когда перед сервером установлен
локальный HTTPS-прокси с действующим сертификатом. Сам Waitress обслуживает
HTTP; по HTTP пароль идёт по локальной сети без шифрования. Используйте
доверенную сеть или настройте HTTPS-прокси, если сеть общая для посторонних.
Не открывайте этот порт в интернет и не делайте переадресацию на роутере.

Сервер перед запуском проверяет, что Docker работает и образ
`abc-pro-judge:latest` существует. Для проверки используются ограничения
памяти, CPU, числа процессов, времени и вывода; в контейнер не передаются
закрытые тесты других задач или база сервера. Запуск графических программ в
серверном контейнере сейчас не предусмотрен: задания должны читать stdin и
писать ответ в stdout.

## Воспроизводимая сборка клиента и проверки

После изменений в IDE и сервере выполните из корня ветки `portable`:

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File scripts\build-portable.ps1
powershell.exe -NoProfile -ExecutionPolicy Bypass -File scripts\build-abc-pro-installer.ps1 -PackageRoot 'Release\PascalABCNET-Portable-win-x64'
powershell.exe -NoProfile -ExecutionPolicy Bypass -File scripts\build-classroom-judge.ps1 -PackageRoot 'Release\PascalABCNET-Portable-win-x64'
.\.codex-build\classroom-venv\Scripts\python.exe -m unittest tests.test_classroom_server -v
& "$env:USERPROFILE\.dotnet\dotnet.exe" run --project tests\ClassroomLocalSmoke\ClassroomLocalSmoke.csproj --configuration Release -- "$(Resolve-Path bin\pabcnetcclear.exe)"
```

Готовый установщик лежит в `Release-abc-pro`; Docker-образ остаётся на
компьютере преподавателя. Сервер не входит в установщик для учеников.

## Ограничения текущей версии

- Задания — отдельные консольные файлы Pascal `.pas`. IDE проверяет их после
  успешной компиляции и запуска. Ошибка компиляции в IDE пока не создаёт
  отдельную запись в таблице преподавателя.
- Открытые тесты помогают ученику, но их результат не является зачётом.
  Сервер всегда проверяет исходный текст независимо от отчёта IDE.
- При закрытии сервера уже выданные ZIP остаются на ученических компьютерах;
  новые результаты поступят только после повторного запуска сервера.
