"""Run local source-description checks and the complete upstream CTest suite.

Usage: python docs/validation/run-upstream-checks.py --build-dir <build>
Requires a configured build, CMake on PATH (or --cmake), and Python 3.
Compiler/platform CI and the GNU C++ unused-string checker remain separate.
"""

import argparse
import os
from pathlib import Path
import subprocess
import sys
import tempfile


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--build-dir", type=Path, required=True)
    parser.add_argument("--config", default="Release")
    parser.add_argument("--cmake", default="cmake")
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[2]
    build = args.build_dir.resolve()
    env = dict(os.environ, PYTHONUTF8="1")

    def run(command):
        print("Running:", subprocess.list2cmdline(list(map(str, command))), flush=True)
        subprocess.run(list(map(str, command)), cwd=root, env=env, check=True)

    # Explicit forward-slash paths match the Linux CI checker's filename rules.
    with tempfile.TemporaryDirectory(prefix="openttd-source-check-") as temporary:
        file_list = Path(temporary) / "sources.txt"
        file_list.write_text("".join(path.relative_to(root).as_posix() + "\n"
                                     for path in sorted((root / "src").rglob("*"))
                                     if path.is_file()), encoding="utf-8")
        run([sys.executable, ".github/file-descriptions.py", file_list])

    # CTest does not build or stage dependencies itself.
    run([args.cmake, "--build", build, "--config", args.config])
    ctest = Path(args.cmake).with_name("ctest.exe" if os.name == "nt" else "ctest")
    run([ctest, "--test-dir", build, "-C", args.config,
         "--output-on-failure", "--timeout", "120"])


if __name__ == "__main__":
    main()
