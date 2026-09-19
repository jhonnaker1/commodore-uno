#!/usr/bin/env python3
"""Enforce the invariants the README advertises.

The headline claim is that the game logic is shared verbatim across the C
ports. That was true every time it was checked by hand -- but nothing kept
it true, and a one-line edit in one port forks the logic silently. This is
the thing that notices.

The port list is DISCOVERED from the filesystem, never hardcoded: a new
port joins the check by existing, so the gate cannot quietly cover fewer
ports than the repo has. Run from the repository root.
"""
import hashlib
import pathlib
import sys

ROOT = pathlib.Path(__file__).resolve().parent.parent

# Files that must be byte-identical in every C port.
IDENTICAL = ["cards.c", "game.c", "ai.c", "ai.h"]

# Files allowed to differ, and the only ports allowed to differ in them.
# Both exceptions are forced by the 68000's alignment rules and are
# described in the main README; anything else differing is a fork.
ALLOWED = {
    "cards.h": {"amiga", "ste"},   # `unsigned short _pad` for 2-byte alignment
    "game.h": {"ste"},             # GameState reordered to keep Cards even
}


def sha(p):
    return hashlib.sha256(p.read_bytes()).hexdigest()


def main():
    ports = sorted(p.parent.parent.name for p in ROOT.glob("*/src/cards.c"))
    if not ports:
        print("FAIL: no ports discovered -- run from the repository root")
        return 1
    print(f"discovered {len(ports)} C ports: {' '.join(ports)}")

    failures = []
    for name in IDENTICAL + list(ALLOWED):
        groups = {}
        for port in ports:
            f = ROOT / port / "src" / name
            if not f.exists():
                failures.append(f"{name}: missing in {port}")
                continue
            groups.setdefault(sha(f), []).append(port)

        majority = max(groups.values(), key=len)
        outliers = sorted(p for g in groups.values() if g is not majority for p in g)
        expected = sorted(ALLOWED.get(name, set()))

        if outliers == expected:
            note = f"({len(majority)} identical" + (
                f", {'/'.join(outliers)} differ as documented)" if outliers else ")")
            print(f"  ok   {name:10s} {note}")
        else:
            failures.append(
                f"{name}: differs in {outliers or 'nothing'}, expected {expected or 'nothing'}")
            print(f"  FAIL {name:10s} differs in {outliers}, expected {expected}")

    # Hardcoded developer paths: the repo moved to $(HOME)/env overrides and
    # should stay there. A literal /Users/<somebody> is a regression.
    stray = []
    for mk in sorted(ROOT.glob("*/Makefile")):
        for i, line in enumerate(mk.read_text().splitlines(), 1):
            if "/Users/" in line and not line.lstrip().startswith("#"):
                stray.append(f"{mk.relative_to(ROOT)}:{i}: {line.strip()}")
    if stray:
        failures.extend(stray)
        print("  FAIL hardcoded developer path in a Makefile:")
        for s in stray:
            print(f"       {s}")
    else:
        print(f"  ok   Makefiles  (no hardcoded /Users/ paths in {len(list(ROOT.glob('*/Makefile')))})")

    if failures:
        print(f"\n{len(failures)} failure(s)")
        return 1
    print("\nall invariants hold")
    return 0


if __name__ == "__main__":
    sys.exit(main())
