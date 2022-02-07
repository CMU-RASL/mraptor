/***** tell emacs we use -*- c++ -*- style comments *****
 * $Revision: 1.40 $ $Author: brennan $ $Date: 2006/11/16 18:55:04 $
 *
 * COPYRIGHT 2004, Carnegie Mellon University 
 *
 * PROJECT: Exploration Robotics (Life in the Atacama)
 *
 * MODULE:
 *
 * FILE: microraptor/client/clientIO.c
 *
 * DESCRIPTION:
 *
 ********************************************************/
#include <stdio.h>
#include <stdlib.h>

#include "clientIO.h"
#include "callbacks.h"
#include "support.h"
#include "RCL.h"
#include "clientComm.h"
#include "mrCommonDefs.h"

using namespace microraptor;

string get_lowest_skew_connection()
{
  string lowest = "";
  timeval skew;
  hash_map<string, timeval>::iterator hIt;

  skew.tv_usec = skew.tv_sec = 99999;

  try {
    FOR_EACH(connPair,  daemon_comm_g->get_connections()) {
      // Clients are in the connections list too
      if(MR_Comm::extract_type_from_module_name((*connPair).first) == "d") {

        // Be sure we always store /something/
        if(lowest.length() <= 0) lowest = (*connPair).first;
        
        hIt = clockSkew_g.find((*connPair).first);
        
        // Make sure we don't try to use a clock skew when we don't
        //  have one
        if(hIt != clockSkew_g.end()) {
          if(timercmp(&((*hIt).second), &skew, <)) {
            lowest = (*connPair).first;
            skew = (*hIt).second;
          }
        }
      }
    }      
  } catch(MRException e) {
    string errStr = "ERROR: ";
    errStr += e.text;
    cerr << errStr << endl;
    print_to_common_buffer(errStr.c_str(), ERROR_COL);
  } 
  return lowest;
}

/* Run a group; the daemon does multi-daemon serialization, so send it to the
   daemon with the lowest skew time (which /might/ be a good estimate of 
   transmission lag, but isn't, really.
*/
void run_group(const char* groupList)
{
  string closeDaemon = get_lowest_skew_connection();
  string cmd = "run ";
  cmd += groupList;
  try 
  {
    daemon_comm_g->send_message(closeDaemon, cmd);
  } 
  catch(MRException e) 
  {
    string errStr = "ERROR: ";
    errStr += e.text;
    cerr << errStr << endl;
    print_to_common_buffer(errStr.c_str(), ERROR_COL);
  }
}

/* Kill a group; the daemon does multi-daemon serialization, so send it to the
   daemon with the lowest skew time (which /might/ be a good estimate of 
   transmission lag, but isn't, really.
*/
void kill_group(const char* groupList)
{
  string closeDaemon = get_lowest_skew_connection();
  string cmd = "kill ";
  cmd += groupList;
  try
  {
    daemon_comm_g->send_message(closeDaemon, cmd);
  } 
  catch(MRException e) 
  {
    string errStr = "ERROR: ";
    errStr += e.text;
    cerr << errStr << endl;
    print_to_common_buffer(errStr.c_str(), ERROR_COL);
  }
}

/* Signal a group; the daemon does multi-daemon serialization, so send it to the
   daemon with the lowest skew time (which /might/ be a good estimate of 
   transmission lag, but isn't, really.
*/
void signal_group(const char* groupList, const char* signal)
{
  string closeDaemon = get_lowest_skew_connection();
  string cmd = "signal ";
  cmd += signal;
  cmd += " ";
  cmd += groupList;
  try
  {
    daemon_comm_g->send_message(closeDaemon, cmd);
  } 
  catch(MRException e) 
  {
    string errStr = "ERROR: ";
    errStr += e.text;
    cerr << errStr << endl;
    print_to_common_buffer(errStr.c_str(), ERROR_COL);
  }
}

