"""Optional differential check against an existing PascalABC.NET compiler.

Only the compatibility source is compiled; no .NET distribution rebuild is run.
Neither Python nor PascalABC.NET is a dependency of the shipped native application.
"""
import argparse
import os
from pathlib import Path
import shutil
import subprocess
import tempfile


def run(command, **kwargs):
    result = subprocess.run(command, capture_output=True, timeout=60, **kwargs)
    if result.returncode:
        raise RuntimeError(f"{command[0]} failed ({result.returncode}): {result.stdout!r} {result.stderr!r}")
    return result.stdout.decode("utf-8-sig").replace("\r\n", "\n")


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--native", type=Path, required=True)
    parser.add_argument("--reference-compiler", type=Path, required=True)
    parser.add_argument("--dotnet-root", type=Path, help="Optional existing portable .NET runtime for the reference compiler")
    args = parser.parse_args()
    fixture = Path(__file__).parent / "fixtures" / "stdlib-compat.pas"
    expected = fixture.with_suffix(".out").read_text(encoding="utf-8")
    env = os.environ.copy()
    if args.dotnet_root:
        env["DOTNET_ROOT"] = str(args.dotnet_root.resolve())
        env["DOTNET_ROOT_X64"] = str(args.dotnet_root.resolve())
    with tempfile.TemporaryDirectory(prefix="ABC Native compatibility ") as temporary:
        source = Path(temporary) / fixture.name
        shutil.copyfile(fixture, source)
        executable = Path(temporary) / "reference.exe"
        compiled = subprocess.run([str(args.reference_compiler.resolve()),
                                   f"/Output:{executable}", "/Debug:0", str(source)],
                                  capture_output=True, timeout=120, env=env)
        if compiled.returncode or not executable.exists():
            raise RuntimeError(f"Reference compile failed: {compiled.stdout!r} {compiled.stderr!r}")
        if executable.with_suffix(".runtimeconfig.json").exists():
            runtime = (args.dotnet_root.resolve() / ("dotnet.exe" if os.name == "nt" else "dotnet")) if args.dotnet_root else shutil.which("dotnet")
            if not runtime:
                raise RuntimeError("The reference program needs .NET; supply --dotnet-root or add dotnet to PATH")
            reference_command = [str(runtime), "exec", str(executable)]
        else:
            reference_command = [str(executable)]
        reference = run(reference_command, cwd=temporary, env=env)
        native = run([str(args.native.resolve()), "run", str(source)], cwd=temporary)
        if native != expected or reference != expected:
            raise AssertionError(f"Output differs:\nExpected:\n{expected}\nReference:\n{reference}\nNative:\n{native}")
        print("PABCSystem compatibility: native output matches PascalABC.NET and the expected corpus")


if __name__ == "__main__":
    main()
