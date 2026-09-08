#!/usr/bin/env python3
"""Bump the FastCast version everywhere it is spelled out.

The version lives in the macros at the top of fastcast.hpp; CMake, CPack, the
nuspec and the release workflow all read it from there.  Two more files repeat
it on purpose and are rewritten here so they cannot drift:

  tests/tests.cpp   the "Version macros" test case pins the expected numbers
  CHANGELOG.md      the "[Unreleased]" section becomes "[X.Y.Z] - <today>"

Usage:
    bump_version.py patch|minor|major     bump one component
    bump_version.py X.Y.Z                 set an explicit version
    bump_version.py --show                print the current version and exit
    add --dry-run to print the new version without touching any file
"""
import datetime
import pathlib
import re
import sys

ROOT = pathlib.Path(__file__).resolve().parent.parent
HEADER = ROOT / "fastcast.hpp"
TESTS = ROOT / "tests" / "tests.cpp"
CHANGELOG = ROOT / "CHANGELOG.md"
REPO_URL = "https://github.com/reach2sayan/FastCast"

def current():
    text = HEADER.read_text()
    parts = []
    for name in ("MAJOR", "MINOR", "PATCH"):
        m = re.search(rf"^#define FASTCAST_VERSION_{name} (\d+)$", text, re.M)
        if not m: sys.exit(f"FASTCAST_VERSION_{name} not found in {HEADER}")
        parts.append(int(m.group(1)))
    return tuple(parts)

def bump(cur, how):
    major, minor, patch = cur
    if how == "major": return (major + 1, 0, 0)
    if how == "minor": return (major, minor + 1, 0)
    if how == "patch": return (major, minor, patch + 1)
    m = re.fullmatch(r"(\d+)\.(\d+)\.(\d+)", how)
    if not m: sys.exit(f"expected patch|minor|major or X.Y.Z, got {how!r}")
    return tuple(int(x) for x in m.groups())

def sub_once(text, pattern, repl, where, flags=re.M):
    new, n = re.subn(pattern, repl, text, count=1, flags=flags)
    if n != 1: sys.exit(f"pattern not found in {where}: {pattern}")
    return new

def rewrite_header(new):
    text = HEADER.read_text()
    for name, value in zip(("MAJOR", "MINOR", "PATCH"), new):
        text = sub_once(text, rf"^(#define FASTCAST_VERSION_{name}) \d+$",
                        rf"\g<1> {value}", HEADER)
    text = sub_once(text, r'^(#define FASTCAST_VERSION_STRING) "\d+\.\d+\.\d+"$',
                    rf'\g<1> "{fmt(new)}"', HEADER)
    HEADER.write_text(text)

def rewrite_tests(new):
    text = TESTS.read_text()
    for name, value in zip(("MAJOR", "MINOR", "PATCH"), new):
        text = sub_once(text, rf"(CHECK\(FASTCAST_VERSION_{name} ==) \d+\);",
                        rf"\g<1> {value});", TESTS)
    numeric = new[0] * 10000 + new[1] * 100 + new[2]
    text = sub_once(text, r"(CHECK\(FASTCAST_VERSION ==) \d+\);",
                    rf"\g<1> {numeric});", TESTS)
    text = sub_once(text, r'(CHECK\(std::string\(FASTCAST_VERSION_STRING\) ==) "\d+\.\d+\.\d+"\);',
                    rf'\g<1> "{fmt(new)}");', TESTS)
    TESTS.write_text(text)

def rewrite_changelog(cur, new):
    text = CHANGELOG.read_text()
    today = datetime.date.today().isoformat()
    text = sub_once(text, r"^## \[Unreleased\]\n",
                    f"## [Unreleased]\n\n## [{fmt(new)}] - {today}\n", CHANGELOG)
    # Comparison links at the bottom.
    text = sub_once(text, r"^\[Unreleased\]: .*$",
                    f"[Unreleased]: {REPO_URL}/compare/v{fmt(new)}...HEAD\n"
                    f"[{fmt(new)}]: {REPO_URL}/compare/v{fmt(cur)}...v{fmt(new)}",
                    CHANGELOG)
    CHANGELOG.write_text(text)

def fmt(v): return ".".join(str(x) for x in v)

def main(argv):
    args = [a for a in argv if not a.startswith("--")]
    flags = {a for a in argv if a.startswith("--")}
    cur = current()
    if "--show" in flags:
        print(fmt(cur))
        return 0
    if len(args) != 1:
        print(__doc__, file=sys.stderr)
        return 2
    
    new = bump(cur, args[0])
    
    if new <= cur:
        sys.exit(f"new version {fmt(new)} is not greater than current {fmt(cur)}")
    print(fmt(new))
    if "--dry-run" in flags:
        return 0
    rewrite_header(new)
    rewrite_tests(new)
    rewrite_changelog(cur, new)
    return 0

if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
