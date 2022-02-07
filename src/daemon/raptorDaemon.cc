/* $Revision: 1.122 $

   $Date: 2006/12/08 16:02:21 $

   $Author: trey $*/
/** @file raptorDaemon.cc

    DESCRIPTION: see mraptor_manual.txt

    */
  /*
  (c) Copyright 2003 CMU. All rights reserved.
*/

/**********************************************************************
 * INCLUDES
 **********************************************************************/

#include "raptorDaemon.h"
#include "mraptordInternal.h"
#include "mrSignals.h"

#ifdef REDHAT6_2
#include <signal.h>
#endif

/**********************************************************************
 * MACROS
 **********************************************************************/

#define DEBUG_DEPENDS (0)
// #define DEBUG_DEFUNCT (0) (now set in Build/header.mak)
#define DEBUG_IPC_CLEANUP (0)
#define MR_CLEANUP_PERIOD_SECS (0.5)
#define MR_FLUSH_COMM_PERIOD_SECS (0.5)
#define MR_RESET_CLEANUP_PERIOD_SECS (10.0)

/**********************************************************************
 * GLOBAL VARIABLES
 **********************************************************************/

Raptor_Daemon *active_daemon_g = NULL;

/**********************************************************************
 * RAPTOR_DAEMON STATIC VARS
 **********************************************************************/

#define MR_HTPAIR(name,func) \
  pair<string,Command_Handler>(name,&Raptor_Daemon::func)

// update num handlers if new handlers are added!
#define MR_NUM_HANDLERS (20)
pair<string,Command_Handler> Raptor_Daemon::_handler_table_temp[] = {
    // from clients
  MR_HTPAIR("set",        _handle_set_variable),
  MR_HTPAIR("get",        _handle_get_variable),
  MR_HTPAIR("sub",        _handle_subscribe),
  MR_HTPAIR("unsub",      _handle_unsubscribe),
  MR_HTPAIR("run",        _handle_run_process),
  MR_HTPAIR("run_slave",  _handle_run_slave),
  MR_HTPAIR("kill",       _handle_kill_process),
  MR_HTPAIR("restart",    _handle_restart_process),
  MR_HTPAIR("ipc",        _handle_send_ipc_message),
  MR_HTPAIR("dload",      _handle_load_config_file),
  MR_HTPAIR("stdin",      _handle_send_to_process_stdin),
  MR_HTPAIR("cancel",     _handle_cancel),
  MR_HTPAIR("shout",      _handle_shout),
  MR_HTPAIR("signal",     _handle_signal),

    // responses from other daemons
  MR_HTPAIR("response",   _handle_response),
  MR_HTPAIR("status",     _handle_status),

    // for users
  MR_HTPAIR("dhelp",      _handle_help),
  MR_HTPAIR("h",          _handle_help),
  MR_HTPAIR("dquit",      _handle_quit),
  MR_HTPAIR("q",          _handle_quit)
};
hash_map<string,Command_Handler > Raptor_Daemon::_handler_table
  (_handler_table_temp, &_handler_table_temp[MR_NUM_HANDLERS]);

bool Raptor_Daemon::_running = false;

/**********************************************************************
 * NON-MEMBER FUNCTIONS
 **********************************************************************/

string mr_to_string(const list<string>& A) {
  rcl::vector v;
  FOR_EACH(eltp, A) {
    v.push_back(*eltp);
  }
  return rcl::toStringCompact(v);
}

static void setSignalHandler(int sig, void (*handler)(int)) {
  struct sigaction act;
  memset (&act, 0, sizeof(act));
  act.sa_handler = handler;
  act.sa_flags = SA_RESTART;
  if (-1 == sigaction (sig, &act, NULL)) {
    cerr << "Runner Daemon:: ERROR: unable to set handler for signal "
	 << sig << endl;
    exit(EXIT_FAILURE);
  }
}

/**********************************************************************
 * RAPTOR_DAEMON FUNCTIONS
 **********************************************************************/

void Raptor_Daemon::_command_help(const string& which_client) {
#define _H(text) send_message(which_client, "help", string("%") + text)
  
_H("");
_H("commands understood by mraptord:");
_H("  cancel");
_H("    cancels the effects of unfinished run commands.  changes status");
_H("    of 'pending' processes to 'not_started'.");
_H("  dhelp");
_H("    shows help about daemon commands.");
_H("  dload my.config");
_H("    loads the given daemon-side config file.");
_H("  dquit");
_H("    quits the daemon, flushing all log files.");
_H("  get <status|config> [-a] ctr [ ctpan ... ]");
_H("  get clock");
_H("    gets information for listed processes, or -a to get all.  get clock");
_H("    returns the current daemon time.");
_H("  ipc message_name");
_H("    sends an ipc message with the given name (format 'NULL', no data)");
_H("  kill [-a] ctr [ ctpan ... ]");
_H("    kills listed processes, or if -a is specified, kills all procs.");
_H("  restart [-a] ctr [ ctpan ... ]");
_H("    restarts listed processes, or if -a is specified, restarts all.");
_H("  run [-a] ctr [ ctpan ... ]");
_H("    runs listed processes, or if -a is specified, runs all processes.");
_H("  set config {groups={...}, processes={...}}");
_H("  set config ctr {");
_H("                  cmd=/bin/ctr,");
_H("                  working_dir=/foo,");
_H("                  log_file=/foo,");
_H("                  env={DISPLAY=\":0.0\"},");
_H("                  ready=\"> \",");
_H("                  depends=[ctpan, foo]");
_H("                 }");
_H("    sets config options for the given process.  only the cmd field");
_H("    is required.");
_H("  shout <message>");
_H("    issues a shout message to all clients that have subscribed with 'sub shout'.");
_H("  signal INT ctr");
_H("    sends the specified signal to the process");
_H("  stdin ctr \"forward 50\"");
_H("    sends the given text to the stdin of a process.");
_H("  sub <status|config|clock|shout>");
_H("  sub stdout ctr [ 50 <time_stamp> ]");
_H("    subscribes to the given message type.  stdout is a special case:");
_H("    the numeric flag optionally specifies a number of past stdout lines");
_H("    to play back from the daemon's buffer; the time stamp suppresses");
_H("    old data from earlier than the specified time.");
_H("  unsub <status|config|clock>");
_H("  unsub stdout ctr");
_H("    unsubscribes to the given message type.");
}

void Raptor_Daemon::_prompt(void) {
  if (use_prompt) {
    cout << "> ";
    cout.flush();
  }
}

void Raptor_Daemon::_pipe_sig_handler(int x) {
  // should do something about the fact that this pipe is broken...
  // we know that the broken pipe is associated with _cur_writing_pid
}

Raptor_Daemon::Raptor_Daemon(void) {
  if (!_running) {
    // install signal handlers
    
#if 0
    setSignalHandler(SIGPIPE, &Raptor_Daemon::_pipe_sig_handler);
#endif
    
    setSignalHandler(SIGHUP, &Raptor_Daemon::_static_signal_handler);
    setSignalHandler(SIGINT, &Raptor_Daemon::_static_signal_handler);
    setSignalHandler(SIGTERM, &Raptor_Daemon::_static_signal_handler);

    // turns out adding a SIGCHLD handler can cause an obscure race condition.
    // we'll rely on a timer instead.
    //setSignalHandler(SIGCHLD, &Raptor_Daemon::_sig_cleanup_exited_children);
    
    make_signal_table();

    _running=true;
  } else {
    cerr << "There can be only one Runner Daemon per process!" << endl;
    exit(-1);
  }
  
  use_prompt = false;
  
  // construct initial config
  stored_config("groups") = rcl::map();
  stored_config("processes") = rcl::map();

  last_cleanup_exited_children_time = getTime();

  checking_pending_processes = false;
  
  rate_limit_max_lines = -1;
  rate_limit_max_lines_after_stdin = -1;
  rate_limit_period = -1.0;
}

Raptor_Daemon::~Raptor_Daemon() {
  // implement me
}

void Raptor_Daemon::_static_cleanup_exited_children(void *clientData)
{
#ifdef DEBUG_DEFUNCT
  static struct timeval tv = getTime();
  struct timeval cur = getTime();
  //fprintf(stderr, "td: %lf\n", timeDiff(cur, tv));
  if(timeDiff(cur, tv) >= 60.0) {
    tv = cur;
    fprintf(stderr, "DEBUG_DEFUNCT: _static_cleanup_exited_children\n");
  }
#endif
  Raptor_Daemon *d = (Raptor_Daemon *) clientData;
  d->_cleanup_exited_children();
}

void Raptor_Daemon::_cleanup_exited_children(void) {
  int status;
  struct rusage resource_usage;
  int pid;
  
  last_cleanup_exited_children_time = getTime();
  
#ifdef DEBUG_DEFUNCT
  static struct timeval tv = getTime();
  struct timeval cur = getTime();
  //fprintf(stderr, "td: %lf\n", timeDiff(cur, tv));
  if(timeDiff(cur, tv) >= 60.0) {
    tv = cur;
    fprintf(stderr, "DEBUG_DEFUNCT: _cleanup_exited_children\n");
  }
#endif
  try {
    while ((pid = safe_wait3(&status, WNOHANG, &resource_usage)) > 0) {
#if DEBUG_DEFUNCT
      cerr << "DEBUG_DEFUNCT: cleaning up pid " << pid << endl;
#endif
      Process_Record *proc = 0;
      FOR_EACH(i,proc_map) {
	if (i->second.get_pid() == pid) {
	  proc = &i->second;
	}
      }
      if (0 == proc) {
	throw MRException
	  (string("in cleanup_exited_children: couldn't find process with pid ")
	   + rcl::toString(pid));
      }
      
      proc->cleanup(status);
    }
  } catch (MRException e) {
    cerr << (e.is_warning ? "WARNING" : "ERROR") << ": " << e.text << endl;
  }
}

