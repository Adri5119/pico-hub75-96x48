#!/usr/bin/env python3
"""Compile the actual driver with test hardware, using g++/clang++ or Zig C++."""
import importlib.util
from pathlib import Path
import shutil
import subprocess
import sys

root = Path(__file__).resolve().parents[1]
out = root / ".pio" / "host-tests"
out.mkdir(parents=True, exist_ok=True)
compiler = shutil.which("g++") or shutil.which("clang++")
if compiler:
    command = [compiler]
elif importlib.util.find_spec("ziglang"):
    command = [sys.executable, "-m", "ziglang", "c++"]
else:
    sys.exit("Install g++/clang++, or: python -m pip install ziglang")
exe = out / ("driver-test.exe" if sys.platform == "win32" else "driver-test")
command += ["-std=c++17", "-Wall", "-Wextra", "-Werror", "-Itests/stubs", "-Isrc",
            "tests/driver_test.cpp", "src/Hub75_96x48.cpp", "-o", str(exe)]
subprocess.run(command, cwd=root, check=True)
subprocess.run([str(exe)], cwd=root, check=True)
