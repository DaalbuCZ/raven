#!/usr/bin/env python3
"""Emit a minimal real resources.pri carrying string resources.

The layout is the published mrm_pri2 description that scripts/pri-extract.py
parses: the file header and table of contents (a 16-byte section identifier,
two 16-bit flags, a 32-bit qualifier, the 32-bit section offset and length),
the PRI descriptor section, the hierarchical schema (scope and item names), the
decision info section (the decision table that counts each item's candidates),
the resource map section (item info and candidates) and the data item section
(UTF-16 string storage). Only the subset a string lookup needs is written; the
fields the reference parser never reads (the qualifier-set region, the embedded
value types, the scope and item tables) are zeroed and noted.

Usage:
  python3 scripts/make_test_pri.py --out testdata/native_loader/test_resources.pri
"""
import argparse
import struct
from pathlib import Path

MAGIC = b"mrm_pri2"
SECTION_HEADER = 32
SECTION_FOOTER = 8
FOOTER_MAGIC = 0xDEF5FADE
HEADER_SIZE = 32
TOC_ENTRY_SIZE = 32

SCOPE_FLAG = 0x10
CANDIDATE_SECTIONED = 1  # the candidate's data lives in a data item section

# the fixture's resources: names are schema paths without the leading slash
RESOURCES = {
    "Fixture/Greeting": "Hello from the PRI",
    "Fixture/Farewell": "Goodbye",
}
SCHEMA_UNIQUE = "Raven.Fixture"


def section(identifier, body):
    """One section: the unread 32-byte header, the body, the footer magic."""
    return (struct.pack(f"<{SECTION_HEADER}s", identifier.encode("ascii"))
            + body
            + struct.pack("<II", FOOTER_MAGIC, 0))


def descriptor(resmap_index, schema_index, decision_index, data_index):
    """The PRI descriptor: the section index lists the resource map, its schema,
    its decision info and its data item sections resolve through. `primary`
    names the resource map a reader starts from."""
    body = struct.pack("<HHHHHHHHHH",
                       0,      # flags, unread
                       0,      # included file count, unread
                       0,      # unread
                       1,      # hierarchical schema index count
                       1,      # decision info index count
                       1,      # resource map index count
                       resmap_index,
                       0,      # referenced file index count
                       1,      # data item index count
                       0)      # unread
    body += struct.pack("<HHHH", schema_index, decision_index, resmap_index, data_index)
    return body


def schema(resources):
    """The hierarchical schema, in the non-extended form: one name table per
    entry, scopes flagged, items carrying their item info index. The scope and
    item tables behind the entries are unread by the reference parser and
    zeroed."""
    entries = [{"name": "", "flags": SCOPE_FLAG, "parent": 0, "index": 0, "depth": 0}]
    for name in resources:
        parts = name.split("/")
        for depth, part in enumerate(parts, start=1):
            path = "/".join(parts[:depth])
            scope = SCOPE_FLAG if depth < len(parts) else 0
            if not any(entry["name"] == part and entry["depth"] == depth
                       for entry in entries if "depth" in entry):
                entries.append({"name": part, "flags": scope, "depth": depth,
                                "parent": 0, "index": 0})
    # parents: each entry's parent is the entry holding its path prefix
    for entry in entries[1:]:
        prefix_depth = entry["depth"] - 1
        entry["parent"] = next(i for i, other in enumerate(entries)
                               if other["depth"] == prefix_depth)
    # full paths, for the path length field the reference parser skips
    for entry in entries:
        parts = []
        current = entry
        for _ in range(64):
            parts.append(current["name"])
            current = entries[current["parent"]]
            if current is entries[0]:
                if current is not entry:
                    parts.append(current["name"])
                break
        entry["path"] = "/".join(reversed(parts))
    # items get their item info index in resource order
    for position, name in enumerate(resources):
        entry = next(e for e in entries if "depth" in e and e["path"] == "/" + name)
        entry["index"] = position
    # the unicode name block: every name nul-terminated and padded so its offset
    # stays a whole count of 4-byte units — the offset field counts 4-byte units,
    # which the reference parser doubles into a character index
    block = ""
    for entry in entries:
        entry["offset"] = len(block) // 2
        block += entry["name"] + "\0"
        if len(block) % 2:
            block += "\0"
    longest = max(len(entry["path"]) for entry in entries)

    body = struct.pack("<HHHH",
                       0,                            # unread
                       len(SCHEMA_UNIQUE),           # unique resource id length
                       0,                            # schema name length
                       0)                            # unread
    body += struct.pack("<20s", b"")                 # HSCHEMA_VERSION_INFO; the extended
                                                     # [def_hnames identifier would share
                                                     # these bytes and is not written
    body += SCHEMA_UNIQUE.encode("utf-16le")
    body += struct.pack("<HHH", 0, longest, 0)       # unread, longest path, unread
    body += struct.pack("<IIIII",
                        len(entries),                # resource count (all entries)
                        sum(1 for e in entries if e["flags"] & SCOPE_FLAG),
                        sum(1 for e in entries if not e["flags"] & SCOPE_FLAG),
                        len(block),                  # unicode name units
                        len(block) + len(SCHEMA_UNIQUE))
    for entry in entries:
        body += struct.pack("<HHHBBHH",
                            entry["parent"],
                            len(entry["path"]),      # path length, unread
                            0xFFFF,                  # first child, unread
                            len(entry["name"]),
                            entry["flags"],
                            entry["offset"],
                            entry["index"])
    body += struct.pack("<8s", b"") * sum(1 for e in entries if e["flags"] & SCOPE_FLAG)
    body += struct.pack("<2s", b"") * sum(1 for e in entries if not e["flags"] & SCOPE_FLAG)
    body += block.encode("utf-16le")
    return body


