/********** tell emacs we use -*- c++ -*- style comments *******************
 * COPYRIGHT 2004, Carnegie Mellon University
 * $Revision: 1.36 $  $Author: trey $  $Date: 2006/12/08 16:02:48 $
 *  
 * @file    mrterm.cc
 * @brief   No brief
 ***************************************************************************/

#include <assert.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>

#include <iostream>
#include <fstream>
#include <string>
#include <vector>

#include "ipc.h"

#include "RCL.h"
#include "mrIPCHelper.h"
#include "clientComm.h"
#include "mrCommonDefs.h"

using namespace std;
using namespace microraptor;

/**********************************************************************
 * TYPE DEFINITIONS
 **********************************************************************/

enum MRTerm_Mode {
  MR_BACKGROUND,
  MR_FOREGROUND,
  MR_SEND_THEN_QUIT,
  MR_WAIT_THEN_QUIT
};

/**********************************************************************
 * GLOBAL VARIABLES
 **********************************************************************/

MR_Comm* comm_g = 0;
string daemon_g = "";
string desired_daemon_host_g = "";
vector<string> execute_commands_g;
MRTerm_Mode mode_g = MR_BACKGROUND;
string foreground_process_g = "";
int num_responses_to_wait_for_g = -1;
bool interactive_g = false;
double connect_timeout_seconds_g = 0;
string user_g = getenv("USER");

/**********************************************************************
 * FUNCTIONS
 **********************************************************************/

static void prompt(void) {
  if (interactive_g) {
    cout << "> ";
    cout.flush();
  }
}

static void interface_help(void) {

#define _H(text) cout << text << endl;

  // print out local help
_H("");
_H("commands understood by mrterm:");
_H("  NOTE: commands not understood by mrterm are forwarded to the daemon");
_H("");
_H("  bg");
_H("    go back into background mode (the default mode), where what you type");
_H("    is interpreted as a command to the mrterm (either processed locally");
_H("    if the command is recognized as a local command, or forwarded to the");
_H("    daemon).");
_H("  fg <process_name>");
_H("    put <process_name> in the foreground.  while in foreground mode,");
_H("    mrterm simulates a console connected to the stdin/stdout of the");
_H("    given process.  in order to send a command to the mrterm instead,");
_H("    preface it with a '/' character.  for example, \"/bg\" puts the client");
_H("    back into background mode.");
_H("  help (or 'h')");
_H("    shows help on the mrterm and mraptord interfaces.");
_H("  load <config_file_name>");
_H("    causes mrterm to load a client-side config file and forward the");
_H("    information to the daemon using 'set' commands.  use 'dload' to load a");
_H("    daemon-side config file.");
_H("  quit (or 'q')");
_H("    quits mrterm.  use 'dquit' to quit the daemon.");
_H("  shout <message>");
_H("    issues a shout to all clients that have subscribed with 'sub shout'.");
_H("");

  // send a help request to the daemon
  comm_g->send_message(daemon_g, "dhelp");
}

