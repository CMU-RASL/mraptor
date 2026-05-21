from __future__ import annotations

from fileinput import filename
from typing import Optional, Tuple

from PySide6.QtCore import QModelIndex, QPoint, Qt, QTimer
from PySide6.QtGui import QAction, QStandardItem, QStandardItemModel
from PySide6.QtWidgets import (
    QAbstractItemView,
    QApplication,
    QFileDialog,
    QMainWindow,
    QMenu,
    QMessageBox,
    QPlainTextEdit,
    QSplitter,
    QTabWidget,
    QTreeView,
    QWidget,
)

from backend import ClawBackend, BackendCallbacks
from models import NodeType, ProcessInfo, ProcessRegistry
from process_pane import ProcessPane


ROLE_NODE_TYPE = Qt.UserRole + 1
ROLE_MACHINE = Qt.UserRole + 2
ROLE_PROCESS = Qt.UserRole + 3


class ClawMainWindow(QMainWindow):
    def __init__(self, backend: Backend) -> None:
        super().__init__()
        self.backend = backend
        self.registry = ProcessRegistry()
        self._tree_rows: dict[Tuple[str, str], QStandardItem] = {}
        self._machine_rows: dict[str, QStandardItem] = {}
        self._panes: dict[Tuple[str, str], ProcessPane] = {}

        self.setWindowTitle("Claw")
        self.resize(1200, 800)

        self.tree = QTreeView()
        self.tree.setSelectionMode(QAbstractItemView.ExtendedSelection)
        self.tree.setContextMenuPolicy(Qt.CustomContextMenu)
        self.tree.customContextMenuRequested.connect(self.open_tree_menu)
        self.tree.doubleClicked.connect(self._view_index)

        self.model = QStandardItemModel()
        self.model.setHorizontalHeaderLabels(["Process", "State", "Idle"])
        self.tree.setModel(self.model)

        self.tabs = QTabWidget()
        self.tabs.setTabsClosable(True)
        self.tabs.tabCloseRequested.connect(self._close_tab)

        self.messages = QPlainTextEdit(readOnly=True)
        self.messages.setMaximumHeight(160)

        right = QSplitter(Qt.Vertical)
        right.addWidget(self.tabs)
        right.addWidget(self.messages)
        right.setStretchFactor(0, 4)
        right.setStretchFactor(1, 1)

        splitter = QSplitter()
        splitter.addWidget(self.tree)
        splitter.addWidget(right)
        splitter.setStretchFactor(0, 1)
        splitter.setStretchFactor(1, 4)
        self.setCentralWidget(splitter)

        self._build_menus()

        callbacks = BackendCallbacks(
            process_changed=self.on_process_changed,
            output_received=self.on_output_received,
            message_received=self.on_message_received,
            disconnected=self.on_disconnected,
        )
        self.backend.start(callbacks)

        self.poll_timer = QTimer(self)
        self.poll_timer.timeout.connect(self.backend.poll)
        self.poll_timer.start(25)

        self.status_timer = QTimer(self)
        self.status_timer.timeout.connect(lambda: self.backend.request_status(None, "-a"))
        self.status_timer.start(1000)

    def _build_menus(self) -> None:
        file_menu = self.menuBar().addMenu("File")
        load_action = QAction("Load", self)
        load_action.triggered.connect(self.load_config)
        file_menu.addAction(load_action)
        file_menu.addSeparator()
        quit_action = QAction("Quit", self)
        quit_action.triggered.connect(QApplication.instance().quit)
        file_menu.addAction(quit_action)

        process_menu = self.menuBar().addMenu("Process")
        process_menu.addAction("View selected", self.view_selected)
        process_menu.addAction("Run selected", self.run_selected)
        process_menu.addAction("Kill selected", self.kill_selected)
        process_menu.addAction("Subscribe selected", self.subscribe_selected)
        process_menu.addAction("Unsubscribe selected", self.unsubscribe_selected)
        process_menu.addSeparator()
        process_menu.addAction("Run all", lambda: self.backend.run_process(None, "-a"))
        process_menu.addAction("Kill all", lambda: self.backend.kill_process(None, "-a"))

    def on_process_changed(self, process: ProcessInfo) -> None:
        process = self.registry.upsert_process(process)
        machine_item = self._machine_rows.get(process.machine)
        if machine_item is None:
            machine_item = QStandardItem(process.machine)
            machine_item.setData(NodeType.MACHINE.value, ROLE_NODE_TYPE)
            machine_item.setData(process.machine, ROLE_MACHINE)
            self._machine_rows[process.machine] = machine_item
            self.model.appendRow([machine_item, QStandardItem(""), QStandardItem("")])

        proc_item = self._tree_rows.get(process.key)
        if proc_item is None:
            proc_item = QStandardItem(process.name)
            proc_item.setData(NodeType.PROCESS.value, ROLE_NODE_TYPE)
            proc_item.setData(process.machine, ROLE_MACHINE)
            proc_item.setData(process.name, ROLE_PROCESS)
            status_item = QStandardItem(process.status)
            idle_item = QStandardItem(process.idle)
            machine_item.appendRow([proc_item, status_item, idle_item])
            self._tree_rows[process.key] = proc_item
            self.tree.expand(machine_item.index())
        else:
            row = proc_item.row()
            parent = proc_item.parent()
            parent.child(row, 1).setText(process.status)
            parent.child(row, 2).setText(process.idle)

        pane = self._panes.get(process.key)
        if pane:
            pane.update_process(process)

    def on_output_received(self, machine: str, process_name: str, text: str) -> None:
        process = self.registry.get_process(machine, process_name)
        if process is None:
            process = self.registry.upsert_process(ProcessInfo(machine=machine, name=process_name))
            self.on_process_changed(process)
        pane = self._ensure_pane(process)
        pane.append_output(text)

    def on_message_received(self, message: str) -> None:
        self.messages.appendPlainText(message.rstrip())

    def on_disconnected(self, message: str) -> None:
        QMessageBox.critical(self, "Backend disconnected", message)
        QApplication.instance().quit()

    def load_config(self) -> None:
        filename, _ = QFileDialog.getOpenFileName(self, "Config File", "", "All Files (*.*)",
                                                  options=QFileDialog.DontUseNativeDialog)
        if filename:
            try:
                self.backend.load_config(filename)
            except Exception as exc:
                QMessageBox.critical(self, "Load Failed", str(exc))

    def _selected_processes(self) -> list[ProcessInfo]:
        results: list[ProcessInfo] = []
        seen: set[Tuple[str, str]] = set()
        for index in self.tree.selectionModel().selectedRows(0):
            proc = self._process_for_index(index)
            if proc and proc.key not in seen:
                results.append(proc)
                seen.add(proc.key)
        return results

    def _process_for_index(self, index: QModelIndex) -> Optional[ProcessInfo]:
        if not index.isValid():
            return None
        item = self.model.itemFromIndex(index)
        if item.data(ROLE_NODE_TYPE) == NodeType.PROCESS.value:
            return self.registry.get_process(item.data(ROLE_MACHINE), item.data(ROLE_PROCESS))
        return None

    def _view_index(self, index: QModelIndex) -> None:
        proc = self._process_for_index(index)
        if proc:
            self.view_process(proc)

    def _ensure_pane(self, process: ProcessInfo) -> ProcessPane:
        pane = self._panes.get(process.key)
        if pane is None:
            pane = ProcessPane(process)
            pane.run_requested.connect(self.backend.run_process)
            pane.kill_requested.connect(self.backend.kill_process)
            pane.subscribe_requested.connect(lambda m, p: self.backend.subscribe(m, p, 100))
            pane.unsubscribe_requested.connect(self.backend.unsubscribe)
            pane.stdin_requested.connect(self.backend.send_stdin)
            pane.hide_requested.connect(self.hide_process)
            self._panes[process.key] = pane
            self.tabs.addTab(pane, process.name)
        return pane

    def view_process(self, process: ProcessInfo) -> None:
        pane = self._ensure_pane(process)
        self.tabs.setCurrentWidget(pane)
        self.backend.subscribe(process.machine, process.name, 100)

    def view_selected(self) -> None:
        for proc in self._selected_processes():
            self.view_process(proc)

    def run_selected(self) -> None:
        for proc in self._selected_processes():
            self.backend.run_process(proc.machine, proc.name)

    def kill_selected(self) -> None:
        for proc in self._selected_processes():
            self.backend.kill_process(proc.machine, proc.name)

    def subscribe_selected(self) -> None:
        for proc in self._selected_processes():
            self.backend.subscribe(proc.machine, proc.name, 100)

    def unsubscribe_selected(self) -> None:
        for proc in self._selected_processes():
            self.backend.unsubscribe(proc.machine, proc.name)

    def hide_process(self, machine: str, process_name: str) -> None:
        pane = self._panes.pop((machine, process_name), None)
        if pane is not None:
            index = self.tabs.indexOf(pane)
            if index >= 0:
                self.tabs.removeTab(index)
            pane.deleteLater()
            self.backend.unsubscribe(machine, process_name)

    def _close_tab(self, index: int) -> None:
        widget = self.tabs.widget(index)
        if isinstance(widget, ProcessPane):
            self.hide_process(widget.process.machine, widget.process.name)
        else:
            self.tabs.removeTab(index)

    def open_tree_menu(self, pos: QPoint) -> None:
        menu = QMenu(self)
        menu.addAction("View selected", self.view_selected)
        menu.addAction("Run selected", self.run_selected)
        menu.addAction("Kill selected", self.kill_selected)
        menu.addAction("Subscribe selected", self.subscribe_selected)
        menu.addAction("Unsubscribe selected", self.unsubscribe_selected)
        signal_menu = menu.addMenu("Signal selected")
        for signal_name in ("SIGINT", "SIGTERM", "SIGKILL", "SIGHUP"):
            signal_menu.addAction(signal_name, lambda checked=False, s=signal_name: self.signal_selected(s))
        menu.exec(self.tree.viewport().mapToGlobal(pos))

    def signal_selected(self, signal_name: str) -> None:
        for proc in self._selected_processes():
            self.backend.signal_process(proc.machine, proc.name, signal_name)
