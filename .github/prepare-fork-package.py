#!/usr/bin/env python3
"""Bundle the tested OpenGFX set, then package a fork build with CPack."""

import argparse
import hashlib
from pathlib import Path
import subprocess
import tempfile
import zipfile


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("platform", choices=("windows-x64", "windows-x86", "linux-x64", "macos-arm64"))
    parser.add_argument("--build", type=Path, default=Path("build"))
    args = parser.parse_args()
    with tempfile.TemporaryDirectory(prefix="openttd-graphics-") as temporary:
        download = Path(temporary) / "opengfx.zip"
        subprocess.run(["curl", "--fail", "--location", "--retry", "3", "--max-time", "120",
                        "https://cdn.openttd.org/opengfx-releases/8.0/opengfx-8.0-all.zip",
                        "--output", str(download)], check=True)
        with zipfile.ZipFile(download) as archive:
            data = archive.read("opengfx-8.0.tar")
    if hashlib.sha256(data).hexdigest() != "9389bcb0807058c80bd95121e978f05d9ef86b4b1bc3ac2da8da8bb02456043c":
        raise RuntimeError("OpenGFX archive does not match the tested version")
    baseset = args.build / "baseset"
    baseset.mkdir(parents=True, exist_ok=True)
    (baseset / "opengfx-8.0.tar").write_bytes(data)
    generator, extension = {"windows": ("ZIP", ".zip"), "linux": ("DEB", ".deb"), "macos": ("Bundle", ".dmg")}[args.platform.split("-")[0]]
    subprocess.run(["cpack", "-G", generator, "-C", "Release"], cwd=args.build, check=True)
    packages = list((args.build / "bundles").glob("*" + extension))
    if len(packages) != 1:
        raise RuntimeError(f"Expected one package, found {packages}")
    output = Path("fork-packages")
    output.mkdir(exist_ok=True)
    (output / f"smooth-zoom-{args.platform}{extension}").write_bytes(packages[0].read_bytes())


if __name__ == "__main__":
    main()
