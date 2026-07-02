#!/usr/bin/env python3
"""Generate cork_data.inc -- the static mapping tables consumed by cork.cpp.

The cork<->UTF-8 conversion tables were originally built at runtime by
converter.cpp's `converter_rep::load()`, which parsed the .scm dictionaries
under TeXmacs/langs/encoding/ into a hashtree<char, string>. This script
reproduces that load logic and emits the same data as compile-time constant
arrays so cork.cpp can do cheap table lookups instead.

Run from the mogan project root:

    python3 lolly/lolly/data/gen_cork_data.py

Output: lolly/lolly/data/cork_data.inc (next to this script).

Behaviour notes (must match converter.cpp exactly):
- `put_prefix_code -> set_label` always overwrites, so when the same key
  appears in multiple dictionaries, the LAST loaded dictionary wins. We
  mirror that with `dict[key] = value` (not setdefault).
- The cork_to_utf8 direction loads symbol-unicode-fallback.scm; the strict
  variant omits it. We emit a separate index table for the strict subset
  that points back into the full entity key/value pools.
"""
import os
import re
import sys

HERE = os.path.dirname(os.path.abspath(__file__))


# --------------------------------------------------------------------------
# .scm dictionary parsing -- mirrors hashtree_from_dictionary / convert_escapes
# --------------------------------------------------------------------------

def parse_scm(path):
    pairs = []
    with open(path, encoding='utf-8') as f:
        text = f.read()
    text = re.sub(r';[^\n]*', '', text)  # strip line comments
    # Each entry is `( atom atom )` where atom is either "quoted" or bare.
    for m in re.finditer(r'\(\s*("[^"]*"|[^\s()]*)\s+("[^"]*"|[^\s()]*)\s*\)',
                         text):
        pairs.append((m.group(1), m.group(2)))
    return pairs


def unquote(s):
    if len(s) >= 2 and s[0] == '"' and s[-1] == '"':
        return s[1:-1]
    return s


def convert_escapes(s, utf8):
    """Mirror converter.cpp's convert_escapes(): #<hex>+ -> raw bytes.

    If utf8 is False the hex value is a single byte (BIT2BIT mode); if True
    it is a Unicode codepoint encoded as UTF-8 (UTF8 mode).
    """
    out = bytearray()
    i = 0
    while i < len(s):
        c = s[i]
        if c != '#':
            out.append(ord(c))
            i += 1
            continue
        i += 1
        num = 0
        while i < len(s) and s[i] in '0123456789ABCDEFabcdef':
            num = 0x10 * num + int(s[i], 16)
            i += 1
        if utf8:
            out.extend(chr(num).encode('utf-8'))
        else:
            out.append(num & 0xFF)
    return bytes(out)


# --------------------------------------------------------------------------
# Locate the dictionary directory (TeXmacs/langs/encoding relative to the
# project root). Walk up from this script's directory until we find it.
# --------------------------------------------------------------------------

def find_dict_dir():
    cur = HERE
    for _ in range(6):
        candidate = os.path.join(cur, 'TeXmacs', 'langs', 'encoding')
        if os.path.isdir(candidate):
            return candidate
        cur = os.path.dirname(cur)
    sys.exit(f"error: could not locate TeXmacs/langs/encoding/ "
             f"starting from {HERE}")


DICT_DIR = find_dict_dir()


# --------------------------------------------------------------------------
# Build the raw mappings: bytes(key) -> bytes(value). Mirrors
# converter_rep::load() per direction. Last-wins on conflict.
#
# Each spec entry: (filename, key_utf8_escape, val_utf8_escape, reverse)
# --------------------------------------------------------------------------

UTF8_TO_CORK_SPEC = [
    # cork->unicode reversed: orig tuple is (cork-bytes, unicode-cp); reverse
    # makes unicode-cp the key (UTF-8 escaped) and cork-bytes the value.
    ("corktounicode.scm",        True,  False, True),
    ("unicode-cork-oneway.scm",  True,  False, False),
    # symbol->unicode reversed: orig tuple is (symbol-name, unicode-cp).
    ("tmuniversaltounicode.scm", True,  False, True),
    ("unicode-symbol-oneway.scm", True, False, True),
]

CORK_TO_UTF8_SPEC = [
    ("corktounicode.scm",          False, True, False),
    ("cork-unicode-oneway.scm",    False, True, False),
    ("tmuniversaltounicode.scm",   False, True, False),
    ("symbol-unicode-oneway.scm",  False, True, False),
    ("symbol-unicode-fallback.scm", False, True, False),
    ("symbol-unicode-math.scm",    False, True, False),
]


