/********** tell emacs we use -*- c++ -*- style comments *******************
 * COPYRIGHT 2004, Carnegie Mellon University
 * $Revision: 1.17 $  $Author: trey $  $Date: 2006/06/23 16:41:11 $
 *  
 * PROJECT: Life in the Atacama
 *
 * @file    processRecord.h
 * @brief   No brief
 ***************************************************************************/

#ifndef INCprocessRecord_h
#define INCprocessRecord_h

/***************************************************************************
 * INCLUDES
 ***************************************************************************/

#include <iostream>
#include <fstream>
#include <map>
// Really a gcc-3.2 thing...
// Looks like it's a gcc-3.0.3 thing too; hopefully it didn't flipflop
//  between 3.0.3 and 3.2
#if (__GNUC__ == 3 && __GNUC_MINOR__ >= 0) || __GNUC__ >= 4
#define _CPP_BACKWARD_BACKWARD_WARNING_H
#define _BACKWARD_BACKWARD_WARNING_H
#include <ext/hash_map>
#else
#include <hash_map>
#endif
#include <list>
#include <unistd.h>
#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "ipc.h"

#include "mrCommonDefs.h"
#include "mrCommonTime.h"
#include "RCL.h"
#include "stdoutBuffer.h"
#include "processStatus.h"
#include "clientComm.h"

using namespace std;
using namespace microraptor;

/***************************************************************************
 * MACROS
 ***************************************************************************/

// this number of stdout lines buffered for each child process
#define MR_NUM_LINES_TO_BUFFER (1000)

// time between successive flushes of a log file (double, seconds).  set
// to 0 to flush every line.
#define MR_LOG_FLUSH_PERIOD (10)

// time between clock updates to clients (double, seconds)
#define MR_CLOCK_UPDATE_PERIOD (60)

// rate limiting of process output.  if we receive more than
// MR_RATE_LIMIT_MAX_LINES stdout lines from a process during a period
// of MR_RATE_LIMIT_PERIOD seconds, a warning is sent and the extra
// lines are suppressed.  an extra condition: if stdin input was
// sent to the process during this period or the previous period,
// raise the rate limit to MR_RATE_LIMIT_MAX_LINES_AFTER_STDIN.
#define MR_USE_RATE_LIMITING 1
#define MR_RATE_LIMIT_PERIOD (1.0)
#define MR_RATE_LIMIT_MAX_LINES (150)
#define MR_RATE_LIMIT_MAX_LINES_AFTER_STDIN (500)

#define MR_DEFAULT_BACKUP_KILL_DELAY (5.0)

/***************************************************************************
 * HELPER STRUCTS
 ***************************************************************************/

class Raptor_Daemon; // forward declaration

typedef void (Raptor_Daemon::*Command_Handler)(const string& which_client,
					       int command_id,
					       const rcl::exp& e);

struct Subscribe_Entry {
  int fd;
  FD_HANDLER_TYPE handler;
  void* clientData;

  Subscribe_Entry(int _fd, FD_HANDLER_TYPE _handler, void* _clientData) :
    fd(_fd),
    handler(_handler),
    clientData(_clientData)
  {}
};

/***************************************************************************
 * MAIN CLASS
 ***************************************************************************/

struct Process_Record {
public:
  struct Impl {
    Impl(const string& _name, Raptor_Daemon* _parent) :
      name(_name),
      parent(*_parent),
      status(MR_NOT_STARTED),
      pid(-99999),
      logging(false),
      log_stream(NULL),
      child_stdin(NULL),
      child_stdout(NULL),
      //is_pending_unfinished_line(false),
      need_to_report_pending(false),
      still_needs_killin(false),
      lazarus(false)
#if MR_USE_RATE_LIMITING
      ,rate_limit_period_start(getTime()),
      rate_limit_lines_during_period(0),
      rate_limit_periods_since_stdin(9999)
#endif
    {}
    ~Impl(void) {
      delete child_stdin;
      delete child_stdout;
    }