static void command_handler(string cmd) {
  rcl::exp e;

  try {
    bool escaped_command = false;
    
    if (MR_FOREGROUND == mode_g && '/' == cmd[0]) {
      cmd = cmd.substr(1);
      escaped_command = true;
    }
    if (MR_FOREGROUND != mode_g || escaped_command) {
      e = rcl::readFromString(cmd);

      // empty line.  just repeat the prompt.
      if (!e[0].defined()) {
	prompt();
	return;
      }

      // local commands that we process here instead of forwarding to
      //   the daemon
      bool processed_command = false;
      if (e[0].getType() == "string") {
	processed_command = true;
	string command = e[0].getString();
	string process_name;
	if (command == "fg") {

	  // FG <PROCESS>: FOREGROUND MODE
	  //
	  // the fg command switches us to foreground mode for a
	  // specified process.  in foreground mode our commands are
	  // forwarded directly to the stdin of the process, and we see
	  // only the stdout of that process.  while in foreground mode,
	  // you can preface commands with the escape character '/' to
	  // instead process them as if in background mode.

	  // if already in foreground mode for another process, put
	  //   that process in the background first.
	  if (MR_FOREGROUND == mode_g) {
	    comm_g->send_message(daemon_g,string("unsub stdout ") + foreground_process_g);
	    mode_g = MR_BACKGROUND;
	  }

	  process_name = e[1].getString();
	  comm_g->send_message(daemon_g,string("sub stdout ") + process_name);
	  mode_g = MR_FOREGROUND;
	  foreground_process_g = process_name;

	  cout << "\n[now in foreground, use /bg to return to normal mode]"
	       << endl;
	} else if (command == "bg") {

	  // BG: BACKGROUND MODE
	  //
	  // the bg command switches us to background mode.  mrterm
	  // starts in this mode.  our commands are processed either by
	  // the daemon or the local command handler.  local commands
	  // include the mode switching commands (fg and bg) and the
	  // load command to forward config settings from a local
	  // config file.

	  if (MR_BACKGROUND == mode_g) {
	    throw MRException("already in background mode");
	  }

	  comm_g->send_message(daemon_g,string("unsub stdout ") + foreground_process_g);
	  mode_g = MR_BACKGROUND;
	  cout << "\n[returned to normal mode]" << endl;
	} else if (command == "load") {

	  // LOAD: LOAD LOCAL CONFIG FILE
	  //
	  // this command tells mrterm to load a config file and forward
	  // the settings to the daemon (using the "set" daemon command)

	  string config_file_name = e[1].getString();

	  rcl::exp config = rcl::loadConfigFile(config_file_name);
	  comm_g->send_message(daemon_g,
			       string("set config ") + rcl::toString(config));
	} else if (command == "help" || command == "h") {
	  interface_help();
	} else if (command == "quit" || command == "q") {
	  cerr << "quitting mrterm" << endl;
	  exit(0);
	} else {
	  processed_command = false;
	}
      }
      if (!processed_command) {
	// command not recognized, forward to the daemon
	comm_g->send_message(daemon_g,cmd);
      }
    } else {
      if (MR_FOREGROUND == mode_g) {
	// got a command to send to the stdin of the foreground process

	rcl::vector fg_message;
	int i = 0;
	fg_message[i++] = "stdin";
	fg_message[i++] = foreground_process_g;
	fg_message[i++] = cmd;
	comm_g->send_message(daemon_g, rcl::toString(fg_message));
      }
    }
  } catch (MRException e) {
    cerr << "ERROR: " << e.text << endl;
  }

  prompt();
}

static void stdin_handler(int fd, void *client_data) {
  char buf[512];
  cin.getline(buf, sizeof(buf));
  // cin.eof() indicates that either we are in an interactive session and
  //   the user typed Ctrl-D, or somebody has piped input in and we've reached
  //   EOF.  in either case, exit cleanly.
  if (cin.eof()) exit(0);

  command_handler(buf);
}

static void connect_handler(const string& which_daemon, void *client_data) {
  // ignore new connections from other clients
  if (comm_g->extract_type_from_module_name(which_daemon) == "c") return;
  
  // if a desired daemon host was specified, ignore connections from other
  // daemons
  if (desired_daemon_host_g != "" &&
      comm_g->extract_host_from_module_name(which_daemon) != desired_daemon_host_g) {
    return;
  }

  if (interactive_g) {
    cout << "mrterm: new connection to daemon " << which_daemon << endl;
  }

#if 0
  int conn_order = comm_g->get_connection_order();
  string order;
  if (conn_order == MR_THIS_MODULE_BEFORE_OTHER_MODULE) {
    order = "this module before other module";
  } else if (conn_order == MR_OTHER_MODULE_BEFORE_THIS_MODULE) {
    order = "other module before this module";
  } else {
    order = "active link";
  }
  cout << "mrterm: connection order was: " << order << endl;
#endif

  if (daemon_g != "") {
#if 0
    // old picky behavior
    cerr << "ERROR: More than one daemon is running; you must use -d to specify\n"
	 << "  which daemon to connect to. Daemons are currently running on (at\n"
	 << "  least) the following hosts: "
	 << comm_g->extract_host_from_module_name(daemon_g)
	 << " and "
	 << comm_g->extract_host_from_module_name(which_daemon)
	 << endl;
    exit(1);
#else
    // new permissive behavior -- assume that the user understands how
    // it works when multiple daemons are running
    return;
#endif
  } else {
    daemon_g = which_daemon;

    // if -e was specified, send commands
    num_responses_to_wait_for_g = execute_commands_g.size();
    for (int i=0; i < (int)execute_commands_g.size(); i++) {
      string cmd = rcl::toString(i) + " " + execute_commands_g[i];
      command_handler(cmd);
    }
    // if -q was specified, quit immediately
    if (MR_SEND_THEN_QUIT == mode_g) {
      exit(0);
    }
    
    // only start listening for stdin commands after we've connected to a daemon
    IPC_subscribeFD(fileno(stdin), stdin_handler, 0);
  }
}