def collect(spec):
    pairs = {}
    for fname, k_utf8, v_utf8, reverse in spec:
        path = os.path.join(DICT_DIR, fname)
        if not os.path.exists(path):
            continue
        for k_raw, v_raw in parse_scm(path):
            k = unquote(k_raw)
            v = unquote(v_raw)
            if reverse:
                key_src, val_src = v, k
            else:
                key_src, val_src = k, v
            key = convert_escapes(key_src, k_utf8)
            val = convert_escapes(val_src, v_utf8)
            pairs[key] = val  # last-wins
    return pairs


# --------------------------------------------------------------------------
# C++ emission helpers
# --------------------------------------------------------------------------

L = []


def emit(s=""):
    L.append(s)


def emit_struct_array(name, items, ty="cork_slice"):
    """Emit `ty name[N] = {{off, len}, ...}` plus return the flat byte pool."""
    emit(f"static const {ty} {name}[{len(items)}] = {{")
    off = 0
    for i in range(0, len(items), 4):
        row = items[i:i + 4]
        cells = []
        for x in row:
            cells.append(f"{{{off}, {len(x)}}}")
            off += len(x)
        emit("    " + ", ".join(cells) + ",")
    emit("};")
    return b''.join(items)


def emit_byte_pool(name, data):
    emit(f"static const unsigned char {name}[{len(data)}] = {{")
    for i in range(0, len(data), 16):
        row = data[i:i + 16]
        emit("    " + ", ".join(f"0x{b:02X}" for b in row) + ",")
    emit("};")


def utf8_to_codepoint(b):
    if len(b) == 1:
        return b[0]
    if len(b) == 2:
        return ((b[0] & 0x1F) << 6) | (b[1] & 0x3F)
    if len(b) == 3:
        return ((b[0] & 0x0F) << 12) | ((b[1] & 0x3F) << 6) | (b[2] & 0x3F)
    return ((b[0] & 0x07) << 18) | ((b[1] & 0x3F) << 12) \
        | ((b[2] & 0x3F) << 6) | (b[3] & 0x3F)


# --------------------------------------------------------------------------
# Main
# --------------------------------------------------------------------------

