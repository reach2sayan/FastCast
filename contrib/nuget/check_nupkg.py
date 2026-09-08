#!/usr/bin/env python3
"""Check that a FastCast .nupkg has the layout consumers rely on.

Usage: check_nupkg.py <package.nupkg>
Lists the archive and exits non-zero if the header or the .targets is missing.
"""
import sys
import zipfile

REQUIRED = ("build/native/FastCast.targets", "build/native/include/fastcast.hpp")


def main() -> int:
    if len(sys.argv) != 2:
        print(__doc__, file=sys.stderr)
        return 2
    names = zipfile.ZipFile(sys.argv[1]).namelist()
    print(*names, sep="\n")
    missing = [n for n in REQUIRED if n not in names]
    for n in missing:
        print(f"missing: {n}", file=sys.stderr)
    return 1 if missing else 0


if __name__ == "__main__":
    sys.exit(main())
