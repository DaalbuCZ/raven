#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
"""Generate type-name data from the stable XBF indices shared with the inspection tools."""
import json
from pathlib import Path
import sys

metadata, output = map(Path, sys.argv[1:])
names = json.loads(metadata.read_text())["types"]
lines = ["/* Generated from xbf-stable-indices.json. */", "static const WCHAR *const xbf_type_names[] = {"]
for index, name in enumerate(names):
    # Retired indices still occur in older compiled dictionaries under their original type names.
    value = "L" + json.dumps(name.split("_Deleted", 1)[0]) if index else "NULL"
    lines.append(f"\t{value},")
lines.append("};")
output.write_text("\n".join(lines) + "\n")