void Raptor_Daemon::static_perform_command(const string& which_client,
					   const string& text,
					   void *client_data)
{
  Raptor_Daemon *d = (Raptor_Daemon *) client_data;
  d->perform_command(which_client, text);
}

void Raptor_Daemon::connect(const string& module_name) {
  string other_module_type =
    MR_Comm::extract_type_from_module_name(module_name);
  if (other_module_type == "d") {
    string other_module_host =
      MR_Comm::extract_host_from_module_name(module_name);
    
    if (comm->get_connection_order() == MR_OTHER_MODULE_BEFORE_THIS_MODULE
	&& other_module_host == comm->get_this_host_name())
      {
	cerr << "ERROR: there is already a copy of mraptord running on local host "
	     << comm->get_this_host_name() << endl;
	exit(EXIT_FAILURE);
      }
    
    connected_daemons[other_module_host] = module_name;
    comm->send_message(module_name, "get status -a");
    comm->send_message(module_name, "sub status");
  }
}

void Raptor_Daemon::static_connect(const string& module_name,
				   void *client_data)
{
  Raptor_Daemon *d = (Raptor_Daemon *) client_data;
  d->connect(module_name);
}

void Raptor_Daemon::disconnect(const string& module_name) {
  string other_module_type = MR_Comm::extract_type_from_module_name(module_name);
  if (other_module_type == "d") {
    string other_module_host = MR_Comm::extract_host_from_module_name(module_name);
    connected_daemons.erase(other_module_host);
  }
}

void Raptor_Daemon::static_disconnect(const string& module_name,
				      void *client_data)
{
  Raptor_Daemon *d = (Raptor_Daemon *) client_data;
  d->disconnect(module_name);
}

void Raptor_Daemon::_options_help(void) {
  cerr <<
    "usage: mraptord OPTIONS\n"
    "  -h or --help\n"
    "     show this help\n"
    "  -p\n"
    "     show a prompt (nice for users, bad for client programs)\n"
    "  -i or --interactive\n"
    "     allow interaction on console (off by default because it\n"
    "     causes problems when the daemon is run in the background)\n"
    "  -c or --config\n"
    "     load the specified config file\n"
    "  -w or --wait-central\n"
    "     do not immediately connect to central on startup.  instead,\n"
    "     run central under microraptor and then connect.  a process\n"
    "     named 'central' must appear in the processes list of the\n"
    "     config file; this specifies how to run central.\n"
    "  -r or --run\n"
    "     run the given process or process group at startup.  this\n"
    "     option can only be specified after the -c option.  multiple\n"
    "     processes can be run with multiple instances of the -r option.\n"
    "  --host\n"
    "     make mraptord pretend it is running on the specified host\n"
    "  --centralhost <host> or --centralhost <host>:<port>\n"
    "     connect to the IPC central server on the specified host/port.\n"
    "  --rate-period <double>\n"
    "     Limit output to the client based on a period of <double> seconds.\n"
    "     Defaults to " << MR_RATE_LIMIT_PERIOD << ".\n"
    "  --rate-limit <int>\n"
    "     Send no more than <int> lines within a rate period to the clients.\n"
    "     Defaults to " << MR_RATE_LIMIT_MAX_LINES << ".\n"
    "  --rate-limit-stdin <int>\n"
    "     The same as --rate-limit, but applies if stdin input was sent to\n"
    "     the process during this or the last rate period.\n"
    "     Defaults to " << MR_RATE_LIMIT_MAX_LINES_AFTER_STDIN << ".\n"
    "  -d or --debug\n"
    "     output extra debugging messages to stdout\n";
  exit(1);
}

void Raptor_Daemon::run(int argc, char **argv) {
  active_daemon_g = this;
  
  use_prompt = false;
  this_host = "";
  wait_central = false;
  be_interactive = false;
  debug_mode = false;
  
  string initial_config_file = "";
  list<string> initial_run_commands;
  
  string arg;
  string host = "";
  char* endPtr; // For strtod error checking
  for (int argi=1; argi < argc; argi++) {
    arg = argv[argi];
    if (arg[0] == '-') {
      if (arg == "-h" || arg == "--help") {
	_options_help();
      } else if (arg == "-p" || arg == "--prompt") {
	use_prompt = true;
      } else if (arg == "-i" || arg == "--interactive") {
	be_interactive = true;
      } else if (arg == "-c" || arg == "--config") {
	if (++argi >= argc) {
	  cerr << "ERROR: -c with no argument" << endl << endl;
	  _options_help();
	}
	initial_config_file = argv[argi];
      } else if (arg == "-w" || arg == "--wait-central") {
	wait_central = true;
      } else if (arg == "-r" || arg == "--run") {
	if (++argi >= argc) {
	  cerr << "ERROR: -r with no argument" << endl << endl;
	  _options_help();
	}
	if (initial_config_file == "") {
	  cerr << "ERROR: -r can only be specified after -c" << endl << endl;
	  _options_help();
	}
	initial_run_commands.push_back( argv[argi] );
      } else if (arg == "--host") {
	if (++argi >= argc) {
	  cerr << "ERROR: --host with no argument" << endl << endl;
	  _options_help();
	}
	this_host = argv[argi];
      } else if (arg == "--centralhost") {
	if (++argi >= argc) {
	  cerr << "ERROR: --centralhost with no argument" << endl << endl;
	  _options_help();
	}
	setenv("CENTRALHOST", argv[argi], 1);
      } else if (arg == "--rate-period") {
	if (++argi >= argc) {
	  cerr << "ERROR: --rate-period with no argument" << endl << endl;
	  _options_help();
	}
        rate_limit_period = strtod(argv[argi], &endPtr);
        if(rate_limit_period == 0 && endPtr == argv[argi]) {
          cerr << "ERROR: --rate-period requires a numeric argument" << endl << endl;
          _options_help();
        }
      } else if (arg == "--rate-limit") {
	if (++argi >= argc) {
	  cerr << "ERROR: --rate-limit with no argument" << endl << endl;
	  _options_help();
	}
        rate_limit_max_lines = strtol(argv[argi], &endPtr, 10);
        if(rate_limit_max_lines == 0 && endPtr == argv[argi]) {
          cerr << "ERROR: --rate-limit requires a numeric argument" << endl << endl;
          _options_help();
        }
      } else if (arg == "--rate-limit-stdin") {
	if (++argi >= argc) {
	  cerr << "ERROR: --rate-limit-stdin with no argument" << endl << endl;
	  _options_help();
	}
        rate_limit_max_lines_after_stdin = strtol(argv[argi], &endPtr, 10);
        if(rate_limit_max_lines_after_stdin == 0 && endPtr == argv[argi]) {
          cerr << "ERROR: --rate-limit-stdin requires a numeric argument" << endl << endl;
          _options_help();
        }
      } else if (arg == "-d" || arg == "--debug") {
	debug_mode = true;
      } else {
	cerr << "ERROR: unknown option " << arg << endl << endl;
	_options_help();
      }
    } else {
      cerr << "ERROR: too many args" << endl << endl;
      _options_help();
    }
  }
  
  try {
    try {
      if (initial_config_file != "") {
	_load_config_file(initial_config_file);
      }
    } catch (MRException& e) {
      // rcl already printed an informative error message
      cout << "mraptord: parsing of initial config file failed, continuing without it" << endl;
    }
    
    // suppress debugging output
    IPCHelper::setVerbose(false);
    
    // d for daemon.  if this_host == "", MR_Comm will fill it in with the
    // default short host name.
    comm = new MR_Comm("d", this_host); 
    
    comm->handle_config_sync
      (fslot(this, &Raptor_Daemon::get_config),
       /* have_local_config = */ (initial_config_file != ""));
    
    if(be_interactive) {
      cout << "being interactive!" << endl;
      comm->connect_daemon_stdio("stdio");
    }
    
    comm->subscribe_messages(&Raptor_Daemon::static_perform_command, this);
    comm->subscribe_connect(&Raptor_Daemon::static_connect, this);
    comm->subscribe_disconnect(&Raptor_Daemon::static_disconnect, this);
    if (!wait_central) {
      comm->connect_to_central();
    }
    if (debug_mode) {
      comm->set_verbose_mode(true);
    }
    
    timers.addPeriodicTimer
      ("_static_cleanup_exited_children", MR_CLEANUP_PERIOD_SECS,
       &Raptor_Daemon::_static_cleanup_exited_children, this);
    timers.addPeriodicTimer
      ("_flush_comm", MR_FLUSH_COMM_PERIOD_SECS,
       &Raptor_Daemon::_flush_comm, this);
    timers.addPeriodicTimer
      ("_flush_logs", MR_LOG_FLUSH_PERIOD,
       &Raptor_Daemon::_static_flush_logs, this);
    timers.addPeriodicTimer
      ("_update_clock", MR_CLOCK_UPDATE_PERIOD,
       &Raptor_Daemon::_update_clock, this);
    timers.addPeriodicTimer
      ("_update_process_idle_time", MR_IDLE_UPDATE_PERIOD,
       &Raptor_Daemon::_update_process_idle_time, this);
    
    x_ipcRegisterExitProc(&Raptor_Daemon::_static_shutdown);
    
    try {
      // must execute central first; otherwise things hang
      if (wait_central) {
	list<string> just_central;
	just_central.push_back("central");
	_execute(just_central, /* from_daemon = */ false, /* which_client = */ "");
      }
      
      // then we can run other initial commands
      if (!initial_run_commands.empty()) {
	_execute(initial_run_commands, /* from_daemon = */ false,
		 /* which_client = */ "");
      }
    } catch (MRException e) {
      // if the -r option fails, just print the error.  it might
      // not be fatal.
      cerr << (e.is_warning ? "WARNING" : "ERROR") << ": " << e.text << endl;
    }
    
    _prompt();
    
#if DEBUG_TIMERS
    timers.setVerbose(true);
#endif
    while (1) {
      // HACK theoretically this block should have the same behavior as
      // IPC_listenWait, but IPC_listenWait seems to be buggy, so we'll
      // stick with this.
      {
	timeval startTime = getTime();
	IPC_listen( SEC_TO_MSEC(MR_TIMER_PERIOD_SECONDS) );
        // It's possible the clock got reset out from under us; let's be paranoid
        timeval doneTime = getTime();
        double elapsedTime = timeDiff( doneTime, startTime );
        if( elapsedTime >= 0 ) {
          double remainingTime = MR_TIMER_PERIOD_SECONDS - elapsedTime;
          if ( remainingTime > 0 ) {
            // remainingTime must be in the (0,0.1] range
            usleep( SEC_TO_USEC(remainingTime) );
          }
          // Eh, for kicks log things if IPC_listen took at least an
          // order of magnitude longer than it should have.  This
          // probably means the clock got reset.
          else if ( remainingTime < -10*MR_TIMER_PERIOD_SECONDS ) {
            cerr << "WARNING: We have detected a futureful temporal discontinuity:" << endl
                 << "         Start time was " << timevalToString(startTime) << endl
                 << "         End time was " << timevalToString(doneTime) << endl
                 << "         Remaining time was: " << remainingTime << endl;
          }
        } else {
          cerr << "WARNING: We have detected a historical temporal discontinuity:" << endl
               << "         Start time was " << timevalToString(startTime) << endl
               << "         End time was " << timevalToString(doneTime) << endl;
        }
      }

      timers.tick();
#if DEBUG_TIMERS
      timers.debugPrint();
#endif
      _ipc_cleanup_workaround();
    }
  } catch (MRException e) {
    // rcl already printed an informative error message
    _shutdown();
  }
}