def main():
    u2c = collect(UTF8_TO_CORK_SPEC)
    c2u = collect(CORK_TO_UTF8_SPEC)

    # utf8_to_cork ASCII table (1-byte keys with codepoint < 128).
    ascii_vals = [b'' for _ in range(128)]
    for kb, vb in u2c.items():
        if len(kb) == 1 and kb[0] < 128:
            ascii_vals[kb[0]] = vb

    # utf8_to_cork multi-codepoint table (key length > 1).
    multi = []
    for kb, vb in u2c.items():
        if len(kb) == 1:
            continue
        multi.append((utf8_to_codepoint(kb), vb))
    multi.sort(key=lambda x: x[0])
    assert all(multi[i][0] < multi[i + 1][0] for i in range(len(multi) - 1)), \
        "codepoints not unique/sorted"

    # cork_to_utf8 1-byte table.
    cork_byte_vals = [b'' for _ in range(256)]
    for kb, vb in c2u.items():
        if len(kb) == 1:
            cork_byte_vals[kb[0]] = vb

    # cork_to_utf8 multi-byte: split into <name> entities and the 3 special
    # non-entity keys (%\x18, %\x18\x18, ...).
    entity = []
    special = []
    for kb, vb in c2u.items():
        if len(kb) == 1:
            continue
        if kb[0] == ord('<') and kb[-1] == ord('>'):
            entity.append((kb, vb))
        else:
            special.append((kb, vb))
    entity.sort(key=lambda x: x[0])
    special.sort(key=lambda x: x[0])

    # strict_cork_to_utf8: cork_to_utf8 minus symbol-unicode-fallback.scm.
    fallback_path = os.path.join(DICT_DIR, "symbol-unicode-fallback.scm")
    fallback_keys = set()
    for k_raw, _ in parse_scm(fallback_path):
        # fallback is loaded as BIT2BIT -> UTF8 (no reverse); the cork-side
        # key is the literal symbol name.
        fallback_keys.add(convert_escapes(unquote(k_raw), False))
    strict_entity = [kv for kv in entity if kv[0] not in fallback_keys]

    # --- emit ---
    emit("// Auto-generated by gen_cork_data.py -- DO NOT EDIT BY HAND.")
    emit("// Mirrors the runtime load in the original converter.cpp:")
    emit("//   corktounicode / unicode-cork-oneway / tmuniversaltounicode /")
    emit("//   unicode-symbol-oneway / cork-unicode-oneway / symbol-unicode-*")
    emit("//")
    emit("// This file is included from inside `namespace lolly { namespace data {`")
    emit("// in cork.cpp -- it must not reopen those namespaces.")
    emit("")
    emit("// A slice into one of the byte pools below.")
    emit("struct cork_slice { int off; int len; };")
    emit("")

    # utf8_to_cork ASCII
    emit("// codepoint < 128: {offset, len} into utf8_to_cork_ascii_pool.")
    emit("// len==0 means the codepoint has no Cork mapping (passthrough).")
    ascii_flat = emit_struct_array("utf8_to_cork_ascii", ascii_vals)
    emit_byte_pool("utf8_to_cork_ascii_pool", ascii_flat)
    emit("")

    # utf8_to_cork multi-codepoint
    emit(f"static const int utf8_to_cork_cp_count = {len(multi)};")
    emit(f"static const uint32_t utf8_to_cork_cp_codepoints[{len(multi)}] = {{")
    for i in range(0, len(multi), 8):
        row = multi[i:i + 8]
        emit("    " + ", ".join(f"0x{cp:06X}" for cp, _ in row) + ",")
    emit("};")
    multi_flat = emit_struct_array("utf8_to_cork_cp_values",
                                   [v for _, v in multi])
    emit_byte_pool("utf8_to_cork_cp_value_pool", multi_flat)
    emit("")

    # cork_to_utf8 1-byte
    emit("// cork byte 0..255: {offset, len} into cork_to_utf8_byte_pool.")
    emit("// len==0 means the cork byte has no UTF-8 mapping (passthrough).")
    cork_byte_flat = emit_struct_array("cork_to_utf8_byte", cork_byte_vals)
    emit_byte_pool("cork_to_utf8_byte_pool", cork_byte_flat)
    emit("")

    # cork_to_utf8 entity
    emit(f"static const int cork_to_utf8_entity_count = {len(entity)};")
    emit("// entity (multi-byte cork key starting with '<' and ending with '>'):")
    emit("// keys sorted for binary search.")
    ent_key_flat = emit_struct_array("cork_to_utf8_entity_keys",
                                     [k for k, _ in entity])
    emit_byte_pool("cork_to_utf8_entity_key_pool", ent_key_flat)
    ent_val_flat = emit_struct_array("cork_to_utf8_entity_values",
                                     [v for _, v in entity])
    emit_byte_pool("cork_to_utf8_entity_value_pool", ent_val_flat)
    emit("")

    # cork_to_utf8 special (non-entity multi-byte keys)
    emit(f"static const int cork_to_utf8_special_count = {len(special)};")
    spec_key_flat = emit_struct_array("cork_to_utf8_special_keys",
                                      [k for k, _ in special])
    emit_byte_pool("cork_to_utf8_special_key_pool", spec_key_flat)
    spec_val_flat = emit_struct_array("cork_to_utf8_special_values",
                                      [v for _, v in special])
    emit_byte_pool("cork_to_utf8_special_value_pool", spec_val_flat)
    emit("")

    # strict_cork_to_utf8: subset indices into the full entity table
    emit(f"static const int strict_cork_to_utf8_entity_count = "
         f"{len(strict_entity)};")
    emit("// strict_cork_to_utf8 entity table: subset of cork_to_utf8_entity,")
    emit("// minus symbol-unicode-fallback.scm. Reuses the same key/value "
         "byte pools.")
    emit(f"static const int strict_cork_to_utf8_entity_indices["
         f"{len(strict_entity)}] = {{")
    ent_index = {k: i for i, (k, _) in enumerate(entity)}
    for i in range(0, len(strict_entity), 16):
        row = strict_entity[i:i + 16]
        emit("    " + ", ".join(str(ent_index[k]) for k, _ in row) + ",")
    emit("};")
    emit("")

    out_path = os.path.join(HERE, "cork_data.inc")
    with open(out_path, 'w') as f:
        f.write("\n".join(L) + "\n")

    print(f"generated {out_path}: {len(L)} lines")
    print(f"  ascii = {len(ascii_vals)} entries, pool = {len(ascii_flat)} bytes")
    print(f"  multi = {len(multi)} entries, value pool = {len(multi_flat)} bytes")
    print(f"  cork byte = {len(cork_byte_vals)} entries, pool = "
          f"{len(cork_byte_flat)} bytes")
    print(f"  entity = {len(entity)} entries, key pool = {len(ent_key_flat)}, "
          f"val pool = {len(ent_val_flat)}")
    print(f"  special = {len(special)} entries")
    print(f"  strict entity = {len(strict_entity)} entries "
          f"(excludes {len(entity) - len(strict_entity)} fallback keys)")


if __name__ == '__main__':
    main()
