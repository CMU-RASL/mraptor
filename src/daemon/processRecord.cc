/********** tell emacs we use -*- c++ -*- style comments *******************
 * COPYRIGHT 2004, Carnegie Mellon University
 * $Revision: 1.28 $  $Author: dom $  $Date: 2008/04/08 18:27:41 $
 *  
 * PROJECT: Life in the Atacama
 *
 * @file    processRecord.cc
 * @brief   No brief
 ***************************************************************************/

/***************************************************************************
 * INCLUDES
 ***************************************************************************/

#include <assert.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>

#include <iostream>

#include "processRecord.h"
#include "raptorDaemon.h"
#include "mraptordInternal.h"
#include "mrSignals.h"

using namespace std;

// #define DEBUG_FDS (1)

/**********************************************************************
 * GLOBAL VARIABLES
 **********************************************************************/

#if DEBUG_FDS
bool subscribed_initialized_g = false;
fd_set subscribed_g;
#endif

/**********************************************************************
 * NON-MEMBER FUNCTIONS
 **********************************************************************/

#if DEBUG_FDS

static void print_subscribed(void) {
  if (!subscribed_initialized_g) {
    cout << "_debug_fds: subscribed_g not initialized" << endl;
  } else {
    cout << "_debug_fds: subscribed fds are [";
    for (int i=0; i < FD_SETSIZE; i++) {
      if (FD_ISSET(i,&subscribed_g)) {
	cout << i << " ";
      }
    }
    cout << "]" << endl;
  }
}

#endif // DEBUG_FDS

static void convertRCLToStringList(std::list<std::string>& result,
				   const rcl::exp& e)
{
  result.clear();

  if (e.defined()) {
    const rcl::vector& v = e.getVector();
    FOR_EACH (vp, v) {
      result.push_back(vp->getString());
    }
  } else {
    /* if e is an undefined expression, leave result as an empty list */
  }
}

#ifdef NO_WORDEXP
// this is obsoleted by use of glibc wordexp()
// However, Cygwin does not yet have wordexp
static void split(const string& stringToSplit, ArgList& argList) {
  char *s = strdup(stringToSplit.c_str());

  char *tok, *tmp = s;
  while (0 != (tok = strtok(tmp," "))) {
    tmp = 0;
    argList.push_back(tok);
  }

  free(s);
}
#endif // NO_WORDEXP

/* if s is "a b c", return "a" */
static string getHead(const string& s)
{
  string::size_type p;
  if (string::npos != (p = s.find_first_of(" \t"))) {
    return s.substr(0,p);
  } else {
    return s;
  }
}

/* if s is "a b c", return "b c" */
static string getTail(const string& s)
{
  string::size_type p;
  if (string::npos != (p = s.find_first_of(" \t"))) {
    return s.substr(p+1);
  } else {
    return "";
  }
}

static string replace_string(const string& s,
			     const string& pattern,
			     const string& replacement) {
  string::size_type pos;
  string ret = s;
  while (string::npos != (pos = ret.find(pattern))) {
    ret.replace(pos, pattern.size(), replacement);
  }
  return ret;
}

static string unused_file_name(const string& name_template) {
  // if the user hasn't put %u in the name template, assume they
  // intend to clobber the old log file every run.
  if (string::npos == name_template.find("%u")) return name_template;

  string file_name;
  char pbuf[32];
  struct stat stat_data;
  bool found_unused_name = false;

  for (int i=0; i < 10000; i++) {
    snprintf(pbuf, sizeof(pbuf), "%04d", i);
    file_name = replace_string(name_template, "%u", pbuf);
    if (-1 == stat(file_name.c_str(), &stat_data)) {
      if (ENOENT == errno) {
	found_unused_name = true;
	break;
      } else if(EOVERFLOW == errno){
	// stat() can fail on large files... since we're just
	// testing if the file exists, continue through this error
	//   -Dom April 8, 2008
	errno = 0;
	continue;
      } else {
	throw MRException
	  (string("while generating unused file name from template ")
	   + name_template + ": stat failed:" + strerror(errno));
      }
    }
  }
  if (found_unused_name) {
    return file_name;
  } else {
    throw MRException
      (string("couldn't generate an unused file name from template ")
       + name_template);
  }
}

/**********************************************************************
 * PROCESS_IMPL FUNCTIONS
 **********************************************************************/

void Process_Impl::subscribe_fd(int fd, FD_HANDLER_TYPE handler, void* clientData) {
  //int ret = IPC_subscribeFD(fd,handler,clientData);
  parent.fds_to_subscribe.push_back( Subscribe_Entry(fd,handler,clientData) );
#if DEBUG_FDS
  if (!subscribed_initialized_g) {
    FD_ZERO(&subscribed_g);
    subscribed_initialized_g = true;
  }
  FD_SET(fd,&subscribed_g);
  cout << "_debug_fds: subscribing to " << fd << endl;
  print_subscribed();
#endif // DEBUG_FDS
}

void Process_Impl::unsubscribe_fd(int fd, FD_HANDLER_TYPE handler) {
  //int ret = IPC_unsubscribeFD(fd,handler);
  parent.fds_to_unsubscribe.push_back( Subscribe_Entry(fd,handler,NULL) );
#if DEBUG_FDS
  assert(subscribed_initialized_g);
  FD_CLR(fd,&subscribed_g);
  cout << "_debug_fds: unsubscribing to " << fd << endl;
  print_subscribed();
#endif // DEBUG_FDS
}

