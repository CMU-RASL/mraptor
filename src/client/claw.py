from __future__ import annotations

import sys

from PySide6.QtWidgets import QApplication
from backend import ClawBackend
from main_window import ClawMainWindow


def main() -> int:
    app = QApplication(sys.argv)
    window = ClawMainWindow(ClawBackend())
    window.show()
    return app.exec()


if __name__ == "__main__":
    raise SystemExit(main())