static void disconnect_handler(const string& which_daemon, void *client_data) {
  cout << "WARNING: disconnected from daemon " << which_daemon << endl;
  daemon_g = "";
}

static void connect_timeout_handler(void *clientData,
				    unsigned long currentTime, 
				    unsigned long scheduledTime)
{
  if ("" == daemon_g) {
    // not connected to a daemon
    cout << "ERROR: mrterm: connect_timeout_handler: could not connect to mraptord for "
	 << connect_timeout_seconds_g << " seconds (exiting)" << endl;
  } else {
    cout << "ERROR: mrterm: connect_timeout_handler: connected to mraptord, sent -e commands, got no response for "
	 << connect_timeout_seconds_g << " seconds (exiting)" << endl;
  }
  exit(EXIT_FAILURE);
}

static void response_handler(const string& which_daemon,
			     const string& text,
			     void *client_data)
{
  rcl::exp e;
  int n;
  istringstream tstream(text);
  char* lbuf = new char[text.size()+1];
  //cout << "text [" << text << "]" << endl;
  while (!tstream.fail()) {
    // handles multi-line buffered input
    tstream.getline( lbuf, text.size()+1, '\n' );
    n = strlen(lbuf);
    // ignore blank line
    if (0 == n) continue;
    // chop trailing newline
    if ('\n' == lbuf[n-1]) lbuf[n-1] = '\0';
    
    //cout << "lbuf [" << lbuf << "]" << endl;

    e = rcl::readFromString(lbuf);
    switch (mode_g) {
    case MR_BACKGROUND:
      if (e[0].getString() == "help") {
	// help response, strip off the 'help %' at the beginning
	cout << e[1].getString() << endl;
      } else {
	// just print the response verbatim
	cout << lbuf << endl;
      }
      break;
    case MR_FOREGROUND:
      if (e[0].getString() == "stdout"
	  && e[1].getString() == foreground_process_g)
	{ 
	  cout << e[4].getString() << endl;
	}
      break;
    case MR_WAIT_THEN_QUIT:
      if (e[0].getString() == "response") {
	if (e.getVector().size() >= 1
	    && e[1].getType() == "string"
	    && e[1].getString() == "config") {
	  /* config response, ignore */
	} else {
	  cout << lbuf << endl;
	  num_responses_to_wait_for_g--;
	  if (0 == num_responses_to_wait_for_g) {
	    exit(0);
	  }
	}
      }
      break;
    case MR_SEND_THEN_QUIT:
      /* probably got the config response, ignore */
      break;
    default:
      abort(); // never reach this point
    }
  }
  delete[] lbuf;
}

static const char*findCentralhost() {
  ifstream confFile;
  confFile.open("/etc/mraptord.conf");
  if (confFile.is_open()) {
    string line, prefix="centralhost=";
    while (not confFile.eof()) {
      getline(confFile, line);
      if (line.compare(0, prefix.length(), prefix) == 0) {
	//printf("%s\n", line.substr(prefix.length()).c_str());
	return line.substr(prefix.length()).c_str();
      }
    }
  }
  return NULL;
}

static void usage(void) {
  cerr <<
    "usage: mrterm OPTIONS\n"
    "  -h or --help Print this help.\n"
    "  -d <host>    Sends commands to the daemon at the specified host.\n"
    "  -e <cmd>     Executes the specified command as if it had been sent to stdin.\n"
    "                 Multiple commands can be specified using multiple instances of -e.\n"
    "  -q           Tells mrterm to quit after sending -e commands.\n"
    "  -w           Tells mrterm to exit after it receives *responses* to -e commands.\n"
    "                 (You should probably use this rather than the older -q option.)\n"
    "  -t           Specifies timeout (seconds) for -q and -w options. Default: no timeout.\n"
    "  -i           Specifies that mrterm is being used interactively.\n"
    "                 A prompt is provided, and some output becomes more verbose.\n"
    "  -v           Tells mrterm to echo all comm traffic to console for debugging.\n"
    " --centralhost Specify host/port where IPC central server is running.\n"
    ;
  exit(1);
}