void Raptor_Daemon::send_message(const string& which_client,
				 const string& message_type,
				 const string& message_text)
{
  comm->send_typed_message(which_client, message_type, message_text);
}

void Raptor_Daemon::send_command_response(const string& which_client,
					  int command_id,
					  const string& message_text)
{
  string text = message_text;
  if (-1 != command_id) {
    text = rcl::toString(command_id) + " " + text;
  }
  send_message(which_client, "response", text);
}

void Raptor_Daemon::_set_process_config(const string& process_name,
					const rcl::exp& proc_config)
{
  if (proc_map.end() == proc_map.find(process_name)) {
    proc_map[process_name] = new Process_Impl(process_name, this);
    proc_vec.push_back(proc_map[process_name]);
  }
  Process_Record& proc = proc_map[process_name];
  
  proc.set_config(proc_config);
}

void Raptor_Daemon::_handle_set_variable(const string& which_client,
					 int command_id,
					 const rcl::exp& e)
{
  /*
    
  syntax:
   1. set config <process_name> <process_config>
   2. set config <entire_config_file> [prior]
   3. set client <client_config>
   4. set buffer block

 examples:
   set config ctr { cmd=/bin/ctr, env={DISPLAY=":0.0", LD_LIBRARY_PATH=/lib} }
   set config {groups={normal="ctr ctpan"},
	       processes={ctr={cmd=ctr},ctpan={cmd=ctpan}}}
   set config {groups={normal="ctr ctpan"},
	       processes={ctr={cmd=ctr},ctpan={cmd=ctpan}}}
	      prior
   set client {DISPLAY => "clienthost:0.0"}
   set buffer block

   */

  string variable_name = e[1].getString();
  
  if (variable_name == "config") {
    if (e[2].getType() == RCL_MAP_TYPE) {
      // syntax case 2
      bool is_prior = (e[3].defined() && e[3].getString() == "prior");
      _update_for_new_config_global(e[2], is_prior);
    } else {
      string child_name = e[2].getString();
      // syntax case 1
      _update_for_new_config_process(child_name, e[3]);
    }
  } else if (variable_name == "client") {
    e[2].getMap(); // (error check)
    client_settings[which_client] = e[2];
  } else if (variable_name == "buffer") {
    if (e[2].getString() != "block") {
      throw MRException(string("set: unknown value ") + e[2].getString()
			+ " for variable buffer; the only legal value is 'block'");
    }
    comm->enable_output_buffering(which_client);
  } else {
    throw MRException(string("can't set unknown variable ") + variable_name);
  }
  send_command_response(which_client, command_id, "ok");
}

Process_Record Raptor_Daemon::get_process(const string& process_name,
					  const string& action_text,
					  bool must_be_local)
{
  if (proc_map.end() == proc_map.find(process_name)) {
    throw MRException
      (string("can't ") + action_text + " unknown process " + process_name);
  }
  Process_Record proc = proc_map[process_name];
  if (must_be_local && !proc.is_local()) {
    throw MRException
      (string("can't ") + action_text + " non-local process " + process_name);
  }
  return proc;
}

void Raptor_Daemon::_get_process_config(const string& which_client,
					const string& process_name)
{
  rcl::vector v;
  v.push_back(process_name);
  v.push_back(get_process(process_name, "get config for").get_config());
  send_message(which_client, "config", rcl::toString(v));
}

void Raptor_Daemon::_handle_get_variable(const string& which_client,
					 int command_id,
					 const rcl::exp& e)
{
   /*

 syntax:
   get <variable> <extra_args>
   1. get status -a
      get status <process1> [ <process2> ... ]
   2. get config -a
      get config <process1> [ <process2> ... ]
   3. get clock

 examples:
   get status -a
   get status ctr ctpan
   get config ctr
   get clock

   */

  string variable_name = e[1].getString();
  
  if (variable_name == "status") {
    
    /**********************************************************************
     * get status
     **********************************************************************/
    rcl::exp status;
    if (e[2].getString() == "-a") {
      FOR_EACH(proc, proc_vec) {
	if (proc->is_local()) {
	  status = proc->get_status_exp();
	  send_message(which_client, "status", rcl::toString(status));
	}
      }
    } else {
      for (int i=2; e[i].defined(); i++) {
	status = get_process(e[i].getString(), "get status of")
	  .get_status_exp();
	send_message(which_client, "status", rcl::toString(status));
      }
    }
  } else if (variable_name == "config") {
    
    /**********************************************************************
     * get config
     **********************************************************************/
    if (e[2].getString() == "-a") {
      send_message(which_client, "config", rcl::toString(stored_config));
    } else {
      for (int i=2; e[i].defined(); i++) {
	_get_process_config(which_client, e[i].getString());
      }
    }
  } else if (variable_name == "clock") {
    
    /**********************************************************************
     * get clock
     **********************************************************************/
    _update_clock(which_client);
    
  } else {
    
    /**********************************************************************
     * bad variable name
     **********************************************************************/
    throw MRException(string("can't get variable ") + variable_name);
    
  }
  
  send_command_response(which_client, command_id, "ok");
}

void Raptor_Daemon::_handle_run_process(const string& which_client,
					int command_id,
					const rcl::exp& e)
{
   /* valid syntax examples:

	run ctr
	run central ctr
	run -a

   */

  list<string> proc_or_group_list;
  string old_pending_procs;

  last_client_to_issue_run_command = which_client;
  old_pending_procs = "";
  FOR_EACH (eltp, procs_with_status[MR_PENDING]) {
    old_pending_procs += (*eltp) + " ";
  }

  // in new system, always run in parallel
  proc_or_group_list.push_back("-p");

  if (e[1].getString() == "-a") {
    FOR_EACH(proc, proc_map) {
      if (proc->second.is_local() && proc->second.is_runnable()) {
	proc_or_group_list.push_back(proc->first);
      }
      if (0 == proc_or_group_list.size()) {
	throw MRException("no non-running processes to run", MR_WARNING);
      }
    }
  } else {
    for (int i=1; e[i].defined(); i++) {
      proc_or_group_list.push_back(e[i].getString());
    }
  }
  
  // execute
  _execute(proc_or_group_list, /* from_daemon = */ false,
	   which_client);
  
  if (old_pending_procs != "") {
    send_command_response(which_client, command_id,
			  "warning %run command issued while processes were still pending from the last command; still pending processes were [" + old_pending_procs + "]");
  } else {
    send_command_response(which_client, command_id, "ok");
  }
}

void Raptor_Daemon::_handle_run_slave(const string& which_daemon,
				      int command_id,
				      const rcl::exp& e)
{
   /*

 syntax:
   run_slave <client> [-p] <process1> [ <process2> ... ]

 examples:
   run_slave c_sputnik_33708 ctr
   run_slave c_sputnik_33708 ctr ctpan
   run_slave c_sputnik_33708 -p ctr ctpan

   */

  string which_client = e[1].getString();
  
  list<string> proc_or_group_list;
  for (int i=2; e[i].defined(); i++) {
    proc_or_group_list.push_back(e[i].getString());
  }
  _execute(proc_or_group_list, /* from_daemon = */ true,
	   which_client);
  
  send_command_response(which_daemon, command_id, "ok");
}

