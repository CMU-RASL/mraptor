/***** tell emacs we use -*- c++ -*- style comments *****
 * $Revision: 1.7 $ $Author: brennan $ $Date: 2004/12/07 19:24:37 $
 *
 * COPYRIGHT 2004, Carnegie Mellon University 
 *
 * PROJECT: Exploration Robotics (Life in the Atacama)
 *
 * MODULE:
 *
 * FILE: microraptor/utils/mrapMonitor.cc
 *
 * DESCRIPTION:
 *
 ********************************************************/
/* TODO:
     - add "run this when process x [exits|runs]" options
     - built-in email notification option
     - "rerun if process x exits N times"
*/
#include <string>
#include "ipc.h"
#include "RCLParser.h"
#include "RCLConfigFile.h"
#include "RCLExp.h"
#include "clientComm.h"

using namespace std;
using namespace microraptor;

MR_Comm* mrap_comm_g = NULL;
rcl::exp monConfig_g;
bool haveConfig_g = false;
bool verbose_g = false;

// Borrowed from microraptor/mrCommonDefs.h
#define FOR_EACH(elt,collection) \
  for (typeof((collection).begin()) elt=(collection).begin(); \
       elt != (collection).end(); elt++)

static void mrapMsgHnd(const string& machName, const string& text, void *client_data)
{
  string procName, status;
  rcl::exp e;
  rcl::map cmap;
  try {
    e = rcl::readFromString(text);
  } catch(MRException err) {
    cerr << "Failed to parse incoming message! " << err.text << endl;
    return;
  }
   
  //if(e[0].defined() && e[0].getType() == "string" && e[0].getValue<string>() != "config") {
  //  cerr << "Received mraptor message \"" << text << "\", from machine " << machName << endl;
  //  cerr << "\tMessage class: " << e[0].getValue<string>() << ", e1 type: " << e[1].getType() << endl;
  // }

  // If it's a valid status message
  try{
  if(e[0].defined() 
     && e[0].getType() == "string" 
     && e[0].getValue<string>() == "status"
     && e[1].getType() == "map")
  {
    status = e[1]("status").getValue<string>();
    procName = e[1]("name").getValue<string>();
    cmap = monConfig_g.getMap();

    // Check if it's a process we care about
    if(!haveConfig_g) {
      cerr << "  " << procName << " is " << status << endl;
    } else if((cmap(procName).defined() && cmap(procName).getType() == "map")
              || cmap("ALL").defined() && cmap("ALL").getType() == "map"){
      if(status == "running" || status == "clean_exit"
         || status == "error_exit" || status == "signal_exit") {
        
        // Probably don't need this if
        if((cmap(procName).defined() && cmap(procName)(status).defined())
           || (cmap("ALL").defined() && cmap("ALL")(status).defined())) {
          string cmd;
          if(cmap(procName).defined() && cmap(procName)(status).defined()) {
            cmd = cmap(procName)(status).getValue<string>();
          } else if(cmap("ALL").defined() && cmap("ALL")(status).defined()) {
            cmd = cmap("ALL")(status).getValue<string>();
          } else {
            cerr << "ERROR: Logic failure in mrap handler!" << endl;
            return;
          }
          // Do s/r of patterns:
          //  %n = process name (actually, name of mraptor process entry; by
          //                     convention, this is the process name)
          //  %s = uncaught signal number (only valid in on_signal_exit)
          //  %t = timestamp (as returned by time(), seconds since the Epoch)
          for(unsigned int i = cmd.find("%"); i < cmd.length(); i = cmd.find("%", i)) {
            if(i+1 >= cmd.length()) {
              break;
            } else if(cmd[i+1] == 'n') {
              cmd.replace(i, 2, procName);
            } else if(cmd[i+1] == 's' && status == "signal_exit") {
              long termsig = e[1]("terminating_signal").getLong();
              char termstr[256];
              sprintf(termstr, "%ld", termsig);
              cmd.replace(i, 2, termstr);
            } else if(cmd[i+1] == 't') {
              long t = time(NULL);
              char timestr[256];
              sprintf(timestr, "%ld", t);
              cmd.replace(i, 2, timestr);
            } else {
              i++;
            }
          }
          if(verbose_g) {
            cerr << "Executing " << status << " command for " << procName << ":" << endl
                 << "\t" << cmd << endl;
          }
          system(cmd.c_str());
        }
      } else {
        if(verbose_g) {
          cerr << "In-progress status message (" << status << ") for process " << procName << "." << endl;
        }
      }
    } else {
      if(verbose_g) {
        cerr << "Ignoring " << status << " message from " << procName << "; not in our config." << endl;
      }
    }
  } // End if it's a valid status message
  } catch(MRException err) {
    cerr << "Caught an exception somewhere in mraphandler..." << endl
         << err.text << endl;
  }
}

static void usage (void)
{
  cerr << "mrmonitor commandline arguments:" << endl
       << "   [-h | --centralhost] <hostname>:  Set Microraptor centralhost.  Defaults" << endl
       << "                                      to value of CENTRALHOST environment variable." << endl
       << "   [-c | --config] <filename>:       Load this config file.  Without a config file," << endl
       << "                                      mrmonitor just prints out all detected status" << endl
       << "                                      changes." << endl
       << "   --debugMicro:                     Debug Microraptor comm - very verbose output." << endl
       << "   [-v | --verbose]                  Verbose output." << endl
       << "   WARNING: mrmonitor is not yet a supported utility.  Use at your own risk." << endl
       << endl;
  exit(-1);
}

