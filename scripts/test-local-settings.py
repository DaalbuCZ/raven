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
        run("nested-write")
        container_root = app / "LocalSettings" / "containers"
        manifests = list(container_root.glob("*/container.rvc"))
        child = next(path for path in manifests if "fixture-child".encode("utf-16le") in path.read_bytes())
        grand = next(path for path in manifests if "fixture-grandchild".encode("utf-16le") in path.read_bytes())

        def snapshot():
            settings = app / "LocalSettings"
            return {path.relative_to(settings): path.read_bytes() for path in settings.rglob("*") if path.is_file()}

        foreign = child.parent / "foreign.bin"
        foreign.write_bytes(b"not a Raven settings record")
        before = snapshot()
        run("nested-guard")
        if snapshot() != before:
            raise RuntimeError("deleting a container with an unknown file changed the settings store")
        foreign.unlink()

        outside_dir = root / "outside"
        outside_dir.mkdir()
        sentinel = outside_dir / "sentinel"
        sentinel.write_bytes(b"outside data")
        foreign_dir = child.parent / "foreign-dir"
        foreign_dir.symlink_to(outside_dir, target_is_directory=True)
        before = snapshot()
        run("nested-guard")
        if snapshot() != before or not foreign_dir.is_symlink() or sentinel.read_bytes() != b"outside data":
            raise RuntimeError("deleting a container followed or removed an unknown directory link")
        foreign_dir.unlink()

        value_path = next(child.parent.glob("*.rvs"))
        value = value_path.read_bytes()
        external_value = outside_dir / "value.rvs"
        external_value.write_bytes(value)
        value_path.unlink()
        value_path.symlink_to(external_value)
        before = snapshot()
        run("nested-guard")
        if snapshot() != before or not value_path.is_symlink() or external_value.read_bytes() != value:
            raise RuntimeError("deleting a container followed or removed a linked settings record")
        value_path.unlink()
        value_path.write_bytes(value)

        manifest = grand.read_bytes()
        for field, offset in (("magic", 0), ("depth", 4), ("parent", 16)):
            damaged = bytearray(manifest)
            damaged[offset] ^= 0x80
            grand.write_bytes(damaged)
            before = snapshot()
            run("nested-guard")
            if snapshot() != before:
                raise RuntimeError(f"deleting a container with damaged {field} changed the settings store")
            grand.write_bytes(manifest)

        manifest = child.read_bytes()
        collision = bytearray(manifest)
        name_offset = collision.find("fixture-child".encode("utf-16le"))
        if name_offset < 0:
            raise RuntimeError("child metadata does not contain its expected name")
        collision[name_offset] ^= 1
        child.write_bytes(collision)
        before = snapshot()
        run("nested-collision")
        if snapshot() != before:
            raise RuntimeError("opening a container with mismatched identity changed the settings store")
        child.write_bytes(manifest)
        run("nested-read")
        run("nested-verify")
        run("nested-weak")
        run("nested-aba")
        run("nested-depth")
    print("LocalSettings persisted and removed across guest processes")


if __name__ == "__main__":
    main()