void Raptor_Daemon::_execute(const string& proc,
			     const string& which_client)
{
  list<string> proc_or_group_list;
  proc_or_group_list.push_back(proc);
  _execute(proc_or_group_list, /* from_daemon = */ false, which_client);
}

void Raptor_Daemon::_execute(const list<string>& proc_or_group_list,
			     bool from_daemon, const string& which_client)
{
  try {
    /**********************************************************************
     * add proc_names and dependencies to pending processes
     **********************************************************************/
    
    list<string> proc_names;
    _expand_groups(proc_or_group_list, proc_names, "run");
#if DEBUG_DEPENDS
    if (debug_mode) {
      cerr << "DEBUG_DEPENDS: _execute: proc_names=" << mr_to_string(proc_names) << endl;
    }
#endif
    
#if 0
    // add any pending processes from last run command to proc_names
    // (this change fixes the "bogus dependency cycle" bug)

    // new on 13 Jun 2005 -- now that serial_depends is gone i don't
    //   think that the bogus dependency cycle bug will happen anymore,
    //   and this addition of pending_procs to proc_names causes weird
    //   behavior, so i'm tentatively removing it.
#if DEBUG_DEPENDS
    if (debug_mode) {
      cout << "DEBUG_DEPENDS: _execute: adding pending_procs to proc_names" << endl;
    }
#endif
    list<string>& pending_procs = procs_with_status[MR_PENDING];
    FOR_EACH (eltp, pending_procs) {
      set_insert( proc_names, *eltp );
    }
#endif
    
    queue<string> frontier;
    FOR_EACH(proc_name, proc_names) {
      frontier.push(*proc_name);
    }
    
    list<string> visited;
    while (!frontier.empty()) {
      string proc_name;
      
      proc_name = frontier.front();
      frontier.pop();
      
      Process_Record proc = proc_map[proc_name];
      
#if DEBUG_DEPENDS
      if (debug_mode) {
	cout << "DEBUG_DEPENDS: _execute: adding " << proc_name
	     << ", depends=" << mr_to_string(proc.impl->depends)
	     << endl;
      }
#endif
      
      // we need to set the process status to pending, and add all of
      // its dependencies to the frontier, *unless* either (1) we have
      // already visited this process, or (2) the process has already
      // been started
      if (! (set_contains( visited, proc_name )
	     || proc.impl->status == MR_STARTING
	     || proc.impl->status == MR_RUNNING)) {
	proc.impl->running_client = which_client;
	proc.set_status(MR_PENDING);
	set_insert( visited, proc_name );
	
	// add dependencies to the frontier, *unless* the run command came
	// from another daemon, in which case that daemon should have expanded
	// dependencies for us already.
	if (!from_daemon) {
	  FOR_EACH(depend_name, proc.impl->depends) {
	    if (proc_map.end() == proc_map.find(*depend_name)) {
	      throw MRException
		(string("process ") + proc_name + " depends on unknown process "
		 + (*depend_name));
	    }
	    frontier.push(*depend_name);
	  }
	}
      }
    }
#if DEBUG_DEPENDS
    if (debug_mode) {
      cerr << "DEBUG_DEPENDS: _execute: after step 1: pending procs = "
	   << mr_to_string(procs_with_status[MR_PENDING]) << endl;
    }
#endif
    
    /**********************************************************************
     * see if there is a viable schedule to run everything
     **********************************************************************/
    
    // if the run command came from a daemon, there should be a viable
    // schedule.
    if (!from_daemon) {
      // set simulated_pending_procs to be the processes that are both
      // (1) in visited and (2) in procs_with_status[MR_PENDING]
      list<string> simulated_pending_procs;
      set_intersection( simulated_pending_procs, visited, procs_with_status[MR_PENDING] );

      list<string> simulated_running_procs = procs_with_status[MR_RUNNING];
      list<string> run_schedule;
      
      FOR_EACH(eltp, procs_with_status[MR_STARTING]) {
	set_insert( simulated_running_procs, *eltp );
      }
      while (!simulated_pending_procs.empty()) {
	list<string> procs_to_remove;
	procs_to_remove.clear();
	FOR_EACH(eltp, simulated_pending_procs) {
	  Process_Record proc;
	  string process_name;
	  process_name = *eltp;
	  proc = proc_map[process_name];
	  
#if DEBUG_DEPENDS
	  if (debug_mode) {
	    cerr << "DEBUG_DEPENDS: _execute: process_name=" << process_name
		 << ", depends=" << mr_to_string(proc.impl->depends)
		 << ", simulated_running_procs="
		 << mr_to_string(simulated_running_procs)
		 << ", running_procs=" << mr_to_string(procs_with_status[MR_RUNNING])
		 << endl;
	  }
#endif
	  
	  if (subset( proc.impl->depends, simulated_running_procs )) {
	    // all dependencies satisfied.  "run" the process
	    run_schedule.push_back(process_name);
	    set_insert( simulated_running_procs, process_name );
	    // can't modify simulated_pending_procs because we're
	    // currently iterating through it, so mark this process for
	    // later removal.
	    procs_to_remove.push_back(process_name);
	  }
	}
	
	if (procs_to_remove.empty()) {
	  // doh!  there are still pending processes, but none of them have
	  // satisfied dependencies. there must be a dependency cycle.
	  rcl::vector v;
	  FOR_EACH(sproc, simulated_pending_procs) {
	    v.push_back(*sproc);

#if DEBUG_DEPENDS
	    if (debug_mode) {
	      string pname;
	      std::list<std::string> tmp, unsat_depends;
	      pname = *sproc;
	      Process_Record proc = proc_map[pname];
	      // unsat_depends = depends - simulated_running_procs
	      set_diff( unsat_depends, proc.impl->depends, simulated_running_procs );
	      cout << "DEBUG_DEPENDS: _execute (cycle): pname=" << pname
		   << " unsat_depends=" << mr_to_string(unsat_depends)
		   << endl;
	    }
#endif
	  }
	  throw MRException
	    (string("dependency cycle detected among processes ")
	     + rcl::toStringCompact(v));
	}
	
	// clean up marked processes
	FOR_EACH(process_name, procs_to_remove) {
	  set_erase( simulated_pending_procs, *process_name );
	}
      }	
      
      /**********************************************************************
       * notify other daemons
       **********************************************************************/
      
      // collect list of which daemons need to be commanded and which
      // processes need to be run
      list<string> daemons_to_command;
      FOR_EACH(process_name, run_schedule) {
	Process_Record proc;
	proc = proc_map[*process_name];
	if (!proc.is_local()) {
	  string module_name;
	  string proc_host;
	  proc_host = proc.impl->host;
	  if (connected_daemons.end() == connected_daemons.find(proc_host)) {
	    cerr << "Connected daemons: ";
	    FOR_EACH(pr,connected_daemons) {
	      cerr << pr->first << ", ";
	    }
	    cerr << endl;
	    throw MRException
	      (string("process ") + (*process_name)
	       + " is required, should run on unknown daemon for host "
	       + proc_host);
	  }
	  module_name = connected_daemons[proc_host];
	  set_insert( daemons_to_command, module_name );
	}
      }
      
      // form and send run commands
      string daemon_name;
      string cmd;
      rcl::vector v;
      v.push_back("run_slave");
      v.push_back(which_client);
      v.push_back("-p");
      FOR_EACH(process_name, run_schedule) {
	v.push_back(*process_name);
      }
      cmd = rcl::toString(v);
      FOR_EACH(eltp, daemons_to_command) {
	daemon_name = *eltp;
	//cerr << "send_message(" << daemon_name << "): " << cmd << endl;
	comm->send_message(daemon_name, cmd);
      }
    } // if (!from_daemon)
    
    /**********************************************************************
     * run anything with satisfied dependencies
     **********************************************************************/
    _check_pending_processes();
    
  } catch (MRException err) {
    _cancel_pending_processes();
    throw MRException(err.text, err.is_warning);
  }
}

void Raptor_Daemon::_cancel_pending_processes(void) {
  // make a copy to iterate through so that it doesn't hurt to modify
  // procs_with_status[MR_PENDING] during the loop
  list<string> pending_procs = procs_with_status[MR_PENDING];
  list<string> other_daemons;
  
  FOR_EACH(eltp, pending_procs) {
    Process_Record proc;
    proc = proc_map[*eltp];
    proc.set_status(MR_NOT_STARTED);
    if (!proc.is_local()) {
      set_insert( other_daemons, proc.impl->host );
    }
  }
  // forward the cancel on to other daemons as necessary
  FOR_EACH(eltp, other_daemons) {
    string proc_host;
    proc_host = *eltp;
    if (connected_daemons.end() != connected_daemons.find(proc_host)) {
#if 0
      cerr << "send_message(" << connected_daemons[proc_host]
	   << "): cancel" << endl;
#endif
      comm->send_message(connected_daemons[proc_host], "cancel");
    }
  }
}

