"""
  Backend interface used by the Qt UI.
"""
from __future__ import annotations
from dataclasses import dataclass
from typing import Callable, Optional, Protocol, runtime_checkable
import os, getpass, socket, re, subprocess

import IPC
from models import ProcessInfo, GroupInfo

def daemon_from_in_msg(msg_name):
    # msg name is "mr_d_<daemon>_<pid>_to..."
    # return d_<daemon>_<pid>
    return msg_name.split('_to')[0][3:]

def daemon_from_out_msg(msg_name):
    # msg name is "mr_c_<module>_<pid>_to_d_<daemon>_<pid>"
    # return d_<daemon>_<pid>
    return msg_name.split('_to_')[1]

def remove_pid(daemon):
    return daemon[:daemon.rindex('_')]

def static_ping_handler(_, moduleName, clawBackend):
    clawBackend._ping_handler(moduleName)

def static_ack_handler(_, moduleName, clawBackend):
    clawBackend._ack_handler(moduleName)

def static_message_handler(msg_ref, text, connection):
    daemon = daemon_from_in_msg(IPC.IPC_msgInstanceName(msg_ref))
    connection.client._message_handler(daemon, text)

def static_handler_change(msg_name, num_handlers, connection):
    connection.client._handler_change(msg_name, num_handlers, connection)

class Connection:
    def __init__(self, daemon, client_backend):
        self.daemon = daemon
        self.client = client_backend
        client_name = client_backend.moduleName
        self.in_msg = f"mr_{daemon}_to_{client_name}"
        self.out_msg = f"mr_{client_name}_to_{daemon}"
        IPC.IPC_defineMsg(self.in_msg, IPC.IPC_VARIABLE_LENGTH, "string")
        IPC.IPC_defineMsg(self.out_msg, IPC.IPC_VARIABLE_LENGTH, "string")
        IPC.IPC_subscribeData(self.in_msg, static_message_handler, self)
        IPC.IPC_subscribeHandlerChange(self.out_msg, static_handler_change, self)
        self.current_num_handlers = IPC.IPC_numHandlers(self.out_msg)

    def send_msg(self, msg):
        IPC.IPC_publishData(self.out_msg, msg)

    def disconnect(self):
        IPC.IPC_unsubscribe(self.in_msg, static_message_handler)
        IPC.IPC_unsubscribeHandlerChange(self.out_msg, static_handler_change)

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
    groups_received: Callable[[str, [dict[str, list[str]]]], None]

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
        self.callbacks.message_received("backend connected.")

    def stop(self) -> None:
        """Release backend resources."""
        self.callbacks = None

    def _ping_handler(self, moduleName):
        if self.verbose: print("PING_HANDLER:", moduleName)
        if (moduleName != self.moduleName and 
            moduleName not in self.connections):
            self._add_connection(moduleName)
            IPC.IPC_publishData("mr_search_ping", self.moduleName);

    def _ack_handler(self, moduleName):
        if self.verbose: print("ACK_HANDLER:", moduleName)
        if (moduleName != self.moduleName and 
            moduleName not in self.connections):
            self._add_connection(moduleName)

    def _add_connection(self, daemonName):
        # The C++ version does fancy stuff - don't think we need it here
        self.connections[daemonName] = Connection(daemonName, self)

    def _remove_connection(self, daemonName):
        self.callbacks.disconnected(daemonName)
        del self.connections[daemonName]

    def _message_handler(self, daemon, lines):
        if self.verbose: print("message_handler:", daemon, lines)
        for line in lines.split('\n'):
            space_idx = line.find(' ')
            cmd, text = line[:space_idx], line[space_idx+1:]
            if cmd == 'stdout':
                self._updateOutput(daemon, text)
            elif cmd == 'config':
                # extract groups
                m = re.search(r'groups=\{(.*?)\},processes=', text, re.DOTALL)
                groups = [GroupInfo(daemon, key, value.split())
                          for key, value in re.findall(r'(\w+)="([^"]*)"', m.group(1))]
                if self.verbose: print("Got groups:", groups)
                self.callbacks.groups_received(daemon, groups)
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
                      None if (len(self.connections) == 0) else
                      next(iter(self.connections.values()))) # Use default machine
        if connection is None:
            self.callbacks.message_received(f"No existing daemon connection!")
        else:
            connection.send_msg(msg)

    def _handler_change(self, msg_name, num_handlers, connection):
        # apparently if a module quickly subscribes and then exits, the central
        # server can send us a spurious change of the number of handlers from
        # 0 to 0.  just ignore it.
        if connection.current_num_handlers != num_handlers:
            connection.current_num_handlers = num_handlers
            if num_handlers == 0:
                daemon = daemon_from_out_msg(msg_name)
                self._remove_connection(daemon)

    def poll(self) -> None:
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
        text = subprocess.run([filename], capture_output=True,
                              text=True, check=True).stdout
        self._sendToConnection(None, f'set config {text}')
        if self.callbacks:
            self.callbacks.message_received(f"Load config requested: {filename}")
        # Update the configuration locally
        self._sendToConnection(None, "get config -a")
        return True 
