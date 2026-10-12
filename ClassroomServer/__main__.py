"""Teacher commands and production server entry point."""

from __future__ import annotations

import argparse
import getpass
import os
import secrets
import sqlite3
import subprocess
import sys
import uuid
from pathlib import Path

from werkzeug.security import generate_password_hash

from .app import MAX_SOURCE, create_app, database, extract_task_file, initialize
from .judge import DockerJudge, JudgeUnavailable, LocalJudge


def main() -> int:
    parser = argparse.ArgumentParser(description="abc.pro classroom server")
    parser.add_argument("--data", type=Path, default=Path(os.environ.get(
        "ABC_CLASSROOM_DATA", Path(__file__).parent / "data")))
    commands = parser.add_subparsers(dest="command", required=True)
    command = commands.add_parser("init-admin", help="Create a teacher account")
    command.add_argument("login")
    command = commands.add_parser("create-group", help="Create an assignment group")
    command.add_argument("name")
    command = commands.add_parser("create-student", help="Create a student in a group")
    command.add_argument("group")
    command.add_argument("login")
    command = commands.add_parser("add-task", help="Import a .pas file beginning with {condition}")
    command.add_argument("group")
    command.add_argument("title")
    command.add_argument("file", type=Path)
    command = commands.add_parser("add-test", help="Add an input/output test for a task")
    command.add_argument("task_id")
    command.add_argument("input", type=Path)
    command.add_argument("expected", type=Path)
    command.add_argument("--public", action="store_true", help="Run this test in student IDE too")
    command = commands.add_parser("serve", help="Serve on a teacher computer")
    command.add_argument("--host", default="0.0.0.0")
    command.add_argument("--port", type=int, default=8080)
    command.add_argument("--judge", choices=("local", "docker"), default="local")
    command.add_argument("--compiler", type=Path, default=Path(os.environ.get(
        "ABC_CLASSROOM_COMPILER", Path(__file__).resolve().parent.parent /
        "Release" / "PascalABCNET-Portable-win-x64" / "bin-net10")))
    command.add_argument("--dotnet", default=os.environ.get("ABC_CLASSROOM_DOTNET"))
    args = parser.parse_args()
    data = args.data.resolve()
    db_path = data / "classroom.sqlite3"
    initialize(db_path)

    if args.command == "serve":
        try:
            from waitress import serve
        except ImportError as exc:
            raise SystemExit("Install ClassroomServer/requirements.txt first") from exc
        if args.judge == "local":
            judge = LocalJudge(args.compiler, args.dotnet)
            try:
                judge.validate()
            except JudgeUnavailable as exc:
                raise SystemExit(str(exc)) from exc
        else:
            image = os.environ.get("ABC_JUDGE_IMAGE", "abc-pro-judge:latest")
            try:
                check = subprocess.run(["docker", "image", "inspect", image],
                                       stdout=subprocess.DEVNULL, stderr=subprocess.PIPE,
                                       text=True, timeout=15, check=False)
            except (OSError, subprocess.TimeoutExpired) as exc:
                raise SystemExit(f"Docker-проверка недоступна: {exc}") from exc
            if check.returncode:
                raise SystemExit("Не найден образ проверяющего контейнера " + image +
                                 ". Запустите Docker и scripts/build-classroom-judge.ps1. " +
                                 check.stderr.strip()[:400])
            judge = DockerJudge(image)
        app = create_app({"DATA_DIR": data, "DATABASE": db_path, "JUDGE": judge})
        print(f"abc.pro classroom listens on {args.host}:{args.port}; "
              f"judge={args.judge}; open http://127.0.0.1:{args.port}/admin", flush=True)
        serve(app, host=args.host, port=args.port, threads=8)
        return 0

    try:
        with database(db_path) as db:
            if args.command == "init-admin":
                password = getpass.getpass("Пароль преподавателя: ")
                if len(password) < 10:
                    raise ValueError("Пароль преподавателя: не менее 10 символов")
                db.execute("INSERT INTO users(username,password_hash,role) VALUES(?,?,'admin')",
                           (args.login, generate_password_hash(password)))
                print("Учётная запись преподавателя создана")
            elif args.command == "create-group":
                db.execute("INSERT INTO groups(name) VALUES(?)", (args.name,))
                print("Группа создана:", args.name)
            elif args.command == "create-student":
                group = db.execute("SELECT id FROM groups WHERE name=?", (args.group,)).fetchone()
                if group is None:
                    raise ValueError("Группа не найдена")
                password = secrets.token_urlsafe(12)
                db.execute("INSERT INTO users(username,password_hash,role,group_id) "
                           "VALUES(?,?,'student',?)",
                           (args.login, generate_password_hash(password), group["id"]))
                print("Логин:", args.login)
                print("Пароль (показывается один раз):", password)
            elif args.command == "add-task":
                group = db.execute("SELECT id FROM groups WHERE name=?", (args.group,)).fetchone()
                if group is None:
                    raise ValueError("Группа не найдена")
                if args.file.suffix.lower() != ".pas":
                    raise ValueError("Сейчас поддерживаются задания в файлах .pas")
                source = args.file.read_text(encoding="utf-8-sig")
                if len(source.encode("utf-8")) > MAX_SOURCE:
                    raise ValueError("Задание превышает лимит 128 КиБ")
                statement, starter = extract_task_file(source)
                if "{" in statement or "}" in statement:
                    raise ValueError("Условие не должно содержать фигурных скобок")
                task_id = str(uuid.uuid4())
                db.execute("INSERT INTO tasks(id,group_id,title,statement,starter) VALUES(?,?,?,?,?)",
                           (task_id, group["id"], args.title, statement, starter))
                print("ID задания:", task_id)
            elif args.command == "add-test":
                task = db.execute("SELECT id FROM tasks WHERE id=?", (args.task_id,)).fetchone()
                if task is None:
                    raise ValueError("Задание не найдено")
                input_text = args.input.read_text(encoding="utf-8-sig")
                expected_text = args.expected.read_text(encoding="utf-8-sig")
                if (len(input_text.encode("utf-8")) > 65536 or
                        len(expected_text.encode("utf-8")) > 65536):
                    raise ValueError("Вход и ответ теста ограничены 64 КиБ каждый")
                count = db.execute("SELECT COUNT(*) FROM tests WHERE task_id=?",
                                   (args.task_id,)).fetchone()[0]
                if count >= 100:
                    raise ValueError("Не более 100 тестов на задание")
                db.execute("INSERT INTO tests(task_id,input_text,expected_text,is_public) "
                           "VALUES(?,?,?,?)",
                           (args.task_id, input_text, expected_text, int(args.public)))
                print("Тест добавлен:", "открытый" if args.public else "закрытый")
    except (ValueError, sqlite3.IntegrityError, OSError) as exc:
        print("Ошибка:", exc, file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
