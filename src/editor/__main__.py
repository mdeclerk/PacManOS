from __future__ import annotations

import sys


MIN_PYTHON = (3, 10)


def _check_runtime() -> None:
    if sys.version_info < MIN_PYTHON:
        major, minor = MIN_PYTHON
        current = f"{sys.version_info.major}.{sys.version_info.minor}.{sys.version_info.micro}"
        raise SystemExit(
            "PacManOS Level Editor requires Python "
            f"{major}.{minor}+ (detected {current})."
        )

    try:
        import tkinter  # noqa: F401
    except Exception as exc:
        raise SystemExit(
            "PacManOS Level Editor requires Tkinter (Tk), but it is not available "
            f"in this Python installation: {exc}"
        ) from exc


def main() -> None:
    _check_runtime()
    from .app import main as run_editor

    run_editor()


if __name__ == "__main__":
    main()