/* Run processes defined in a space-separated list in processList.
   A null value for both arguments results in running all known processes.
   A null value for processList results in running all known processes on
    the listed machines.
   Returns true on success, false on failure.
*/
bool run_process(char* machList, char* processList)
{
  string command;
  GList* list;
  GList* walker;
  ProcessData* pd;
  
  if(machList == NULL && processList == NULL) {
    try {
      FOR_EACH(connPair, daemon_comm_g->get_connections()) {
        // Clients are in the connections list too
        if(MR_Comm::extract_type_from_module_name((*connPair).first) == "d") {
          //cerr << "Sending run -a to " << (*connPair).first << endl;
          daemon_comm_g->send_message((*connPair).first, "run -a");
        }
      }      
    } catch(MRException e) {
      string errStr = "ERROR: ";
      errStr += e.text;
      cerr << errStr << endl;
      print_to_common_buffer(errStr.c_str(), ERROR_COL);
      return false;
    } 
    return true;
  }

  if(machList == NULL) return false;

  list = strings_to_glist(processList, NULL, machList);
  if(list == NULL) return false;

  try {
    for(walker = g_list_first(list); walker != NULL; walker = walker->next) {
      pd = (ProcessData*) walker->data;
      if(pd == NULL) continue;
      command = "run ";
      /* If we only pass a machine list, fire all up on the given machines */
      if(processList == NULL) {
        command += "-a";
      } else {
        command += pd->procName;
      }
      daemon_comm_g->send_message(string(pd->machName), command);
    } /* for(walker = list; walker != NULL; walker = walker->next) */
  } catch(MRException e) {
    string errStr = "ERROR: ";
    errStr += e.text;
    cerr << errStr << endl;
    print_to_common_buffer(errStr.c_str(), ERROR_COL);
    return false;
  }
  return true;
}


/* Kill processes defined in a space-separated list in processList.
   A null value for both arguments results in killing all known processes.
   A null value for processList results in killing all known processes on
    the listed machines.
   Returns true on success, false on failure.
*/
bool kill_process(char* machList, char* processList)
{
  string command;
  GList* list;
  GList* walker;
  ProcessData* pd;
  
  if(machList == NULL && processList == NULL) {
    try {
      FOR_EACH(connPair, daemon_comm_g->get_connections()) {
        // Clients are in the connections list too
        if(MR_Comm::extract_type_from_module_name((*connPair).first) == "d") {
          daemon_comm_g->send_message((*connPair).first, "kill -a");
        }
      }      
    } catch(MRException e) {
      string errStr = "ERROR: ";
      errStr += e.text;
      cerr << errStr << endl;
      print_to_common_buffer(errStr.c_str(), ERROR_COL);
      return false;
    } 
    return true;
  }

  if(machList == NULL) return false;

  list = strings_to_glist(processList, NULL, machList);
  if(list == NULL) return false;

  try {
    for(walker = g_list_first(list); walker != NULL; walker = walker->next) {
      pd = (ProcessData*) walker->data;
      if(pd == NULL) continue;
      command = "kill ";
      /* If we only pass a machine list, fire all up on the given machines */
      if(processList == NULL) {
        command += "-a";
      } else {
        command += pd->procName;
      }
      daemon_comm_g->send_message(string(pd->machName), command);
    } /* for(walker = list; walker != NULL; walker = walker->next) */
  } catch(MRException e) {
    string errStr = "ERROR: ";
    errStr += e.text;
    print_to_common_buffer(errStr.c_str(), ERROR_COL);
    return false;
  }
  return true;
}

/* Signal processes defined in a space-separated list in processList
 * with the signal 'signal'.
 *
 * A null value for both arguments results in signaling all known processes.
 * A null value for processList results in signaling all known processes on
 *  the listed machines.
 *
 * Returns true on success, false on failure.
 */
