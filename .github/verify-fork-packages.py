#!/usr/bin/env python3
"""Reject a release containing mismatched engine/network revisions."""

import argparse
import json
from pathlib import Path


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("directory", type=Path)
    parser.add_argument("revision")
    args = parser.parse_args()
    common = None
    for platform in ("windows-x64", "windows-x86", "linux-x64", "macos-arm64"):
        metadata = json.loads((args.directory / f"build-{platform}.json").read_text(encoding="utf-8"))
        if metadata["platform"] != platform or metadata["revision_hash"] != args.revision or metadata["revision_modified"] != "0":
            raise RuntimeError(f"Incorrect source revision in {platform} package")
        identity = (metadata["revision"], metadata["revision_hash"], metadata["revision_tagged"])
        if common is not None and identity != common:
            raise RuntimeError(f"Mismatched engine/network revision in {platform} package")
        common = identity
    print(f"All four packages share clean engine revision: {common[0]}")


if __name__ == "__main__":
    main()
