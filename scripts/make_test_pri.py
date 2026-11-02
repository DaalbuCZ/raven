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


def schema(resources, extended=False):
    """The hierarchical schema, in the non-extended form: one name table per
    entry, scopes flagged, items carrying their item info index. The scope and
    item tables behind the entries are unread by the reference parser and
    zeroed."""
    entries = [{"name": "", "flags": SCOPE_FLAG, "parent": 0, "index": 0, "depth": 0, "path": ""}]
    paths = {"": 0}
    for name in resources:
        parts = name.split("/")
        for depth, part in enumerate(parts, start=1):
            path = "/" + "/".join(parts[:depth])
            if path in paths:
                continue
            parent_path = "/" + "/".join(parts[:depth - 1]) if depth > 1 else ""
            entries.append({"name": part, "flags": SCOPE_FLAG if depth < len(parts) else 0,
                            "depth": depth, "parent": paths[parent_path], "index": 0, "path": path})
            paths[path] = len(entries) - 1
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
    ascii_block = b""
    if extended:
        for entry in entries[1:]:
            entry["flags"] |= 0x20
            entry["offset"] = len(ascii_block)
            ascii_block += entry["name"].encode("ascii") + b"\0"
        block = ""  # the empty root needs no UTF-16 name storage
    longest = max(len(entry["path"]) for entry in entries)

    body = struct.pack("<HHHH",
                       0,                            # unread
                       len(SCHEMA_UNIQUE),           # unique resource id length
                       0,                            # schema name length
                       0)                            # unread
    if extended:
        body += struct.pack("<16s", b"[def_hnamesx]")
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
    if extended:
        body += struct.pack("<I", len(ascii_block))
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
    body += block.encode("utf-16le") + ascii_block
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
    values = list(resources.values())
    strings = [value for value in values if isinstance(value, str)]
    string_index = blob_index = 0
    for value in values:
        if isinstance(value, str):
            item = string_index
            string_index += 1
        else:
            item = len(strings) + blob_index
            blob_index += 1
        body += struct.pack("<BBHHH", CANDIDATE_SECTIONED, 4 if isinstance(value, str) else 2, 0, item, data_index)
    return body


def data_items(resources):
    """The data item section: a string table of (offset, length) pairs — both
    counting bytes, the way the Shapr3D package's index stores them — then the
    string data. No blob storage; the strings are nul-terminated with the
    terminator outside the counted length."""
    values = list(resources.values())
    strings = [value for value in values if isinstance(value, str)]
    blobs = [value for value in values if isinstance(value, bytes)]
    payload = b""
    string_table = b""
    for value in strings:
        encoded = value.encode("utf-16le")
        string_table += struct.pack("<HH", len(payload), len(encoded))
        payload += encoded + b"\0\0"
    blob_table = b""
    for value in blobs:
        blob_table += struct.pack("<II", len(payload), len(value))
        payload += value
    return struct.pack("<IHHI", 0, len(strings), len(blobs), len(payload)) + string_table + blob_table + payload


def application_dictionary_xbf(unknown_child=False):
    """Application resources with two merged dictionaries, the shape the compiled App.xaml carries: one plain and one that
    loads its entries through Source. A document that names a missing type replaces the first child with an unresolved
    reference."""
    child = 0x7FFF if unknown_child else 371
    nodes = (b"\x01"                                     # PushScope, the scope the Application object lands in
             + b"\x17\x1f\x80"                           # CreateTypeBeginInit Application (stable 31)
             + b"\x14\x73\x81"                           # PushScopeCreateTypeBeginInit ResourceDictionary (the Resources value)
             + b"\x13\xaf\x82"                           # PushScopeGetValue MergedDictionaries (stable 687)
             + b"\x14" + struct.pack("<H", 0x8000 | child) + b"\x21\x08"
             + b"\x14\x73\x81"                           # the second child, whose Source loads a document
             + b"\x1b\xb0\x82\x05" + struct.pack("<H", 0)  # SetValueTypeConvertedConstant ResourceDictionary_Source
             + b"\x21\x08"
             + b"\x02"                                   # PopScope (MergedDictionaries)
             + b"\x21"                                   # EndInitPopScope (the Resources value)
             + b"\x02")                                  # PopScope (the Application scope)
    return compiled_document(["ms-appx:///Fixture/Source.xaml"], nodes)


def compiled_document(strings, nodes):
    """A compiled XBF document whose metadata carries only a string table.

    Every type and property the fixtures name is a stable index, so the six metadata tables reduce to the string table and
    five empty ones. The layout is the one the loader's parser validates: the version words, the six table offsets relative
    to the metadata start, the null-terminated string table, then one substream holding `nodes`."""
    entries = b""
    for text in strings:
        entries += struct.pack("<I", len(text)) + text.encode("utf-16le") + b"\x00\x00"
    string_table = struct.pack("<I", len(strings)) + entries
    offsets = [56]  # the string table itself, then the five empty tables
    position = 56 + len(string_table)
    for _ in range(5):
        offsets.append(position)
        position += 4
    metadata = (struct.pack("<ii", 2, 1) + b"".join(struct.pack("<Q", offset) for offset in offsets)
                + string_table + struct.pack("<5I", 0, 0, 0, 0, 0))
    assert len(metadata) == position
    return (b"XBF\x00" + struct.pack("<II", len(metadata), 12 + len(nodes)) + metadata
            + struct.pack("<III", 1, 0, len(nodes)) + nodes)


