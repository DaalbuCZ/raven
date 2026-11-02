#!/usr/bin/env python3
"""Resolve a resource name in a Package Resource Index to its data and extract it.

The layout follows the published description of the PRI format (mrm_pri2):
header and table of contents, the PRI descriptor section, the hierarchical
schema (names), the decision info section (candidate counts), the resource map
section (item info and candidates) and the data item sections (string and blob
storage). A resource name is matched by its item name, its full path is built
from the parent scopes, and the first candidate of its decision selects the
data item.

Usage:
  python3 scripts/pri-extract.py resources.pri --name App.xbf [--out file]
  python3 scripts/pri-extract.py resources.pri --list
"""
import argparse
import struct
import sys
from pathlib import Path

SECTION_HEADER = 32
SECTION_FOOTER = 8
FOOTER_MAGIC = 0xDEF5FADE


class Pri:
    def __init__(self, data):
        self.data = data
        if data[0:8] != b"mrm_pri2":
            raise SystemExit(f"unsupported PRI version identifier {data[0:8]!r}")
        self.total_size, self.toc_offset, self.first_section, self.sections, _, _ = struct.unpack_from("<IIIHHH", data, 12)
        self.entries = []
        self.first_of = {}
        for index in range(self.sections):
            at = self.toc_offset + index * 32
            identifier = data[at:at + 16].split(b"\x00", 1)[0].decode("ascii", "replace")
            flags, section_flags, qualifier, offset, length = struct.unpack_from("<HHIII", data, at + 16)
            self.entries.append((identifier, self.first_section + offset, length))
            self.first_of.setdefault(identifier, index)

    def body(self, index):
        identifier, start, length = self.entries[index]
        footer = struct.unpack_from("<I", self.data, start + length - SECTION_FOOTER)
        if footer[0] != FOOTER_MAGIC:
            raise SystemExit(f"section {index} ({identifier}) lacks its footer")
        return self.data[start + SECTION_HEADER:start + length - SECTION_FOOTER]


def descriptor(pri):
    body = pri.body(pri.first_of["[mrm_pridescex]"])
    (flags, included, _, hschemas, decisions, resmaps, primary, reffiles, dataitems, _) = struct.unpack_from("<HHHHHHHHHH", body, 0)
    at = 20
    def indices(count):
        nonlocal at
        values = list(struct.unpack_from(f"<{count}H", body, at)) if count else []
        at += count * 2
        return values
    return {
        "hschemas": indices(hschemas),
        "decisions": indices(decisions),
        "resmaps": indices(resmaps),
        "primary": primary,
        "reffiles": indices(reffiles),
        "dataitems": indices(dataitems),
    }


def wide(pri, section_index):
    return pri.body(section_index)


def schema(pri, section_index):
    body = wide(pri, section_index)
    at = 0
    _, unique_length, name_length, _ = struct.unpack_from("<HHHH", body, at)
    at += 8
    extended = body[at:at + 16].startswith(b"[def_hnames")
    if extended:
        hname = body[at:at + 16].split(b"\x00", 1)[0].decode("ascii", "replace").rstrip()
        at += 16
    else:
        hname = ""
    at += 20  # HSCHEMA_VERSION_INFO
    unique = body[at:at + unique_length * 2].decode("utf-16le", "replace").rstrip("\x00")
    at += unique_length * 2
    name = body[at:at + name_length * 2].decode("utf-16le", "replace").rstrip("\x00")
    at += name_length * 2
    at += 6  # unknown, longest path, unknown
    resource_count, scope_count, item_count, unicode_length, total_length = struct.unpack_from("<IIIII", body, at)
    if resource_count > 200000 or scope_count > 200000 or item_count > 200000 or unicode_length > 1000000:
        raise SystemExit(f"hierarchical schema looks implausible: {resource_count} names, {scope_count} scopes, "
                         f"{item_count} items, {unicode_length} name units")
    at += 20
    ascii_length = 0
    if extended and hname.startswith("[def_hnamesx]"):
        ascii_length, = struct.unpack_from("<I", body, at)
        at += 4
    entries = []
    for _ in range(resource_count):
        parent, path_length, first, name_len, flags, offset, index_property = struct.unpack_from("<HHHBBHH", body, at)
        at += 12
        # the 16-bit offset field counts into a name block that can exceed 64 Ki
        # units; the entry flags' low two bits carry the offset's bits 16-17
        offset |= (flags & 3) << 16
        entries.append({"parent": parent, "path_length": path_length, "first": first, "name_len": name_len,
                        "flags": flags, "offset": offset, "index": index_property})
    at += scope_count * 8 + item_count * 2
    unicode_block = body[at:at + unicode_length * 2].decode("utf-16le", "replace")
    at += unicode_length * 2
    ascii_block = body[at:at + ascii_length].decode("ascii", "replace") if ascii_length else ""
    for entry in entries:
        if entry["flags"] & 0x20:
            text = ascii_block[entry["offset"]:entry["offset"] + (entry["name_len"] or 1024)]
            text = text.split("\x00", 1)[0]
        else:
            start = entry["offset"] * 2
            text = unicode_block[start:start + (entry["name_len"] or 1024)]
            text = text.split("\x00", 1)[0]
        entry["name"] = text
    for entry in entries:
        parts = []
        current = entry
        seen = set()
        while current is not None and len(parts) < 64 and id(current) not in seen:
            seen.add(id(current))
            parts.append(current["name"])
            parent = current["parent"]
            current = entries[parent] if 0 <= parent < len(entries) and entries[parent] is not current else None
        entry["path"] = "/".join(reversed(parts))
    return {"unique": unique, "name": name, "extended": extended, "hname": hname, "entries": entries}


