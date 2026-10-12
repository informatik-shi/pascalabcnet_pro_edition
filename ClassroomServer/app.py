"""Flask API, teacher dashboard and SQLite storage for abc.pro classrooms."""

from __future__ import annotations

import hashlib
import io
import json
import os
import re
import secrets
import sqlite3
import threading
import time
import zipfile
from concurrent.futures import ThreadPoolExecutor
from contextlib import contextmanager
from pathlib import Path
from typing import Iterator

from flask import Flask, Response, abort, jsonify, redirect, render_template_string, request, send_file, session, stream_with_context, url_for
from werkzeug.security import check_password_hash

from .judge import DockerJudge, JudgeUnavailable


TASK_HEADER = re.compile(r"\A\{abc\.pro-task:\s*([0-9a-fA-F-]{36})\s*\n", re.I)
SOURCE_HEADER = re.compile(r"\A\{([^{}]*)\}", re.S)
MAX_SOURCE = 128 * 1024
TOKEN_LIFETIME = 12 * 60 * 60
ONLINE_WINDOW = 90

SCHEMA = """
CREATE TABLE IF NOT EXISTS groups (
    id INTEGER PRIMARY KEY, name TEXT NOT NULL UNIQUE
);
CREATE TABLE IF NOT EXISTS users (
    id INTEGER PRIMARY KEY, username TEXT NOT NULL UNIQUE,
    password_hash TEXT NOT NULL, role TEXT NOT NULL CHECK(role IN ('admin','student')),
    group_id INTEGER REFERENCES groups(id), last_seen INTEGER
);
CREATE TABLE IF NOT EXISTS tokens (
    token_hash TEXT PRIMARY KEY, user_id INTEGER NOT NULL REFERENCES users(id),
    expires_at INTEGER NOT NULL
);
CREATE TABLE IF NOT EXISTS tasks (
    id TEXT PRIMARY KEY, group_id INTEGER NOT NULL REFERENCES groups(id),
    title TEXT NOT NULL, statement TEXT NOT NULL, starter TEXT NOT NULL,
    UNIQUE(group_id,title)
);
CREATE TABLE IF NOT EXISTS tests (
    id INTEGER PRIMARY KEY, task_id TEXT NOT NULL REFERENCES tasks(id) ON DELETE CASCADE,
    input_text TEXT NOT NULL, expected_text TEXT NOT NULL, is_public INTEGER NOT NULL DEFAULT 0
);
CREATE TABLE IF NOT EXISTS progress (
    user_id INTEGER NOT NULL REFERENCES users(id), task_id TEXT NOT NULL REFERENCES tasks(id),
    status TEXT NOT NULL, local_passed INTEGER, local_total INTEGER,
    updated_at INTEGER NOT NULL, last_submission_id INTEGER,
    PRIMARY KEY(user_id,task_id)
);
CREATE TABLE IF NOT EXISTS submissions (
    id INTEGER PRIMARY KEY, user_id INTEGER NOT NULL REFERENCES users(id),
    task_id TEXT NOT NULL REFERENCES tasks(id), source TEXT NOT NULL,
    status TEXT NOT NULL, detail TEXT NOT NULL DEFAULT '', created_at INTEGER NOT NULL,
    completed_at INTEGER
);
CREATE TABLE IF NOT EXISTS events (
    id INTEGER PRIMARY KEY, created_at INTEGER NOT NULL
);
CREATE INDEX IF NOT EXISTS idx_progress_task ON progress(task_id);
CREATE INDEX IF NOT EXISTS idx_submissions_user ON submissions(user_id,created_at);
"""


@contextmanager
def database(path: Path) -> Iterator[sqlite3.Connection]:
    connection = sqlite3.connect(path, timeout=20)
    connection.row_factory = sqlite3.Row
    connection.execute("PRAGMA foreign_keys=ON")
    connection.execute("PRAGMA busy_timeout=20000")
    try:
        with connection:
            yield connection
    finally:
        connection.close()


def initialize(path: Path) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    with database(path) as db:
        db.execute("PRAGMA journal_mode=WAL")
        db.executescript(SCHEMA)


