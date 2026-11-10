"""Compile actual plugin sources and unchanged SDK, without Xcode build phases."""

import argparse
import os
from pathlib import Path
import subprocess
import tempfile


ROOT = Path(__file__).resolve().parents[2]
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument("--source-root", type=Path, default=ROOT)
parser.add_argument("cases", nargs="*", default=[
    "callbacks", "callback-results", "callback-errors", "path", "conversion", "arrays",
])
args = parser.parse_args()
source = args.source_root.resolve()
api = source / "4D Plugin API"

with tempfile.TemporaryDirectory(prefix="media-key-ownership-") as directory:
    executable = str(Path(directory) / "ownership")
    subprocess.run([
        "clang++", "-std=c++11", "-x", "objective-c++", "-fno-objc-arc",
        "-include", "Carbon/Carbon.h", "-include", "Cocoa/Cocoa.h",
        "-I", str(source), "-I", str(api), "-I", str(api / "Classes"),
        "-Wno-deprecated-declarations", "-Wno-writable-strings",
        "-g", "-O1", "-fno-omit-frame-pointer", "-fsanitize=address,undefined",
        str(ROOT / ".github/tests/ownership.mm"), str(api / "4DPluginAPI.c"),
        *map(str, sorted((api / "Classes").glob("*.cpp"))),
        "-framework", "Carbon", "-framework", "Cocoa", "-framework", "IOKit",
        "-o", executable,
    ], check=True)
    environment = dict(os.environ, ASAN_OPTIONS="detect_leaks=0:halt_on_error=1",
                       UBSAN_OPTIONS="halt_on_error=1:print_stacktrace=1")
    failed = []
    for case in args.cases:
        result = subprocess.run([executable, case], env=environment)
        if result.returncode:
            failed.append(case)
    if failed:
        raise SystemExit("FAILED: " + ", ".join(failed))
