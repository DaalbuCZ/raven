#!/usr/bin/env python3
"""Decode an XBF (compiled XAML) file: metadata tables and the node stream.

The reader follows the format used by Microsoft's XbfParser: the file header
(metadata size, node stream size), the metadata (version, six table offsets, a
64-byte hash, string/assembly/type-namespace/type/property/xml-namespace
tables), the substream table and the node records. Node payloads use persisted
node references (a 16-bit value whose top bit marks a stable index), constants
tagged by their type, and strings as a 32-bit length followed by UTF-16 units.

A PRI path with --name resolves the resource through scripts/pri-extract.py
first, so `App.xbf` can be decoded straight from the package.

Usage:
  python3 scripts/xbf-dump.py file.xbf [--limit N]
  python3 scripts/xbf-dump.py resources.pri --name App.xbf
"""
import argparse
import importlib.util
import json
import struct
import sys
from pathlib import Path

STABLE_PATH = Path(__file__).with_name("xbf-stable-indices.json")
STABLE = json.loads(STABLE_PATH.read_text()) if STABLE_PATH.exists() else {"types": [], "properties": []}

NODE_NAMES = {
    1: "PushScope", 2: "PopScope", 3: "AddNamespace", 4: "PushConstant", 7: "SetValue",
    8: "AddToCollection", 9: "AddToDictionary", 10: "AddToDictionaryWithKey", 11: "CheckPeerType",
    12: "SetConnectionId", 13: "SetName", 14: "GetResourcePropertyBag", 15: "SetCustomRuntimeData",
    17: "SetDeferredProperty", 18: "PushScopeAddNamespace", 19: "PushScopeGetValue",
    20: "PushScopeCreateTypeBeginInit", 21: "PushScopeCreateTypeWithConstantBeginInit",
    22: "PushScopeCreateTypeWithTypeConvertedConstantBeginInit", 23: "CreateTypeBeginInit",
    24: "CreateTypeWithConstantBeginInit", 25: "CreateTypeWithTypeConvertedConstantBeginInit",
    26: "SetValueConstant", 27: "SetValueTypeConvertedConstant", 28: "SetValueTypeConvertedResolvedProperty",
    29: "SetValueTypeConvertedResolvedType", 30: "SetValueFromStaticResource", 31: "SetValueFromTemplateBinding",
    32: "SetValueFromMarkupExtension", 33: "EndInitPopScope", 34: "ProvideStaticResourceValue",
    35: "ProvideThemeResourceValue", 36: "SetValueFromThemeResource", 38: "BeginConditionalScope",
    39: "EndConditionalScope", 40: "EndInitProvideValuePopScope",
}
SCOPE_NODES = {1, 18, 20, 21, 22, 40}
END_SCOPE_NODES = {2, 33, 39}
NO_PAYLOAD = {1, 2, 8, 9, 33, 39, 40}
NAMESPACE_NODES = {3, 18}
CONSTANT_NODES = {4, 10, 12, 13, 14, 34, 35}
PROPERTY_NODES = {7, 19, 32}
TYPE_NODES = {20, 23}
TYPE_CONSTANT_NODES = {21, 22, 24, 25}
PROPERTY_CONSTANT_NODES = {26, 27, 30, 36}
PROPERTY_TYPE_NODES = {29}
PROPERTY_PROPERTY_NODES = {28, 31}


class Reader:
    def __init__(self, data):
        self.data = data
        self.at = 0

    def byte(self):
        value = self.data[self.at]
        self.at += 1
        return value

    def u16(self):
        value = struct.unpack_from("<H", self.data, self.at)[0]
        self.at += 2
        return value

    def i32(self):
        value = struct.unpack_from("<i", self.data, self.at)[0]
        self.at += 4
        return value

    def u32(self):
        value = struct.unpack_from("<I", self.data, self.at)[0]
        self.at += 4
        return value

    def i64(self):
        value = struct.unpack_from("<q", self.data, self.at)[0]
        self.at += 8
        return value

    def f32(self):
        value = struct.unpack_from("<f", self.data, self.at)[0]
        self.at += 4
        return value

    def vint(self):
        result = 0
        shift = 0
        while True:
            value = self.byte()
            result |= (value & 0x7F) << shift
            if not value & 0x80:
                return result
            shift += 7

    def string(self):
        length = self.i32()
        if length < 0 or length > 1 << 20:
            raise ValueError(f"implausible string length {length} at {self.at - 4:#x}")
        text = self.data[self.at:self.at + length * 2].decode("utf-16le", "replace")
        self.at += length * 2
        return text

    def persisted(self):
        value = self.u16()
        return value & 0x8000 != 0, value & 0x7FFF