    string name;
    Raptor_Daemon& parent;

    /* these are extracted from the config */
    string cmd;
    string working_dir;
    string log_file;
    string host;
    rcl::map env;
    string ready;
    string kill_cmd;
    string backup_kill_cmd;
    double backup_kill_delay;
    list<string> depends;
    list<string> stdin_commands;
    string debugger;

    int status;
    pid_t pid;
    timeval last_stdout_time; // last time we received data on stdout
    timeval exit_time; // when the process exited
    int exit_value;
    int terminating_signal;
    
    string running_client; // the client that gave the cmd to run this process
    bool logging;
    timeval last_log_flush_time; // only valid if logging
    ofstream *log_stream;
    // number of idle update periods since we've seen new stdout data
    int num_idle_periods; 

    FILE *child_stdin;
    // Byte_Buffer is like ifstream, but tweaked to avoid blocking issues.
    Line_Buffer *child_stdout;
    // is_pending_unfinished_line is true if (a) we got an unfinished
    // line from child stdout (meaning we didn't publish it) and (b) no
    // stdout data has been published since that happened.
    //bool is_pending_unfinished_line;

    // earlier, there was an annoyance that the daemon would always send
    // a "status pending" message before a "status running" message, even
    // if there were no processes that needed to be run.  this variable
    // was created to help avoid that.
    bool need_to_report_pending;

    // initially false.  set to true when we attempt to kill a child
    // nicely.  cleared if the child listens to reason.  then when the
    // backup_kill_cmd timer goes off, if it's still true, we blow the
    // child away.
    bool still_needs_killin;
    
    // initially false.  set to true when we handle a restart command.
    // as soon as the process is good and dead, we resurrect it.
    bool lazarus;

    // when the process runs under a debugger, gdb is told to load a
    // uniquely named temporary symlink to the binary instead of the
    // binary itself.  this way, when the gdb 'run' command is issued,
    // the resulting process will have a unique name in the process
    // table, and we can use killall to send it signals.  this is the
    // name of the symlink.
    string debugger_binary_symlink;

#if MR_USE_RATE_LIMITING
    timeval rate_limit_period_start;
    int rate_limit_lines_during_period;
    int rate_limit_periods_since_stdin;
#endif
    
    bool is_local(void) const;

    static void _static_child_stdout_handler(int fd, void *client_data);
    void _log_and_send_stdout_line(const char* line_text,
				   bool apply_rate_limiting);
    void _child_stdout_handler(bool cleanup = false);
    static void static_backup_kill_timer_handler(void *clientData);
    void backup_kill_timer_handler(void);
    string _form_stdout_line(const Line_Data *ld);
    void _send_stdout_line(const string& which_client,
			   const string& line_text);
    void _handle_stdout_timer(void);
    void set_config(const rcl::exp& e);
    rcl::exp get_config(void);
    void cleanup(int wait_status);
    void log_flush(void);
    void playback(const string& which_client,
		  int playback_lines,
		  timeval last_received_time);
    void send_stdin_line(const string& which_client,
			 const string& line);
    void send_signal(const string& which_client,
		     int sigNum);
    void execute(const string& which_client);
    void kill(const string& which_client, bool do_restart = false);
    void run_kill_command(const string& which_client,
			  const string& _cmd);
    rcl::exp get_status_exp(void);
    void set_status(int _status, bool need_to_report = true);
    bool is_runnable(void) const {
      return Process_Status::is_runnable(status);
    }
    void update_idle_time(void);
    void print(std::ostream& out) const;
    void subscribe_fd(int fd, FD_HANDLER_TYPE handler, void* clientData);
    void unsubscribe_fd(int fd, FD_HANDLER_TYPE handler);
  };

  SmartRef< Impl > impl;
  Process_Record(void) {}
  Process_Record(Impl *_impl) : impl(_impl) {}