def decisions(pri, section_index):
    body = pri.body(section_index)
    distinct, qualifiers, sets, count, index_count, value_length = struct.unpack_from("<HHHHHH", body, 0)
    at = 12
    table = [struct.unpack_from("<HH", body, at + index * 4) for index in range(count)]
    return {"entries": table, "distinct": distinct, "qualifiers": qualifiers, "sets": sets, "values": value_length}


def resource_map(pri, section_index):
    body = pri.body(section_index)
    (env_length, env_count, hschema_index, hschema_length, decision_index, value_types, item_groups, groups,
     iteminfo_count, candidate_count, embedded_length, extension_length) = struct.unpack_from("<HHHHHHHHIIII", body, 0)
    at = 32
    at += env_count * 0x22C + hschema_length + value_types * 8
    item_group_table = [struct.unpack_from("<HH", body, at + index * 4) for index in range(item_groups)]
    at += item_groups * 4
    group_table = [struct.unpack_from("<HH", body, at + index * 4) for index in range(groups)]
    at += groups * 4
    iteminfo_table = [struct.unpack_from("<HH", body, at + index * 4) for index in range(iteminfo_count)]
    at += iteminfo_count * 4
    at += extension_length
    candidates = []
    for _ in range(candidate_count):
        kind, value_type = body[at], body[at + 1]
        if kind == 0:
            length, offset = struct.unpack_from("<HI", body, at + 2)
            candidates.append({"kind": 0, "value_type": value_type, "length": length, "offset": offset})
            at += 8
        elif kind == 1:
            source, item, section = struct.unpack_from("<HHH", body, at + 2)
            candidates.append({"kind": 1, "value_type": value_type, "item": item, "section": section})
            at += 8
        else:
            raise SystemExit(f"unknown candidate type {kind}")
    return {"hschema": hschema_index, "decision": decision_index, "iteminfo": iteminfo_table,
            "candidates": candidates, "item_groups": item_group_table, "groups": group_table}


def data_items(pri, section_index):
    body = pri.body(section_index)
    _, string_count, blob_count, total = struct.unpack_from("<IHHI", body, 0)
    at = 12
    strings = [struct.unpack_from("<HH", body, at + index * 4) for index in range(string_count)]
    at += string_count * 4
    blobs = [struct.unpack_from("<II", body, at + index * 8) for index in range(blob_count)]
    at += blob_count * 8
    stored = at
    return {"strings": strings, "blobs": blobs, "stored": stored, "body": body}


def resolve(pri, name):
    desc = descriptor(pri)
    resmap_index = desc["primary"] if desc["primary"] != 0xFFFF else desc["resmaps"][0]
    resmap = resource_map(pri, resmap_index)
    schema_info = schema(pri, resmap["hschema"])
    decn = decisions(pri, resmap["decision"])
    matches = [entry for entry in schema_info["entries"] if entry["name"] == name or entry["path"].endswith("/" + name)]
    if not matches:
        matches = [entry for entry in schema_info["entries"] if name in entry["path"]]
    results = []
    for entry in matches:
        index_property = entry["index"]
        if index_property >= len(resmap["iteminfo"]):
            continue
        decision, first_candidate = resmap["iteminfo"][index_property]
        if decision >= len(decn["entries"]):
            continue
        first_set, set_count = decn["entries"][decision]
        for candidate_index in range(first_candidate, first_candidate + set_count):
            candidate = resmap["candidates"][candidate_index]
            data_section = candidate["section"] if candidate["kind"] == 1 else None
            if candidate["kind"] == 1:
                items = data_items(pri, data_section)
                if candidate["item"] < len(items["strings"]):
                    offset, length = items["strings"][candidate["item"]]
                    # the string table's offset and length both count bytes
                    # (the Shapr3D index stores its XBF blobs through it)
                else:
                    blob = candidate["item"] - len(items["strings"])
                    if blob >= len(items["blobs"]):
                        continue
                    offset, length = items["blobs"][blob]
                results.append({"path": entry["path"], "candidate": candidate_index,
                                "data": items["body"][items["stored"] + offset:items["stored"] + offset + length]})
            else:
                results.append({"path": entry["path"], "candidate": candidate_index, "data": b"",
                                "note": "embedded data (not in mrm_pri2)"})
    return results


def main():
    parser = argparse.ArgumentParser(description="Resolve and extract a PRI resource")
    parser.add_argument("pri", type=Path)
    parser.add_argument("--name")
    parser.add_argument("--list", action="store_true")
    parser.add_argument("--out", type=Path)
    args = parser.parse_args()
    pri = Pri(args.pri.read_bytes())
    if args.list:
        desc = descriptor(pri)
        for section_index in desc["hschemas"]:
            info = schema(pri, section_index)
            print(f"schema {section_index}: {len(info['entries'])} names")
            for entry in info["entries"]:
                print(f"  {entry['index']:5d} {'scope' if entry['flags'] & 0x10 else 'item ':5s} {entry['path']}")
        return 0
    if not args.name:
        raise SystemExit("either --name or --list is required")
    results = resolve(pri, args.name)
    if not results:
        print(f"{args.name}: not found", file=sys.stderr)
        return 1
    for result in results:
        data = result["data"]
        print(f"{result['path']} candidate {result['candidate']}: {len(data)} bytes {data[:8].hex()}")
        if args.out and data:
            args.out.write_bytes(data)
            print(f"wrote {args.out}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