class Xbf:
    def __init__(self, data):
        self.data = data
        reader = Reader(data)
        if data[0:4] != b"XBF\x00":
            raise ValueError("missing XBF magic")
        reader.at = 4
        self.metadata_size = reader.u32()
        self.node_size = reader.u32()
        self.metadata_start = reader.at
        self.read_metadata()
        self.read_substreams()

    def read_metadata(self):
        reader = Reader(self.data)
        reader.at = self.metadata_start
        self.major, self.minor = reader.i32(), reader.i32()
        offsets = [reader.i64() for _ in range(6)]
        self.hash = self.data[reader.at:reader.at + 64]
        reader.at += 64
        base = self.metadata_start
        (self.string_offset, self.assembly_offset, self.namespace_offset, self.type_offset,
         self.property_offset, self.xmlns_offset) = offsets

        def parse_strings(terminated):
            probe = Reader(self.data)
            probe.at = base + self.string_offset
            count = probe.i32()
            if count < 0 or count > 1000000:
                raise ValueError(f"implausible string count {count}")
            out = []
            for _ in range(count):
                out.append(probe.string())
                if terminated:
                    probe.u16()
            return out, probe.at

        entries, terminated = None, False
        for candidate in (False, True):
            try:
                values, end = parse_strings(candidate)
            except (ValueError, struct.error):
                continue
            if end == base + self.assembly_offset:
                entries, terminated = values, candidate
                break
        if entries is None:
            raise ValueError("string table does not end at the assembly table")
        self.null_terminated = terminated
        self.strings = entries

        reader.at = base + self.assembly_offset
        self.assemblies = [(reader.i32(), reader.i32()) for _ in range(reader.i32())]
        reader.at = base + self.namespace_offset
        self.namespaces = [(reader.i32(), reader.i32()) for _ in range(reader.i32())]
        reader.at = base + self.type_offset
        self.types = [(reader.u32(), reader.i32(), reader.i32()) for _ in range(reader.i32())]
        reader.at = base + self.property_offset
        self.properties = [(reader.u32(), reader.i32(), reader.i32()) for _ in range(reader.i32())]
        reader.at = base + self.xmlns_offset
        self.xmlns = [reader.i32() for _ in range(reader.i32())]

    def type_name(self, identifier):
        if identifier < 0 or identifier >= len(self.types):
            return f"type#{identifier}"
        _, namespace_id, name_id = self.types[identifier]
        name = self.strings[name_id] if 0 <= name_id < len(self.strings) else f"str#{name_id}"
        if 0 <= namespace_id < len(self.namespaces):
            namespace_id = self.namespaces[namespace_id][1]
            prefix = self.strings[namespace_id] if 0 <= namespace_id < len(self.strings) else ""
            return f"{prefix}.{name}" if prefix else name
        return name

    def property_name(self, identifier):
        if identifier < 0 or identifier >= len(self.properties):
            return f"property#{identifier}"
        _, declaring, name_id = self.properties[identifier]
        name = self.strings[name_id] if 0 <= name_id < len(self.strings) else f"str#{name_id}"
        return f"{self.type_name(declaring)}.{name}" if declaring >= 0 else name

    def reference(self, trusted, identifier, kind):
        if trusted:
            table = STABLE["types" if kind == "type" else "properties"]
            return table[identifier] if identifier < len(table) else f"stable-{kind}#{identifier}"
        return self.type_name(identifier) if kind == "type" else self.property_name(identifier)

    def shared_string(self, reader):
        trusted, identifier = reader.persisted()
        if trusted or identifier >= len(self.strings):
            return f"stable-string#{identifier}" if trusted else f"string#{identifier}"
        return self.strings[identifier]

    def constant(self, reader):
        kind = reader.byte()
        if kind == 0:
            return "none"
        if kind == 1:
            return "false"
        if kind == 2:
            return "true"
        if kind == 3:
            return repr(reader.f32())
        if kind == 4:
            return str(reader.i32())
        if kind == 5:
            return repr(self.shared_string(reader))
        if kind == 6:
            return f"thickness({reader.f32()},{reader.f32()},{reader.f32()},{reader.f32()})"
        if kind == 7:
            return f"gridlength({reader.u32()},{reader.f32()})"
        if kind == 8:
            return f"color(#{reader.u32():08x})"
        if kind == 9:
            return repr(reader.string())
        if kind == 10:
            return "null"
        if kind == 11:
            return f"enum(stable-type#{reader.u16()}={reader.u32()})"
        raise ValueError(f"unknown constant type {kind} at {reader.at - 1:#x}")

    def read_substreams(self):
        reader = Reader(self.data)
        reader.at = self.metadata_start + self.metadata_size
        count = reader.i32()
        table = [(reader.u32(), reader.u32()) for _ in range(count)]
        start = reader.at
        # A substream's nodes span [node_offset, line_offset); what follows up to the next node section is line info,
        # which carries no nodes. Files compiled without line info (the app packages) keep the sections contiguous,
        # which is the special case line_offset[i] == node_offset[i + 1].
        self.substreams = [(start + node_offset, start + line_offset) for node_offset, line_offset in table]
        self.substreams_start = start

    def nodes(self, limit=0):
        output = []
        for index, (begin, end) in enumerate(self.substreams):
            output.append(f"substream {index}: nodes {begin:#x}..{end:#x} ({end - begin} bytes)")
            reader = Reader(self.data)
            reader.at = begin
            depth = 0
            while reader.at < end and (not limit or len(output) < limit):
                at = reader.at
                kind = reader.byte()
                name = NODE_NAMES.get(kind, f"unknown({kind})")
                detail = ""
                if kind in NO_PAYLOAD:
                    pass
                elif kind in NAMESPACE_NODES:
                    trusted, identifier = reader.persisted()
                    prefix = reader.string()
                    uri = self.xmlns[identifier] if identifier < len(self.xmlns) else -1
                    detail = f" prefix={prefix!r} xmlns={self.strings[uri]!r}"
                elif kind in CONSTANT_NODES:
                    detail = " " + self.constant(reader)
                elif kind in (7, 19, 32):
                    trusted, identifier = reader.persisted()
                    detail = f" {self.reference(trusted, identifier, 'property')}"
                elif kind == 11:
                    detail = f" {reader.string()!r}"
                elif kind == 17:
                    trusted, identifier = reader.persisted()
                    substream = reader.vint()
                    static_count = reader.vint()
                    theme_count = reader.vint()
                    static = [self.shared_string(reader) for _ in range(static_count)]
                    theme = [self.shared_string(reader) for _ in range(theme_count)]
                    detail = f" {self.reference(trusted, identifier, 'property')} substream={substream} static={static} theme={theme}"
                elif kind in TYPE_NODES:
                    trusted, identifier = reader.persisted()
                    detail = f" {self.reference(trusted, identifier, 'type')}"
                elif kind in TYPE_CONSTANT_NODES:
                    trusted, identifier = reader.persisted()
                    detail = f" {self.reference(trusted, identifier, 'type')} {self.constant(reader)}"
                elif kind in PROPERTY_CONSTANT_NODES:
                    trusted, identifier = reader.persisted()
                    detail = f" {self.reference(trusted, identifier, 'property')} {self.constant(reader)}"
                elif kind in PROPERTY_TYPE_NODES:
                    trusted, identifier = reader.persisted()
                    trusted_type, type_id = reader.persisted()
                    detail = f" {self.reference(trusted, identifier, 'property')} {self.reference(trusted_type, type_id, 'type')}"
                elif kind in PROPERTY_PROPERTY_NODES:
                    trusted, identifier = reader.persisted()
                    trusted_value, value_id = reader.persisted()
                    detail = (f" {self.reference(trusted, identifier, 'property')} "
                              f"{self.reference(trusted_value, value_id, 'property')}")
                elif kind == 38:
                    trusted, identifier = reader.persisted()
                    detail = f" {self.reference(trusted, identifier, 'type')} {self.shared_string(reader)}"
                else:
                    output.append(f"{'  ' * depth}[{at:#x}] {name}{detail} <unparsed>")
                    break
                if kind in END_SCOPE_NODES and depth:
                    depth -= 1
                output.append(f"{'  ' * depth}[{at:#x}] {name}{detail}")
                if kind in SCOPE_NODES:
                    depth += 1
        return output


def load_pri_resource(pri_path, name):
    spec = importlib.util.spec_from_file_location("prifmt", Path(__file__).with_name("pri-extract.py"))
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    pri = module.Pri(pri_path.read_bytes())
    results = module.resolve(pri, name)
    if not results or not results[0]["data"]:
        raise SystemExit(f"{name}: not found in {pri_path}")
    return results[0]["data"]


def main():
    parser = argparse.ArgumentParser(description="Decode a compiled XAML (XBF) file")
    parser.add_argument("path", type=Path)
    parser.add_argument("--name", help="resolve this resource name from a PRI file instead of treating the path as XBF")
    parser.add_argument("--limit", type=int, default=0, help="print only the first N node lines")
    args = parser.parse_args()
    data = load_pri_resource(args.path, args.name) if args.name else args.path.read_bytes()
    xbf = Xbf(data)
    print(f"XBF v{xbf.major}.{xbf.minor}, metadata {xbf.metadata_size} bytes, node streams {xbf.node_size} bytes; "
          f"{len(xbf.strings)} strings, {len(xbf.types)} types, {len(xbf.properties)} properties, "
          f"{len(xbf.xmlns)} xmlns")
    for line in xbf.nodes(args.limit):
        print(line)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
