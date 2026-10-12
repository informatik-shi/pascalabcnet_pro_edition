import io
import json
import tempfile
import unittest
import uuid
import zipfile
from pathlib import Path

from werkzeug.security import generate_password_hash

from ClassroomServer.app import create_app, database, extract_task_file, make_task_file


class FakeJudge:
    def judge(self, source, tests):
        return ("solved", "Все тесты пройдены") if "Writeln(5)" in source else (
            "wrong", "Неверный ответ")


class ClassroomServerTests(unittest.TestCase):
    def setUp(self):
        self.directory = tempfile.TemporaryDirectory()
        self.database_path = Path(self.directory.name) / "classroom.sqlite3"
        self.app = create_app({"TESTING": True, "TESTING_SYNC_JUDGE": True,
                               "SECRET_KEY": "test-only", "DATA_DIR": self.directory.name,
                               "DATABASE": self.database_path, "JUDGE": FakeJudge()})
        self.client = self.app.test_client()
        self.task_id = str(uuid.uuid4())
        self.other_task_id = str(uuid.uuid4())
        with database(self.database_path) as db:
            group = db.execute("INSERT INTO groups(name) VALUES('7-А')").lastrowid
            other = db.execute("INSERT INTO groups(name) VALUES('8-Б')").lastrowid
            db.execute("INSERT INTO users(username,password_hash,role) VALUES(?,?,'admin')",
                       ("teacher", generate_password_hash("teacher-pass")))
            db.execute("INSERT INTO users(username,password_hash,role,group_id) "
                       "VALUES(?,?,'student',?)",
                       ("alice", generate_password_hash("alice-pass"), group))
            db.execute("INSERT INTO users(username,password_hash,role,group_id) "
                       "VALUES(?,?,'student',?)",
                       ("bob", generate_password_hash("bob-pass"), other))
            db.execute("INSERT INTO tasks(id,group_id,title,statement,starter) VALUES(?,?,?,?,?)",
                       (self.task_id, group, "Сумма", "Выведите сумму", "begin\nend.\n"))
            db.execute("INSERT INTO tasks(id,group_id,title,statement,starter) VALUES(?,?,?,?,?)",
                       (self.other_task_id, other, "Разность", "Выведите разность", "begin end."))
            db.execute("INSERT INTO tests(task_id,input_text,expected_text,is_public) "
                       "VALUES(?,?,?,?)", (self.task_id, "2 3\n", "5\n", 1))
            db.execute("INSERT INTO tests(task_id,input_text,expected_text,is_public) "
                       "VALUES(?,?,?,?)", (self.task_id, "4 1\n", "5\n", 0))

    def tearDown(self):
        self.directory.cleanup()

    def login(self, username="alice", password="alice-pass"):
        response = self.client.post("/api/login", json={"login": username, "password": password})
        self.assertEqual(response.status_code, 200)
        return {"Authorization": "Bearer " + response.json["token"]}

    def source(self, body="begin Writeln(5) end."):
        return "{abc.pro-task: " + self.task_id + "\nВыведите сумму\n}\n" + body

    def test_login_archive_public_tests_and_group_isolation(self):
        self.assertEqual(self.client.post("/api/login", json={"login": "alice",
                          "password": "bad"}).status_code, 401)
        auth = self.login()
        response = self.client.get("/api/assignments.zip", headers=auth)
        self.assertEqual(response.status_code, 200)
        with zipfile.ZipFile(io.BytesIO(response.data)) as archive:
            manifest = json.loads(archive.read("assignment.json"))
            self.assertEqual(len(manifest["tasks"]), 1)
            source = archive.read(manifest["tasks"][0]["file"]).decode("utf-8")
            self.assertIn("{abc.pro-task: " + self.task_id, source)
            self.assertIn("Выведите сумму", source)
        tests = self.client.get(f"/api/tasks/{self.task_id}/tests", headers=auth).json["tests"]
        self.assertEqual(tests, [{"input": "2 3\n", "expected": "5\n"}])
        self.assertEqual(self.client.get(f"/api/tasks/{self.other_task_id}/tests",
                                         headers=auth).status_code, 404)
        self.assertEqual(self.client.post("/api/logout", headers=auth).status_code, 200)
        self.assertEqual(self.client.get("/api/me", headers=auth).status_code, 401)

    def test_progress_summary_updates_after_server_verdict(self):
        auth = self.login()
        self.assertEqual(self.client.post(f"/api/tasks/{self.task_id}/start",
                                          headers=auth).status_code, 200)
        self.assertEqual(self.client.get("/api/me", headers=auth).json["tasks"][0]["status"],
                         "started")
        self.assertEqual(self.client.post("/api/submissions", headers=auth,
            json={"task_id": self.task_id, "source": self.source("begin Writeln(4) end."),
                  "local_passed": 0, "local_total": 1}).status_code, 202)
        self.assertEqual(self.client.get("/api/me", headers=auth).json["tasks"][0]["status"],
                         "wrong")
        self.assertEqual(self.client.get("/admin/status").status_code, 401)
        self.assertEqual(self.client.post("/admin/login", data={
            "login": "teacher", "password": "teacher-pass"}).status_code, 302)
        before = self.client.get("/admin/status")
        alice_before = next(row for row in before.json["summary"]
                            if row["student"] == "alice")
        self.assertEqual((alice_before["solved"], alice_before["wrong"],
                          alice_before["active"]), (0, 1, 0))
        response = self.client.post("/api/submissions", headers=auth,
            json={"task_id": self.task_id, "source": self.source(),
                  "local_passed": 1, "local_total": 1})
        self.assertEqual(response.status_code, 202)
        self.assertEqual(self.client.get("/api/submissions/" + str(response.json["submission_id"]),
                                         headers=auth).json["status"], "solved")
        status = self.client.get("/admin/status")
        self.assertEqual(status.status_code, 200)
        alice = next(row for row in status.json["summary"] if row["student"] == "alice")
        self.assertEqual((alice["solved"], alice["wrong"], alice["active"], alice["total"]),
                         (1, 0, 0, 1))
        retry = self.client.post("/api/submissions", headers=auth,
            json={"task_id": self.task_id,
                  "source": self.source("begin Writeln(4) end."),
                  "local_passed": 0, "local_total": 1})
        self.assertEqual(self.client.get("/api/submissions/" + str(retry.json["submission_id"]),
                                         headers=auth).json["status"], "wrong")
        summary_after_retry = self.client.get("/admin/status").json["summary"]
        alice_after_retry = next(row for row in summary_after_retry
                                 if row["student"] == "alice")
        self.assertEqual(alice_after_retry["solved"], 1)
        with database(self.database_path) as db:
            self.assertGreaterEqual(db.execute("SELECT count(*) FROM events").fetchone()[0], 4)

    def test_rejects_wrong_task_marker_and_cross_group_submission(self):
        auth = self.login("bob", "bob-pass")
        response = self.client.post("/api/submissions", headers=auth,
            json={"task_id": self.task_id, "source": self.source(),
                  "local_passed": 0, "local_total": 0})
        self.assertEqual(response.status_code, 404)
        auth = self.login()
        response = self.client.post("/api/submissions", headers=auth,
            json={"task_id": self.task_id, "source": "begin end.",
                  "local_passed": 0, "local_total": 0})
        self.assertEqual(response.status_code, 400)

    def test_task_comment_round_trip(self):
        condition, starter = extract_task_file("{Напишите программу}\nbegin end.\n")
        self.assertEqual(condition, "Напишите программу")
        with database(self.database_path) as db:
            task = db.execute("SELECT * FROM tasks WHERE id=?", (self.task_id,)).fetchone()
        self.assertTrue(make_task_file(task).startswith("{abc.pro-task: "))


if __name__ == "__main__":
    unittest.main()