// proc_or_group_list is a list of names which refer to either a process
// or a group.  _expand_groups() expands the groups and puts the result
// in proc_list.  throws an exception if a name is neither a process nor
// a group.
//
// _expand_groups acts like LISP macroexpand-1.  groups cannot contain
// other groups.  this avoids issues like detecting infinite regress.
// if you really want groups that contain other groups, you can do it
// using perl in the config file, like the following:
//
// $group1 = "a b c";
// ...
// groups => { group1 => $group1, group2 => "x $group1 y" }
void Raptor_Daemon::_expand_groups(const list<string>& proc_or_group_list,
				   list<string>& proc_list,
				   const string& calling_command)
{
  rcl::exp group_exp;
  string group_string;
  rcl::vector group_proc_vec;
  list<string> unchecked_proc_list;
  
  // first, expand any groups in the list and pass through non-groups
  FOR_EACH(proc_or_group, proc_or_group_list) {
    if (stored_config("groups").defined()
	&& (group_exp = stored_config("groups")(*proc_or_group)).defined()) {
      group_string = group_exp.getString();
      group_proc_vec = rcl::readFromString(group_string).getVector();
      FOR_EACH(proc_name_exp, group_proc_vec) {
	unchecked_proc_list.push_back(proc_name_exp->getString());
      }
    } else {
      unchecked_proc_list.push_back(*proc_or_group);
    }
  }
  
  // now everything in unchecked_proc_list should either be "-p" or a process.
  // check, and add actual processes to proc_list
  proc_list.clear();
  FOR_EACH(proc_name, unchecked_proc_list) {
    if (*proc_name == "-p") {
      /* serial_execution = false; */
    } else if (proc_map.end() == proc_map.find(*proc_name)) {
      throw MRException(string("can't ") + calling_command + " " + (*proc_name)
			+ ", which is neither a process nor a group");
    } else {
      proc_list.push_back(*proc_name);
    }
  }
}

void Raptor_Daemon::_check_pending_processes(void) {
  // checking_pending_processes serves as a guard so that
  //   _check_pending_processes cannot indirectly call itself
  if (checking_pending_processes) return;
  checking_pending_processes = true;

  // each call to _check_pending_processes_internal may affect
  //   the list of pending processes, so we need to keep cycling
  //   until there are no more changes
  while (_check_pending_processes_internal());

  checking_pending_processes = false;
}

// returns true if any processes were executed
bool Raptor_Daemon::_check_pending_processes_internal(void) {
  // make a copy to iterate through so that it doesn't hurt to modify
  // procs_with_status[MR_PENDING] during the loop
  list<string> pending_procs = procs_with_status[MR_PENDING];
  int num_executed_processes = 0;
  
  FOR_EACH (eltp, pending_procs) {
    Process_Record proc = proc_map[*eltp];
    if (proc.is_local() && proc.get_status() == MR_PENDING) {
#if DEBUG_DEPENDS
      if (debug_mode) {
	cerr << "DEBUG_DEPENDS: _check_pending_processes: check_pending: name=" << *eltp
	     << ", depends=" << mr_to_string(proc.impl->depends)
	     << ", running_procs=" << mr_to_string(procs_with_status[MR_RUNNING])
	     << endl;
      }
#endif
      if (subset( proc.impl->depends, procs_with_status[MR_RUNNING] ))
	{
	  //cerr << "executing " << pr->first << endl;
	  proc.execute(last_client_to_issue_run_command);
	  num_executed_processes++;
	}
      else {
	// if this is the first time we are checking whether proc is
	// eligible for execution and it isn't, then report it has status
	// pending.
	if (proc.impl->need_to_report_pending) {
	  send_message(MR_SUBSCRIBED_MODULES, "status",
		       rcl::toString(proc.get_status_exp()));
	  proc.impl->need_to_report_pending = false;
	}
      }
    }
  }

  return (num_executed_processes > 0);
}

void Raptor_Daemon::_kill_processes(const string& which_client,
				    const list<string>& proc_or_group_list,
				    bool do_restart)
{
  // expand any groups
  list<string> proc_names;
  string calling_command = do_restart ? "restart" : "kill";
  _expand_groups( proc_or_group_list, proc_names, calling_command );
  
  // kill what needs killin'
  FOR_EACH(proc_name, proc_names) {
    try {
      get_process(*proc_name, calling_command,
		  /* must_be_local = */ false)
	.kill(which_client, do_restart);
    } catch (MRException err) {
      cerr << "Failed to kill process " << *proc_name
	   << ": " << err.text << endl;
    }
  }
}

void Raptor_Daemon::_handle_kill_process(const string& which_client,
					 int command_id,
					 const rcl::exp& e)
{
   /* valid syntax examples:

	kill ctr
	kill central ctr
	kill -a

   */

  list<string> proc_or_group_list;
  if (e[1].getString() == "-a") {
    int num_killed = 0;
    FOR_EACH(pr, proc_map) {
      if (pr->second.is_local()) {
	if (!pr->second.is_runnable()) {
	  pr->second.kill(which_client);
	  num_killed++;
	}
      } else {
	proc_or_group_list.push_back(pr->first);
      }
    }
    if (0 == (num_killed + proc_or_group_list.size())) {
      throw MRException("no running processes to kill", MR_WARNING);
    }
  } else {
    // get the list of process or group names
    for (int i=1; e[i].defined(); i++) {
      proc_or_group_list.push_back(e[i].getString());
    }
  }

  if (!proc_or_group_list.empty()) {
    _kill_processes(which_client, proc_or_group_list);
  }
  
  send_command_response(which_client, command_id, "ok");
}

void Raptor_Daemon::_handle_restart_process(const string& which_client,
					    int command_id,
					    const rcl::exp& e)
{
   /* valid syntax examples:

	restart ctr
	restart central ctr
	restart -a

   */

  list<string> proc_or_group_list;
  if (e[1].getString() == "-a") {
    FOR_EACH(pr, proc_map) {
      proc_or_group_list.push_back(pr->first);
    }
  } else {
    // get the list of process or group names
    for (int i=1; e[i].defined(); i++) {
      proc_or_group_list.push_back(e[i].getString());
    }
  }

  _kill_processes(which_client, proc_or_group_list, /* do_restart = */ true);
  send_command_response(which_client, command_id, "ok");
}

void Raptor_Daemon::_handle_send_ipc_message(const string& which_client,
					     int command_id,
					     const rcl::exp& e)
{
  /* valid syntax examples:
     
	ipc my_message_name

  */
  string message_name = e[1].getString();
  IPCHelper::setVerbose(false);
  IPCHelper::defineMessage(message_name, "NULL");
  IPCHelper::setVerbose(true);
  IPC_publish(message_name.c_str(), 0, 0);
  
  send_command_response(which_client, command_id, "ok");
}

void Raptor_Daemon::_subscribe(const string& which_client,
			       const string& message_type)
{
  comm->add_subscriber(message_type, which_client);
}

void Raptor_Daemon::_static_flush_logs(void *client_data)
{
  Raptor_Daemon *d = (Raptor_Daemon *) client_data;
  d->_flush_logs();
}

void Raptor_Daemon::_flush_logs(void)
{
  FOR_EACH(pair, proc_map) {
    pair->second.log_flush();
  }
}

void Raptor_Daemon::_flush_comm(void *client_data)
{
  Raptor_Daemon *d = (Raptor_Daemon *) client_data;
  d->_flush_comm();
}


void Raptor_Daemon::_flush_comm(void)
{
  comm->maybe_flush_output_buffers();
}

void Raptor_Daemon::_update_clock(void *client_data)
{
  Raptor_Daemon *d = (Raptor_Daemon *) client_data;
  d->_update_clock(MR_SUBSCRIBED_MODULES);
}


void Raptor_Daemon::_update_clock(const string& which_client)
{
  timeval tv;
  gettimeofday(&tv, 0);
  char buf[128];
  snprintf(buf, sizeof(buf), "%ld.%06ld", tv.tv_sec, tv.tv_usec);
  send_message(which_client, "clock", buf);
}


void Raptor_Daemon::_update_process_idle_time(void *client_data)
{
  Raptor_Daemon *d = (Raptor_Daemon *) client_data;
  d->_update_process_idle_time(MR_SUBSCRIBED_MODULES);
}


void Raptor_Daemon::_update_process_idle_time(const string& which_client)
{
  FOR_EACH(pr, proc_map) {
    if (pr->second.is_local()
	&& Process_Status::has_console(pr->second.get_status()))
      {
	pr->second.update_idle_time();
      }
  }
}


void Raptor_Daemon::_unsubscribe(const string& which_client,
				 const string& message_type)
{
  comm->remove_subscriber(message_type, which_client);
}

void Raptor_Daemon::_handle_subscribe(const string& which_client,
				      int command_id,
				      const rcl::exp& e)
{
  /*
    
 syntax:
   1. sub <message_type>
   2. sub stdout <process> [ <playback_lines> <last_received_time> ]

 examples:
   sub status
   sub config
   sub stdout ctr
   sub stdout ctr 50
   sub stdout ctr 50 1047247438.775163

  */

  string message_type = e[1].getString();
  
  if (message_type == "stdout") {
    string process_name = e[2].getString();
    
    int playback_lines = 0;
    if (e[3].defined()) {
      playback_lines = e[3].getLong();
    }
    
    timeval last_received_time = timevalOf(0);
    if (e[4].defined()) {
      last_received_time = timevalOf(e[4].getDouble());
    }
    
    Process_Record proc =
      get_process(process_name, "subscribe to stdout of");
    
    _subscribe(which_client, string("stdout ") + process_name);
    proc.playback(which_client, playback_lines, last_received_time);
  } else if ((message_type == "status")
	     || (message_type == "config")
	     || (message_type == "clock")
	     || (message_type == "shout")) {
    _subscribe(which_client, message_type);
  } else {
    throw MRException
      (string("can't subscribe to unknown message type ")
       + message_type);
  }
  
  send_command_response(which_client, command_id, "ok");
}

