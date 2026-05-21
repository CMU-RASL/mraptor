from __future__ import annotations

import re
from dataclasses import dataclass

from PySide6.QtCore import Signal
from PySide6.QtWidgets import (
    QCheckBox,
    QHBoxLayout,
    QLabel,
    QLineEdit,
    QPlainTextEdit,
    QPushButton,
    QVBoxLayout,
    QWidget,
)

from models import ProcessInfo


@dataclass#(slots=True)
class ProcessCommand:
    machine: str
    process: str
    text: str


class ProcessPane(QWidget):
    run_requested = Signal(str, str)
    kill_requested = Signal(str, str)
    subscribe_requested = Signal(str, str)
    unsubscribe_requested = Signal(str, str)
    stdin_requested = Signal(str, str, str)
    hide_requested = Signal(str, str)

    def __init__(self, process: ProcessInfo) -> None:
        super().__init__()
        self.process = process
        self._raw_lines: list[str] = []

        self.title = QLabel(process.title)
        self.status = QLabel(process.status or "unknown")
        self.idle = QLabel(process.idle)

        self.output = QPlainTextEdit(readOnly=True)
        self.stdin = QLineEdit()
        self.stdin.setPlaceholderText("Send stdin to process and press Enter")
        self.stdin.returnPressed.connect(self._send_stdin)

        self.filter_enabled = QCheckBox("Filter")
        self.filter_text = QLineEdit()
        self.filter_text.setPlaceholderText("Python regular expression")
        self.filter_enabled.toggled.connect(self._refresh_output)
        self.filter_text.textChanged.connect(self._refresh_output)

        run_button = QPushButton("Run")
        kill_button = QPushButton("Kill")
        sub_button = QPushButton("Sub")
        unsub_button = QPushButton("Unsub")
        hide_button = QPushButton("Hide")

        run_button.clicked.connect(lambda: self.run_requested.emit(process.machine, process.name))
        kill_button.clicked.connect(lambda: self.kill_requested.emit(process.machine, process.name))
        sub_button.clicked.connect(lambda: self.subscribe_requested.emit(process.machine, process.name))
        unsub_button.clicked.connect(lambda: self.unsubscribe_requested.emit(process.machine, process.name))
        hide_button.clicked.connect(lambda: self.hide_requested.emit(process.machine, process.name))

        header = QHBoxLayout()
        header.addWidget(self.title, 2)
        header.addWidget(QLabel("State:"))
        header.addWidget(self.status)
        header.addWidget(QLabel("Idle:"))
        header.addWidget(self.idle)
        header.addStretch(1)
        for button in (run_button, kill_button, sub_button, unsub_button, hide_button):
            header.addWidget(button)

        filter_row = QHBoxLayout()
        filter_row.addWidget(self.filter_enabled)
        filter_row.addWidget(self.filter_text)

        layout = QVBoxLayout(self)
        layout.addLayout(header)
        layout.addWidget(self.output)
        layout.addLayout(filter_row)
        layout.addWidget(self.stdin)

    def update_process(self, process: ProcessInfo) -> None:
        self.process = process
        self.title.setText(process.title)
        self.status.setText(process.status or "unknown")
        self.idle.setText(process.idle)

    def append_output(self, text: str) -> None:
        self._raw_lines.extend(text.splitlines())
        if text.endswith("\n") and (not self._raw_lines or self._raw_lines[-1] != ""):
            pass
        self._refresh_output()

    def _refresh_output(self) -> None:
        lines = self._raw_lines
        if self.filter_enabled.isChecked() and self.filter_text.text():
            try:
                pattern = re.compile(self.filter_text.text())
                lines = [line for line in self._raw_lines if pattern.search(line)]
                self.filter_text.setToolTip("")
            except re.error as exc:
                self.filter_text.setToolTip(str(exc))
        old_scroll = self.output.verticalScrollBar().value()
        at_bottom = old_scroll == self.output.verticalScrollBar().maximum()
        self.output.setPlainText("\n".join(lines))
        if at_bottom:
            self.output.verticalScrollBar().setValue(self.output.verticalScrollBar().maximum())

    def _send_stdin(self) -> None:
        text = self.stdin.text()
        if not text:
            return
        self.stdin_requested.emit(self.process.machine, self.process.name, text)
        self.stdin.clear()