rcl::exp Process_Impl::get_config(void) {
  rcl::exp ret = parent.stored_config("processes")(name);
  if (!ret.defined()) {
    throw MRException
      (string("internal error: can't get config for process ") + name);
  }
  return ret;
}

bool Process_Impl::is_local(void) const {
  return host == "" || host == parent.get_this_host_name();
}

void Process_Impl::_static_child_stdout_handler(int fd, void *client_data) {
  Process_Impl *d = (Process_Impl *) client_data;
  d->_child_stdout_handler(false);
}

static void _static_handle_stdout_timer(void *clientData)
{
  Process_Impl* proc = (Process_Impl*) clientData;
  proc->_handle_stdout_timer();
}

void Process_Impl::_handle_stdout_timer(void) {
  //if (is_pending_unfinished_line)
  _child_stdout_handler(/* cleanup = */ true);
}

void Process_Impl::_log_and_send_stdout_line(const char* line_text,
					     bool apply_rate_limiting)
{
  if (logging) {
    // don't use endl,  because that always flushes
    (*log_stream) << line_text << "\n";
  }

#if MR_USE_RATE_LIMITING
  if (apply_rate_limiting) {
    double rate_limit_period = parent._get_rate_limit_period() >= 0 ? parent._get_rate_limit_period() : MR_RATE_LIMIT_PERIOD;
    int rate_limit_max_lines = parent._get_rate_limit_max_lines() >= 0 ? parent._get_rate_limit_max_lines() : MR_RATE_LIMIT_MAX_LINES;
    int rate_limit_max_lines_after_stdin = parent._get_rate_limit_max_lines_after_stdin() >= 0 ? parent._get_rate_limit_max_lines_after_stdin() : MR_RATE_LIMIT_MAX_LINES_AFTER_STDIN;
    
    // zero the count every time the period turns over
    timeval now = getTime();
    if (timeDiff( now, rate_limit_period_start )
	> rate_limit_period) {
      rate_limit_period_start = now;
      rate_limit_lines_during_period = 0;
      rate_limit_periods_since_stdin++;
    }
    
    int current_rate_limit =
      (rate_limit_periods_since_stdin <= 2)
      ? rate_limit_max_lines_after_stdin
      : rate_limit_max_lines;
    if (rate_limit_lines_during_period < current_rate_limit) {
      _send_stdout_line(MR_SUBSCRIBED_MODULES, line_text);
      
      // add one to the count for the current line
      rate_limit_lines_during_period++;
      
      // if the count just became too high, warn clients that
      // further lines will be suppressed
      if (rate_limit_lines_during_period == current_rate_limit) {
	// construct a fake stdout line containing the warning and send it.
        // This message is a bit too long now that we've added the
        // MRAPTORD:... prefix; need to split it into two.
	Line_Data warn;
	warn.time_stamp = now;
	warn.line_type = 'c';
	snprintf(warn.data, sizeof(warn.data),
		 "MRAPTORD: [warning] rate limit: sent %d lines during %.1f s, ",
		 current_rate_limit, rate_limit_period);
	string warn_text = _form_stdout_line(&warn);
	_send_stdout_line(MR_SUBSCRIBED_MODULES, warn_text);
        if(child_stdout) child_stdout->pushline(warn.data, warn.line_type);

	warn.line_type = 'x';
	snprintf(warn.data, sizeof(warn.data), "temporarily suppressing further data");
	warn_text = _form_stdout_line(&warn);
	_send_stdout_line(MR_SUBSCRIBED_MODULES, warn_text);
        if(child_stdout) child_stdout->pushline(warn.data, warn.line_type);
      }
    }
  } else {
    // by user request this line is outside the rate limit system
    _send_stdout_line(MR_SUBSCRIBED_MODULES, line_text);
  }
#else /* if MR_USE_RATE_LIMITING / else */
  _send_stdout_line(MR_SUBSCRIBED_MODULES, line_text);
#endif
}

void Process_Impl::_child_stdout_handler(bool cleanup) {
  Line_Data *ld;
  int stdout_status;
  while (1) {
    stdout_status = child_stdout->getline(&ld, cleanup);
    if (BUFFER_GOT_LINE != stdout_status) break;
    //is_pending_unfinished_line = false;
    last_stdout_time = ld->time_stamp;
    update_idle_time();

    string line_text = _form_stdout_line(ld);
    _log_and_send_stdout_line(line_text.c_str(), /* apply_rate_limiting = */ true);

    // if status is 'starting', look for ready string in output
    if (MR_STARTING == status) {
      if (0 != strstr(ld->data, ready.c_str())) {
#if 0
	cout << "found ready string: "
	     << "data = " << ld->data
	     << ", ready = " << ready << endl;
#endif
	// found ready string, transition to 'running' status
	set_status(MR_RUNNING);
	parent._check_pending_processes();
      }
    }
  }

  if (BUFFER_UNFINISHED_LINE == stdout_status) {
    parent.timers.addOneShotTimer
      ("Process_Impl::_static_handle_stdout_timer", 0.1,
       _static_handle_stdout_timer, this);
  }
}

