#!/usr/bin/env python3
"""Generate a C ABI header for selected Windows Runtime interfaces from a winmd.

The Windows SDK's C headers cover only part of the Windows Runtime namespaces;
the UWP XAML classes, which the XAML component implements, are absent. This tool
reads the metadata of a winmd (the file Windows keeps in System32\\WinMetadata)
and writes the interface declarations the component builds against: the vtable
in metadata order after the IInspectable methods, the IID from the Guid
attribute, and one method entry per MethodDef.

Parameter types are not decoded. Every Windows Runtime method returns HRESULT
and passes its arguments in integer or pointer registers for the interfaces
selected here, so a word-sized slot preserves the ABI. A signature that needs a
different treatment (a floating-point parameter, for instance) has to be
declared by hand instead.

Requires dnfile: python3 -m pip install --target /tmp/winmd-lib dnfile
then PYTHONPATH=/tmp/winmd-lib python3 scripts/generate-winrt-header.py ...
"""
import argparse
import hashlib
from pathlib import Path

import dnfile


def resolved(value):
    return value.row if hasattr(value, "row") else value


def guid_of(reader, typedef):
    for attribute in reader.net.mdtables.CustomAttribute.rows:
        if resolved(attribute.Parent) is not typedef:
            continue
        constructor = resolved(attribute.Type)
        owner = resolved(getattr(constructor, "Class", None))
        if getattr(owner, "TypeName", "") != "GuidAttribute":
            continue
        blob = bytes(attribute.Value.value)
        if len(blob) < 18 or blob[0:2] != b"\x01\x00":
            raise SystemExit(f"Guid attribute of {typedef.TypeName} has an unexpected blob: {blob.hex()}")
        data = blob[2:18]
        return (int.from_bytes(data[0:4], "little"), int.from_bytes(data[4:6], "little"),
                int.from_bytes(data[6:8], "little"), data[8:16])
    return None


def compressed(data, offset):
    first = data[offset]
    if first & 0x80 == 0:
        return first, offset + 1
    if first & 0xC0 == 0x80:
        return ((first & 0x3F) << 8) | data[offset + 1], offset + 2
    return int.from_bytes(data[offset + 1:offset + 5], "big") | ((first & 0x1F) << 24), offset + 5


def abi_parameter_count(method):
    """Parameters the ABI passes: the declared ones, plus the return value when it is not void.

    A Windows Runtime method always returns HRESULT; a non-void logical return (a property getter or a factory's created object)
    becomes one more out parameter. The MethodDef signature carries both facts: the calling convention, the declared parameter
    count and the return element type.
    """
    signature = bytes(method.Signature.value)
    offset = 1
    if signature[0] & 0x10:
        _, offset = compressed(signature, offset)
    declared, offset = compressed(signature, offset)
    return declared + (0 if signature[offset] == 0x01 else 1)


def abi_name(namespace, name):
    return "__x_ABI_" + "_".join("C" + part for part in namespace.split(".")) + "_C" + name


def emit(namespace, name, iid, methods):
    abi = abi_name(namespace, name)
    lines = [f"typedef struct {abi} {abi};", f"typedef struct {abi}Vtbl {{", "\tBEGIN_INTERFACE"]
    standard = [("QueryInterface", "HRESULT", "REFIID riid, void **object"), ("AddRef", "ULONG", ""), ("Release", "ULONG", ""),
                ("GetIids", "HRESULT", "ULONG *count, IID **iids"), ("GetRuntimeClassName", "HRESULT", "HSTRING *name"),
                ("GetTrustLevel", "HRESULT", "TrustLevel *level")]
    for method, result_type, arguments in standard:
        signature = "{0} *This".format(abi) + (", " + arguments if arguments else "")
        lines += [f"\t{result_type} (STDMETHODCALLTYPE *{method})({signature});", ""]
    for index, (method, count) in enumerate(methods):
        parameters = [f"{abi} *This"] + [f"uint64_t a{number + 1}" for number in range(count)]
        lines.append(f"\tHRESULT (STDMETHODCALLTYPE *{method})({', '.join(parameters)});")
    lines += [f"\tEND_INTERFACE", f"}} {abi}Vtbl;", "", f"interface {abi} {{", f"\tCONST_VTBL {abi}Vtbl *lpVtbl;", "};", ""]
    lines.append("DEFINE_GUID(IID_{0}, 0x{1:08x}, 0x{2:04x}, 0x{3:04x}, {4});".format(
        abi[1:], iid[0], iid[1], iid[2], ", ".join(f"0x{byte:02x}" for byte in iid[3])))
    return lines


def main():
    parser = argparse.ArgumentParser(description="Generate C declarations for Windows Runtime interfaces from a winmd")
    parser.add_argument("--winmd", type=Path, required=True)
    parser.add_argument("--namespace", required=True)
    parser.add_argument("--types", required=True, help="comma separated interface names, in any order")
    parser.add_argument("--out", type=Path, required=True)
    args = parser.parse_args()
    wanted = [name for name in args.types.split(",") if name]
    data = args.winmd.read_bytes()
    reader = dnfile.dnPE(str(args.winmd), clr_lazy_load=False)
    found = {}
    for typedef in reader.net.mdtables.TypeDef.rows:
        if str(typedef.TypeNamespace) != args.namespace or str(typedef.TypeName) not in wanted:
            continue
        iid = guid_of(reader, typedef)
        if not iid:
            raise SystemExit(f"{args.namespace}.{typedef.TypeName} carries no Guid attribute")
        methods = []
        seen = {}
        for item in typedef.MethodList:
            method = resolved(item)
            name = str(method.Name)
            # One interface can declare the same name twice with different signatures; the vtable is positional, so the C member
            # names have to be unique while the order stays as the metadata gives it.
            seen[name] = seen.get(name, 0) + 1
            if seen[name] > 1:
                name = f"{name}{seen[name]}"
            methods.append((name, abi_parameter_count(method)))
        found[str(typedef.TypeName)] = (iid, methods)
    missing = [name for name in wanted if name not in found]
    if missing:
        raise SystemExit("missing from the winmd: " + ", ".join(missing))
    output = [f"/* Generated from {args.winmd.name} (SHA-256 {hashlib.sha256(data).hexdigest()}) by "
              "scripts/generate-winrt-header.py. Do not edit. */",
              "#pragma once", "", "#include <windows.h>", "#include <inspectable.h>", "#include <winstring.h>", "#include <stdint.h>", ""]
    for name in wanted:
        output += emit(args.namespace, name, *found[name])
    args.out.write_text("\n".join(output) + "\n")
    print(f"{args.out}: {len(wanted)} interface(s) from {args.winmd}")


if __name__ == "__main__":
    main()
