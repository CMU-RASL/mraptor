/* Tell emacs this is a -*- c++ -*- header
   $Revision: 1.70 $

   $Date: 2006/12/08 16:02:21 $

   $Author: trey $
   @file raptorDaemon.h
*/

#ifndef raptorDaemon_H
#define raptorDaemon_H

/**********************************************************************
 * INCLUDES
 **********************************************************************/

#include "processRecord.h"
#include "mrSets.h"
#include "mrTimers.h"

/**********************************************************************
 * MAIN CLASS
 **********************************************************************/

struct Raptor_Daemon : public FSlotObject {
  string get_this_host_name(void) { return comm->get_this_host_name(); }

  int write_to_pid(pid_t target, const char * data);
  
  void perform_command(const string& which_client, const string& input);
  static void static_perform_command(const string& which_client,
				     const string& text,
				     void *client_data);
  static void static_connect(const string& which_client,
			     void *client_data);
  void connect(const string& which_client);

  static void static_disconnect(const string& which_client,
			     void *client_data);
  void disconnect(const string& which_client);

  void send_message(const string& which_client,
		    const string& message_type,
		    const string& message_text);
  void send_command_response(const string& which_client,
			     int command_id,
			     const string& message_text);

  Raptor_Daemon(void);
  ~Raptor_Daemon();

  void run(int argc, char **argv);

protected:
  friend class Process_Record::Impl;

  static bool _running;
  static void _pipe_sig_handler(int x);

  static pair<string,Command_Handler> _handler_table_temp[];
  //static hash_map<string,Command_Handler,hash<string> > _handler_table;
  static hash_map<string,Command_Handler > _handler_table;

  //hash_map< string, Process_Record, hash<string> > proc_map;
  hash_map< string, Process_Record > proc_map;
  vector< Process_Record > proc_vec;
  rcl::map stored_groups;

  hash_map< string, rcl::exp > client_settings;

  bool use_prompt;
  bool be_interactive;
  string this_host;
  rcl::map stored_config;
  bool wait_central;
  bool debug_mode;
  // Rate limiting options; if -1, fall back on MR_RATE_* in processRecord.h
  int rate_limit_max_lines;
  int rate_limit_max_lines_after_stdin;
  double rate_limit_period;

  std::list<string> procs_with_status[MR_MAX_STATUS];
  hash_map<string, string> connected_daemons;
  string last_client_to_issue_run_command;
  timeval last_cleanup_exited_children_time;

  // make this a SmartRef so we don't have to instantly call the MR_Comm
  // constructor, but it still gets automatically deleted at
  // destructor-time.
  SmartRef< MR_Comm > comm;

  // serves as a guard so that _check_pending_processes() cannot call itself
  bool checking_pending_processes;

  // HACK there is an ipc bug wherein if you (1) subscribe to an fd, (2)
  // unsubscribe to the fd, (3) close the fd within an ipc handler,
  // sometimes (2) doesn't get processed properly before (3).
  // therefore, instead of doing (3) inside a handler, we queue fds in
  // fds_to_close and actually close them later.
  std::list<Subscribe_Entry> fds_to_subscribe;
  std::list<Subscribe_Entry> fds_to_unsubscribe;
  std::list<int> fds_to_close;
  std::list<FILE*> files_to_close;
  
  TimerQueue timers;

  Process_Record get_process(const string& process_name,
			     const string& action_text,
			     bool must_be_local = true);

