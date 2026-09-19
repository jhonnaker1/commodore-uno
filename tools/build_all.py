#!/usr/bin/env python3
"""Clean-build every port and every alternate target.

Slow (minutes) and needs the full toolchain set, so it is deliberately NOT
part of `make check` -- a gate nobody can afford to run is a gate nobody
runs. Targets are DERIVED from each port's own .PHONY line rather than
listed here, so an alternate build joins this sweep by existing.
"""
import pathlib
import re
import subprocess
import sys

ROOT = pathlib.Path(__file__).resolve().parent.parent

# Needs the user's purchased C64 OS and CMD HD ROMs, which cannot be
# assumed present. Skipped loudly, never silently.
NEEDS_PURCHASED_ROMS = {"c64os"}

# Targets that launch an emulator, need a ROM dump, or just tidy up.
SKIP = re.compile(r"^(run|clean|install|charset|all)$|^run-")


def targets_for(mk):
    phony = []
    for line in mk.read_text().splitlines():
        if line.startswith(".PHONY:"):
            phony += line.split(":", 1)[1].split()
    return [t for t in dict.fromkeys(phony) if t and not SKIP.match(t)]


MISSING_TOOL = re.compile(
    r"make(?:\[\d+\])?: (\S+): No such file or directory|"
    r"(\S+): command not found|"
    r"execvp: (\S+)")


def run(port, target):
    """Returns (status, output). status is 'ok', 'fail', or the name of a
    compiler that is not on PATH.

    That last case matters: a toolchain this machine has not got installed
    is not a defect in the code, and reporting it as one teaches people to
    ignore the gate. Six toolchains means somebody is always missing one."""
    r = subprocess.run(["make"] + ([target] if target else []),
                       cwd=ROOT / port, capture_output=True, text=True)
    out = r.stdout + r.stderr
    if r.returncode == 0:
        return "ok", out
    m = MISSING_TOOL.search(out)
    if m:
        return next(g for g in m.groups() if g), out
    return "fail", out


def main():
    ports = sorted(p.parent.name for p in ROOT.glob("*/Makefile"))
    failures = []
    skipped = []
    built = 0

    for port in ports:
        if port in NEEDS_PURCHASED_ROMS:
            print(f"{port:11s} SKIPPED -- needs purchased ROMs")
            continue
        subprocess.run(["make", "clean"], cwd=ROOT / port,
                       capture_output=True, text=True)
        for target in [None] + targets_for(ROOT / port / "Makefile"):
            label = f"{port}/{target or 'all'}"
            status, out = run(port, target)
            if status == "ok":
                built += 1
                print(f"  ok   {label}")
            elif status == "fail":
                print(f"  FAIL {label}")
                print("\n".join("        " + l for l in out.splitlines()[-4:]))
                failures.append(label)
            else:
                print(f"  --   {label}  (no {status} on PATH)")
                skipped.append(f"{label} [{status}]")

    print(f"\n{built} built, {len(failures)} failed, "
          f"{len(skipped)} skipped for a missing toolchain")
    if skipped:
        print("not built here: " + ", ".join(skipped))
    if failures:
        print("FAILED: " + ", ".join(failures))
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