void Raptor_Daemon::_handle_unsubscribe(const string& which_client,
					int command_id,
					const rcl::exp& e)
{
  /*
    
 syntax:
   1. unsub <message_type>
   2. unsub stdout <process>

 examples:
   unsub status
   unsub config
   unsub stdout ctr

  */
  string message_type = e[1].getString();
  
  if (message_type == "stdout") {
    string process_name = e[2].getString();
    _unsubscribe(which_client, string("stdout ") + process_name);
  } else if ((message_type == "status")
	     || (message_type == "config")
	     || (message_type == "clock")) {
    _unsubscribe(which_client, message_type);
  } else {
    throw MRException
      (string("can't unsubscribe from unknown message type")
       + message_type);
  }
  
  send_command_response(which_client, command_id, "ok");
}

void Raptor_Daemon::_update_for_new_config_global(const rcl::exp& incoming_config,
						  bool is_prior)
{
  // we are going to merge the incoming config and the current config.
  // the question is what to do with same-name processes between the
  // two.  one of them will be the "victim" and will have its same-name
  // processes/groups overwritten with the ones from the other.  the
  // right behavior is that whichever config has been loaded more
  // recently should win.  the way we tell is the following: an incoming
  // config that was around before we connected will be marked as
  // "prior" when it is sent (the marking is done by code in clientComm.h).
  
  // figure out which config is the victim
  rcl::map fresh_config, victim_config, combined_config;
  if (is_prior) {
    fresh_config = stored_config;
    victim_config = incoming_config.getMap();
  } else {
    fresh_config = incoming_config.getMap();
    victim_config = stored_config;
  }
  
  // put them together
  combined_config = victim_config;
  combined_config("processes").addFrom(fresh_config("processes"));
  combined_config("groups").addFrom(fresh_config("groups"));
  
  // keep the combined config
  stored_config = combined_config;
  
  rcl::map process_map = stored_config("processes").getMap();
  rcl::map groups = stored_config("groups").getMap();
  FOR_EACH ( pr, process_map ) {
    // sanity check: complain if there are same-name items in
    //               processes and groups
    if (groups(pr->first).defined()) {
      throw MRException
	(string("in config: ") + pr->first
	  + " is both a process and a group. make up your mind!");
    }
    // move the RCL data into the actual process data structures
    _set_process_config(pr->first, pr->second);
  }
}

void Raptor_Daemon::_update_for_new_config_process(const string& proc_name,
						   const rcl::exp& e)
{
  stored_config("processes")(proc_name) = e;
  _set_process_config(proc_name, e);
}

void Raptor_Daemon::_load_config_file(const string& config_file_name) {
  _update_for_new_config_global(rcl::loadConfigFile(config_file_name),
				/* is_prior = */ false);
}

void Raptor_Daemon::_handle_load_config_file(const string& which_client,
					     int command_id,
					     const rcl::exp& e)
{
  /* valid syntax examples:
     
	load my.config

  */

  _load_config_file(e[1].getString());
  send_command_response(which_client, command_id, "ok");
}

void Raptor_Daemon::_handle_send_to_process_stdin(const string& which_client,
						  int command_id,
						  const rcl::exp& e)
{
  /* valid syntax examples:
     
	stdin ctr "f 50"

  */

  Process_Record proc = get_process(e[1].getString(),
				    "forward to stdin of");
  proc.send_stdin_line(which_client, e[2].getString());
  
  send_command_response(which_client, command_id, "ok");
}

void Raptor_Daemon::_handle_cancel(const string& which_client,
				   int command_id,
				   const rcl::exp& e)
{
  /* valid syntax examples:

	cancel

  */

  _cancel_pending_processes();
  send_command_response(which_client, command_id, "ok");
}

void Raptor_Daemon::_handle_shout(const string& which_client,
				  int command_id,
				  const rcl::exp& e)
{
  /* valid syntax examples:

     shout c_trey@monk_9918 %polo!

  */

  rcl::vector shoutBody;
  shoutBody.push_back(which_client);     /* sending client */
  shoutBody.push_back(e[1].getString()); /* message */

  send_message(MR_SUBSCRIBED_MODULES, "shout", rcl::toString(shoutBody));
  send_command_response(which_client, command_id, "ok");
}

void Raptor_Daemon::_handle_signal(const string& which_client,
				   int command_id,
				   const rcl::exp& e)
{
  /* valid syntax examples:

     signal INT ctr
     signal INT ctr ctpan
     signal INT -a

  */

  // figure out which signal to use
  string sigName = e[1].getString();
  int sigNum = get_sig_num_for_name(sigName.c_str());
  if (-1 == sigNum) {
    throw MRException
      (string("unknown signal ") + sigName);
  }

  // collect list of processes or groups to signal
  list<string> proc_or_group_list;
  if (e[2].getString() == "-a") {
    FOR_EACH(pr, proc_map) {
      if (!pr->second.is_runnable()) {
	  proc_or_group_list.push_back(pr->first);
      }
    }	
    if (0 == proc_or_group_list.size()) {
      throw MRException("no running processes to signal", MR_WARNING);
    }
  } else {
    for (int i=2; e[i].defined(); i++) {
      proc_or_group_list.push_back(e[i].getString());
    }
  }

  // expand any groups in the list
  std::list<std::string> proc_names;
  _expand_groups(proc_or_group_list, proc_names, "signal");

  // signal every process in the resulting list
  int num_signaled = 0;
  FOR_EACH (elt, proc_names) {
    Process_Record proc = get_process(*elt, "signal");
    if (!proc.is_runnable()) {
      if (proc.is_local()) {
	// local process; send the signal on this host
	proc.impl->send_signal(which_client, sigNum);
      } else {
	// remote process; pass the signal command on to the appropriate daemon
	comm->send_message(connected_daemons[proc.impl->host],
			   string("signal ") + sigName + proc.impl->name);
      }
      num_signaled++;
    }
  }
  if (0 == num_signaled) {
    throw MRException("none of the processes you tried to signal were running", MR_WARNING);
  }

  send_command_response(which_client, command_id, "ok");
}

void Raptor_Daemon::_handle_response(const string& which_daemon,
				     int command_id,
				     const rcl::exp& e)
{
  // this message is different in that it should only come when
  // we send a command to another daemon
  
  /* valid syntax examples:

	response ok
	response warning "text"
	response error "text"

  */
  
  string response_type = e[1].getString();
  bool do_forward = false;
  bool is_warning = false;
  if (response_type == "warning") {
    is_warning = true;
    do_forward = true;
  } else if (response_type == "error") {
    is_warning = false;
    do_forward = true;
  }
  
  if (do_forward) {
    cerr << (is_warning ? "WARNING" : "ERROR") << ": " << e[2].getString() << endl;
    if ("" != last_client_to_issue_run_command) {
      rcl::vector forward_exp;
      forward_exp.push_back( e[1] ); // response type
      forward_exp.push_back( e[2] ); // text
      send_command_response(last_client_to_issue_run_command, -1, rcl::toString(forward_exp));
    }
  }
}

void Raptor_Daemon::_handle_status(const string& which_daemon,
				   int command_id,
				   const rcl::exp& e)
{
  // this message is different in that it should only come from another
  // daemon
  rcl::exp status_hash = e[1];
  string process_name = status_hash("name").getString();
  string new_status_string = status_hash("status").getString();
  int new_status = Process_Status::from_string(new_status_string);
  
#if 0
  cerr << "got status from daemon " << which_daemon << " for "
       << process_name << ": " << new_status_string << endl;
#endif
  
  if (proc_map.end() == proc_map.find(process_name)) {
    throw MRException
      (string("got status for unknown process ") + process_name);
  }
  Process_Record proc = proc_map[process_name];
  if (proc.is_local()) {
    throw MRException
      (comm->get_this_module_name()
       + ": got status update for local process " + process_name
       + " from " + which_daemon
       + ".  is the same process running on both daemons?");
  }
  proc_map[process_name].set_status(new_status);
  _check_pending_processes();
}

void Raptor_Daemon::_handle_help(const string& which_client,
				 int command_id,
				 const rcl::exp& e)
{
  _command_help(which_client);
}

void Raptor_Daemon::_static_shutdown(void) {
  active_daemon_g->_shutdown();
}

void Raptor_Daemon::_static_signal_handler(int sig) {
  cout << "mraptord: caught signal " << sig << endl;
  _static_shutdown();
}

void Raptor_Daemon::_shutdown(void) {
  // this guard keeps us from trying to shut down multiple times
  static bool shutting_down = false;
  if (shutting_down) return;
  shutting_down = true;

  cout << "mraptord: shutdown: sending kill_cmd to all processes" << endl;
  int num_active_processes = 0;
  FOR_EACH(pr, proc_map) {
    Process_Record::Impl* proc = pr->second.impl.pointer();
    if (proc->is_local() && !proc->is_runnable()
	&& proc->kill_cmd != "") {
      // prefix with - to tell safe_system to ignore failures
      if (proc->kill_cmd[0] != '-') {
	proc->kill_cmd = string("-") + proc->kill_cmd;
      }
      proc->run_kill_command("shutdown", proc->kill_cmd);
      num_active_processes++;
    }
  }

  if (num_active_processes > 0) {
    cout << "mraptord: shutdown: waiting for " << MR_DEFAULT_BACKUP_KILL_DELAY
	 << " seconds" << endl;
    sleep((int)MR_DEFAULT_BACKUP_KILL_DELAY);
    
    cout << "mraptord: shutdown: sending backup_kill_cmd to all processes"
	 << endl;
    FOR_EACH(pr, proc_map) {
      Process_Record::Impl* proc = pr->second.impl.pointer();
      if (proc->is_local() && !proc->is_runnable()
	  && proc->backup_kill_cmd != "") {
	// prefix with - to tell safe_system to ignore failures
	if (proc->backup_kill_cmd[0] != '-') {
	  proc->backup_kill_cmd = string("-") + proc->backup_kill_cmd;
	}
	proc->run_kill_command("shutdown", proc->backup_kill_cmd);
      }
    }
  } else {
    cout << "mraptord: shutdown: (no active processes)" << endl;
  }

  cout << "mraptord: shutdown: flushing logs" << endl;
  _flush_logs();

#if 0
  // put disconnect before kill command, because we may be killing central (!)
  IPC_disconnect();
  
  // the process kill setup just got more complicated... it's not clear
  // that we want to automatically execute it on shutdown.  some
  // experimentation
  // is in order.
  
  // kill -9 child processes
  FOR_EACH(pr, proc_map) {
    if (pr->second.is_local() && !pr->second.is_runnable()) {
      pr->second.impl->kill(which_client);
    }
  }
#endif

  exit(0);
}

