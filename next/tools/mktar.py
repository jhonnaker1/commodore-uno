#!/usr/bin/env python3
"""Packs a built UNO.app into a tar file for the release:

    python3 tools/mktar.py path/to/UNO.app uno-next.tar

UNO.app is built on the NeXT (see README.md); this runs on the host. The
archive holds UNO.app/ and UNO.app/UNO, in the old ustar format NeXTSTEP's
own tar reads, with fixed dates, owners and modes, so the same program
always packs into the same bytes. Unpack it on the NeXT with
`tar xf uno-next.tar`."""
import io
import os
import sys
import tarfile

app, out = sys.argv[1], sys.argv[2]
STAMP = 1795737600  # 2026-11-27 00:00 UTC, the day the port was finished

with open(os.path.join(app, "UNO"), "rb") as f:
    program = f.read()

buf = io.BytesIO()
with tarfile.open(fileobj=buf, mode="w", format=tarfile.USTAR_FORMAT) as tar:
    d = tarfile.TarInfo("UNO.app")
    d.type = tarfile.DIRTYPE
    d.mode = 0o755
    d.mtime = STAMP
    tar.addfile(d)
    f = tarfile.TarInfo("UNO.app/UNO")
    f.size = len(program)
    f.mode = 0o755
    f.mtime = STAMP
    tar.addfile(f, io.BytesIO(program))
with open(out, "wb") as f:
    f.write(buf.getvalue())