int main(int argc, char** argv)
{
  char* tmpHost = getenv("CENTRALHOST");
  string chalCent = (tmpHost ? string(tmpHost) : "localhost");
  bool debugMrapComm = false;

  // Silly parse of commandline arguments.  Make this more robust
  for(int i = 1; i < argc; i++) {
    if(!strcmp(argv[i], "-h")
       || !strcmp(argv[i], "--centralhost") ) {
      if(i+1 < argc) {
        chalCent = argv[++i];
      } else {
        cerr << "Error: centralhost switch without argument." << endl;
        usage();
      }
    }
    else if(!strcmp(argv[i], "-c")
            || !strcmp(argv[i], "--config") ) {
      if(i+1 < argc && argv[i+1][0] != '-') {
        try {
          monConfig_g = rcl::loadConfigFile(argv[++i]);
          haveConfig_g = true;
        } catch (MRException err) {
          cerr << "Error loading config file: " << err.text << endl;
          usage();
        }        
      } else {
        cerr << "Error: config switch without argument." << endl;
        usage();
      }
    }
    else if(!strcmp(argv[i], "-v") || !strcmp(argv[i], "--verbose")) {
      verbose_g = true;
    }
    // Are we debugging microraptor comm?
    else if(!strcmp(argv[i], "--debugMicro") ) {
      debugMrapComm = true;
    }
    else {
      cerr << "Error: unknown argument \"" << argv[i] << "\"." << endl;
      usage();
    }
  } // End parsing of commandline arguments

  if(!haveConfig_g) {
    cerr << "WARNING: No config file loaded; mrmonitor will print" << endl
         << "         change of status notices to stdout only." << endl;
  } else {
    // Add bookkeeping info to config
    try {
      //vector< pair<string, rcl::exp> > cfgBody = monConfig_g.getValue<RCLHash>().body;
      //vector< pair<string, rcl::exp> > cfgBody = monConfig_g.getMap().body;
      FOR_EACH(proc, monConfig_g.getMap()) { //cfgBody) {
        proc->second("restart_on_clean_exit_cnt") = 0;
        proc->second("restart_on_error_exit_cnt") = 0;
        proc->second("restart_on_signal_exit_cnt") = 0;
      }
    } catch (MRException err) {
      cerr << "Error adding bookkeeping values: " << err.text << endl;
      usage();
    } 
  }

  if(verbose_g) {
    cerr << "Config + bookkeeping:" << endl;
    monConfig_g.writeConfig(cerr, 1);
    cerr << endl;
    IPCHelper::setVerbose(true);
  } else {
    IPCHelper::setVerbose(false);
  }


  // Now, connect to the Microraptor IPC network and define any
  // necessary messages.  See claw.c:336 in the microraptor source for
  // the subscribe lines not currently included here
  mrap_comm_g = new MR_Comm("c"); // connect as client
  if(debugMrapComm) {
    mrap_comm_g->set_verbose_mode(true);
  }

  // Subvert the environment in order to avoid modifying the microraptor code
  //  Eventually, connect_to_central should take an optional centralhost argument
  char* chostEnv = getenv("CENTRALHOST");
  if(chostEnv != NULL) chostEnv = strdup(chostEnv);
  setenv("CENTRALHOST", chalCent.c_str(), 1 /* overwrite */);
  // This defines the messages we'll send as well as connecting to central
  mrap_comm_g->connect_to_central();
  // Revert centralhost and clean up
  setenv("CENTRALHOST", chostEnv, 1);
  free(chostEnv);

  cerr << "Waiting to hear from the microraptor daemon..." << endl;
  bool done = false;
  IPC_listenClear(0);
  while(!done) {
    FOR_EACH(chit, mrap_comm_g->get_connections()) {
      if(MR_Comm::extract_type_from_module_name((*chit).first) == "d") {
        done = true;
        break;
      }
    }
    if(!done) {
      IPC_listenClear(250);
    }
  }
  cerr << "Howdy, Mr. Microraptor!" << endl;

  mrap_comm_g->send_message_all("set client {DISPLAY=\":0.0\"}");

  try {
    mrap_comm_g->subscribe_messages(mrapMsgHnd, NULL);
    mrap_comm_g->send_message_all("sub status");
  } catch(MRException e) {
    string errStr = "ERROR: ";
    errStr += e.text;
    cerr << errStr << endl;
  }  

  IPC_dispatch();
}





/***************************************************************
 * Revision History:
 * $Log: mrapMonitor.cc,v $
 * Revision 1.7  2004/12/07 19:24:37  brennan
 * Fixed more RCL-related bugs.
 * Old version of RCL allowed:  mapvariable("foo")("bar").defined()
 *   even if mapvariable("foo") wasn't defined; new version throws
 *   an exception.
 *
 * Revision 1.6  2004/12/07 17:59:53  brennan
 * Fixed mrmonitor problem; new RCL types caused it to never notice process
 * status changes.
 *
 * Revision 1.5  2004/12/02 18:26:12  brennan
 * Fixed mrmonitor to compile with the new RCL syntax, and added a
 * BUILD_UTILS make option to build it.  mrmonitor is currently only used
 * by the Valerie project.
 *
 * Revision 1.4  2004/06/28 15:46:18  trey
 * fixed to properly find ipc.h and libipc.a in new atacama external area
 *
 * Revision 1.3  2004/04/28 18:58:53  dom
 * Appended log directive
 * 
 ***************************************************************/
