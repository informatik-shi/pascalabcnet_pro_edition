"""Run a submitted Pascal program in a disposable, resource-limited container."""

from __future__ import annotations

import os
import subprocess
import tempfile
import time
import uuid
from pathlib import Path


class JudgeUnavailable(RuntimeError):
    pass


def _normalized(text: str) -> str:
    return text.replace("\r\n", "\n").replace("\r", "\n").rstrip("\n")


def _work_size(work: Path, limit: int) -> int:
    size = 0
    for path in work.rglob("*"):
        try:
            if path.is_file():
                size += path.stat().st_size
        except OSError:
            continue
        if size > limit:
            break
    return size


class DockerJudge:
    def __init__(self, image: str = "abc-pro-judge:latest") -> None:
        self.image = image

    def _run(self, work: Path, args: list[str], *, input_data: bytes = b"",
             timeout: float = 20, output_limit: int = 65536) -> tuple[int, str]:
        name = "abc-pro-judge-" + uuid.uuid4().hex[:16]
        mount = f"type=bind,src={work.resolve()},dst=/work"
        command = [
            "docker", "run", "--rm", "--name", name, "--network", "none",
            "--memory", "256m", "--cpus", "1", "--pids-limit", "64",
            "--ulimit", "fsize=67108864:67108864",
            "--cap-drop", "ALL", "--security-opt", "no-new-privileges",
            "--read-only", "--tmpfs", "/tmp:rw,nosuid,size=64m",
            "--mount", mount, "--workdir", "/work", self.image, *args,
        ]
        with tempfile.TemporaryFile() as stream_in, tempfile.TemporaryFile() as stream_out:
            stream_in.write(input_data)
            stream_in.seek(0)
            try:
                process = subprocess.Popen(command, stdin=stream_in, stdout=stream_out,
                                           stderr=subprocess.STDOUT)
            except OSError as exc:
                raise JudgeUnavailable(f"Docker недоступен: {exc}") from exc
            deadline = time.monotonic() + timeout
            interrupted = False
            try:
                while process.poll() is None:
                    work_size = _work_size(work, 64 * 1024 * 1024)
                    if (time.monotonic() > deadline or
                            os.fstat(stream_out.fileno()).st_size > output_limit or
                            work_size > 64 * 1024 * 1024):
                        interrupted = True
                        process.kill()
                        break
                    time.sleep(0.05)
                process.wait(timeout=2)
            finally:
                if interrupted:
                    subprocess.run(["docker", "rm", "-f", name],
                                   stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL,
                                   timeout=5, check=False)
            if interrupted:
                return 124, "Превышен лимит времени, вывода или диска"
            stream_out.seek(0)
            output = stream_out.read(output_limit + 1).decode("utf-8", "replace")
            if len(output) > output_limit:
                return 124, "Превышен лимит объёма вывода"
            if process.returncode in (125, 126, 127):
                raise JudgeUnavailable("Docker не запустил проверку: " + output[:1000])
            return process.returncode, output

    def judge(self, source: str, tests: list[dict]) -> tuple[str, str]:
        if not tests:
            return "error", "Для задания не добавлены серверные тесты"
        with tempfile.TemporaryDirectory(prefix="abc-pro-judge-") as directory:
            work = Path(directory)
            (work / "solution.pas").write_text(source, encoding="utf-8")
            try:
                code, output = self._run(
                    work, ["dotnet", "/opt/abc/bin-net10/pabcnetcclear.dll",
                           "/Output:/work/solution.exe", "/work/solution.pas"],
                    timeout=30,
                )
                if code != 0:
                    return "wrong", "Ошибка компиляции или превышен лимит: " + output[:1000]
                if not (work / "solution.exe").exists():
                    return "error", "Компилятор не создал исполняемый файл"
                for number, test in enumerate(tests, 1):
                    code, output = self._run(
                        work, ["dotnet", "exec", "/work/solution.exe"],
                        input_data=test["input_text"].encode("utf-8"), timeout=5,
                    )
                    if code != 0 or _normalized(output) != _normalized(test["expected_text"]):
                        return "wrong", f"Не пройден серверный тест {number}"
            except (OSError, subprocess.SubprocessError) as exc:
                raise JudgeUnavailable(f"Не удалось выполнить Docker-проверку: {exc}") from exc
        return "solved", "Все серверные тесты пройдены"