def push_string(index):
    return b"\x04\x05" + struct.pack("<H", index)


def dictionary_key(index):
    return b"\x0a\x05" + struct.pack("<H", index)


def resource_dictionary_xbf(entries, source=None):
    """A standalone ResourceDictionary document: the root is the dictionary, and every entry is a string value under a
    string key (PushConstant then AddToDictionaryWithKey). `source` names a document the root loads by reference, the
    encoding the compiled App.xaml uses for its merged and theme dictionaries."""
    strings = []
    nodes = b"\x14\x73\x81"  # PushScopeCreateTypeBeginInit ResourceDictionary (stable 371)

    def string_id(text):
        strings.append(text)
        return len(strings) - 1

    for key, value in entries:
        nodes += push_string(string_id(value))
        nodes += dictionary_key(string_id(key))
    if source:
        # SetValueTypeConvertedConstant ResourceDictionary_Source (stable property 688)
        nodes += b"\x1b\xb0\x82\x05" + struct.pack("<H", string_id(source))
    nodes += b"\x21"  # EndInitPopScope
    return compiled_document(strings, nodes)


def theme_dictionary_xbf():
    """A ResourceDictionary whose ThemeDictionaries holds a Default dictionary, the shape a themed App.xaml carries: one
    entry on the root map and one under the theme key."""
    strings = ["Caption", "Theme root", "Accent", "red", "Default"]
    nodes = (b"\x14\x73\x81"                                   # root ResourceDictionary
             + push_string(1) + dictionary_key(0)              # "Caption" -> "Theme root"
             + b"\x13\xb1\x82"                                 # PushScopeGetValue ThemeDictionaries (stable 689)
             + b"\x14\x73\x81"                                 # the Default dictionary
             + push_string(3) + dictionary_key(2)              # "Accent" -> "red"
             + b"\x21"                                         # EndInitPopScope (the Default dictionary)
             + dictionary_key(4)                               # AddToDictionaryWithKey "Default"
             + b"\x02"                                         # PopScope (ThemeDictionaries)
             + b"\x21")                                        # EndInitPopScope (the root)
    return compiled_document(strings, nodes)


def textblock_xbf():
    """A TextBlock plus a two-entry null-terminated metadata string table."""
    nodes = b"\x14\x8c\x81\x1a\x20\x83\x09" + struct.pack("<I", 6) + "Loaded".encode("utf-16le") + b"\x02"
    document = bytearray(184)
    document[:4] = b"XBF\0"
    struct.pack_into("<IIII", document, 4, 160, 12 + len(nodes), 2, 1)
    for index, offset in enumerate((0x78, 0x8c, 0x90, 0x94, 0x98, 0x9c)):
        struct.pack_into("<Q", document, 20 + index * 8, offset)
    struct.pack_into("<I", document, 132, 2)
    struct.pack_into("<IHHIHH", document, 136, 1, ord("A"), 0, 1, ord("B"), 0)
    struct.pack_into("<III", document, 172, 1, 0, len(nodes))
    return bytes(document) + nodes



def build(resources, extended=False):
    """The whole file: header, table of contents, then the sections packed
    back to back. Section offsets are relative to where the section data
    starts."""
    bodies = [
        ("[mrm_pridescex]", None),
        ("[mrm_hschema0]", schema(resources, extended)),
        ("[mrm_decinfo0]", decisions(resources)),
        ("[mrm_resmap0]", None),
        ("[mrm_data0]", data_items(resources)),
    ]
    if extended:
        bodies[0], bodies[1], bodies[2] = bodies[2], bodies[0], bodies[1]
    descriptor_index = 1 if extended else 0
    schema_index = 2 if extended else 1
    decision_index = 0 if extended else 2
    resmap_index = 3
    data_index = 4
    bodies[descriptor_index] = (bodies[descriptor_index][0],
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
    parser.add_argument("--extended", action="store_true", help="use extended ASCII-name schema")
    parser.add_argument("--with-xbf", action="store_true", help="add a compiled TextBlock resource")
    args = parser.parse_args()
    resources = dict(RESOURCES)
    if args.with_xbf:
        resources["Files/Fixture/Message.xbf"] = textblock_xbf()
        malformed = bytearray(textblock_xbf())
        malformed[142] = 1  # the first string's required terminator
        resources["Files/Fixture/Invalid.xbf"] = bytes(malformed)
        resources["Files/Fixture/Dictionaries.xbf"] = application_dictionary_xbf()
        resources["Files/Fixture/MissingType.xbf"] = application_dictionary_xbf(unknown_child=True)
        resources["Files/Fixture/Source.xbf"] = resource_dictionary_xbf(
            [("Greeting", "Hello from Source"), ("Farewell", "Goodbye")])
        resources["Files/Fixture/Theme.xbf"] = theme_dictionary_xbf()
        # a dictionary whose Source names itself: the setter's recursion bound must refuse it instead of looping
        resources["Files/Fixture/Loop.xbf"] = resource_dictionary_xbf(
            [("Greeting", "Hello")], source="ms-appx:///Fixture/Loop.xaml")
    args.out.write_bytes(build(resources, args.extended))
    print(f"{args.out}: {len(resources)} resources")


if __name__ == "__main__":
    raise SystemExit(main())