bool signal_process(char* machList, char* processList, const char* signal)
{
  string command;
  GList* list;
  GList* walker;
  ProcessData* pd;
  
  if(machList == NULL && processList == NULL) {
    try {
      command = "signal ";
      command += signal;
      command += " -a";
      FOR_EACH(connPair, daemon_comm_g->get_connections()) {
        // Clients are in the connections list too
        if(MR_Comm::extract_type_from_module_name((*connPair).first) == "d") {
          daemon_comm_g->send_message((*connPair).first, command);
        }
      }      
    } catch(MRException e) {
      string errStr = "ERROR: ";
      errStr += e.text;
      cerr << errStr << endl;
      print_to_common_buffer(errStr.c_str(), ERROR_COL);
      return false;
    } 
    return true;
  }

  if(machList == NULL) return false;

  list = strings_to_glist(processList, NULL, machList);
  if(list == NULL) return false;

  try {
    // Loop through processes
    for(walker = g_list_first(list); walker != NULL; walker = walker->next) {
      pd = (ProcessData*) walker->data;
      if(pd == NULL) continue;
      command = "signal ";
      command += signal;
      command += " ";
      /* If we only pass a machine list, fire all up on the given machines */
      if(processList == NULL) {
        command += "-a";
      } else {
        command += pd->procName;
      }
      daemon_comm_g->send_message(string(pd->machName), command);
    } /* for(walker = list; walker != NULL; walker = walker->next) */
  } catch(MRException e) {
    string errStr = "ERROR: ";
    errStr += e.text;
    print_to_common_buffer(errStr.c_str(), ERROR_COL);
    return false;
  }
  return true;
}

/* Issues quit command to server */
void kill_server(const char* machName)
{
  
  try {
    daemon_comm_g->send_message(string(machName), "dquit");
  } catch(MRException e) {
    string errStr = "ERROR: ";
    errStr += e.text;
    print_to_common_buffer(errStr.c_str(), ERROR_COL);
  }
}

/* Sends text to stdin of process */
void stdin_to_process(const char* machName, const char* processName, const char* data)
{
  string command = "stdin ";
  string datatmp = data;

  if(processName == NULL || data == NULL) return;

  // Escape quotes
  for(unsigned int i = 0; i < datatmp.length(); i++) {
    if(datatmp[i] == '"') {
      datatmp.replace(i, 1, "\\\"");
      i++;
    }
  }

  command += processName;
  command += " \"";
  command += datatmp;
  command += "\"";

  try {
    daemon_comm_g->send_message(string(machName), command);
  } catch(MRException e) {
    string errStr = "ERROR: ";
    errStr += e.text;
    print_to_common_buffer(errStr.c_str(), ERROR_COL);
  }
}

/* Wrapper for sub_process; grabs number of playback lines from
   preferences.
   returns true if successful, false if  already subscribed 
*/
bool sub_process_prefs_playback(const char* machName,
                                const char* processName)
{
  return sub_process(machName, processName, prefs_get_back_history_length());
}

/* Subscribes to given process, with "playback" playback lines,
   returns true if successful, false if  already subscribed 
 */
bool sub_process(const char* machName, 
                 const char* processName, 
                 int playback)
{
  string command = "sub stdout ";
  // Is there a better way to get a number into a string?
  char pbStr[255];
  ProcessData pd;
  timeval lastSeen_tv;
  hash_map<ProcessData, pair<bool, timeval>, size_t (*)(ProcessData), bool (*)(ProcessData, ProcessData)>::iterator sit;

  if(processName == NULL) return false;

  pd.machName = strdup(machName);
  pd.procName = strdup(processName);
  pd.status = NULL;

  sit = subscriptions_g->find(pd);
  if(sit != subscriptions_g->end()) {
    // pd isn't used now
    free_process_data_members(&pd);
    // Filter redundant subscriptions...
    if((*sit).second.first) {
      return false;
    }
    (*sit).second.first = true;
    lastSeen_tv = (*sit).second.second;
  } else {
    timeval tv;
    tv.tv_sec = tv.tv_usec = 0;
    lastSeen_tv = tv;
    subscriptions_g->insert(pair<ProcessData, pair<bool, timeval> >(pd, pair<bool, timeval>(true, tv)));
  }

  command += processName;
  command += " ";
  sprintf(pbStr, "%d %ld.%ld", playback, lastSeen_tv.tv_sec, lastSeen_tv.tv_usec);
  command += pbStr;

  try {
    daemon_comm_g->send_message(string(machName), command);
  } catch(MRException e) {
    string errStr = "ERROR: ";
    errStr += e.text;
    print_to_common_buffer(errStr.c_str(), ERROR_COL);
  }

  return true;
}