void Process_Impl::cleanup(int wait_status) {
  exit_time = getTime();

  // maybe get last unfinished line of stdout data
  _child_stdout_handler(/* cleanup = */ true);
  unsubscribe_fd(child_stdout->get_fd(),
		 &Process_Impl::_static_child_stdout_handler);

  // set new status
  if (WIFEXITED(wait_status)) {
    exit_value = WEXITSTATUS(wait_status);
    set_status(exit_value ? MR_ERROR_EXIT : MR_CLEAN_EXIT);
  } else {
    terminating_signal = WTERMSIG(wait_status);
    set_status(MR_SIGNAL_EXIT);
  }

  // flush and close the log and close the files used to talk with the process
  if (logging) {
    log_stream->flush();
    logging = false;
    delete log_stream;
    log_stream = NULL;
  }
#if DEBUG_FDS
  cout << "_debug_fds: closing " << fileno(child_stdin) << endl;
#endif
  parent.files_to_close.push_back( child_stdin );
  child_stdin = 0;

  // close the child_stdout fd (but this keeps the buffer around so the
  // client can retroactively get lines)
#if DEBUG_FDS
  cout << "_debug_fds: closing " << child_stdout->get_fd() << endl;
#endif
  parent.fds_to_close.push_back( child_stdout->get_fd() );
  child_stdout->release_fd();
  
  pid = -99999;
  still_needs_killin = false;

  // remove the debugger_binary_symlink if it exists
  if (debugger_binary_symlink != "") {
    safe_system("-rm -f " + debugger_binary_symlink);
    debugger_binary_symlink = "";
  }

  if (lazarus) {
    // And when he thus had spoken, he cried with a loud voice,
    // "Lazarus, come forth!"
    parent._execute(name, running_client);
  }
}

void Process_Impl::log_flush(void) {
  if (logging) log_stream->flush();
}

