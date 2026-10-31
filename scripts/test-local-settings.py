#!/usr/bin/env python3
"""Check settings across separate guest processes in an isolated prefix and application directory."""
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile


def main():
    raven = Path(sys.argv[1]).resolve()
    fixture = Path(sys.argv[2]).resolve()
    with tempfile.TemporaryDirectory(prefix="raven-settings-test-") as temporary:
        root = Path(temporary)
        prefix = root / "prefix"
        app = prefix / "drive_c" / "settings-fixture"
        app.mkdir(parents=True)
        target = app / fixture.name
        shutil.copy2(fixture, target)
        def run(mode):
            env = os.environ.copy()
            env["RAVEN_PREFIX"] = str(prefix)
            result = subprocess.run(
                [str(raven), "--run-process", "--execution=native", "--timeout-ms=15000", str(target), "--", mode],
                env=env, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True, timeout=30,
            )
            if result.returncode or "Native entry returned 0 " not in result.stdout:
                raise RuntimeError(f"{mode} process failed ({result.returncode}):\n{result.stdout}")
        run("write")
        records = list((app / "LocalSettings").glob("*.rvs"))
        record = next(path for path in records if "fixture-integer".encode("utf-16le") in path.read_bytes())
        original = record.read_bytes()
        corrupted = b"BAD!" + original[4:]
        record.write_bytes(corrupted)
        run("bad")
        if record.read_bytes() != corrupted:
            raise RuntimeError("a failed insert modified a corrupt settings record")
        record.write_bytes(original)
        run("read")
        run("verify")
    print("LocalSettings persisted and removed across guest processes")


if __name__ == "__main__":
    main()