/* Unsubscribes to given process; 
   returns true if successful, false if  already unsubscribed */
bool unsub_process(const char* machName, const char* processName)
{
  string command = "unsub stdout ";
  ProcessData pd;
  bool needToFreePD = false;
  hash_map<ProcessData, pair<bool, timeval>, size_t (*)(ProcessData), bool (*)(ProcessData, ProcessData)>::iterator sit;
  hash_map<ProcessData, bool, size_t (*)(ProcessData), bool (*)(ProcessData, ProcessData)>::iterator oit;

  if(processName == NULL) return false;

  pd.machName = strdup(machName);
  pd.procName = strdup(processName);
  pd.status = NULL;

  sit = subscriptions_g->find(pd);
  if(sit != subscriptions_g->end()) {
    // Filter redundant subscriptions...
    if(!((*sit).second.first)) {
      free_process_data_members(&pd);
      return false;
    }
    (*sit).second.first = false;
    needToFreePD = true;
  } else {
    timeval tv;
    tv.tv_sec  = tv.tv_usec = 0;
    subscriptions_g->insert(pair<ProcessData, pair<bool, timeval> >(pd, pair<bool, timeval>(false, tv)));
  }

  command += processName;

  try {
    daemon_comm_g->send_message(string(machName), command);
  } catch(MRException e) {
    string errStr = "ERROR: ";
    errStr += e.text;
    print_to_common_buffer(errStr.c_str(), ERROR_COL);
  }


  // Force overwrite info to false to avoid lingering \r problems
  oit = overwrite_g->find(pd);
  if(oit != overwrite_g->end()) {
    (*oit).second = false;
  } // If it's not there, it's assumed to be false

  if(needToFreePD)
    free_process_data_members(&pd);

  return true;
}

/* Requests numlines old lines for the given process.  This is a bit
 * of hack: we unsubscribe from, then resubscribe to the process.
 * This is a noop if we're already subscribed. */
void request_playback(const char* machName,
                      const char* processName,
                      int numlines)
{
  char pbStr[255];
  ProcessData pd;
  hash_map<ProcessData, pair<bool, timeval>, size_t (*)(ProcessData), bool (*)(ProcessData, ProcessData)>::iterator sit;        

  if(processName == NULL) return;;

  pd.machName = strdup(machName);
  pd.procName = strdup(processName);
  pd.status = NULL;

  sit = subscriptions_g->find(pd);

  // If we're not subscribed, noop.
  if(sit == subscriptions_g->end()
     || !((*sit).second.first) ) {
    return;
  }

  string unsubCommand = "unsub stdout ";
  unsubCommand += processName;

  sprintf(pbStr, " %d", numlines);
  string resubCommand = "sub stdout ";
  resubCommand += processName;
  resubCommand += pbStr;

  try {
    daemon_comm_g->send_message(string(machName), unsubCommand);
    daemon_comm_g->send_message(string(machName), resubCommand);
  } catch(MRException e) {
    string errStr = "ERROR: ";
    errStr += e.text;
    print_to_common_buffer(errStr.c_str(), ERROR_COL);
  }
}

/* Sends both of:
   (1) get clock: gets an initial clock message
   (2) sub clock: subscribes to a once-a-minute clock message
*/
void get_clock(const char* machName)
{
  try {
    daemon_comm_g->send_message(string(machName), "get clock");
    daemon_comm_g->send_message(string(machName), "sub clock");
  } catch(MRException e) {
    string errStr = "ERROR: ";
    errStr += e.text;
    print_to_common_buffer(errStr.c_str(), ERROR_COL);
  }
}