def make_task_file(task: sqlite3.Row) -> str:
    statement = task["statement"]
    if "{" in statement or "}" in statement:
        raise ValueError("Условие не должно содержать фигурных скобок")
    return "{abc.pro-task: " + task["id"] + "\n" + statement.strip() + "\n}\n\n" + task["starter"].lstrip("\r\n")


def extract_task_file(source: str) -> tuple[str, str]:
    match = SOURCE_HEADER.match(source.lstrip("\ufeff"))
    if match is None:
        raise ValueError("Файл задания должен начинаться условием в {фигурных скобках}")
    statement = match.group(1).strip()
    if not statement:
        raise ValueError("Условие задачи пустое")
    return statement, source.lstrip("\ufeff")[match.end():].lstrip("\r\n")


def _upsert_progress(db: sqlite3.Connection, user_id: int, task_id: str,
                     status: str, *, submission_id: int | None = None,
                     local_passed: int | None = None,
                     local_total: int | None = None) -> None:
    db.execute(
        """INSERT INTO progress(user_id,task_id,status,local_passed,local_total,updated_at,last_submission_id)
           VALUES(?,?,?,?,?,?,?) ON CONFLICT(user_id,task_id) DO UPDATE SET
           status=excluded.status, local_passed=excluded.local_passed,
           local_total=excluded.local_total, updated_at=excluded.updated_at,
           last_submission_id=excluded.last_submission_id""",
        (user_id, task_id, status, local_passed, local_total, int(time.time()), submission_id),
    )
    db.execute("INSERT INTO events(created_at) VALUES(?)", (int(time.time()),))