void Raptor_Daemon::_handle_quit(const string& which_client,
				 int command_id,
				 const rcl::exp& e)
{
  send_command_response(which_client, command_id, "ok");
  _shutdown();
}

void Raptor_Daemon::perform_command(const string& which_client,
				    const string& input) {
  int command_id = -1;
  try {
    rcl::exp e = rcl::readFromString(input);
    //cout << "stdin command was: " << e << endl;

    if (e[0].defined() && e[0].getType() == rcl::typeName<long>()) {
      command_id = (int) e[0].getLong();
      e.getVector().deleteElement(0);
    }

    // handle noop
    if (!e[0].defined()) {
      send_command_response(which_client, command_id, "ok");
      return;
    }

    string cmd = e[0].getString();

    if (_handler_table.find(cmd) == _handler_table.end()) {
      throw MRException(string("can't perform unknown command ") + cmd);
    }
    Command_Handler handler = _handler_table[cmd];
    (this->*handler)(which_client, command_id, e);

  } catch (MRException e) {
    rcl::vector response;
    response.push_back( e.is_warning ? "warning" : "error" );
    response.push_back( e.text );
    cerr << (e.is_warning ? "WARNING" : "ERROR") << ": " << e.text << endl;
    send_command_response(which_client, command_id, rcl::toString(response));
  }
}

void Raptor_Daemon::_ipc_cleanup_workaround(void) {
  if (!fds_to_subscribe.empty()) {
    FOR_EACH ( ep, fds_to_subscribe ) {
#if DEBUG_IPC_CLEANUP
      cout << "DEBUG_IPC_CLEANUP: IPC_subscribeFD " << ep->fd << endl;
#endif
      IPC_subscribeFD( ep->fd, ep->handler, ep->clientData );
    }
    fds_to_subscribe.clear();
  }
  if (!fds_to_unsubscribe.empty()) {
    FOR_EACH ( ep, fds_to_unsubscribe ) {
#if DEBUG_IPC_CLEANUP
      cout << "DEBUG_IPC_CLEANUP: IPC_unsubscribeFD " << ep->fd << endl;
#endif
      IPC_unsubscribeFD( ep->fd, ep->handler );
    }
    fds_to_unsubscribe.clear();
  }
  if (!fds_to_close.empty()) {
    FOR_EACH ( fdp, fds_to_close ) {
#if DEBUG_IPC_CLEANUP
      cout << "DEBUG_IPC_CLEANUP: safe_close " << *fdp << endl;
#endif
      safe_close( *fdp );
    }
    fds_to_close.clear();
  }
  if (!files_to_close.empty()) {
    FOR_EACH ( filep, files_to_close ) {
#if DEBUG_IPC_CLEANUP
      cout << "DEBUG_IPC_CLEANUP: safe_fclose " << fileno(*filep) << endl;
#endif
      safe_fclose( *filep );
    }
    files_to_close.clear();
  }
}

int Raptor_Daemon::_get_rate_limit_max_lines() {
  return rate_limit_max_lines;
}

int Raptor_Daemon::_get_rate_limit_max_lines_after_stdin() {
  return rate_limit_max_lines_after_stdin;
}

double Raptor_Daemon::_get_rate_limit_period() {
  return rate_limit_period;
}


void Raptor_Daemon::get_config(rcl::exp& config_exp) {
  config_exp = stored_config;
  //cout << "config_exp = " << config_exp << endl;
}