/* Requests status of a process or processes 
   Returns true on success, false on failure.
*/
bool get_status_process(const char* machName, const char* processName)
{
  string command = "get status ";
  
  if(processName == NULL) return false;

  /* If we have a null machine name, get 'em all 
     This should imply that processName is -a; it doesn't make all
     that much sense otherwise.
   */
  try {
    if(machName == NULL) {
      FOR_EACH(connPair, daemon_comm_g->get_connections()) {
        // Clients are in the connections list too
        if(MR_Comm::extract_type_from_module_name((*connPair).first) == "d") {
          command = "get status ";
          command += processName;
          daemon_comm_g->send_message((*connPair).first, command);      
        }
      }
    } else {    
      command += processName;
      daemon_comm_g->send_message(string(machName), command);
    }
  } catch(MRException e) {
    string errStr = "ERROR: ";
    errStr += e.text;
    print_to_common_buffer(errStr.c_str(), ERROR_COL);
    return false;
  }
  return true;
}

/* Requests the current configuration of 
   a process or processes 
   Returns true on success, false on failure.
*/
bool get_config_process(const char* machName, const char* processName)
{
  string command = "get config ";
  
  if(processName == NULL) return false;

  /* If we have a null machine name, get 'em all 
     This should imply that processName is -a; it doesn't make all
     that much sense otherwise.
   */
  try {
    if(machName == NULL) {
      // Get configs from clients too; what the hey.
      FOR_EACH(connPair, daemon_comm_g->get_connections()) {
        command = "get config ";
        command += processName;
        daemon_comm_g->send_message((*connPair).first, command);      
      }
    } else {    
      command += processName;
      daemon_comm_g->send_message(string(machName), command);
    }
  } catch(MRException e) {
    string errStr = "ERROR: ";
    errStr += e.text;
    print_to_common_buffer(errStr.c_str(), ERROR_COL);
    return false;
  }
  return true;
}


/* Loads remote config file 
   Returns true on success, false on failure.
*/
bool load_config(const char* machName, const char* configFilename)
{
  string command = "dload ";

  if(configFilename == NULL) return false;
  command += configFilename;

  try {
    daemon_comm_g->send_message(string(machName), command);
  } catch(MRException e) {
    string errStr = "ERROR: ";
    errStr += e.text;
    print_to_common_buffer(errStr.c_str(), ERROR_COL);
    return false;
  }
  return true;
}

void
send_local_config(rcl::exp local_config)
{
  /* if no local config has been read in yet, don't try to send one */
  if (!local_config.defined()) return;

  daemon_comm_g->send_message
    (MR_ALL_MODULES, string("set config ") + rcl::toString(local_config));

  /* Update our local display */
  get_status_process(MR_ALL_MODULES, "-a");
} /* End send_local_config */


void set_process( const char* machName, 
                  const char* host,
                  const char* processName,
                  const char* cmd,
                  const char* working_dir,
                  const char* log_dir,
                  const char* env)
{
  rcl::exp command;
  rcl::exp conf;

  command[0] = "set config";
  command[1] = processName;

  if(processName == NULL || cmd == NULL) return;
  conf("cmd") = processName;

  if(working_dir != NULL && strlen(working_dir) > 0) {
    conf("working_dir") = working_dir;
  }
  if(log_dir != NULL && strlen(log_dir) > 0) {
    conf("log_file") = log_dir;
  }
  if(env != NULL && strlen(env) > 0) {
    conf("env") = rcl::readFromString(env)[0];
  }

  command[2] = conf;

  try {
    cerr << "Sending to " << machName << ": \"";
    cerr << rcl::toString(command);
    cerr << "\"" << endl;
    daemon_comm_g->send_message(string(machName), rcl::toString(command));
  } catch(MRException e) {
    string errStr = "ERROR: ";
    errStr += e.text;
    print_to_common_buffer(errStr.c_str(), ERROR_COL);
  }

  /* Do implicit get to update */
  get_status_process(machName, processName);
}