  void _handle_set_variable(const string& which_client, int command_id,
			    const rcl::exp& e);
  void _handle_get_variable(const string& which_client,
			    int command_id,
			    const rcl::exp& e);
  void _handle_run_process(const string& which_client, int command_id,
			   const rcl::exp& e);
  void _handle_run_slave(const string& which_daemon, int command_id,
			 const rcl::exp& e);
  void _handle_kill_process(const string& which_client, int command_id,
			    const rcl::exp& e);
  void _handle_restart_process(const string& which_client, int command_id,
			       const rcl::exp& e);
  void _handle_send_ipc_message(const string& which_client, int command_id,
				const rcl::exp& e);
  void _handle_subscribe(const string& which_client, int command_id,
			 const rcl::exp& e);
  void _handle_unsubscribe(const string& which_client, int command_id,
			   const rcl::exp& e);
  void _handle_load_config_file(const string& which_client, int command_id,
				const rcl::exp& e);
  void _handle_send_to_process_stdin(const string& which_client,
				     int command_id,
				     const rcl::exp& e);
  void _handle_get_clock(const string& which_client,
			 int command_id,
			 const rcl::exp& e);
  void _handle_cancel(const string& which_client,
		      int command_id,
		      const rcl::exp& e);
  void _handle_shout(const string& which_client,
		      int command_id,
		      const rcl::exp& e);
  void _handle_signal(const string& which_client,
		      int command_id,
		      const rcl::exp& e);
  void _handle_response(const string& which_client,
			int command_id,
			const rcl::exp& e);
  void _handle_status(const string& which_client,
		      int command_id,
		      const rcl::exp& e);
  void _handle_help(const string& which_client, int command_id,
		    const rcl::exp& e);
  void _handle_quit(const string& which_client, int command_id,
		    const rcl::exp& e);

  void _shutdown(void);
  static void _static_shutdown(void);
  static void _static_signal_handler(int sig);
  void _kill_processes(const string& which_client,
		       const list<string>& proc_or_group_list,
		       bool do_restart = false);

  static void _static_cleanup_exited_children(void *clientData);
  void _cleanup_exited_children(void);

  void _command_help(const string& which_client);
  void _options_help(void);
  void _prompt(void);
  void _subscribe(const string& which_client, const string& message_type);
  void _unsubscribe(const string& which_client, const string& message_type);
  void _playback(const string& which_client,
		 const string& process_name,
		 int playback_lines);
  void _set_process_config(const string& process_name, const rcl::exp& config);
  void _get_process_config(const string& which_client,
			   const string& process_name);
  void _flush_logs(void);
  static void _static_flush_logs(void *client_data);
  void _flush_comm(void);
  static void _flush_comm(void *client_data);

  void _update_clock(const string& which_client);
  static void _update_clock(void *client_data);

  void _update_process_idle_time(const string& which_client);
  static void _update_process_idle_time(void *client_data);

  void _expand_groups(const list<string>& proc_or_group_list,
		      list<string>& proc_list,
		      const string& calling_command);
  void _execute(const string& proc,
		const string& which_client);
  void _execute(const list<string>& proc_names, bool from_daemon,
		const string& which_client);
  void _cancel_pending_processes(void);
  void _check_pending_processes(void);
  bool _check_pending_processes_internal(void);
  void _load_config_file(const string& config_file);
  void _update_for_new_config_global(const rcl::exp& incoming_config,
				     bool is_prior);
  void _update_for_new_config_process(const string& proc_name,
				      const rcl::exp& e);

  void _ipc_cleanup_workaround(void);

  int _get_rate_limit_max_lines();
  int _get_rate_limit_max_lines_after_stdin();
  double _get_rate_limit_period();

  // sets the value of config_exp to the current config
  void get_config(rcl::exp& config_exp);
};

extern Raptor_Daemon *active_daemon_g;

#endif //ifndef raptorDaemon_H