void Process_Impl::set_config(const rcl::exp& e) {

  cmd = e("cmd").getString();

#define RD_GET_WITH_DEFAULT(FIELD,DEFAULT) \
  FIELD = e(#FIELD).defined() ? e(#FIELD).getString() : DEFAULT;

  RD_GET_WITH_DEFAULT( working_dir        , ""              );
  RD_GET_WITH_DEFAULT( log_file           , ""              );
  RD_GET_WITH_DEFAULT( host               , ""              );
  RD_GET_WITH_DEFAULT( ready              , ""              );
  RD_GET_WITH_DEFAULT( kill_cmd           , "kill %p"       );
  RD_GET_WITH_DEFAULT( backup_kill_cmd    , "kill -KILL %p" );
  RD_GET_WITH_DEFAULT( debugger           , ""              );

  backup_kill_delay = e("backup_kill_delay").defined() ?
    e("backup_kill_delay").getDouble() : MR_DEFAULT_BACKUP_KILL_DELAY;

  if (e("env").defined()) {
    env = e("env").getMap();
    // error check
    FOR_EACH ( pr, env ) {
      // causes an error if it's not a string
      pr->second.getString();
    }
  } else {
    // set to be an empty map
    env.clear();
  }

  convertRCLToStringList(depends, e("depends"));
  convertRCLToStringList(stdin_commands, e("stdin_commands"));
}

string Process_Impl::_form_stdout_line(const Line_Data* ld)
{
  char buf[128];
  timeval current_time;
  gettimeofday(&current_time, 0);
  snprintf(buf, sizeof(buf), "%lu.%06lu %c %%%s",
	   ld->time_stamp.tv_sec,
	   ld->time_stamp.tv_usec,
	   ld->line_type,
	   ld->data);
  return buf;
}
			       

void Process_Impl::_send_stdout_line(const string& which_client,
				     const string& line_text)
{
  parent.send_message(which_client, string("stdout ") + name, line_text);
}
			       

void Process_Impl::playback(const string& which_client,
			    int playback_lines,
			    timeval last_received_time)
{
  // if process hasn't started yet
  if (0 == child_stdout) return;

  int num_stored_lines = child_stdout->get_num_stored_lines();
  int lines_to_send = MIN(playback_lines, num_stored_lines);
  Line_Data *ld;
  for (int i = lines_to_send; i >= 1; i--) {
    ld = child_stdout->get_stored_line(i);
    // 1e-4 epsilon so we don't get one repeat line
    if (timeDiff( ld->time_stamp, last_received_time ) > 1e-4) {
      _send_stdout_line(which_client, _form_stdout_line(ld));
    }
  }
}

void Process_Impl::send_stdin_line(const string& which_client,
				   const string& line) {
  if (!Process_Status::has_console(status)) {
    throw MRException
      (string("can't forward to stdin of process ")
       + name + ", because it's not running");
  }
#if MR_USE_RATE_LIMITING
  rate_limit_periods_since_stdin = 0;
#endif
  fprintf(child_stdin, "%s\n", line.c_str());

  // echo stdin line to all subscribed clients.
  char line_text[1024];
  char echo_text[1024];
  timeval now = getTime();
  snprintf(line_text, sizeof(line_text),
	   "MRAPTORD: [stdin] [due_to %s] %s",
	   which_client.c_str(), line.c_str());
  snprintf(echo_text, sizeof(echo_text),
           "%lu.%06lu x %%%s",
           now.tv_sec, now.tv_usec, line_text);
  _log_and_send_stdout_line(echo_text, /* apply_rate_limiting = */ false);
  if(child_stdout) child_stdout->pushline(line_text);
}

void Process_Impl::send_signal(const string& which_client,
			       int sigNum)
{
  if (!is_local()) {
    throw MRException
      (string("can't send a signal to process ") + name + ", which is not local to this daemon");
  }
  if (is_runnable()) {
    throw MRException
      (string("can't send a signal to process ") + name + ", which is not running");
  }

  if ("" != debugger) {
    // construct kill command for inferior process under debugger
    const char* sig_name = get_name_for_sig_num(sigNum);
    string cmd_subst = "killall -%s %p";
    cmd_subst = replace_string(cmd_subst, "%s", sig_name);
    cmd_subst = replace_string(cmd_subst, "%p", debugger_binary_symlink);

    // notification to all subscribed clients.
    char line_text[1024];
    char text[1024];
    timeval now = getTime();
    snprintf(line_text, sizeof(line_text),
	     "MRAPTORD: [action] [due_to %s] sending signal: %s (via '%s')",
	     which_client.c_str(), sig_name,
	     cmd_subst.c_str());
    snprintf(text, sizeof(text),
	     "%lu.%06lu x %%%s",
	     now.tv_sec, now.tv_usec, line_text);
    _log_and_send_stdout_line(text, /* apply_rate_limiting = */ false);
    if(child_stdout) child_stdout->pushline(line_text);

    // actual kill call
    safe_system(cmd_subst);
  } else {
    // notification to all subscribed clients.
    char line_text[1024];
    char text[1024];
    timeval now = getTime();
    snprintf(line_text, sizeof(line_text),
	     "MRAPTORD: [action] [due_to %s] sending signal: %s (via kill() syscall)",
	     which_client.c_str(), get_name_for_sig_num(sigNum));
    snprintf(text, sizeof(text),
	     "%lu.%06lu x %%%s",
	     now.tv_sec, now.tv_usec, line_text);
    _log_and_send_stdout_line(text, /* apply_rate_limiting = */ false);
    if(child_stdout) child_stdout->pushline(line_text);

    // actual kill call
    safe_kill(pid, sigNum);
  }
}

void Process_Impl::execute(const string& which_client) {
  //print(cout);

  // don't resurrect the same process twice
  lazarus = false;

  if (status == MR_RUNNING || status == MR_STARTING) {
    throw Execute_Exception
      (string("process ") + name
       + " is already running; not running second copy",
       MR_WARNING);
  }
  
  // modify command if the process is to be run under gdb
  // ("mybinary arg1 arg2" becomes "gdb mybinary").
  string processed_cmd;
  if (debugger == "") {
    processed_cmd = cmd;
  } else {
    // create a uniquely-named symlink to the binary for
    // gdb to load... this way we can use killall to send
    // signals to the inferior process later.
    char buf[256];
    snprintf(buf, sizeof(buf), "/tmp/mrlinkXXXXXX");
    int fd = safe_mkstemp(buf);
    safe_close(fd);
    string wdir = "";
    // If the command uses an absolute path, don't use the working directory.
    string::size_type firstNonSpace = getHead(cmd).find_first_not_of(" \t\n");
    if(firstNonSpace == string::npos || getHead(cmd)[firstNonSpace] != '/') {
      wdir = working_dir;
      if(wdir.size() > 0 && wdir[wdir.size()-1] != '/') {
        wdir += "/";
      }
    }
    safe_system(string("ln -sf ") + wdir + getHead(cmd) + " " + buf);
    debugger_binary_symlink = buf;

    processed_cmd = debugger + " " + debugger_binary_symlink;
  }

  if (log_file != "") {
    char dirbuf[1024];
    snprintf(dirbuf, sizeof(dirbuf), "%s", log_file.c_str());

#ifdef NO_WORDEXP
    // dirname was allowed to modify its argument or point to
    //   statically allocated memory, so we can too
    char *dname;
    char *walker;
    if(dirbuf == NULL || strlen(dirbuf) <= 0) dname = ".";
    else {
      if(strcmp(dirbuf, "/") == 0) 
	dname = "/";
      else if((walker = rindex(dirbuf, '/')) == NULL)
	dname = ".";
      else {
	walker[0] = '\0';
	dname = dirbuf;
      }
    }      
#else
    char *dname = dirname(dirbuf);
#endif

    safe_system(string("mkdir -p -m 777 ") + dname);

    string log_file_with_name = replace_string(log_file, "%n", name);
    string log_file_full = unused_file_name(log_file_with_name); // replaces %u

    log_stream = new ofstream(log_file_full.c_str());
    if (! (*log_stream) ) {
      logging = false;
      throw Execute_Exception("couldn't open log file " + log_file_full
			      + " for writing: " + strerror(errno));
    }
    safe_system(string("-chmod 666 ") + log_file_full);

    // create a 'latest' symlink so that, for instance, if there are a bunch
    // of logs a_0000.log, a_0001.log, we have a link a_latest.log that
    // always points to the most recent numbered log file.
    string log_file_latest = replace_string(log_file_with_name, "%u",
					    "latest");
    safe_system(string("-ln -sf ") + log_file_full + " "
		+ log_file_latest);
    safe_system(string("-chmod 666 ") + log_file_latest);

    // set last_log_flush_time to be the beginning of time (i.e. 1970)
    last_log_flush_time.tv_sec = 0;
    last_log_flush_time.tv_usec = 0;
    logging = true;
  }

  int child_stdin_pipe[2], child_stdout_ptty[2];

  // can just do a pipe for stdin because we can set the
  //   buffering properties here
  safe_pipe(child_stdin_pipe);
  // but in order to get the right buffering properties on
  //   the stdout of the child, we need it to think it is
  //   on a tty.  hence we open a ptty pair instead of a pipe.
  safe_openpty(&child_stdout_ptty[READ_PIPE], &child_stdout_ptty[WRITE_PIPE],
	       0, 0, 0);

  /* the pipe/ptty fd's should not get propagated across the exec() call
     (although the dup2'd copies should and do get propagated; dup2 clears
     the FD_CLOEXEC flag in the copy) */
  safe_fcntl(child_stdout_ptty[READ_PIPE], F_SETFD, FD_CLOEXEC);
  safe_fcntl(child_stdout_ptty[WRITE_PIPE], F_SETFD, FD_CLOEXEC);
  safe_fcntl(child_stdin_pipe[READ_PIPE], F_SETFD, FD_CLOEXEC);
  safe_fcntl(child_stdin_pipe[WRITE_PIPE], F_SETFD, FD_CLOEXEC);

  pid = safe_fork();

#ifdef NO_WORDEXP
  ArgList argList;
#else
  wordexp_t expanded;
#endif

  if (0 == pid) {

    /**********************************************************************/
    /* child */
    /**********************************************************************/

#if MR_IPC_SILENT_CLOSE_PATCH
    x_ipcCloseInternal(/* informServer = */ 0);
#endif

    try {

      /* set up stdin and stdout to talk to parent */
      safe_dup2(child_stdin_pipe[READ_PIPE], fileno(stdin));

      safe_dup2(child_stdout_ptty[WRITE_PIPE], fileno(stdout));
      safe_dup2(child_stdout_ptty[WRITE_PIPE], fileno(stderr));

      //cerr << "working_dir = " << working_dir << endl;
      // change to the appropriate directory, if we need to.
      if (working_dir != "") {
	safe_chdir(working_dir);
      }
      
      // set any environment variables
#ifndef NO_CHILD_ENV_SETTING      
      rcl::exp this_client_settings = parent.client_settings[running_client];
      FOR_EACH ( pr, env ) {
	string env_var_name = pr->first;
	string env_var_value = pr->second.getString();
	
	if (string::npos != env_var_value.find("%c")) {
	  if (this_client_settings.defined()
	      && this_client_settings(env_var_name).defined()) {
	    string client_value =
	      this_client_settings(env_var_name).getString();
	    env_var_value = replace_string(env_var_value, "%c", client_value);
	    //cerr << "replaced: new value = " << env_var_value << endl;
	  } else {
	    throw MRException
	      (string("can't replace %c in env variable ") + env_var_name
	       + ": no corresponding client setting");
	  }
	}
	
	setenv(env_var_name.c_str(), env_var_value.c_str(),
	       /* overwrite = */ true);
      }
#endif

#if 0
      cout << "child pid is: " << getpid() << endl;
      cout << "command is: " << cmd << endl;
#endif
      
#ifndef NO_WORDEXP
      // this expands any shell variables, use of ~ for home directory,
      // command substitution with backticks, etc.
      int wordexp_status = wordexp(processed_cmd.c_str(), &expanded,
				   /* flags = */ WRDE_UNDEF | WRDE_SHOWERR);

      // WRDE_UNDEF: If the input refers to a shell variable that is not
      // defined, report an error. (actually, has no effect in my tests)

      // WRDE_SHOWERR: Do show any error messages printed by commands
      // run by command substitution. More precisely, allow these
      // commands to inherit the standard error output stream of the
      // current process. By default, wordexp gives these commands a
      // standard error stream that discards all output.
      
      // error reporting
#define WORDEXP_ERR(errval,text) \
      if (wordexp_status == errval) { \
        cerr << "MRAPTORD: [error] error in command expansion: " << text << endl; \
	exit(EXIT_FAILURE); \
      }

      // some of these errors (e.g. WRDE_CMDSUB) should only happen with
      // different wordexp() flags than we've chosen; keep them here because
      // we're paranoid.
      WORDEXP_ERR(WRDE_BADCHAR,"Unquoted invalid character such as `|'.");
      WORDEXP_ERR(WRDE_BADVAL,"Undefined shell variable.");
      WORDEXP_ERR(WRDE_CMDSUB,"Attempted use of command substitution.");
      WORDEXP_ERR(WRDE_NOSPACE,"Out of memory.");
      WORDEXP_ERR(WRDE_SYNTAX,"Syntax error (for example, an unmatched quoting character).");

      // this print loop should show the expanded version of the command
      cout << "MRAPTORD: [action] [due_to " << which_client << "] executing: ";
      // minor hack: not using rcl::toString() on argv[0]
      //   avoids quoting the binary name if it starts with "./"
      cout << expanded.we_wordv[0] << " ";
      for (unsigned int i=1; i < expanded.we_wordc; i++) {
	cout << rcl::toString(string(expanded.we_wordv[i])) << " ";
      }
      cout << endl;
#else // NO_WORDEXP is defined
      split(processed_cmd, argList);
      cout << "MRAPTORD: [action] [due_to " << which_client
	   << "] executing: " << processed_cmd << endl;
#endif
      
    } catch (MRException e) {
      cerr << "MRAPTORD: [error] exception raised while starting process between fork() and execvp() calls: "
	   << e.text << endl;
      exit(EXIT_FAILURE);
    }

#ifndef NO_WORDEXP
    execvp(expanded.we_wordv[0], expanded.we_wordv);
#else
    execvp(argList.argv[0], argList.argv);
#endif

    // would call wordfree() to free up expanded, but no point since
    // we're exiting anyway.

    // if we reach here, there has been an error
    cerr << "MRAPTORD: [error] execvp() call failed with command string `" << processed_cmd
	 << "': " << strerror(errno) << endl;
    exit(EXIT_FAILURE);

  } else {
    /**********************************************************************/
    /* parent */
    /**********************************************************************/

#if DEBUG_FDS
    cout << "_debug_fds: closing " << child_stdin_pipe[READ_PIPE]
	 << " and " << child_stdout_ptty[WRITE_PIPE] << endl;
#endif
    parent.fds_to_close.push_back( child_stdin_pipe[READ_PIPE] );
    parent.fds_to_close.push_back( child_stdout_ptty[WRITE_PIPE] );

    child_stdin = safe_fdopen(child_stdin_pipe[WRITE_PIPE], "w");
    // switch from fully buffered to line buffered
    setlinebuf(child_stdin);
    
    if (NULL == child_stdout) {
      child_stdout = new Line_Buffer(child_stdout_ptty[READ_PIPE],
				     MR_NUM_LINES_TO_BUFFER);
    } else {
      child_stdout->reopen(child_stdout_ptty[READ_PIPE]);
    }
    
    last_stdout_time = getTime();
    num_idle_periods = 0;
    
    subscribe_fd(child_stdout->get_fd(),
		 &Process_Impl::_static_child_stdout_handler,
		 this);
    
    // set status to 'starting', meaning we will watch for the ready
    // pattern and then transition to 'running'. if the ready pattern is
    // an empty string, the transition to 'running' will be almost
    // instantaneous, so we should not bother to report the intermediate
    // 'starting' state.
    set_status(MR_STARTING, /* need_to_report = */ ("" != ready));

    // if running the process under gdb, we should send a "set args"
    // command to set up the command line options when it is run (if the
    // original command was "mybinary arg1 arg2", the command to send to
    // gdb is "set args arg1 arg2").
    if (debugger != "") {
      send_stdin_line("start_under_debugger", string("set args ") + getTail(cmd));
    }
  }
}

void Process_Impl::run_kill_command(const string& which_client, const string& _cmd) {
#if DEBUG_KILL
  cout << "DEBUG_KILL: run_kill_command"
       << " name="  << name
       << " _cmd=[" << _cmd
       << "]" << endl;
#endif
  if (_cmd == "") return;

  string cmd_subst = replace_string(_cmd, "%p", rcl::toString(pid));

  // forward kill message to all subscribed clients.
  char line_text[1024];
  char kill_text[1024];
  timeval now = getTime();
  snprintf(line_text, sizeof(line_text),
	   "MRAPTORD: [action] [due_to %s] trying to kill process with '%s'",
	   which_client.c_str(), cmd_subst.c_str());
  snprintf(kill_text, sizeof(kill_text),
	   "%lu.%06lu x %%%s",
	   now.tv_sec, now.tv_usec, line_text);
  _log_and_send_stdout_line(kill_text, /* apply_rate_limiting = */ false);
  if(child_stdout) child_stdout->pushline(line_text);

#if DEBUG_KILL
  cout << "DEBUG_KILL: run_kill_command"
       << " cmd_subst=" << cmd_subst
       << endl;
#endif
  safe_system(cmd_subst);
}

void Process_Impl::static_backup_kill_timer_handler
  (void *clientData)
{
  Process_Impl* proc = (Process_Impl*) clientData;
  proc->backup_kill_timer_handler();
}

void Process_Impl::backup_kill_timer_handler(void) {
#if DEBUG_KILL
  cout << "DEBUG_KILL: backup_kill_timer_handler"
       << " name="  << name
       << " still_needs_killin=" << still_needs_killin
       << " backup_kill_cmd=" << backup_kill_cmd
       << endl;
#endif
  if (still_needs_killin) run_kill_command("backup_kill_timer", backup_kill_cmd);
}

void Process_Impl::kill(const string& which_client, bool do_restart) {
  if (is_runnable()) {
#if 0
    throw Execute_Exception
      (string("attempting to kill process ") + name
       + ", which is not currently running", MR_WARNING);
#endif
    if (do_restart) {
      parent._execute(name, which_client);
    }

    /* new behavior: silently do nothing if the process is already
       dead.  avoids issues with killing groups, especially across
       multiple daemons. */
    return;
  }
  
  if (!is_local()) {
    if (parent.connected_daemons.end()
	== parent.connected_daemons.find(host))
      {
	throw MRException
	  (string("can't kill process ") + name
	   + " running on unknown daemon for host " + host);
      }
    parent.comm->send_message(parent.connected_daemons[host],
			      string(do_restart ? "restart " : "kill ")
			      + name);
  } else {
    if (status == MR_PENDING) {
      // if process is pending, kill => cancel and restart => no-op
      if (do_restart) {
	/* no-op */
      } else {
	set_status(MR_NOT_STARTED);
      }
    } else {
      // if the command was a restart, set the process up for resurrection
      if (do_restart) {
	lazarus = true;
	running_client = which_client;
      }

      // send initial kill command
      run_kill_command(which_client, kill_cmd);
      
      // install a timer to run the backup kill command
      if (backup_kill_cmd != "") {
	still_needs_killin = true;

	parent.timers.addOneShotTimer
	  ("Process_Impl::static_backup_kill_timer_handler", backup_kill_delay,
	   static_backup_kill_timer_handler, this);
      }
    }
  }
}

rcl::exp Process_Impl::get_status_exp(void) {
  rcl::map statusExp;
  statusExp("name") = name;
  statusExp("status") = Process_Status::to_string(status);
  
  // fill in extra info depending on status
  switch (status) {
  case MR_NOT_STARTED:
  case MR_PENDING:
    // add nothing
    break;
  case MR_STARTING:
  case MR_RUNNING:
    statusExp("pid") = pid;
    statusExp("last_stdout_time") = last_stdout_time.tv_sec;
    statusExp("idle") = (num_idle_periods * MR_IDLE_UPDATE_PERIOD);
    break;
  case MR_CLEAN_EXIT:
    statusExp("exit_time") = exit_time.tv_sec;
    break;
  case MR_ERROR_EXIT:
    statusExp("exit_value") = exit_value;
    statusExp("exit_time") = exit_time.tv_sec;
    break;
  case MR_SIGNAL_EXIT:
    statusExp("terminating_signal") = terminating_signal;
    statusExp("exit_time") = exit_time.tv_sec;
    break;
  default:
    abort(); // never reach this point
  }
  
  return statusExp;
}

void Process_Impl::set_status(int new_status, bool need_to_report) {
#if 0
  cout << "calling set_status. proc=" << name << ", new_status=" << new_status
       << endl;
#endif

  int old_status = status;
  status = new_status;
  need_to_report_pending = false;
  
  if (!Process_Status::is_runnable(old_status)) {
    set_erase( parent.procs_with_status[old_status], name );
  }
  if (!Process_Status::is_runnable(new_status)) {
    set_insert( parent.procs_with_status[new_status], name );
  }

  if (is_local()) {
    if (parent.wait_central && new_status == MR_RUNNING && name == "central") {
      // if we were waiting to connect to central until after we run it,
      // now is the time to connect.
      parent.comm->connect_to_central();
      parent.wait_central = false;
    }
    
    if (need_to_report) {
      // delay reporting status if it is pending; we will only report it
      // if we check that the process is eligible to run and find that it
      // isn't
      if (status == MR_PENDING) {
	need_to_report_pending = true;
      } else {
	// send status as a "status" response
	parent.send_message(MR_SUBSCRIBED_MODULES, "status",
			    rcl::toString(get_status_exp()));
	
	
	// construct human-readable explanation of the status
	string readable = Process_Status::to_string(new_status).c_str();
	if (MR_SIGNAL_EXIT == new_status) {
	  const char* sig_name = get_name_for_sig_num(terminating_signal);
	  if (NULL == sig_name) {
	    readable += string(" -- signal ") + rcl::toString(terminating_signal);
	  } else {
	    readable += string(" -- SIG") + sig_name + " ("
	      + get_description_for_sig_num(terminating_signal) + ")";
	  }
	} else if (MR_ERROR_EXIT == new_status) {
	  readable += string(" (exit value ") + rcl::toString(exit_value)
	    + ")";
	}
	
	// also send status as a "stdout" response
        char line_text[1024];
	char text[1024];
	timeval now = getTime();
	snprintf(line_text, sizeof(line_text),
		 "MRAPTORD: [status] process status is now: %s",
		 readable.c_str());
	snprintf(text, sizeof(text),
		 "%lu.%06lu x %%%s",
		 now.tv_sec, now.tv_usec, line_text);
	_log_and_send_stdout_line(text, /* apply_rate_limiting = */ false);
        if(child_stdout) child_stdout->pushline(line_text);
      }
    }

    if (MR_RUNNING == new_status) {
      /* send any stdin_commands now that the process is ready for them */
      FOR_EACH (cmdP, stdin_commands) {
	send_stdin_line("config_stdin_commands", *cmdP);
      }
    }
  } /* if (is_local()) */
}

void Process_Impl::update_idle_time(void) {
  int old_periods = num_idle_periods;
  
  int new_periods = (int)
    ( timeDiff( getTime(), last_stdout_time ) / MR_IDLE_UPDATE_PERIOD );
  num_idle_periods = new_periods;
  
  if (old_periods != new_periods) {
#if 0
    cout << "update_idle_time: "
	 << "last_stdout_time = " << last_stdout_time.tv_sec
	 << ", new_periods = " << new_periods
	 << ", old_periods = " << old_periods << endl;
#endif
    parent.send_message(MR_SUBSCRIBED_MODULES, "status",
			rcl::toString(get_status_exp()));
  }
}

ostream& operator<<(ostream& out, const list<string>& lst) {
  out << "[";
  bool first = true;
  FOR_EACH (sp, lst) {
    if (!first) {
      out << ", ";
    }
    out << (*sp);
    first = false;
  }
  out << "]";
  return out;
}

void Process_Impl::print(std::ostream& out) const {
  out << "process_impl {" << endl;
  
  out << "  -------- config" << endl;
  out << "  name = " << name << endl;
  out << "  cmd = \"" << cmd << "\"" << endl;
  out << "  working_dir = \"" << working_dir << "\"" << endl;
  out << "  log_file = \"" << log_file << "\"" << endl;
  out << "  host = \"" << host << "\"" << endl;
  out << "  env = ";
  env.writeConfig(out, /* indent = */ 2);
  out << endl;
  out << "  ready = \"" << ready << "\"" << endl;
  out << "  kill_cmd = \"" << kill_cmd << "\"" << endl;
  out << "  backup_kill_cmd = \"" << backup_kill_cmd << "\"" << endl;
  out << "  backup_kill_delay = " << backup_kill_delay << endl;
  out << "  depends = " << depends << endl;
  
  out << endl;
  out << "  -------- status" << endl;
  out << "  status = " << Process_Status::to_string(status) << endl;
  out << "  pid = " << pid << endl;
  out << "  last_stdout_time = " << timevalToString(last_stdout_time) << endl;
  out << "  exit_time = " << timevalToString(exit_time) << endl;
  out << "  logging = " << (logging ? "true" : "false") << endl;
  out << "  last_log_flush_time = " << timevalToString(last_log_flush_time) << endl;
  out << "  num_idle_periods = " << num_idle_periods << endl;
  out << "  need_to_report_pending = " << (need_to_report_pending ? "true" : "false") << endl;
  
#if MR_USE_RATE_LIMITING
  out << endl;
  out << "  -------- rate limiting" << endl;
  out << "  rate_limit_period_start = " << timevalToString(rate_limit_period_start) << endl;
  out << "  rate_limit_lines_during_period = " << rate_limit_lines_during_period << endl;
  out << "  rate_limit_periods_since_stdin = " << rate_limit_periods_since_stdin << endl;
#endif
  
  out << "}" << endl;
}

std::ostream& operator<<(std::ostream& out, const Process_Record& proc) {
  proc.print(out);
  return out;
}

/***************************************************************************
 * REVISION HISTORY:
 * $Log: processRecord.cc,v $
 * Revision 1.28  2008/04/08 18:27:41  dom
 * Updated unused_file_name() to check for overflow after stat() on large files.
 *
 * Revision 1.27  2007/02/23 21:56:25  brennan
 * Guarded child_stdout calls to push MRAPTORD: status messages onto buffer;
 * we were seeing some crashes probably stemming from this.
 *
 * Revision 1.26  2006/12/20 19:06:54  brennan
 * 'MRAPTORD:' outputs are now pushed into the relevant line buffers,
 * so that they're returned when a playback is requested.
 *
 * Revision 1.25  2006/11/16 18:55:43  brennan
 * Made rate limits (the period, limit, and limit-after-stdin) command
 * line arguments for mraptord, and added options to mraptord.conf.
 *
 * Split up rate limit warning into a two messages (a continued line and
 * its terminator), since it was getting truncated by mraptord's
 * 80-character line limit.
 *
 * Revision 1.24  2006/11/13 01:52:58  trey
 * switched "run" command under debugger to "set args" command
 *
 * Revision 1.23  2006/06/23 16:41:11  trey
 * added support for daemon "signal" command and for running under a debugger
 *
 * Revision 1.22  2006/06/21 22:16:30  trey
 * swapped order of status logging messages so that they are less confusing
 *
 * Revision 1.21  2006/06/21 21:31:52  trey
 * added support for stdin_commands and debugger config fields; added extra status information that is now logged and sent as stdout responses to clients
 *
 * Revision 1.20  2005/08/09 02:29:08  trey
 * added debug statement, hoping to squash bug where backup kill command does not get executed
 *
 * Revision 1.19  2005/06/27 19:26:25  trey
 * turned off DEBUG_FDS
 *
 * Revision 1.18  2005/06/27 19:04:27  trey
 * attempted to fix defunct bug by removing all dependence on ipc timers
 *
 * Revision 1.17  2005/06/24 17:59:27  trey
 * fixed problems with closing files
 *
 * Revision 1.16  2005/06/24 00:17:18  trey
 * removed C_FILE_STREAMS ifdefs, C_FILE_STREAMS is now always on
 *
 * Revision 1.15  2005/06/12 21:20:51  trey
 * remove serial_depends field
 *
 * Revision 1.14  2004/12/10 01:27:52  trey
 * fixed problem with one-shot timers clobbering other one-shot timers
 *
 * Revision 1.13  2004/08/11 19:38:34  trey
 * possibly fixed bug of occasional missing prompts
 *
 * Revision 1.12  2004/08/03 02:00:31  trey
 * *finished* fixing the shadowed variable declaration bug
 *
 * Revision 1.11  2004/08/02 20:28:26  trey
 * fixed bug with shadowed declaration of "status" variable
 *
 * Revision 1.10  2004/08/02 18:57:44  trey
 * sigh... tried to fix stderr output again
 *
 * Revision 1.5  2004/05/27 13:58:37  trey
 * added support for stdout_wait_for_end_of_line (changed default behavior to *NOT* wait for end-of-line)
 *
 * Revision 1.4  2004/05/26 15:21:53  trey
 * fixed behavior of restart command
 *
 * Revision 1.3  2004/05/26 14:10:04  trey
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