/***************************************************************
 * Revision History:
 * $Log: clientIO.c,v $
 * Revision 1.40  2006/11/16 18:55:04  brennan
 * Fixed bug where RCL syntax errors in set process config dialogs nuked
 * the contents of the field; the original data is now used and an error
 * message is printed to the common buffer.
 *
 * Added backup kill cmd and kill timeout to process configs.
 *
 * Made process config multiline text boxes expand with the window
 *
 * Added 'stdin_commands' to process configs.  This is a vector, but uses
 * a multiline entry in the config editing dialog.
 *
 * Added dialog during connection to central that allows graphical
 * cancellation in the event of incorrect centralhost designation.
 * Responsiveness is poor, but there's no way around that without hacking
 * IPC directly.
 *
 * Added debugger support to process configs.  Set 'debugger' to 'gdb' to
 * run the process under GDB.  This does not make use of any PATH
 * environment variable: the command must either be absolute, or relative
 * to the working_dir setting for the process.  When the process starts,
 * it will load the arguments, and stop at a gdb prompt to allow the user
 * to set breakpoints, etc.  Just type 'run' to run the process with all
 * of its arguments.  This makes use of Trey's debugger support in the daemon.
 *
 * Added icon for use in the taskbar (16x16; client/imageHeaders/miniClawImage.h)
 *
 * Added 'send signal to process' capability, using new 'signal <sig>
 * <process>' message type.  This is included everywhere run and kill
 * commands are.  Note: the popup menu that comes up when clicking the
 * signal button when viewing a process requires two clicks to select an
 * item.  This is a bug, but I haven't figured it out yet.
 *
 * New mraptord status messages supplant the claw-generated process
 * status messages, and are formatted to display which user issued run,
 * kill, and signal commands.
 *
 * Added client-side regexp filtering of process output.  This is a small
 * 'Filter' entry box in the lower-right of the process window.  Enter a
 * POSIX (e.g. grep -E)-style regexp and press [enter] to filter the
 * stdout.  Clear the entry box and press [enter] to deactivate
 * filtering.  The regexp will be displayed in green when active and
 * black when inactive.  Red denotes a regexp that is being edited while
 * the previous regexp is active.  A tooltip appears explaining this if
 * you hover over the entery box.  The regexp interface can be hidden by
 * setting the client-side preference "stdout_filtering" to 0 or false.
 *
 * Added limited chat support, via a dialog tabbed with the
 * minibuffer/error output.  This uses the new
 * shout/dshout messages.
 * Added graceful support for central crashes: a dialog is displayed to
 * inform the user of the crash, then claw exits.
 *
 * Fixed bug in which hiding, then re-viewing a process didn't fill the
 * buffer with recent output.
 *
 * Fixed newline bug in ClawText that resulted in there always being a
 * blank line at the bottom.
 *
 * Added (optional) timestamping to client output.  This is controlled
 * via the client preferences dialog.
 *
 * Added commandline and preferences option to specify the default width
 * of the configuration dialogs in claw. (--config-width /
 * default_config_width)
 *
 * Revision 1.39  2004/11/25 05:20:48  brennan
 * Fixed run-all bug (using a GList* instead of its data)
 *
 * Patched a bunch of memory leaks, and all the nasty ones.  There are four
 * known remaining leaks, all dealing with the widget used to view the output
 * of a process, with a sum total of ~90 bytes leaked per process viewed.
 * Not worth my time at the moment. :-)
 *
 * Revision 1.38  2004/11/23 07:01:54  brennan
 * A number of memory leaks and free/delete mismatches repaired.
 * Some errors detected by valgrind remain, and will be fixed soon.
 *
 * Revision 1.37  2004/04/28 18:58:50  dom
 * Appended log directive
 * 
 ***************************************************************/
