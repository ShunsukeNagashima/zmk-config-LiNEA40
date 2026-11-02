"""Generate a Keymap Editor comparison from the actual keymap and OS key presets."""

import argparse
import json
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
KEYMAP = ROOT / "config/LiNEA40.keymap"
PRESETS = ROOT / "dts/behaviors/linea40_profile_keys.dtsi"
PREVIEW = ROOT / "config/LiNEA40-OS-preview.keymap"
METADATA = ROOT / "config/LiNEA40-OS-preview.json"


def node_span(text, name):
    """Locate a named node, including nested nodes, in this repository's DTS source."""
    match = re.search(rf"(?m)^    {re.escape(name)} \{{", text)
    if match is None:
        raise ValueError(f"Missing node: {name}")
    opening = text.index("{", match.start())
    depth = 1
    cursor = opening + 1
    while depth:
        if cursor >= len(text):
            raise ValueError(f"Unclosed node: {name}")
        depth += (text[cursor] == "{") - (text[cursor] == "}")
        cursor += 1
    return match.start(), cursor + 1, text[opening + 1 : cursor - 1]


def child_nodes(body):
    nodes = re.findall(r"(?ms)^        (\w+) \{\n(.*?)^        \};", body)
    if not nodes:
        raise ValueError("No child nodes found")
    return nodes


def generate():
    source = KEYMAP.read_text()
    presets = PRESETS.read_text()
    profile = re.search(r"#define LINEA40_WINDOWS_PROFILE (\d+)", presets).group(1)
    replacements = {"Mac": {}, "Windows": {}}
    for label, body in re.findall(r"(\w+):\s*\w+\s*\{(.*?)\};", presets, re.DOTALL):
        for os_name, prop in (("Mac", "mac-keycode"), ("Windows", "windows-keycode")):
            value = re.search(rf"{prop}\s*=\s*<([^>]+)>;", body)
            if value is None:
                raise ValueError(f"Missing {prop} in {label}")
            replacements[os_name][label] = "&kp " + value.group(1)

    def resolve(text, os_name):
        for label, binding in replacements[os_name].items():
            text = re.sub(rf"&{re.escape(label)}\b", binding, text)
        return text

    keymap_start, keymap_end, keymap_body = node_span(source, "keymap")
    layers = child_nodes(keymap_body)
    count = len(layers)
    constants = dict(re.findall(r"(?m)^#define (\w+) (\d+)\s*$", source))

    def layer_number(value):
        number = int(constants.get(value, value))
        if not 0 <= number < count:
            raise ValueError(f"Invalid layer reference: {value}")
        return number

    rendered_layers = []
    for os_name, offset in (("Mac", 0), ("Windows", count)):
        for name, body in layers:
            body = resolve(body, os_name)
            # The Windows comparison occupies the second set of layers.
            body = re.sub(
                r"&(lt|mo|to|tog|sl)\s+(\w+)",
                lambda m, offset=offset: f"&{m[1]} {layer_number(m[2]) + offset}",
                body,
            )
            rendered_layers.append(f"        {os_name}_{name} {{\n{body}        }};")
    keymap = '    keymap {\n        compatible = "zmk,keymap";\n\n'
    keymap += "\n\n".join(rendered_layers) + "\n    };"
    source = source[:keymap_start] + keymap + source[keymap_end:]

    combo_start, combo_end, combo_body = node_span(source, "combos")
    combos = child_nodes(combo_body)
    rendered_combos = []
    for name, original_body in combos:
        for os_name, offset in (("Mac", 0), ("Windows", count)):
            body = resolve(original_body, os_name)
            restriction = re.search(r"layers\s*=\s*<([^>]+)>;", body)
            indices = (
                [layer_number(value) for value in restriction[1].split()]
                if restriction
                else range(count)
            )
            restriction_text = "layers = <" + " ".join(str(i + offset) for i in indices) + ">;"
            if restriction:
                body = body[: restriction.start()] + restriction_text + body[restriction.end() :]
            else:
                body += "            " + restriction_text + "\n"
            rendered_combos.append(f"        {os_name}_{name} {{\n{body}        }};")
    combo_node = '    combos {\n        compatible = "zmk,combos";\n\n'
    combo_node += "\n\n".join(rendered_combos) + "\n    };"
    source = source[:combo_start] + combo_node + source[combo_end:]
    source = source.replace("#include <behaviors/linea40_profile_keys.dtsi>\n", "")

    banner = (
        "// GENERATED COMPARISON ONLY - do not edit or flash this keymap.\n"
        "// Edit config/LiNEA40.keymap and dts/behaviors/linea40_profile_keys.dtsi,\n"
        "// main: GitHub Actions regenerates this file after each source change.\n"
        "// For local changes, run: make preview\n"
        f"// Windows layers show Bluetooth profile {profile}; Mac layers show other profiles.\n"
        "// The firmware build continues to use config/LiNEA40.keymap.\n\n"
    )
    metadata = json.loads((ROOT / "config/LiNEA40.json").read_text())
    metadata["id"] = "LiNEA40-OS-preview"
    metadata["name"] = "LiNEA40 Mac / Windows comparison"
    return {
        PREVIEW: banner + source,
        METADATA: json.dumps(metadata, indent=2, ensure_ascii=False) + "\n",
    }


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--check", action="store_true", help="Fail if the preview is out of date")
    args = parser.parse_args()
    stale = []
    for path, content in generate().items():
        if args.check:
            if not path.exists() or path.read_text() != content:
                stale.append(str(path.relative_to(ROOT)))
        else:
            path.write_text(content)
            print(f"Generated {path.relative_to(ROOT)}")
    if stale:
        print("Run make preview to update: " + ", ".join(stale), file=sys.stderr)
        return 1
    if args.check:
        print("OS preview matches the firmware keymap and presets")
    return 0


if __name__ == "__main__":
    sys.exit(main())
