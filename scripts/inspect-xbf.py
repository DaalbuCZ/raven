#!/usr/bin/env python3
"""Inspect compiled XAML (XBF) blobs in a resources.pri or a raw .xbf file.

The XBF container is found by its signature rather than through the resource
index: a PRI stores the compiled pages as separate blobs, and the signature
scan gives their byte ranges. Each blob's header is reported (version, the two
section sizes, the header size) and the UTF-16 string table is decoded, which is
where type, property and resource names live. The node stream that follows is
not decoded yet.

Usage:
  python3 scripts/inspect-xbf.py [--limit N] [--strings N] PATH
"""
import argparse
import re
import struct
from pathlib import Path

SIGNATURE = b"XBF\x00"
UTF16_RUN = re.compile(rb"(?:[\x20-\x7e]\x00){3,}")


def blobs(data):
    positions = [match.start() for match in re.finditer(re.escape(SIGNATURE), data)]
    for index, start in enumerate(positions):
        end = positions[index + 1] if index + 1 < len(positions) else len(data)
        yield index, start, data[start:end]


def header(blob):
    if len(blob) < 24:
        return None
    magic, first, second, version, flags, head = struct.unpack_from("<IIIIII", blob, 0)
    return {"magic": magic, "first": first, "second": second, "version": version, "flags": flags, "head": head}


def strings(blob):
    # The string table follows the fixed header; the first size in the header covers it in the blobs observed so far.
    start = 0
    return [match.group().decode("utf-16le", "replace") for match in UTF16_RUN.finditer(blob[start:])]


def main():
    parser = argparse.ArgumentParser(description="Inspect XBF blobs in a PRI or a compiled XAML file")
    parser.add_argument("path", type=Path)
    parser.add_argument("--limit", type=int, default=0, help="inspect only the first N blobs")
    parser.add_argument("--strings", type=int, default=0, help="print the first N decoded strings of each blob")
    args = parser.parse_args()
    data = args.path.read_bytes()
    found = 0
    versions = {}
    for index, start, blob in blobs(data):
        if args.limit and found >= args.limit:
            break
        found += 1
        info = header(blob)
        if not info:
            print(f"{index}: {start:#x} too short")
            continue
        versions[info["version"]] = versions.get(info["version"], 0) + 1
        names = strings(blob)
        print(f"{index}: {start:#x} size {len(blob)} version {info['version']} sections {info['first']}+{info['second']} "
              f"head {info['head']} strings {len(names)}")
        for name in names[:args.strings]:
            print(f"    {name}")
    print(f"{found} XBF blob(s); versions {versions}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
