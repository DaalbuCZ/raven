#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
"""Generate type-name and property-name data from the stable XBF indices shared with the inspection tools."""
import json
from pathlib import Path
import sys

metadata, output = map(Path, sys.argv[1:])
data = json.loads(metadata.read_text())
names = data["types"]
lines = ["/* Generated from xbf-stable-indices.json. */", "static const WCHAR *const xbf_type_names[] = {"]
for index, name in enumerate(names):
    # Retired indices still occur in older compiled dictionaries under their original type names.
    value = "L" + json.dumps(name.split("_Deleted", 1)[0]) if index else "NULL"
    lines.append(f"\t{value},")
lines.append("};")
# The property stable table: the loader resolves stable property references by short name (the name the
# element setters and the metadata property tables carry), so the "Type_Property" spelling loses its prefix.
lines.append("static const WCHAR *const xbf_property_names[] = {")
for index, name in enumerate(data.get("properties", [])):
    value = "L" + json.dumps(name.split("_", 1)[-1]) if index and name else "NULL"
    lines.append(f"\t{value},")
lines.append("};")
output.write_text("\n".join(lines) + "\n")
