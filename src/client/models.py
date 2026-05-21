from __future__ import annotations

from dataclasses import dataclass, field
from enum import Enum
from typing import Dict, Iterable, Optional, Tuple


class NodeType(str, Enum):
    MACHINE = "machine"
    GROUP = "group"
    PROCESS = "process"
    GROUP_MEMBER = "group_member"


@dataclass#(slots=True)
class ProcessInfo:
    name: str
    machine: str
    status: str = ""
    idle: str = ""
    subscribed: bool = False
    group: Optional[str] = None

    @property
    def key(self) -> Tuple[str, str]:
        return (self.machine, self.name)

    @property
    def title(self) -> str:
        return f"{self.name} @ {self.machine}"


@dataclass#(slots=True)
class GroupInfo:
    name: str
    members: set[Tuple[str, str]] = field(default_factory=set)


class ProcessRegistry:
    """In-memory replacement for the old GtkCTree row-data state."""

    def __init__(self) -> None:
        self.processes: Dict[Tuple[str, str], ProcessInfo] = {}
        self.groups: Dict[str, GroupInfo] = {}

    def upsert_process(self, process: ProcessInfo) -> ProcessInfo:
        existing = self.processes.get(process.key)
        if existing is None:
            self.processes[process.key] = process
            return process
        existing.status = process.status or existing.status
        existing.idle = process.idle or existing.idle
        existing.subscribed = process.subscribed
        existing.group = process.group or existing.group
        return existing

    def remove_process(self, machine: str, name: str) -> None:
        key = (machine, name)
        self.processes.pop(key, None)
        for group in self.groups.values():
            group.members.discard(key)

    def add_group(self, name: str, members: Iterable[Tuple[str, str]] = ()) -> GroupInfo:
        group = self.groups.setdefault(name, GroupInfo(name=name))
        group.members.update(members)
        return group

    def get_process(self, machine: str, name: str) -> Optional[ProcessInfo]:
        return self.processes.get((machine, name))

    def machines(self) -> list[str]:
        return sorted({machine for machine, _ in self.processes})