def create_app(config: dict | None = None) -> Flask:
    app = Flask(__name__)
    data_dir = Path(os.environ.get("ABC_CLASSROOM_DATA", Path(__file__).parent / "data"))
    app.config.update(DATA_DIR=data_dir, DATABASE=data_dir / "classroom.sqlite3",
                      JUDGE=DockerJudge(os.environ.get("ABC_JUDGE_IMAGE", "abc-pro-judge:latest")),
                      SESSION_COOKIE_HTTPONLY=True, SESSION_COOKIE_SAMESITE="Lax",
                      MAX_CONTENT_LENGTH=MAX_SOURCE * 6 + 16384)
    if config:
        app.config.update(config)
    app.config["DATA_DIR"] = Path(app.config["DATA_DIR"])
    app.config["DATABASE"] = Path(app.config["DATABASE"])
    initialize(app.config["DATABASE"])
    secret_path = app.config["DATA_DIR"] / "session.key"
    if "SECRET_KEY" not in app.config:
        if not secret_path.exists():
            secret_path.write_text(secrets.token_hex(32), encoding="ascii")
        app.secret_key = secret_path.read_text(encoding="ascii").strip()
    executor = ThreadPoolExecutor(max_workers=2, thread_name_prefix="abc-judge")
    judge_slots = threading.BoundedSemaphore(16)

    def current_user() -> sqlite3.Row | None:
        authorization = request.headers.get("Authorization", "")
        if not authorization.startswith("Bearer "):
            return None
        token = authorization[7:].strip()
        if not token or len(token) > 128:
            return None
        with database(app.config["DATABASE"]) as db:
            return db.execute(
                """SELECT u.* FROM tokens t JOIN users u ON u.id=t.user_id
                   WHERE t.token_hash=? AND t.expires_at>?""",
                (hashlib.sha256(token.encode("ascii", "ignore")).hexdigest(), int(time.time())),
            ).fetchone()

    def student() -> sqlite3.Row:
        user = current_user()
        if user is None:
            abort(401)
        if user["role"] != "student":
            abort(403)
        return user

    def student_task(db: sqlite3.Connection, user: sqlite3.Row, task_id: str) -> sqlite3.Row:
        task = db.execute("SELECT * FROM tasks WHERE id=? AND group_id=?",
                          (task_id, user["group_id"])).fetchone()
        if task is None:
            abort(404)
        return task

    @app.errorhandler(400)
    @app.errorhandler(401)
    @app.errorhandler(403)
    @app.errorhandler(404)
    @app.errorhandler(413)
    def api_error(error):
        if request.path.startswith("/api/"):
            return jsonify(error=error.description), error.code
        return error

    @app.get("/api/health")
    def health():
        return jsonify(status="ok")

    @app.post("/api/login")
    def login():
        payload = request.get_json(silent=True) or {}
        if not isinstance(payload, dict):
            abort(400, "Ожидается JSON-объект")
        username = str(payload.get("login", "")).strip()
        password = str(payload.get("password", ""))
        with database(app.config["DATABASE"]) as db:
            user = db.execute("SELECT * FROM users WHERE username=?", (username,)).fetchone()
            if user is None or not check_password_hash(user["password_hash"], password):
                return jsonify(error="Неверный логин или пароль"), 401
            token = secrets.token_urlsafe(32)
            now = int(time.time())
            db.execute("DELETE FROM tokens WHERE expires_at<=?", (now,))
            db.execute("INSERT INTO tokens(token_hash,user_id,expires_at) VALUES(?,?,?)",
                       (hashlib.sha256(token.encode("ascii")).hexdigest(), user["id"],
                        now + TOKEN_LIFETIME))
            db.execute("UPDATE users SET last_seen=? WHERE id=?", (now, user["id"]))
        return jsonify(token=token, login=username, role=user["role"],
                       expires_in=TOKEN_LIFETIME)

    @app.post("/api/logout")
    def logout():
        user = current_user()
        if user is None:
            abort(401)
        token = request.headers["Authorization"][7:].strip()
        with database(app.config["DATABASE"]) as db:
            db.execute("DELETE FROM tokens WHERE token_hash=?",
                       (hashlib.sha256(token.encode("ascii")).hexdigest(),))
        return jsonify(ok=True)

    @app.post("/api/heartbeat")
    def heartbeat():
        user = student()
        with database(app.config["DATABASE"]) as db:
            db.execute("UPDATE users SET last_seen=? WHERE id=?",
                       (int(time.time()), user["id"]))
        return jsonify(ok=True)

    @app.get("/api/me")
    def me():
        user = student()
        with database(app.config["DATABASE"]) as db:
            rows = db.execute(
                """SELECT t.id,t.title,COALESCE(p.status,'not_started') status,
                   p.local_passed,p.local_total FROM tasks t LEFT JOIN progress p
                   ON p.task_id=t.id AND p.user_id=? WHERE t.group_id=? ORDER BY t.title""",
                (user["id"], user["group_id"]),
            ).fetchall()
        return jsonify(login=user["username"], tasks=[dict(row) for row in rows])

    @app.get("/api/assignments.zip")
    def assignments():
        user = student()
        with database(app.config["DATABASE"]) as db:
            rows = db.execute("SELECT * FROM tasks WHERE group_id=? ORDER BY title",
                              (user["group_id"],)).fetchall()
            group = db.execute("SELECT name FROM groups WHERE id=?",
                               (user["group_id"],)).fetchone()
        data = io.BytesIO()
        manifest = []
        with zipfile.ZipFile(data, "w", zipfile.ZIP_DEFLATED) as archive:
            for index, task in enumerate(rows, 1):
                name = f"task-{index:02d}-{task['id'][:8]}.pas"
                archive.writestr(name, make_task_file(task))
                manifest.append({"id": task["id"], "title": task["title"], "file": name})
            archive.writestr("assignment.json", json.dumps(
                {"format": "abc.pro-classroom-v1", "group": group["name"], "tasks": manifest},
                ensure_ascii=False, indent=2))
        data.seek(0)
        return send_file(data, mimetype="application/zip", as_attachment=True,
                         download_name="abc-pro-assignments.zip")

    @app.get("/api/tasks/<task_id>/tests")
    def public_tests(task_id: str):
        user = student()
        with database(app.config["DATABASE"]) as db:
            student_task(db, user, task_id)
            tests = db.execute("SELECT input_text,expected_text FROM tests WHERE task_id=? "
                               "AND is_public=1 ORDER BY id", (task_id,)).fetchall()
        return jsonify(tests=[{"input": row["input_text"], "expected": row["expected_text"]}
                              for row in tests])

    @app.post("/api/tasks/<task_id>/start")
    def start_task(task_id: str):
        user = student()
        status = "started"
        with database(app.config["DATABASE"]) as db:
            student_task(db, user, task_id)
            existing = db.execute("SELECT status FROM progress WHERE user_id=? AND task_id=?",
                                  (user["id"], task_id)).fetchone()
            if existing is None or existing["status"] != "solved":
                _upsert_progress(db, user["id"], task_id, "started")
            else:
                status = "solved"
        return jsonify(status=status)

    def judge_submission(submission_id: int, source: str, tests: list[dict]) -> None:
        try:
            status, detail = app.config["JUDGE"].judge(source, tests)
        except JudgeUnavailable as exc:
            status, detail = "error", str(exc)
        except Exception as exc:
            app.logger.exception("Judge failure for submission %s", submission_id)
            status, detail = "error", "Ошибка проверки: " + type(exc).__name__
        with database(app.config["DATABASE"]) as db:
            row = db.execute("SELECT user_id,task_id FROM submissions WHERE id=?",
                             (submission_id,)).fetchone()
            db.execute("UPDATE submissions SET status=?,detail=?,completed_at=? WHERE id=?",
                       (status, detail[:2000], int(time.time()), submission_id))
            latest = db.execute("SELECT last_submission_id FROM progress WHERE user_id=? AND task_id=?",
                                (row["user_id"], row["task_id"])).fetchone()
            if latest and latest["last_submission_id"] == submission_id:
                db.execute("""UPDATE progress SET
                    status=CASE WHEN status='solved' THEN 'solved' ELSE ? END,
                    updated_at=? WHERE user_id=? AND task_id=?""",
                           (status, int(time.time()), row["user_id"], row["task_id"]))
                db.execute("INSERT INTO events(created_at) VALUES(?)", (int(time.time()),))

    def run_judge(submission_id: int, source: str, tests: list[dict]) -> None:
        try:
            judge_submission(submission_id, source, tests)
        finally:
            judge_slots.release()

    @app.post("/api/submissions")
    def submit():
        user = student()
        payload = request.get_json(silent=True) or {}
        if not isinstance(payload, dict):
            abort(400, "Ожидается JSON-объект")
        task_id = str(payload.get("task_id", ""))
        source = payload.get("source")
        if not isinstance(source, str) or len(source.encode("utf-8")) > MAX_SOURCE:
            abort(400, "Некорректный или слишком длинный исходный текст")
        match = TASK_HEADER.match(source.lstrip("\ufeff"))
        if match is None or match.group(1).lower() != task_id.lower():
            abort(400, "Идентификатор задачи в начале файла не совпадает")
        local_passed = payload.get("local_passed")
        local_total = payload.get("local_total")
        if not all(type(value) is int and 0 <= value <= 100
                   for value in (local_passed, local_total)) or local_passed > local_total:
            abort(400, "Некорректный результат локальной проверки")
        if not judge_slots.acquire(blocking=False):
            return jsonify(error="Очередь проверок занята; повторите позже"), 429
        try:
            with database(app.config["DATABASE"]) as db:
                student_task(db, user, task_id)
                previous = db.execute(
                    "SELECT status FROM progress WHERE user_id=? AND task_id=?",
                    (user["id"], task_id)).fetchone()
                tests = [dict(row) for row in db.execute(
                    "SELECT input_text,expected_text FROM tests WHERE task_id=? ORDER BY id",
                    (task_id,)).fetchall()]
                cursor = db.execute(
                    "INSERT INTO submissions(user_id,task_id,source,status,created_at) "
                    "VALUES(?,?,?,'pending',?)",
                    (user["id"], task_id, source, int(time.time())),
                )
                submission_id = cursor.lastrowid
                visible_status = "solved" if previous and previous["status"] == "solved" else "pending"
                _upsert_progress(db, user["id"], task_id, visible_status,
                                 submission_id=submission_id, local_passed=local_passed,
                                 local_total=local_total)
            if app.config.get("TESTING_SYNC_JUDGE"):
                run_judge(submission_id, source, tests)
            else:
                executor.submit(run_judge, submission_id, source, tests)
        except BaseException:
            if not app.config.get("TESTING_SYNC_JUDGE"):
                judge_slots.release()
            raise
        return jsonify(submission_id=submission_id, status="pending"), 202

    @app.get("/api/submissions/<int:submission_id>")
    def submission(submission_id: int):
        user = student()
        with database(app.config["DATABASE"]) as db:
            row = db.execute("SELECT status,detail,task_id FROM submissions WHERE id=? AND user_id=?",
                             (submission_id, user["id"])).fetchone()
        if row is None:
            abort(404)
        return jsonify(status=row["status"], task_id=row["task_id"],
                       detail=row["detail"] if row["status"] == "error" else "")

    @app.route("/admin/login", methods=["GET", "POST"])
    def admin_login():
        error = ""
        if request.method == "POST":
            with database(app.config["DATABASE"]) as db:
                user = db.execute("SELECT * FROM users WHERE username=? AND role='admin'",
                                  (request.form.get("login", ""),)).fetchone()
            if user and check_password_hash(user["password_hash"],
                                            request.form.get("password", "")):
                session.clear()
                session["admin_id"] = user["id"]
                return redirect(url_for("admin_dashboard"))
            error = "Неверный логин или пароль"
        return render_template_string("""<!doctype html><html lang="ru"><meta charset="utf-8">
        <title>abc.pro — вход преподавателя</title><style>body{font:16px sans-serif;max-width:420px;
        margin:12vh auto;background:#f6f7fa}form{display:grid;gap:12px;background:white;padding:24px;
        border-radius:12px}input,button{font:inherit;padding:10px}b{color:#a22}</style>
        <form method="post"><h1>abc.pro · преподаватель</h1><b>{{error}}</b>
        <input name="login" placeholder="Логин" required autocomplete="username">
        <input name="password" type="password" placeholder="Пароль" required autocomplete="current-password">
        <button>Войти</button></form></html>""", error=error)

    def require_admin() -> None:
        admin_id = session.get("admin_id")
        if admin_id is None:
            abort(401)
        with database(app.config["DATABASE"]) as db:
            row = db.execute("SELECT id FROM users WHERE id=? AND role='admin'",
                             (admin_id,)).fetchone()
        if row is None:
            abort(403)

    @app.get("/admin")
    def admin_dashboard():
        if "admin_id" not in session:
            return redirect(url_for("admin_login"))
        require_admin()
        return render_template_string("""<!doctype html><html lang="ru"><meta charset="utf-8">
        <title>abc.pro — класс</title><style>body{font:15px sans-serif;max-width:1200px;margin:25px auto;
        color:#222}table{border-collapse:collapse;width:100%}td,th{padding:10px;border-bottom:1px solid #ddd;
        text-align:left}th{background:#eee}.online{color:#167a3d}.offline{color:#888}
        .solved{color:#167a3d}.wrong,.error{color:#b22}.pending{color:#8a6500}</style>
        <h1>abc.pro · класс</h1><p><a href="{{url_for('admin_logout')}}">Выйти</a>
        · Данные обновляются при каждом результате проверки</p>
        <h2>Сводка по ученикам</h2><table><thead><tr><th>Группа</th><th>Ученик</th>
        <th>Подключение</th><th>Решено</th><th>Неверно</th><th>В работе</th><th>Всего задач</th>
        </tr></thead><tbody id="summary"></tbody></table>
        <h2>Задания</h2><table><thead><tr><th>Группа</th><th>Ученик</th>
        <th>Подключение</th><th>Задание</th><th>Статус</th><th>Локальные тесты</th>
        <th>Обновлено</th></tr></thead><tbody id="rows"></tbody></table>
        <script>const labels={not_started:'Не начата',started:'Задача запущена',pending:'Проверяется',
        solved:'Задача решена',wrong:'Решена неверно',error:'Ошибка проверки'};
        async function refresh(){let response=await fetch('/admin/status');if(!response.ok)return;
        let data=await response.json(), body=document.getElementById('rows');body.replaceChildren();
        let summary=document.getElementById('summary');summary.replaceChildren();
        for(let r of data.summary){let tr=document.createElement('tr');
        [r.group,r.student,r.online?'В сети':'Не в сети',r.solved,r.wrong,r.active,r.total]
        .forEach((value,i)=>{let td=document.createElement('td');td.textContent=value;
        if(i==2)td.className=r.online?'online':'offline';tr.appendChild(td)});summary.appendChild(tr)}
        for(let r of data.rows){let tr=document.createElement('tr');let values=[r.group,r.student,
        r.online?'В сети':'Не в сети',r.task,labels[r.status]||r.status,
        r.local_total==null?'—':`${r.local_passed}/${r.local_total}`,
        r.updated_at?new Date(r.updated_at*1000).toLocaleString():'—'];
        values.forEach((value,i)=>{let td=document.createElement('td');td.textContent=value;
        if(i==2)td.className=r.online?'online':'offline';if(i==4)td.className=r.status;
        tr.appendChild(td)});body.appendChild(tr)}}refresh();
        new EventSource('/admin/events').onmessage=refresh;setInterval(refresh,10000)</script></html>""")

    @app.get("/admin/status")
    def admin_status():
        require_admin()
        now = int(time.time())
        with database(app.config["DATABASE"]) as db:
            users = db.execute("""SELECT g.name group_name,u.username,u.last_seen,u.id
                FROM users u JOIN groups g ON g.id=u.group_id WHERE u.role='student'
                ORDER BY g.name,u.username""").fetchall()
            rows = db.execute("""SELECT g.name group_name,u.username,u.last_seen,t.title,
                COALESCE(p.status,'not_started') status,p.local_passed,p.local_total,p.updated_at,u.id user_id
                FROM users u JOIN groups g ON g.id=u.group_id
                JOIN tasks t ON t.group_id=g.id LEFT JOIN progress p
                ON p.user_id=u.id AND p.task_id=t.id WHERE u.role='student'
                ORDER BY g.name,u.username,t.title""").fetchall()
        summary = {user["id"]: {"group": user["group_name"], "student": user["username"],
                                "online": user["last_seen"] is not None and
                                now - user["last_seen"] <= ONLINE_WINDOW,
                                "solved": 0, "wrong": 0, "active": 0, "total": 0}
                   for user in users}
        for row in rows:
            item = summary[row["user_id"]]
            item["total"] += 1
            if row["status"] == "solved":
                item["solved"] += 1
            elif row["status"] == "wrong":
                item["wrong"] += 1
            elif row["status"] in ("started", "pending"):
                item["active"] += 1
        return jsonify(summary=list(summary.values()), rows=[{"group": r["group_name"], "student": r["username"],
                             "online": r["last_seen"] is not None and
                             now - r["last_seen"] <= ONLINE_WINDOW,
                             "task": r["title"], "status": r["status"],
                             "local_passed": r["local_passed"],
                             "local_total": r["local_total"],
                             "updated_at": r["updated_at"]} for r in rows])

    @app.get("/admin/events")
    def admin_events():
        require_admin()
        try:
            last_id = max(0, int(request.headers.get("Last-Event-ID", "0")))
        except ValueError:
            last_id = 0
        if last_id == 0:
            with database(app.config["DATABASE"]) as db:
                last_id = db.execute("SELECT COALESCE(MAX(id),0) FROM events").fetchone()[0]

        @stream_with_context
        def stream():
            nonlocal last_id
            while True:
                with database(app.config["DATABASE"]) as db:
                    newest = db.execute("SELECT MAX(id) FROM events WHERE id>?",
                                        (last_id,)).fetchone()[0]
                if newest is not None:
                    last_id = newest
                    yield f"id: {last_id}\ndata: refresh\n\n"
                else:
                    yield ": keepalive\n\n"
                time.sleep(1)

        return Response(stream(), mimetype="text/event-stream",
                        headers={"Cache-Control": "no-cache", "X-Accel-Buffering": "no"})

    @app.get("/admin/logout")
    def admin_logout():
        session.clear()
        return redirect(url_for("admin_login"))

    return app
