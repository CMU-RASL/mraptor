#! /usr/bin/env python3
from __future__ import annotations

import sys
from fileinput import filename
from typing import Optional, Tuple

from PySide6.QtCore import QModelIndex, QPoint, Qt, QTimer
from PySide6.QtGui import QAction, QStandardItem, QStandardItemModel, QColor
from PySide6.QtWidgets import (QAbstractItemView, QApplication, QFileDialog,
                               QMainWindow, QMenu, QMessageBox, QPlainTextEdit,
                               QSplitter, QTabWidget, QTreeView, QWidget,
                               QHeaderView,)

from backend import ClawBackend, BackendCallbacks
from models import NodeType, ProcessInfo, ProcessRegistry, GroupInfo
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
        self._group_rows: dict[Tuple[str, str], QStandardItem] = {}
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
        self.model.setHorizontalHeaderLabels(["Process", "Status"])
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
            groups_received=self.on_groups_received,
        )
        self.backend.start(callbacks)

        self.poll_timer = QTimer(self)
        self.poll_timer.timeout.connect(self.backend.poll)
        self.poll_timer.start(25)

        self.status_timer = QTimer(self)
        self.status_timer.timeout.connect(self._getAllStatuses)
        self.status_timer.start(1000)

        # Reallocate the process/status pane size
        QTimer.singleShot(0, self._resize_tree_columns)

        self.tree.setExpandsOnDoubleClick(False)
        self.tree.clicked.connect(self._tree_clicked)

    def _getAllStatuses(self):
        for machine in self._machine_rows:
            self.backend.request_status(machine, "-a")

    def _runAll(self):
        for machine in self._machine_rows:
            self.backend.run_process(machine, "-a")

    def _killAll(self):
        for machine in self._machine_rows:
            self.backend.kill_process(machine, "-a")

    def _resize_tree_columns(self):
        header = self.tree.header()
        pane_width = self.tree.viewport().width()
        header.resizeSection(0, int(0.65 * pane_width))  # Process
        header.resizeSection(1, int(0.35 * pane_width)) # Status

    def resizeEvent(self, event) -> None:
        super().resizeEvent(event)
        self._resize_tree_columns()

    def _tree_clicked(self, index: QModelIndex) -> None:
        if not index.isValid():
            return

        item = self.model.itemFromIndex(index)
        node_type = item.data(ROLE_NODE_TYPE)
        if node_type == NodeType.GROUP.value:
            self.tree.setExpanded(index, not self.tree.isExpanded(index))
        elif node_type == NodeType.PROCESS.value:
            proc = self.registry.get_process(item.data(ROLE_MACHINE), item.data(ROLE_PROCESS))
            if proc: self.view_process(proc)

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
        process_menu.addAction("Run all", self._runAll)
        process_menu.addAction("Kill all", self._killAll)

    status_colors = {"not_started": QColor("gray"), "running": QColor("green"),
                     "signal_exit": QColor("red"),  "error_exit": QColor("red"),
                     "clean_exit": QColor("gray"),  "pending": QColor("orange"),
                     "starting": QColor("yellow"),  "group": QColor("blue"),}

    def on_process_changed(self, process: ProcessInfo) -> None:
        process = self.registry.upsert_process(process)
        machine_item = self._machine_rows.get(process.machine)
        if machine_item is None:
            machine_item = QStandardItem(process.machine)
            machine_item.setData(NodeType.MACHINE.value, ROLE_NODE_TYPE)
            machine_item.setData(process.machine, ROLE_MACHINE)
            self._machine_rows[process.machine] = machine_item
            self.model.appendRow([machine_item, QStandardItem("")])

        proc_item = self._tree_rows.get(process.key)
        if proc_item is None:
            proc_item = QStandardItem(process.name)
            proc_item.setData(NodeType.PROCESS.value, ROLE_NODE_TYPE)
            proc_item.setData(process.machine, ROLE_MACHINE)
            proc_item.setData(process.name, ROLE_PROCESS)
            status_item = QStandardItem(process.status)
            machine_item.appendRow([proc_item, status_item])
            self._tree_rows[process.key] = proc_item
            self.tree.expand(machine_item.index())
        else:
            status_item = proc_item.parent().child(proc_item.row(), 1)
            status_item.setText(process.status)

        # Set the font color according to status
        status_item.setForeground(self.status_colors[process.status])

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

    def on_groups_received(self, daemon: str, groups: List[GroupInfo]) -> None:
        machine_item = self._machine_rows.get(daemon)
        if machine_item is None:
            machine_item = QStandardItem(daemon)
            machine_item.setData(NodeType.MACHINE.value, ROLE_NODE_TYPE)
            machine_item.setData(daemon, ROLE_MACHINE)
            self._machine_rows[daemon] = machine_item
            self.model.appendRow([machine_item, QStandardItem("")])

        incoming_groups = {group.name for group in groups}

        for key in [k for k in self._group_rows if k[0] == daemon and k[1] not in incoming_groups]:
            group_item = self._group_rows.pop(key)
            parent = group_item.parent()
            if parent is not None:
                parent.removeRow(group_item.row())

        for group in groups:
            group_name, group_members = group.name, group.members
            group_item = self._group_rows.get((daemon, group_name))
            if group_item is None:
                group_item = QStandardItem(group_name)
                group_item.setData(NodeType.GROUP.value, ROLE_NODE_TYPE)
                group_item.setData(daemon, ROLE_MACHINE)
                group_item.setData(group_name, ROLE_PROCESS)
                status_item = QStandardItem("group")
                machine_item.appendRow([group_item, status_item])
                self._group_rows[(daemon, group_name)] = group_item
            else:
                status_item = group_item.parent().child(group_item.row(), 1)
                status_item.setText("group")
            # Set the font color according to status
            status_item.setForeground(self.status_colors["group"])

            self.registry.add_group(daemon, group_name, group_members)

    # Daemon has gone down - clean up on aisle 5
    def on_disconnected(self, daemon: str) -> None:
        self.on_message_received(f"WARNING: {daemon} disconnected!")
        # close all open tabs for this daemon
        doomed = [key for key in self._panes if key[0] == daemon]
        for machine, process_name in doomed:
            self.hide_process(machine, process_name)

        # remove the machine row from the tree
        machine_item = self._machine_rows.pop(daemon, None)
        if machine_item is not None:
            self.model.removeRow(machine_item.row())

        # remove process rows from the registry/tree caches
        doomed_rows = [key for key in self._tree_rows if key[0] == daemon]
        for key in doomed_rows:
            self._tree_rows.pop(key, None)

        # remove any group rows for this daemon
        doomed_groups = [key for key in self._group_rows if key[0] == daemon]
        for key in doomed_groups:
            self._group_rows.pop(key, None)

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
            return self.registry.get_process(item.data(ROLE_MACHINE),
                                             item.data(ROLE_PROCESS))
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

    def _processes_for_name(self, process_name: str) -> list[ProcessInfo]:
        results: list[ProcessInfo] = []
        seen: set[Tuple[str, str]] = set()
        for (machine, name), _item in self._tree_rows.items():
            if name != process_name:
                continue
            proc = self.registry.get_process(machine, name)
            if proc and proc.key not in seen:
                results.append(proc)
                seen.add(proc.key)
        return results

    def run_group(self, machine: str, group_name: str) -> None:
        group = self.registry.get_group(machine, group_name)
        if group is not None:
            for process_name in group.members:
                self.backend.run_process(machine, process_name)

    def kill_group(self, machine: str, group_name: str) -> None:
        group = self.registry.get_group(machine, group_name)
        if group is not None:
            for process_name in group.members:
                self.backend.kill_process(machine, process_name)

    def open_tree_menu(self, pos: QPoint) -> None:
        index = self.tree.indexAt(pos)
        item = self.model.itemFromIndex(index) if index.isValid() else None
        node_type = item.data(ROLE_NODE_TYPE) if item else None
        menu = QMenu(self)
        if node_type == NodeType.GROUP.value:
            group_name = item.text()
            daemon = item.parent().text()
            group = self.registry.get_group(daemon, group_name)
            member_text = "(none)" if group.members is None else ", ".join(group.members)  
            label = QAction("Procs: " + member_text, menu)
            label.setEnabled(False)
            menu.addAction(label)
            menu.addSeparator()
            menu.addAction("Run selected", lambda d=daemon, g=group_name: self.run_group(d, g))
            menu.addAction("Kill selected", lambda d=daemon, g=group_name: self.kill_group(d, g))
        elif node_type == NodeType.PROCESS.value:
            menu.addAction("View selected", self.view_selected)
            menu.addAction("Run selected", self.run_selected)
            menu.addAction("Kill selected", self.kill_selected)
            menu.addAction("Subscribe selected", self.subscribe_selected)
            menu.addAction("Unsubscribe selected", self.unsubscribe_selected)
            signal_menu = menu.addMenu("Signal selected")
            for signal_name in ("INT", "TERM", "KILL", "HUP"):
                signal_menu.addAction("SIG"+signal_name, lambda checked=False, s=signal_name: self.signal_selected(s))
        menu.exec(self.tree.viewport().mapToGlobal(pos))

    def signal_selected(self, signal_name: str) -> None:
        for proc in self._selected_processes():
            self.backend.signal_process(proc.machine, proc.name, signal_name)

def main() -> int:
    app = QApplication(sys.argv)
    window = ClawMainWindow(ClawBackend())
    window.show()
    return app.exec()


if __name__ == "__main__":
    raise SystemExit(main())
