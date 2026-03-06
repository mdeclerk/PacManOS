from __future__ import annotations

import tkinter as tk
from tkinter import colorchooser, messagebox, simpledialog, ttk

from .bootstrap import bootstrap_levels
from .codec import read_levels_file, write_levels_file
from .models import (
    MAP_COLS,
    MAP_ROWS,
    LevelRecord,
    TILE_GHOST_SPAWN,
    TILE_PACMAN_SPAWN,
    TILE_WALL_FORCED_BASE,
)
from .paths import LEVELS_PATH, TILESET_PATH, TILE_SIZE
from .render import (
    argb_to_hex,
    hex_to_argb,
    init_wall_tile_images,
    make_wall_preview,
    render_canvas,
)
from .tools import TOOL_DEFS, apply_tool, shortcut_levels

DEFAULT_TINTS = [
    0xFF3C8DFF,
    0xFF54B87A,
    0xFFFF8A3D,
    0xFFDE4AA5,
    0xFF3FC7D5,
]


class LevelEditor:
    def __init__(self, root: tk.Tk) -> None:
        self.root = root
        self.root.title("PacManOS Level Editor")
        self.root.geometry("1180x760")

        self.levels: list[LevelRecord] = []
        self.selected_index = -1
        self.current_tool = tk.StringVar(value="empty")
        self.tint_var = tk.StringVar(value="#3C8DFF")
        self.status_var = tk.StringVar(value="")
        self.status_fg = "#7FD66B"
        self.dragging = False
        self.wall_preview_images: dict[int, tk.PhotoImage] = {}
        self.wall_tile_images: dict[int, tk.PhotoImage] = {}
        self.tinted_wall_cache: dict[tuple[int, int], tk.PhotoImage] = {}
        self.tileset_image: tk.PhotoImage | None = None
        self.modified_level_ids: set[int] = set()
        self.tool_shortcuts = shortcut_levels()

        self._init_tileset_images()
        self._build_ui()
        self._load_levels()

    @property
    def current_level(self) -> LevelRecord | None:
        if 0 <= self.selected_index < len(self.levels):
            return self.levels[self.selected_index]
        return None

    def _build_ui(self) -> None:
        self.root.columnconfigure(0, minsize=240)
        self.root.columnconfigure(1, weight=1)
        self.root.columnconfigure(2, minsize=260)
        self.root.rowconfigure(0, weight=1)

        left = ttk.Frame(self.root, padding=8)
        left.grid(row=0, column=0, sticky="nsew")
        left.columnconfigure(0, weight=1)
        left.rowconfigure(4, weight=1)

        ttk.Label(left, text="levels.bin").grid(row=0, column=0, sticky="w")
        file_actions = ttk.Frame(left)
        file_actions.grid(row=1, column=0, sticky="ew", pady=(6, 10))
        file_actions.columnconfigure(0, weight=1)
        ttk.Button(file_actions, text="Save", command=self._save_levels).grid(row=0, column=0, sticky="ew", pady=(0, 6))
        ttk.Button(file_actions, text="Reload", command=self._restore_levels).grid(row=1, column=0, sticky="ew", pady=(0, 6))
        ttk.Button(file_actions, text="Reset to Defaults", command=self._reset_to_defaults).grid(row=2, column=0, sticky="ew")

        ttk.Separator(left, orient="horizontal").grid(row=2, column=0, sticky="ew", pady=(2, 10))
        ttk.Label(left, text="Levels").grid(row=3, column=0, sticky="w")
        self.level_list = tk.Listbox(left, height=10, width=24, exportselection=False)
        self.level_list.grid(row=4, column=0, sticky="nsew")
        self.level_list.bind("<<ListboxSelect>>", self._on_select_level)

        level_actions = ttk.Frame(left)
        level_actions.grid(row=5, column=0, sticky="ew", pady=(8, 0))
        level_actions.columnconfigure(0, weight=1)
        ttk.Button(level_actions, text="New", command=self._add_level).grid(row=0, column=0, sticky="ew", pady=(0, 4))
        ttk.Button(level_actions, text="Clone", command=self._clone_level).grid(row=1, column=0, sticky="ew", pady=(0, 4))
        ttk.Button(level_actions, text="Rename", command=self._rename_level).grid(row=2, column=0, sticky="ew", pady=(0, 4))
        ttk.Button(level_actions, text="Remove", command=self._remove_level).grid(row=3, column=0, sticky="ew", pady=(0, 6))
        ttk.Separator(level_actions, orient="horizontal").grid(row=4, column=0, sticky="ew", pady=(0, 6))
        ttk.Button(level_actions, text="Move Up", command=lambda: self._move_level(-1)).grid(row=5, column=0, sticky="ew", pady=(0, 4))
        ttk.Button(level_actions, text="Move Down", command=lambda: self._move_level(1)).grid(row=6, column=0, sticky="ew")

        center = ttk.Frame(self.root, padding=8)
        center.grid(row=0, column=1, sticky="nsew")
        center.rowconfigure(0, weight=1)
        center.columnconfigure(0, weight=1)

        self.canvas = tk.Canvas(
            center,
            width=MAP_COLS * TILE_SIZE,
            height=MAP_ROWS * TILE_SIZE,
            bg="#05070A",
            highlightthickness=1,
            highlightbackground="#253043",
        )
        self.canvas.grid(row=0, column=0, sticky="nsew")
        self.canvas.bind("<Button-1>", self._on_canvas_down)
        self.canvas.bind("<B1-Motion>", self._on_canvas_drag)
        self.canvas.bind("<ButtonRelease-1>", self._on_canvas_up)

        right = ttk.Frame(self.root, padding=8)
        right.grid(row=0, column=2, sticky="ns")

        ttk.Label(right, text="Tools").grid(row=0, column=0, sticky="w")
        for idx, (key, label, value) in enumerate(TOOL_DEFS, start=1):
            ttk.Radiobutton(
                right,
                text=f"({key}) {label}",
                value=value,
                variable=self.current_tool,
            ).grid(row=idx, column=0, sticky="w")

        wall_row = len(TOOL_DEFS) + 1
        ttk.Separator(right, orient="horizontal").grid(row=wall_row, column=0, sticky="ew", pady=8)
        ttk.Label(right, text="Forced Walls (0-F)").grid(row=wall_row + 1, column=0, sticky="w")

        forced_frame = ttk.Frame(right)
        forced_frame.grid(row=wall_row + 2, column=0, sticky="w")
        self._init_wall_previews()
        for mask in range(16):
            r = mask // 4
            c = mask % 4
            ttk.Radiobutton(
                forced_frame,
                text=f"{mask:X}",
                value=f"wall_forced_{mask}",
                variable=self.current_tool,
                image=self.wall_preview_images[mask],
                compound="left",
            ).grid(row=r, column=c, sticky="w", padx=(0, 10), pady=2)

        tint_row = wall_row + 3
        ttk.Separator(right, orient="horizontal").grid(row=tint_row, column=0, sticky="ew", pady=8)
        ttk.Label(right, text="Wall Tint").grid(row=tint_row + 1, column=0, sticky="w")
        tint_entry = ttk.Entry(right, textvariable=self.tint_var, width=12)
        tint_entry.grid(row=tint_row + 2, column=0, sticky="w")
        tint_entry.bind("<Return>", self._on_tint_entry_commit)
        tint_entry.bind("<FocusOut>", self._on_tint_entry_commit)
        ttk.Button(right, text="Pick Color", command=self._pick_tint).grid(row=tint_row + 3, column=0, sticky="ew", pady=(6, 0))

        status = tk.Label(self.root, textvariable=self.status_var, relief="sunken", anchor="w", fg=self.status_fg)
        status.grid(row=1, column=0, columnspan=3, sticky="ew")
        self.status_label = status
        self.root.bind("<KeyPress>", self._on_key_shortcut)

    def _load_levels(self) -> None:
        try:
            self.levels = read_levels_file(LEVELS_PATH)
            self.modified_level_ids.clear()
        except FileNotFoundError:
            bootstrap_levels()
            self.levels = read_levels_file(LEVELS_PATH)
            self.modified_level_ids.clear()
            self.status_var.set(f"Bootstrapped defaults to {LEVELS_PATH}")
        except Exception as exc:
            try:
                bootstrap_levels()
                self.levels = read_levels_file(LEVELS_PATH)
                self.modified_level_ids.clear()
                self.status_var.set(f"Load failed; reset defaults in {LEVELS_PATH}")
            except Exception:
                messagebox.showerror("Failed to load levels.bin", str(exc))
                raise

        self._refresh_level_list()

    def _save_levels(self) -> None:
        try:
            write_levels_file(LEVELS_PATH, self.levels)
            self.modified_level_ids.clear()
            self._refresh_level_list()
            self.status_var.set(f"Saved {LEVELS_PATH}")
        except Exception as exc:
            messagebox.showerror("Save failed", str(exc))

    def _restore_levels(self) -> None:
        if not messagebox.askyesno("Restore", "Reload levels.bin and discard unsaved changes?"):
            return
        try:
            self.levels = read_levels_file(LEVELS_PATH)
            self.modified_level_ids.clear()
            self.selected_index = 0
            self._refresh_level_list()
            self.status_var.set(f"Restored {LEVELS_PATH}")
        except FileNotFoundError:
            try:
                bootstrap_levels()
                self.levels = read_levels_file(LEVELS_PATH)
                self.modified_level_ids.clear()
                self.selected_index = 0
                self._refresh_level_list()
                self.status_var.set(f"levels.bin missing; bootstrapped defaults to {LEVELS_PATH}")
            except Exception as exc:
                messagebox.showerror("Restore failed", str(exc))
        except Exception as exc:
            messagebox.showerror("Restore failed", str(exc))

    def _reset_to_defaults(self) -> None:
        if not messagebox.askyesno("Reset to Defaults", "Overwrite levels.bin with default levels?"):
            return
        try:
            bootstrap_levels()
            self.levels = read_levels_file(LEVELS_PATH)
            self.modified_level_ids.clear()
            self.selected_index = 0
            self._refresh_level_list()
            self.status_var.set(f"Reset defaults in {LEVELS_PATH}")
        except Exception as exc:
            messagebox.showerror("Reset failed", str(exc))

    def _refresh_level_list(self) -> None:
        self.level_list.delete(0, tk.END)
        for rec in self.levels:
            self.level_list.insert(tk.END, rec.name)

        if not self.levels:
            self.selected_index = -1
            self._render_canvas()
            return

        if self.selected_index < 0:
            self.selected_index = 0
        if self.selected_index >= len(self.levels):
            self.selected_index = len(self.levels) - 1

        self.level_list.selection_clear(0, tk.END)
        self.level_list.selection_set(self.selected_index)
        self.level_list.see(self.selected_index)
        self._sync_tint_from_level()
        self._render_canvas()

    def _sync_tint_from_level(self) -> None:
        rec = self.current_level
        if rec is None:
            return
        self.tint_var.set(argb_to_hex(rec.wall_tint_argb))

    def _on_select_level(self, _event: tk.Event | None = None) -> None:
        selection = self.level_list.curselection()
        if not selection:
            return
        self.selected_index = int(selection[0])
        self._sync_tint_from_level()
        self._render_canvas()

    def _add_level(self) -> None:
        name = simpledialog.askstring("Add level", "Level name:", parent=self.root)
        if name is None:
            return

        tiles = bytearray(MAP_ROWS * MAP_COLS)
        tiles[1 * MAP_COLS + 1] = TILE_PACMAN_SPAWN
        tiles[1 * MAP_COLS + 2] = TILE_GHOST_SPAWN
        tint = DEFAULT_TINTS[len(self.levels) % len(DEFAULT_TINTS)]
        rec = LevelRecord(
            name=name,
            wall_tint_argb=tint,
            tiles=tiles,
        )
        self.levels.append(rec)
        self.modified_level_ids.add(id(rec))
        self.selected_index = len(self.levels) - 1
        self._refresh_level_list()

    def _clone_level(self) -> None:
        src = self.current_level
        if src is None:
            return

        clone_name = self._next_clone_name(src.name)
        clone = LevelRecord(
            name=clone_name,
            wall_tint_argb=src.wall_tint_argb,
            tiles=bytearray(src.tiles),
        )
        insert_at = self.selected_index + 1
        self.levels.insert(insert_at, clone)
        self.modified_level_ids.add(id(clone))
        self.selected_index = insert_at
        self._refresh_level_list()

    def _next_clone_name(self, base_name: str) -> str:
        existing = {m.name for m in self.levels}
        candidate = f"{base_name} Copy"
        if candidate not in existing:
            return candidate
        n = 2
        while True:
            candidate = f"{base_name} Copy {n}"
            if candidate not in existing:
                return candidate
            n += 1

    def _rename_level(self) -> None:
        rec = self.current_level
        if rec is None:
            return
        name = simpledialog.askstring("Rename level", "Level name:", initialvalue=rec.name, parent=self.root)
        if name is None:
            return
        rec.name = name
        self.modified_level_ids.add(id(rec))
        self._refresh_level_list()

    def _remove_level(self) -> None:
        if self.current_level is None:
            return
        if len(self.levels) <= 1:
            messagebox.showwarning("Cannot remove", "At least one level is required.")
            return
        del self.levels[self.selected_index]
        if self.selected_index >= len(self.levels):
            self.selected_index = len(self.levels) - 1
        self._refresh_level_list()

    def _move_level(self, direction: int) -> None:
        if self.current_level is None:
            return
        src = self.selected_index
        dst = src + direction
        if dst < 0 or dst >= len(self.levels):
            return
        moving = self.levels.pop(src)
        self.levels.insert(dst, moving)
        self.modified_level_ids.add(id(moving))
        self.selected_index = dst
        self._refresh_level_list()

    def _pick_tint(self) -> None:
        rec = self.current_level
        if rec is None:
            return
        _rgb, hex_color = colorchooser.askcolor(color=argb_to_hex(rec.wall_tint_argb), parent=self.root)
        if not hex_color:
            return
        self.tint_var.set(hex_color.upper())
        self._apply_tint()

    def _on_tint_entry_commit(self, _event: tk.Event | None = None) -> None:
        self._apply_tint()

    def _apply_tint(self) -> None:
        rec = self.current_level
        if rec is None:
            return
        try:
            rec.wall_tint_argb = hex_to_argb(self.tint_var.get())
        except Exception as exc:
            messagebox.showerror("Invalid color", str(exc))
            return
        self.modified_level_ids.add(id(rec))
        self._render_canvas()

    def _on_canvas_down(self, event: tk.Event) -> None:
        self.dragging = True
        self._apply_tool_at(event.x, event.y)

    def _on_canvas_drag(self, event: tk.Event) -> None:
        if not self.dragging:
            return
        self._apply_tool_at(event.x, event.y)

    def _on_canvas_up(self, _event: tk.Event) -> None:
        self.dragging = False

    def _on_key_shortcut(self, event: tk.Event) -> None:
        widget = self.root.focus_get()
        if isinstance(widget, tk.Entry):
            return
        if isinstance(widget, ttk.Entry):
            return

        tool = self.tool_shortcuts.get(event.char)
        if tool is not None:
            self.current_tool.set(tool)

    def _apply_tool_at(self, x: int, y: int) -> None:
        rec = self.current_level
        if rec is None:
            return

        col = x // TILE_SIZE
        row = y // TILE_SIZE
        if row < 0 or row >= MAP_ROWS or col < 0 or col >= MAP_COLS:
            return

        err = apply_tool(rec, row, col, self.current_tool.get())
        if err:
            self.status_var.set(err)
            return

        self.modified_level_ids.add(id(rec))
        self._render_canvas()

    def _init_tileset_images(self) -> None:
        if self.tileset_image is not None:
            return
        self.tileset_image, self.wall_tile_images = init_wall_tile_images(str(TILESET_PATH))

    def _init_wall_previews(self) -> None:
        if self.wall_preview_images:
            return
        for mask in range(16):
            self.wall_preview_images[mask] = make_wall_preview(self.wall_tile_images[mask])

    def _render_canvas(self) -> None:
        self.canvas.delete("all")
        rec = self.current_level
        if rec is None:
            return
        render_canvas(self.canvas, rec, self.wall_tile_images, self.tinted_wall_cache)


def main() -> None:
    root = tk.Tk()
    LevelEditor(root)
    root.mainloop()