/*REVISION HISTORY

  $Log: raptorDaemon.h,v $
  Revision 1.70  2006/12/08 16:02:21  trey
  replaced "dshout <client> <message>" command with "shout <message>"

  Revision 1.69  2006/11/16 18:55:43  brennan
  Made rate limits (the period, limit, and limit-after-stdin) command
  line arguments for mraptord, and added options to mraptord.conf.

  Split up rate limit warning into a two messages (a continued line and
  its terminator), since it was getting truncated by mraptord's
  80-character line limit.

  Revision 1.68  2006/06/23 16:42:07  trey
  added support for daemon "signal" command

  Revision 1.67  2006/06/21 21:32:35  trey
  added and rearranged arguments to some functions; things are now a bit more consistent

  Revision 1.66  2006/06/14 01:40:52  trey
  added "sub shout" and "dshout" commands

  Revision 1.65  2005/06/27 19:26:46  trey
  cleaned up some cruft from debugging

  Revision 1.64  2005/06/27 19:04:27  trey
  attempted to fix defunct bug by removing all dependence on ipc timers

  Revision 1.63  2005/06/24 18:00:59  trey
  major changes to eliminate some uses of std::hash_map, which seems to have weird behavior; also fixed file closing issues

  Revision 1.62  2005/06/13 20:03:46  trey
  fixed problem with kill -a on multiple daemons, fixed confusing error message for bad process name

  Revision 1.61  2005/06/12 21:22:03  trey
  removed serial run option, added _reset_cleanup timer that may fix the defunct bug

  Revision 1.60  2005/05/18 01:06:39  trey
  added support for output buffering on comm traffic

  Revision 1.59  2004/05/26 15:21:54  trey
  fixed behavior of restart command

  Revision 1.58  2004/05/26 14:10:05  trey
  added support for restart command

  Revision 1.57  2004/04/19 14:49:26  trey
  split raptorDaemon.cc into two files

  Revision 1.56  2004/04/09 20:31:33  trey
  major enhancement to rcl interface

  Revision 1.55  2004/02/12 21:17:40  trey
  now catch SIGHUP and remove dependence on tca.h

  Revision 1.54  2004/02/12 20:55:10  trey
  made shutdown cleaner

  Revision 1.53  2004/02/10 17:39:44  brennan
  Made things happy with gcc 3.0.3.

  Revision 1.52  2003/11/21 19:47:47  trey
  killing a process now issues SIGTERM, wait 5 seconds, SIGKILL; may improve cleanup for some processes

  Revision 1.51  2003/11/15 17:53:40  brennan
  Split up CYGWIN #define into more reasonable/understandable chunks.  Completed move to FIRE build system.

  Revision 1.50  2003/10/29 23:54:15  trey
  fixed weird bug that was causing mraptord to hang when a process was started and killed multiple times; added debug print() function for Process_Impl

  Revision 1.49  2003/10/05 02:55:06  trey
  re-fixed some gcc 3.2 issues

  Revision 1.48  2003/10/04 21:56:48  trey
  tweaked things to eliminate the need for the common/overlap directory and libmrcommonoverlap.a

  Revision 1.47  2003/08/28 02:18:38  trey
  enabled multiple -r options to run multiple processes; fixed problem with -p (parallel execution) expansion; found a work-around for problems that relate to running central under mraptord

  Revision 1.46  2003/08/19 15:51:31  brennan
  Changes made in Acapulco during IJCAI03

  Revision 1.46  2003/08/12 05:54:47  brennan
  Turned down max number of lines before line limiting.

  Revision 1.45  2003/08/08 19:19:58  trey
  made changes to enable compilation under g++ 3.2

  Revision 1.44  2003/08/08 15:29:27  brennan
  Attempted to make microraptor compile:
    (1) on Cygwin and
    (2) under gcc 3.2
  This attempt has not yet succeeded

  Revision 1.43  2003/08/08 15:20:23  brennan
  client: added ZVT terminal display option
  daemon: no net effect.

  Revision 1.42  2003/06/14 22:28:40  trey
  added debug mode option

  Revision 1.41  2003/06/10 01:44:10  brennan
  Upped the max lines limit.

  Revision 1.40  2003/06/09 16:16:17  trey
  fixed useless reporting of status "pending" just before status "running"

  Revision 1.39  2003/06/06 17:55:13  trey
  relaxed rate limiting of process stdout right after user gives stdin input

  Revision 1.38  2003/06/06 16:13:08  trey
  new config merging code ensures that the latest config is the one everyone gets

  Revision 1.37  2003/06/06 01:46:08  trey
  added support for syncing config between daemons

  Revision 1.36  2003/03/24 16:32:56  trey
  switched central-specific rate limiting for a general form that keeps the stdout line rate for each process below about 20 Hz

  Revision 1.35  2003/03/22 21:56:38  trey
  added "set client" command

  Revision 1.34  2003/03/18 00:44:09  trey
  added custom kill command feature

  Revision 1.33  2003/03/17 00:52:41  trey
  fixed problem with random ordering of processes in claw display

  Revision 1.32  2003/03/14 04:22:42  trey
  added idle time updates

  Revision 1.31  2003/03/14 03:01:12  trey
  rationalized command names, added ability to subscribe to config and status messages

  Revision 1.30  2003/03/13 15:51:05  trey
  fixed problem with names that refer to both process and group

  Revision 1.29  2003/03/12 15:54:02  trey
  lots of new features, including run or kill groups by name and new command-line options for starting central under mraptord

  Revision 1.28  2003/03/11 02:56:14  trey
  added forwarding of error messages from slave daemon to the right client

  Revision 1.27  2003/03/11 01:52:36  trey
  modified get_config and set_config commands to add "groups" option to config file

  Revision 1.26  2003/03/11 00:34:37  trey
  added default serial execution when multiple processes are passed with the run command, and -p option to disable serial execution

  Revision 1.25  2003/03/10 22:55:22  trey
  added dependency checking

  Revision 1.24  2003/03/10 17:18:29  trey
  made get_clock have a subscribe functionality instead of only giving an immediate response

  Revision 1.23  2003/03/10 16:53:46  trey
  added ready monitoring

  Revision 1.22  2003/03/10 16:02:40  trey
  added new process status values: pending and starting

  Revision 1.21  2003/03/10 05:35:11  trey
  added get_clock command and ability of daemons to connect to each other

  Revision 1.20  2003/03/09 22:29:46  trey
  made log file names unique (changed log_dir to log_file with template patterns), added ability to suppress stdout history lines already received by client

  Revision 1.19  2003/03/03 17:21:40  trey
  fixed problem with one-char-at-a-time stdout data, improved handling of run -a and kill -a

  Revision 1.18  2003/02/25 21:43:40  trey
  added clock response so client can correct for clock skew

  Revision 1.17  2003/02/24 22:09:21  trey
  fixed reading of perl config files

  Revision 1.16  2003/02/20 19:57:45  trey
  made command names for mrterm and mraptord more consistent, added better help facilities to mrterm

  Revision 1.15  2003/02/20 15:37:34  trey
  fixed config response to actually tell you the name of the process :)

  Revision 1.14  2003/02/20 15:21:57  trey
  switched to get_config/set_config commands, added exit_time and last_stdout_time fields in status, also cleaned out some old stuff

  Revision 1.13  2003/02/19 20:01:43  trey
  major code cleaning

  Revision 1.12  2003/02/19 10:18:55  trey
  updated to use new clientComm version

  Revision 1.11  2003/02/19 00:46:51  trey
  implemented logging, updated format of stdout responses, improved cleanup

  Revision 1.10  2003/02/17 16:22:04  trey
  implemented IPC client connection

  Revision 1.9  2003/02/16 06:39:36  trey
  added stdin command

  Revision 1.8  2003/02/16 06:20:38  trey
  minor fixes, added -p command-line option, prompting now off by default

  Revision 1.7  2003/02/16 04:27:50  trey
  added playback capability, load command, fixed many bugs

  Revision 1.6  2003/02/16 02:31:10  trey
  full initial command set now implemented

  Revision 1.5  2003/02/14 21:54:10  trey
  made status reporting mostly work

  Revision 1.4  2003/02/14 18:30:33  trey
  many fixes, started implementation of kill, ipc, get commands

  Revision 1.3  2003/02/14 03:28:12  trey
  fixed some pipe issues

  Revision 1.2  2003/02/13 23:21:38  trey
  moved Execute_Exception to safeSystem.h

  Revision 1.1  2003/02/13 22:41:30  trey
  renamed files

  Revision 1.3  2003/02/06 16:35:57  trey
  fixed include after name change

  Revision 1.2  2003/02/05 22:50:21  trey
  remote runner now uses rcl, at least a bit

  Revision 1.1  2003/02/05 03:57:54  trey
  initial atacama check-in


  (c) Copyright 2003 CMU. All rights reserved.
*/