int main(int argc, char **argv) {
  string fifo_name = "";
  string daemon_ipc_name = "";
  string arg;
  bool debug_mode = false;
  bool noCentralhost = true;
  for (int argi=1; argi < argc; argi++) {
    arg = argv[argi];
    if (arg == "-h" || arg == "--help") {
      usage();
    } else if (arg == "-d" || arg == "--daemon") {
      if (++argi >= argc) {
	cerr << "got -d without argument" << endl << endl;
	usage();
      }
      desired_daemon_host_g = argv[argi];
    } else if (arg == "-e" || arg == "--execute") {
      if (++argi >= argc) {
	cerr << "got -e without argument" << endl << endl;
	usage();
      }
      execute_commands_g.push_back(argv[argi]);
    } else if (arg == "-q" || arg == "--quit") {
      mode_g = MR_SEND_THEN_QUIT;
    } else if (arg == "-w" || arg == "--wait") {
      mode_g = MR_WAIT_THEN_QUIT;
      IPCHelper::setVerbose(false);
    } else if (arg == "-t" || arg == "--timeout") {
      if (++argi >= argc) {
	cerr << "got -t without argument" << endl << endl;
	usage();
      }
      connect_timeout_seconds_g = atof(argv[argi]);
    } else if (arg == "-i" || arg == "--interactive") {
      interactive_g = true;
    } else if (arg == "-v" || arg == "--verbose") {
      debug_mode = true;
    } else if (arg == "--centralhost") {
      if (++argi >= argc) {
	cerr << "got --centralhost without argument" << endl << endl;
	usage();
      }
      setenv("CENTRALHOST", argv[argi], /* overwrite = */ 1);
      noCentralhost = false;
    } else {
      cerr << "ERROR: too many args" << endl << endl;
      usage();
    }
  }

  if (noCentralhost) {
    const char *centralhost = findCentralhost();
    if (centralhost != NULL)
      setenv("CENTRALHOST", centralhost, /* overwrite = */ 1);
  }

  comm_g = new MR_Comm("c"); // c indicates client

  // we can make these subscribe calls before or after connecting to IPC central
  comm_g->subscribe_messages(response_handler, 0);
  comm_g->subscribe_connect(connect_handler, 0);
  comm_g->subscribe_disconnect(disconnect_handler, 0);

  if ((MR_SEND_THEN_QUIT == mode_g
       || MR_WAIT_THEN_QUIT == mode_g)
      && connect_timeout_seconds_g > 0) {
    IPC_addTimer( SEC_TO_MSEC(connect_timeout_seconds_g), 1, &connect_timeout_handler, NULL );
  }

  if (debug_mode) comm_g->set_verbose_mode(true);

  // the microraptor network runs over IPC. it has an internal notion of
  // "module name" which is similar to IPC's notion.  when you run
  // mraptord or claw, they connect to IPC in such a way that the IPC
  // module name is the same as the microraptor module name.  we'll also
  // follow that convention here -- but it is not necessary.
  IPCHelper::reliableConnect(comm_g->get_this_module_name());

  // suppress a bunch of output about message registration when we call
  // initialize() below
  IPCHelper::setVerbose(false);

  // initialize() must be called after the various subscribe_xxx() functions
  // in order to make sure we notice the relevant events.  also after
  // we are connected to central, because it makes IPC calls.
  comm_g->initialize();

  prompt();

  IPC_dispatch();

  return 0;
}