/*REVISION HISTORY

  $Log: raptorDaemon.cc,v $
  Revision 1.122  2006/12/08 16:02:21  trey
  replaced "dshout <client> <message>" command with "shout <message>"

  Revision 1.121  2006/11/16 18:55:43  brennan
  Made rate limits (the period, limit, and limit-after-stdin) command
  line arguments for mraptord, and added options to mraptord.conf.

  Split up rate limit warning into a two messages (a continued line and
  its terminator), since it was getting truncated by mraptord's
  80-character line limit.

  Revision 1.120  2006/11/13 18:19:11  trey
  added support for signaling multiple processes with one command

  Revision 1.119  2006/06/23 16:42:07  trey
  added support for daemon "signal" command

  Revision 1.118  2006/06/21 22:15:47  trey
  improved stdout status logging; no longer report transition to starting state when it is guaranteed to be brief

  Revision 1.117  2006/06/21 21:32:28  trey
  added and rearranged arguments to some functions; things are now a bit more consistent

  Revision 1.116  2006/06/14 01:51:44  trey
  no longer shut down if loading of initial config file fails

  Revision 1.115  2006/06/14 01:40:52  trey
  added "sub shout" and "dshout" commands

  Revision 1.114  2006/06/06 19:31:19  brennan
  Moved to subpackaging the RPMs, extended the release script and header.mak
  to properly support automated release building.

  Revision 1.113  2006/06/04 05:27:03  brennan
  First pass at modularized RPMs, and changes to build under gcc 2.96.

  Revision 1.112  2005/06/27 21:17:05  trey
  rewrote main loop; should be more robust under high load

  Revision 1.111  2005/06/27 19:26:46  trey
  cleaned up some cruft from debugging

  Revision 1.110  2005/06/27 19:04:27  trey
  attempted to fix defunct bug by removing all dependence on ipc timers

  Revision 1.109  2005/06/24 18:01:00  trey
  major changes to eliminate some uses of std::hash_map, which seems to have weird behavior; also fixed file closing issues

  Revision 1.108  2005/06/23 02:55:01  trey
  fixed spurious were_pending_procs warnings

  Revision 1.107  2005/06/13 20:03:46  trey
  fixed problem with kill -a on multiple daemons, fixed confusing error message for bad process name

  Revision 1.106  2005/06/13 17:45:41  trey
  fixed flaw in how simulated_pending_procs was being calculated

  Revision 1.105  2005/06/13 14:42:42  trey
  fixed some weird behavior when starting a process when other processes are pending

  Revision 1.104  2005/06/12 21:21:56  trey
  removed serial run option, added _reset_cleanup timer that may fix the defunct bug

  Revision 1.103  2005/05/24 20:43:46  trey
  DEBUG_DEPENDS now on by default; added warning about running a process while there are other processes still pending

  Revision 1.102  2005/05/18 01:06:39  trey
  added support for output buffering on comm traffic

  Revision 1.101  2005/05/05 22:08:59  trey
  fixed problem of bogus dependency cycles

  Revision 1.100  2004/12/15 08:23:18  brennan
  Added a few more defunct debugging printouts.

  Revision 1.99  2004/12/01 06:58:47  brennan
  Added platform support for RH 6.2; mostly, headers are in different places.
  Note that this will only work on a RH 6.2 system hacked to use a newer
  gcc than ships with 6.2 by default (tested on Valerie with gcc 3.2.3).

  Revision 1.98  2004/08/02 20:28:26  trey
  fixed bug with shadowed declaration of "status" variable

  Revision 1.97  2004/05/26 15:21:53  trey
  fixed behavior of restart command

  Revision 1.96  2004/05/26 14:10:05  trey
  added support for restart command

  Revision 1.95  2004/04/19 15:36:56  trey
  fixed problem with daemons not getting initial state from each other at connection time

  Revision 1.94  2004/04/19 14:57:55  trey
  fixed indentation

  Revision 1.93  2004/04/19 14:49:25  trey
  split raptorDaemon.cc into two files

  Revision 1.92  2004/04/15 18:19:57  trey
  avoided problem with /tmp/mrlogs not being world-writable

  Revision 1.91  2004/04/12 19:26:47  trey
  switched some rcl calls to use rcl::vector::push_back

  Revision 1.90  2004/04/09 20:31:33  trey
  major enhancement to rcl interface

  Revision 1.89  2004/04/08 16:42:56  trey
  overhauled rcl::exp

  Revision 1.88  2004/02/12 21:17:40  trey
  now catch SIGHUP and remove dependence on tca.h

  Revision 1.87  2004/02/12 20:55:10  trey
  made shutdown cleaner

  Revision 1.86  2003/11/21 19:47:47  trey
  killing a process now issues SIGTERM, wait 5 seconds, SIGKILL; may improve cleanup for some processes

  Revision 1.85  2003/11/20 20:29:22  trey
  got rid of warning when user attempts to kill a non-running process; this was causing problems when a partially-running group was killed

  Revision 1.84  2003/11/20 19:29:40  brennan
  Added try..catch around kill process calls.

  Revision 1.83  2003/11/20 02:36:14  trey
  tweaked usage help

  Revision 1.82  2003/11/20 01:06:37  trey
  removed uninitialized variable warning

  Revision 1.81  2003/11/15 17:53:40  brennan
  Split up CYGWIN #define into more reasonable/understandable chunks.  Completed move to FIRE build system.

  Revision 1.80  2003/10/29 23:54:14  trey
  fixed weird bug that was causing mraptord to hang when a process was started and killed multiple times; added debug print() function for Process_Impl

  Revision 1.79  2003/10/29 17:59:58  trey
  fixed problem with dependency cycle detection

  Revision 1.78  2003/10/18 05:42:25  brennan
  Added #include <sys/resource.h> for struct rusage

  Revision 1.77  2003/10/18 04:43:11  brennan
  Added --centralhost argument.

  Revision 1.76  2003/10/07 17:55:02  brennan
  Fixed cross-daemon dependency issue (one daemon not knowing about the other);
  dependency loop detection still broken.

  Revision 1.75  2003/08/28 02:18:38  trey
  enabled multiple -r options to run multiple processes; fixed problem with -p (parallel execution) expansion; found a work-around for problems that relate to running central under mraptord

  Revision 1.74  2003/08/27 20:58:54  trey
  attempted to fix problem of mraptord children inheriting too many file descriptors

  Revision 1.73  2003/08/27 18:29:08  trey
  fixed obscure dependency bug that resulted when trying to run a process that (a) depends on something and (b) was already pending before the run command

  Revision 1.72  2003/08/08 19:19:58  trey
  made changes to enable compilation under g++ 3.2

  Revision 1.71  2003/08/08 15:29:27  brennan
  Attempted to make microraptor compile:
    (1) on Cygwin and
    (2) under gcc 3.2
  This attempt has not yet succeeded

  Revision 1.70  2003/06/19 19:39:44  brennan
  Made microraptor build system happy with Cygwin.

  Revision 1.69  2003/06/16 17:01:20  brennan
  Conflict resolution.

  Revision 1.68  2003/06/16 15:16:20  trey
  fixed bug in editing dependencies of a process

  Revision 1.67  2003/06/14 22:52:15  trey
  tweaked usage help about -i option

  Revision 1.66  2003/06/14 22:28:40  trey
  added debug mode option

  Revision 1.65  2003/06/14 21:50:09  trey
  fixed problem with merging in partially defined config expressions

  Revision 1.64  2003/06/14 21:47:07  trey
  fixed problem with merging in partially defined config expressions

  Revision 1.63  2003/06/14 15:45:39  brennan
  Conflict resolution.

  Revision 1.62  2003/06/09 16:16:17  trey
  fixed useless reporting of status "pending" just before status "running"

  Revision 1.61  2003/06/09 15:44:56  trey
  added support for shell expansion of command strings (shell variables, ~, backtick command substitution)

  Revision 1.60  2003/06/06 17:55:13  trey
  relaxed rate limiting of process stdout right after user gives stdin input

  Revision 1.59  2003/06/06 16:24:07  trey
  updated comments

  Revision 1.58  2003/06/06 16:13:08  trey
  new config merging code ensures that the latest config is the one everyone gets

  Revision 1.57  2003/06/06 01:46:08  trey
  added support for syncing config between daemons

  Revision 1.56  2003/03/28 17:38:15  trey
  rearranged locations of files so that the atacama project can use rcl without causing namespace conflicts

  Revision 1.55  2003/03/24 16:32:56  trey
  switched central-specific rate limiting for a general form that keeps the stdout line rate for each process below about 20 Hz

  Revision 1.54  2003/03/22 23:14:49  trey
  several bug fixes

  Revision 1.53  2003/03/22 21:56:38  trey
  added "set client" command

  Revision 1.52  2003/03/22 20:45:29  trey
  added ability to set specific environment variables of the config

  Revision 1.51  2003/03/18 00:44:09  trey
  added custom kill command feature

  Revision 1.50  2003/03/17 01:10:16  trey
  added sanity check when another daemon updates our status

  Revision 1.49  2003/03/17 00:52:41  trey
  fixed problem with random ordering of processes in claw display

  Revision 1.48  2003/03/14 19:41:54  trey
  fixed problem where the daemon was trying to send status for non-local processes

  Revision 1.47  2003/03/14 04:22:42  trey
  added idle time updates

  Revision 1.46  2003/03/14 03:01:12  trey
  rationalized command names, added ability to subscribe to config and status messages

  Revision 1.45  2003/03/13 15:51:05  trey
  fixed problem with names that refer to both process and group

  Revision 1.44  2003/03/12 18:04:37  trey
  fixed to take into accoun that load_config_file no longer returns a vector

  Revision 1.43  2003/03/12 17:56:21  trey
  fixed problem of core dump on certain fatal errors; we now shut down cleanly

  Revision 1.42  2003/03/12 17:16:03  trey
  fixed corner case. if a1 is both a process and a group, "run a1" should expand the group, not run the process

  Revision 1.41  2003/03/12 15:54:02  trey
  lots of new features, including run or kill groups by name and new command-line options for starting central under mraptord

  Revision 1.40  2003/03/11 02:56:14  trey
  added forwarding of error messages from slave daemon to the right client

  Revision 1.39  2003/03/11 01:52:36  trey
  modified get_config and set_config commands to add "groups" option to config file

  Revision 1.38  2003/03/11 00:34:37  trey
  added default serial execution when multiple processes are passed with the run command, and -p option to disable serial execution

  Revision 1.37  2003/03/10 22:55:22  trey
  added dependency checking

  Revision 1.36  2003/03/10 17:18:29  trey
  made get_clock have a subscribe functionality instead of only giving an immediate response

  Revision 1.35  2003/03/10 16:53:46  trey
  added ready monitoring

  Revision 1.34  2003/03/10 16:02:40  trey
  added new process status values: pending and starting

  Revision 1.33  2003/03/10 05:35:11  trey
  added get_clock command and ability of daemons to connect to each other

  Revision 1.32  2003/03/09 22:40:21  trey
  fixed behavior in case %u is not specified in log_file option

  Revision 1.31  2003/03/09 22:29:46  trey
  made log file names unique (changed log_dir to log_file with template patterns), added ability to suppress stdout history lines already received by client

  Revision 1.30  2003/03/07 19:55:30  trey
  added code to correctly use patched IPC, but currently turned off and kept in workaround to avoid needing to install the patch everywhere

  Revision 1.29  2003/03/07 15:54:19  trey
  added explicit IPC_disconnect() so that clients know we are going down

  Revision 1.28  2003/03/03 17:21:40  trey
  fixed problem with one-char-at-a-time stdout data, improved handling of run -a and kill -a

  Revision 1.27  2003/02/26 05:09:23  trey
  implemented chris's suggestion for \r handling

  Revision 1.26  2003/02/25 21:43:40  trey
  added clock response so client can correct for clock skew

  Revision 1.25  2003/02/24 22:09:21  trey
  fixed reading of perl config files

  Revision 1.24  2003/02/20 19:57:45  trey
  made command names for mrterm and mraptord more consistent, added better help facilities to mrterm

  Revision 1.23  2003/02/20 15:37:33  trey
  fixed config response to actually tell you the name of the process :)

  Revision 1.22  2003/02/20 15:21:57  trey
  switched to get_config/set_config commands, added exit_time and last_stdout_time fields in status, also cleaned out some old stuff

  Revision 1.21  2003/02/19 20:01:43  trey
  major code cleaning

  Revision 1.20  2003/02/19 17:33:52  trey
  added proper disconnect handling

  Revision 1.19  2003/02/19 10:18:55  trey
  updated to use new clientComm version

  Revision 1.18  2003/02/19 02:33:45  trey
  added handy debug stdout output before we execvp the managed process

  Revision 1.17  2003/02/19 01:56:40  trey
  modified "stdout/<process>" to be "stdout <process>" for easier parsing

  Revision 1.16  2003/02/19 01:16:27  trey
  mraptor_manual.txt

  Revision 1.15  2003/02/19 00:46:51  trey
  implemented logging, updated format of stdout responses, improved cleanup

  Revision 1.14  2003/02/17 16:22:04  trey
  implemented IPC client connection

  Revision 1.13  2003/02/16 21:11:22  trey
  fixed possible future problems with lines being too long for line buffer

  Revision 1.12  2003/02/16 06:39:36  trey
  added stdin command

  Revision 1.11  2003/02/16 06:20:38  trey
  minor fixes, added -p command-line option, prompting now off by default

  Revision 1.10  2003/02/16 04:27:50  trey
  added playback capability, load command, fixed many bugs

  Revision 1.9  2003/02/16 02:31:10  trey
  full initial command set now implemented

  Revision 1.8  2003/02/15 05:45:40  trey
  added stdoutBuffer

  Revision 1.7  2003/02/14 22:08:52  trey
  added status update after run command, documented get response syntax

  Revision 1.6  2003/02/14 21:54:10  trey
  made status reporting mostly work

  Revision 1.5  2003/02/14 18:30:33  trey
  many fixes, started implementation of kill, ipc, get commands

  Revision 1.4  2003/02/14 05:57:13  trey
  child stdin/stdout now have correct buffering properties

  Revision 1.3  2003/02/14 03:28:12  trey
  fixed some pipe issues

  Revision 1.2  2003/02/13 23:21:16  trey
  now include safeSystem.h

  Revision 1.1  2003/02/13 22:41:30  trey
  renamed files

  Revision 1.3  2003/02/05 22:50:21  trey
  remote runner now uses rcl, at least a bit

  Revision 1.2  2003/02/05 21:41:36  trey
  added makefile, a bit of cleanup

  Revision 1.1  2003/02/05 03:57:54  trey
  initial atacama check-in
*/
