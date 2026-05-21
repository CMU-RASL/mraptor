"""
  Backend interface used by the Qt UI.
"""
from __future__ import annotations
import os
from dataclasses import dataclass
from typing import Callable, Optional, Protocol, runtime_checkable
import os, getpass, socket

import IPC
from models import ProcessInfo

def daemon_from_msg(msg_name):
    # msg name is "mr_d_<daemon>_<pid>_to..."
    # return d_<daemon>_<pid>
    return msg_name.split('_to')[0][3:]

def remove_pid(daemon):
    return daemon[:daemon.rindex('_')]

def static_ping_handler(_, moduleName, clawBackend):
    clawBackend._ping_handler(moduleName)

def static_ack_handler(_, moduleName, clawBackend):
    clawBackend._ack_handler(moduleName)

def static_message_handler(msg_ref, text, clawBackend):
    daemon = daemon_from_msg(IPC.IPC_msgInstanceName(msg_ref))
    clawBackend._message_handler(daemon, text)

class Connection:
    def __init__(self, daemon, client_backend):
        client = client_backend.moduleName
        self.daemon = daemon
        self.client = client        
        self.in_msg = f"mr_{daemon}_to_{client}"
        self.out_msg = f"mr_{client}_to_{daemon}"
        IPC.IPC_defineMsg(self.in_msg, IPC.IPC_VARIABLE_LENGTH, "string")
        IPC.IPC_defineMsg(self.out_msg, IPC.IPC_VARIABLE_LENGTH, "string")
        IPC.IPC_subscribeData(self.in_msg, static_message_handler, client_backend)

    def send_msg(self, msg):
        IPC.IPC_publishData(self.out_msg, msg)

@dataclass#(slots=True)
class BackendCallbacks:
    """Callbacks a backend uses to publish events into the UI.

    Backends should call these from the GUI thread.  If a backend receives data
    on a worker thread, it should marshal back to Qt first, or the UI wrapper
    should do so with signals.
    """
    process_changed: Callable[[ProcessInfo], None]
    output_received: Callable[[str, str, str], None]
    message_received: Callable[[str], None]
    disconnected: Callable[[str], None]


class ClawBackend:
    def __init__(self, verbose=False) -> None:
        self.callbacks = None
        self.processes = []
        self.connections = {}
        self.verbose = verbose
        # Connect to IPC & subscribe
        centralhost = os.environ.get('CENTRALHOST','localhost:1382')
        hostname = socket.gethostname().split('.')[0]
        self.moduleName = f"c_{getpass.getuser()}@{hostname}_{os.getpid()}"
        IPC.IPC_connectModule(self.moduleName, centralhost)
        IPC.IPC_defineMsg("mr_search_ping", IPC.IPC_VARIABLE_LENGTH, "string")
        IPC.IPC_defineMsg("mr_search_ack", IPC.IPC_VARIABLE_LENGTH, "string")
        IPC.IPC_subscribeData("mr_search_ping", static_ping_handler, self)
        IPC.IPC_subscribeData("mr_search_ack", static_ack_handler, self)
        IPC.IPC_publishData("mr_search_ping", self.moduleName)

    def start(self, callbacks: BackendCallbacks) -> None:
        """Connect to the backend and retain callbacks for future events."""
        self.callbacks = callbacks
        callbacks.message_received("backend connected.")

    def stop(self) -> None:
        """Release backend resources."""
        self.callbacks = None

    def _ping_handler(self, moduleName):
        if self.verbose: print("PING_HANDLER:", moduleName)
        if (moduleName != self.moduleName and 
            moduleName not in self.connections):
            self.try_add(moduleName)
            IPC.IPC_publishData("mr_search_ping", self.moduleName);

    def _ack_handler(self, moduleName):
        if self.verbose: print("ACK_HANDLER:", moduleName)
        if (moduleName != self.moduleName and 
            moduleName not in self.connections):
            self._try_add(moduleName)

    def _try_add(self, daemonName):
        # The C++ version does fancy stuff - don't think we need it here
        self.connections[daemonName] = Connection(daemonName, self)

    def _message_handler(self, daemon, lines):
        if self.verbose: print("message_handler:", daemon, lines)
        for line in lines.split('\n'):
            space_idx = line.find(' ')
            cmd, text = line[:space_idx], line[space_idx+1:]
            if cmd == 'stdout':
                self._updateOutput(daemon, text)
            elif cmd == 'config':
                print("Received config")
            elif cmd == 'response':
                if text != 'ok':
                    self.callbacks.message_received(text)
            elif cmd == 'status':
                self._updateProcessStatus(daemon, text)
                
    def _getProcessStatuses(self):
        for connection in self.connections.values():
            connection.send_msg("get status -a")

    def _updateProcessStatus(self, machine, proc_status):
        # proc_status of the form {name="<x>", status="<y>""}
        name = proc_status.split('"')[1]
        status = proc_status.split('"')[-2]
        self.callbacks.process_changed(ProcessInfo(machine=machine, name=name,
                                                   status=status, idle=""))

    buffer = ""
    def _updateOutput(self, machine, text):
        # text is of form: process time.time x/c % line
        # x/c indicates line breaks ('c' indicates the line is continued)
        delim_idx = text.index(' %')
        prologue, msg = text[:delim_idx], text[delim_idx+2:]
        process, time, eol = prologue.split(' ')
        if eol == 'c':
            self.buffer += msg
        else:
            if len(self.buffer) > 0: 
                msg = self.buffer + msg
                self.buffer = ""
            self.callbacks.output_received(machine, process, msg)

    def _sendToConnection(self, machine, msg):
        connection = (self.connections.get(machine) if machine is not None else
                      next(iter(self.connections.values()))) # Use default machine
        if connection is None:
            self.callbacks.message_received(f"No existing daemon connection!")
        else:
            connection.send_msg(msg)

    count = 0
    def poll(self) -> None:
        if self.count%20 == 0:
            self._getProcessStatuses()
            self.count = 0
        self.count += 1
        # IPC here
        IPC.IPC_listenWait(100)

    def run_process(self, machine: Optional[str], process: Optional[str]) -> bool:
        self._sendToConnection(machine, f"run {process}")
        return True

    def kill_process(self, machine: Optional[str], process: Optional[str]) -> bool:
        self._sendToConnection(machine, f"kill {process}")
        return True

    def signal_process(self, machine: str, process: str, signal_name: str) -> bool:
        self._sendToConnection(machine, f"signal {signal_name} {process}")
        return True

    def subscribe(self, machine: str, process: str, playback_lines: int = 100) -> bool:
        self._sendToConnection(machine, f"sub stdout {process}")
        return True

    def unsubscribe(self, machine: str, process: str) -> bool:
        self._sendToConnection(machine, f"unsub stdout {process}")
        return True

    def send_stdin(self, machine: str, process: str, text: str) -> bool:
        text = text.replace("'", "\\'").replace('"', '\\"')
        self._sendToConnection(machine, f'stdin {process} "{text}"')
        return True

    def request_status(self, machine: Optional[str] = None, process: Optional[str] = None) -> bool:
        self._sendToConnection(machine, f"get status {process}")
        return True

    def load_config(self, filename:str) -> bool:
        if self.callbacks:
            self.callbacks.message_received(f"Load config requested: {filename}")
        return True 