def decisions(resources):
    """The decision info: one decision per item, each selecting one candidate
    set of one candidate. The qualifier and value regions behind the table are
    unread by the reference parser and zeroed."""
    count = len(resources)
    body = struct.pack("<HHHHHH",
                       1,        # distinct qualifiers
                       0,        # qualifier region length, unread
                       count,    # candidate set count
                       count,    # decision entry count
                       0,        # index count, unread
                       0)        # value region length, unread
    for index in range(count):
        body += struct.pack("<HH", index, 1)  # first set, set count
    return body


def resource_map(resources, schema_index, decision_index, data_index):
    """The resource map: item info (decision and first candidate per item) and
    the candidates themselves, each pointing at its data item section. The
    environment strings, the embedded schema copy, the value type table, the
    item groups and groups are absent; the extension region is empty."""
    count = len(resources)
    body = struct.pack("<HHHHHHHHIIII",
                       0,             # environment length
                       0,             # environment count
                       schema_index,  # the hierarchical schema section
                       0,             # embedded schema length
                       decision_index,
                       0,             # value type count
                       0,             # item group count
                       0,             # group count
                       count,         # item info count
                       count,         # candidate count
                       0,             # embedded data length
                       0)             # extension length
    for index in range(count):
        body += struct.pack("<HH", index, index)  # decision, first candidate
    for index in range(count):
        body += struct.pack("<BBHHH",
                            CANDIDATE_SECTIONED,
                            4,                    # value type, unread by the reference parser
                            0,                    # referenced file source
                            index,                # data item string index
                            data_index)           # data item section (table of contents index)
    return body


def data_items(resources):
    """The data item section: a string table of (offset, length) pairs — the
    offset counts UTF-16 units, the reference parser doubles it, while the
    length already counts bytes — then the string data. No blob storage; the
    strings are nul-terminated with the terminator outside the counted length."""
    strings = list(resources.values())
    body = struct.pack("<IHHI", 0, len(strings), 0,
                       sum(len(value) + 1 for value in strings))
    offset = 0
    for value in strings:
        body += struct.pack("<HH", offset, len(value) * 2)
        offset += len(value) + 1
    for value in strings:
        body += (value + "\0").encode("utf-16le")
    return body


def build(resources):
    """The whole file: header, table of contents, then the sections packed
    back to back. Section offsets are relative to where the section data
    starts."""
    bodies = [
        ("[mrm_pridescex]", None),
        ("[mrm_hschema0]", schema(resources)),
        ("[mrm_decinfo0]", decisions(resources)),
        ("[mrm_resmap0]", None),
        ("[mrm_data0]", data_items(resources)),
    ]
    schema_index = 1
    decision_index = 2
    resmap_index = 3
    data_index = 4
    bodies[0] = (bodies[0][0],
                 descriptor(resmap_index, schema_index, decision_index, data_index))
    bodies[3] = (bodies[3][0],
                 resource_map(resources, schema_index, decision_index, data_index))
    sections = [(identifier, section(identifier, body)) for identifier, body in bodies]
    first_section = HEADER_SIZE + len(sections) * TOC_ENTRY_SIZE
    toc = b""
    offset = 0
    for identifier, packed in sections:
        toc += struct.pack("<16sHHIII",
                           identifier.encode("ascii"),
                           0, 0, 0, offset, len(packed))
        offset += len(packed)
    header = MAGIC
    header += struct.pack("<I", 0)                 # unread, before the counted fields
    header += struct.pack("<IIIHHH",
                          first_section + offset,  # total size
                          HEADER_SIZE,             # table of contents offset
                          first_section,           # where section offsets start
                          len(sections),
                          0, 0)                    # reserved
    header += struct.pack("<2s", b"")              # pad to 32 bytes
    return header + toc + b"".join(packed for _, packed in sections)


def main():
    parser = argparse.ArgumentParser(description="Emit a minimal real resources.pri")
    parser.add_argument("--out", type=Path, required=True)
    args = parser.parse_args()
    args.out.write_bytes(build(RESOURCES))
    print(f"{args.out}: {len(RESOURCES)} string resources")


if __name__ == "__main__":
    raise SystemExit(main())