  int get_pid(void) const { return impl->pid; }
  bool is_local(void) const { return impl->is_local(); }
  bool is_runnable(void) const { return impl->is_runnable(); }

  void set_config(const rcl::exp& e) { impl->set_config(e); }
  rcl::exp get_config(void) { return impl->get_config(); }
  void cleanup(int wait_status) { impl->cleanup(wait_status); }
  void log_flush(void) { impl->log_flush(); }
#if 0
  string _form_stdout_line(const Line_Data* ld) { impl->_form_stdout_line(ld); }
  void _send_stdout_line(const string& which_client,
			 const string& line_text)
  {
    impl->_send_stdout_line(which_client, line_text);
  }
#endif
  void playback(const string& which_client, int playback_lines,
		timeval last_received_time)
  {
    impl->playback(which_client, playback_lines, last_received_time);
  }
  void send_stdin_line(const string& which_client,
		       const string& line) { impl->send_stdin_line(which_client, line); }
  void execute(const string& which_client) { impl->execute(which_client); }
  void kill(const string& which_client, bool do_restart = false) {
    impl->kill(which_client, do_restart);
  }
  rcl::exp get_status_exp(void) { return impl->get_status_exp(); }
  int get_status(void) const { return impl->status; }
  void set_status(int _status, bool need_to_report = true)
    { impl->set_status(_status, need_to_report); }
  void update_idle_time(void) {
    impl->update_idle_time();
  }
  void print(std::ostream& out) const { impl->print(out); }
};

std::ostream& operator<<(std::ostream& out, const Process_Record& proc);

typedef Process_Record::Impl Process_Impl;


#endif // INCprocessRecord_h

/***************************************************************************
 * REVISION HISTORY:
 * $Log: processRecord.h,v $
 * Revision 1.17  2006/06/23 16:41:11  trey
 * added support for daemon "signal" command and for running under a debugger
 *
 * Revision 1.16  2006/06/21 22:16:30  trey
 * swapped order of status logging messages so that they are less confusing
 *
 * Revision 1.15  2006/06/21 21:31:52  trey
 * added support for stdin_commands and debugger config fields; added extra status information that is now logged and sent as stdout responses to clients
 *
 * Revision 1.14  2005/06/27 19:04:27  trey
 * attempted to fix defunct bug by removing all dependence on ipc timers
 *
 * Revision 1.13  2005/06/24 17:59:27  trey
 * fixed problems with closing files
 *
 * Revision 1.12  2005/06/24 00:17:18  trey
 * removed C_FILE_STREAMS ifdefs, C_FILE_STREAMS is now always on
 *
 * Revision 1.11  2005/06/12 21:20:51  trey
 * remove serial_depends field
 *
 * Revision 1.10  2004/08/11 19:38:34  trey
 * possibly fixed bug of occasional missing prompts
 *
 * Revision 1.9  2004/08/02 20:28:26  trey
 * fixed bug with shadowed declaration of "status" variable
 *
 * Revision 1.8  2004/08/02 18:57:44  trey
 * sigh... tried to fix stderr output again
 *
 * Revision 1.7  2004/07/29 22:45:24  trey
 * fixed flaws in stderr buffering
 *
 * Revision 1.6  2004/06/28 15:46:18  trey
 * fixed to properly find ipc.h and libipc.a in new atacama external area
 *
 * Revision 1.5  2004/05/27 13:58:37  trey
 * added support for stdout_wait_for_end_of_line (changed default behavior to *NOT* wait for end-of-line)
 *
 * Revision 1.4  2004/05/26 15:21:53  trey
 * fixed behavior of restart command
 *
 * Revision 1.3  2004/05/26 14:10:05  trey
 * added support for restart command
 *
 * Revision 1.2  2004/04/28 17:15:21  dom
 * Added header with copyright information to source and Makefiles.
 *
 * Revision 1.1  2004/04/19 14:49:25  trey
 * split raptorDaemon.cc into two files
 *
 *
 ***************************************************************************/
