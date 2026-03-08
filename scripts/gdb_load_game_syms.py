"""
Auto-load debug symbols for dynamically-loaded game modules.

Breaks on gdb_game_loaded(), matches the loaded module to an on-disk ELF
by raw file size, replicates the kernel's section layout, and issues
add-symbol-file so breakpoints and stepping work in game code.

Override: set $game_elf = "out/debug/helloworld/helloworld.elf"
"""

import gdb
import struct
import os
import glob

TAG = "[game-syms]"
SHF_ALLOC = 0x2


def _log(msg):
    gdb.write("{} {}\n".format(TAG, msg))


def _workspace_root():
    for obj in gdb.objfiles():
        if "engineos" in obj.filename:
            # .../out/debug/engineos/engineos.elf -> ...
            return os.path.dirname(os.path.dirname(
                os.path.dirname(os.path.dirname(obj.filename))))
    return os.getcwd()


def _parse_elf32_sections(path):
    """Parse ELF32 section headers. Returns [(name, flags, size, align), ...]."""
    with open(path, "rb") as f:
        data = f.read()
    if data[:4] != b"\x7fELF":
        raise ValueError("not an ELF: " + path)

    ehdr = struct.unpack_from("<16sHHIIIIIHHHHHH", data, 0)
    e_shoff, e_shentsize, e_shnum, e_shstrndx = ehdr[6], ehdr[11], ehdr[12], ehdr[13]

    shdrs = [struct.unpack_from("<IIIIIIIIII", data, e_shoff + i * e_shentsize)
             for i in range(e_shnum)]

    strtab = data[shdrs[e_shstrndx][4]:][:shdrs[e_shstrndx][5]]

    def name(off):
        end = strtab.find(b"\0", off)
        return strtab[off:end if end >= 0 else len(strtab)].decode("ascii", "replace")

    return [(name(s[0]), s[2], s[5], s[8]) for s in shdrs]


def _compute_layout(sections):
    """Replicate kernel layout_alloc_sections (elf.c). Returns {name: offset}."""
    cursor, addrs = 0, {}
    for name, flags, size, align in sections:
        if not (flags & SHF_ALLOC):
            continue
        a = align or 1
        cursor = (cursor + a - 1) & ~(a - 1)
        if size:
            addrs[name] = cursor
        cursor += size
    return addrs


def _find_game_elf(raw_size):
    """Match loaded module to on-disk ELF by file size."""
    try:
        val = gdb.parse_and_eval("$game_elf")
        if val.type.code == gdb.TYPE_CODE_ARRAY:
            path = val.string()
            if path and os.path.isfile(path):
                return path
    except gdb.error:
        pass

    root = _workspace_root()
    candidates = sorted(
        p for p in glob.glob(os.path.join(root, "out", "debug", "*", "*.elf"))
        if "engineos" not in os.path.basename(os.path.dirname(p)))

    if not candidates:
        return None
    if raw_size > 0:
        for p in candidates:
            if os.path.getsize(p) == raw_size:
                return p

    if len(candidates) > 1:
        _log("ambiguous match, using first; override with $game_elf")
    return candidates[0]


def _build_add_symbol_cmd(elf_path, base, section_offsets):
    """Build the add-symbol-file command string."""
    text = section_offsets.pop(".text", 0)
    parts = ["add-symbol-file", elf_path, "0x{:x}".format(base + text)]
    for name, off in sorted(section_offsets.items()):
        parts.extend(["-s", name, "0x{:x}".format(base + off)])
    return " ".join(parts)


class _GameLoadedBP(gdb.Breakpoint):
    def __init__(self):
        super().__init__("gdb_game_loaded", internal=True)
        self.silent = True

    def stop(self):
        try:
            frame = gdb.selected_frame()
            load_start = int(frame.read_var("load_start"))
            raw_size = int(frame.read_var("raw_elf_size"))

            elf_path = _find_game_elf(raw_size)
            if not elf_path:
                _log("ERROR: no game ELF found in out/debug/")
                return True

            offsets = _compute_layout(_parse_elf32_sections(elf_path))
            if not offsets:
                _log("ERROR: no ALLOC sections in " + elf_path)
                return True

            base = load_start - min(offsets.values())
            cmd = _build_add_symbol_cmd(elf_path, base, offsets)
            _log(os.path.basename(elf_path) + " -> 0x{:x}".format(base))

            saved = gdb.parameter("confirm")
            gdb.execute("set confirm off")
            gdb.execute(cmd, from_tty=False, to_string=True)
            gdb.execute("set confirm {}".format("on" if saved else "off"))

            _log("symbols loaded")
        except Exception as e:
            _log("ERROR: {}".format(e))

        return False


_GameLoadedBP()
_log("ready")