/***************************************************************************
 * REVISION HISTORY:
 * $Log: mrterm.cc,v $
 * Revision 1.36  2006/12/08 16:02:48  trey
 * removed special handling of shout command, no longer needed after changes in daemon
 *
 * Revision 1.35  2006/06/14 01:41:34  trey
 * added "shout" support
 *
 * Revision 1.34  2005/06/30 21:39:02  trey
 * added workaround for clientComm bug (sometimes the first response arrives before the connect handler is called)
 *
 * Revision 1.33  2005/06/02 22:19:51  trey
 * switched default behavior of mrterm to have no timeout for compatibility with older scripts
 *
 * Revision 1.32  2005/06/01 00:47:02  trey
 * added -t option, enabled by default
 *
 * Revision 1.31  2005/05/31 21:39:16  trey
 * added -w option
 *
 * Revision 1.30  2005/05/17 22:12:07  trey
 * added output buffering support
 *
 * Revision 1.29  2004/08/21 19:40:27  trey
 * made behavior more permissive in the presence of multiple instances of mraptord
 *
 * Revision 1.28  2004/07/19 03:50:30  trey
 * added --centralhost command-line argument
 *
 * Revision 1.27  2004/06/28 15:46:18  trey
 * fixed to properly find ipc.h and libipc.a in new atacama external area
 *
 * Revision 1.26  2004/04/28 17:15:21  dom
 * Added header with copyright information to source and Makefiles.
 *
 * Revision 1.25  2004/04/09 20:31:33  trey
 * major enhancement to rcl interface
 *
 * Revision 1.24  2004/04/08 16:42:56  trey
 * overhauled RCLExp
 *
 * Revision 1.23  2004/03/10 20:23:44  trey
 * cleaned up interaction with com library a bit, fixed a bug in fg mode
 *
 * Revision 1.22  2003/12/01 20:47:13  trey
 * fixed fg/bg commands to use current daemon command syntax
 *
 * Revision 1.21  2003/12/01 20:33:29  trey
 * mrterm no longer quits by default after executing -e statements; cause old behavior with -q
 *
 * Revision 1.20  2003/10/04 21:56:48  trey
 * tweaked things to eliminate the need for the common/overlap directory and libmrcommonoverlap.a
 *
 * Revision 1.19  2003/08/27 20:06:20  trey
 * added some features for non-interactive use, and ability to specify which daemon to connect to
 *
 * Revision 1.18  2003/08/08 19:19:58  trey
 * made changes to enable compilation under g++ 3.2
 *
 * Revision 1.17  2003/03/28 17:38:15  trey
 * rearranged locations of files so that the atacama project can use rcl without causing namespace conflicts
 *
 * Revision 1.16  2003/03/14 03:01:51  trey
 * adjusted for change in command name set_config -> set config
 *
 * Revision 1.15  2003/03/12 15:57:07  trey
 * MR_Comm now handles connecting to central.  this allows mraptord to operate disconnected until after it runs central.
 *
 * Revision 1.14  2003/03/11 01:51:40  trey
 * modified load config file to handle new "groups" config option
 *
 * Revision 1.13  2003/03/07 00:35:19  trey
 * added function to determine connection order
 *
 * Revision 1.12  2003/02/25 23:45:12  trey
 * made mrterm locally handle quit command (as it should have done before)
 *
 * Revision 1.11  2003/02/24 16:29:52  trey
 * added "executing" config file ability
 *
 * Revision 1.10  2003/02/20 19:57:45  trey
 * made command names for mrterm and mraptord more consistent, added better help facilities to mrterm
 *
 * Revision 1.9  2003/02/19 17:33:41  trey
 * added proper disconnect handling
 *
 * Revision 1.8  2003/02/19 10:19:17  trey
 * made improved clientComm
 *
 * Revision 1.7  2003/02/19 00:46:05  brennan
 * Changed clientComm to keep track of daemon name.
 *
 * Revision 1.6  2003/02/17 17:19:45  trey
 * added help command
 *
 * Revision 1.5  2003/02/17 16:22:10  trey
 * implemented IPC client connection
 *
 * Revision 1.4  2003/02/16 21:11:42  trey
 * fixed possible future problems with lines being too long for line buffer
 *
 * Revision 1.3  2003/02/16 18:09:19  trey
 * made comm abstraction layer for clients
 *
 * Revision 1.2  2003/02/16 07:05:11  trey
 * fixed foreground mode
 *
 * Revision 1.1  2003/02/16 06:19:12  trey
 * initial check-in
 *
 *
 ***************************************************************************/
