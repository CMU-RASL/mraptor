/***** tell emacs we use -*- c++ -*- style comments *****
 * $Revision: 1.17 $ $Author: brennan $ $Date: 2006/11/16 18:55:04 $
 *
 * COPYRIGHT 2004, Carnegie Mellon University 
 *
 * PROJECT: Exploration Robotics (Life in the Atacama)
 *
 * MODULE:
 *
 * FILE: microraptor/client/clientIO.h
 *
 * DESCRIPTION:
 *
 ********************************************************/
#ifndef __CLIENTIO_H__
#define __CLIENTIO_H__

#include "RCL.h"

/* clientIO.h
     A batch of wrapper functions to abstract away communication with
     the server / daemon process.
*/

/* Run a group; the daemon does multi-daemon serialization, so send it to the
   daemon with the lowest skew time (which /might/ be a good estimate of 
   transmission lag, but isn't, really.
*/
void run_group(const char* groupList);

/* Kill a group; the daemon does multi-daemon serialization, so send it to the
   daemon with the lowest skew time (which /might/ be a good estimate of 
   transmission lag, but isn't, really.
*/
void kill_group(const char* groupList);

/* Signal a group; the daemon does multi-daemon serialization, so send
   it to the daemon with the lowest skew time (which /might/ be a good
   estimate of transmission lag, but isn't, really.
*/
void signal_group(const char* groupList, const char* signal);

/* Run processes defined in a space-separated list in processList.
   A null value of processList results in running all known processes.
   Returns true on success, false on failure.
*/
bool run_process(char* machList, char* processList);

/* Kill processes defined in a space-separated list in processList.
   A null value of processList results in killing all known processes.
   Returns true on success, false on failure.
*/
bool kill_process(char* machList, char* processList);

/* Signal processes defined in a space-separated list in processList.
   Returns true on success, false on failure.
*/
bool signal_process(char* machList, char* processList, const char* signal);

/* Issues quit command to server */
void kill_server(const char* machName);

/* Sends text to stdin of process */
void stdin_to_process(const char* machName,
                      const char* processName, 
                      const char* data);

/* Wrapper for sub_process; grabs number of playback lines from
   preferences. 
   returns true if successful, false if already subscribed 
*/
bool sub_process_prefs_playback(const char* machName,
                                const char* processName);

/* Subscribes to given process, with playback playback lines 
   returns true if successful, false if already subscribed 
*/
bool sub_process(const char* machName,
                 const char* processName, 
                 int playback);

/* Unsubscribes to given process 
   returns true if successful, false if already unsubscribed 
*/
bool unsub_process(const char* machName,
                   const char* processName);

/* Requests numlines old lines for the given process */
void request_playback(const char* machName,
                      const char* processName,
                      int numlines);

/* Sends a get_clock, which subscribes to once-a-minute clock
   message */
void get_clock(const char* machName);

/* Requests status of a process or processes.
   Returns true on success, false on failure.
 */
bool get_status_process(const char* machName,
                        const char* processName);

/* Requests the current configuration of 
   a process or processes 
   Returns true on success, false on failure.
*/
bool get_config_process(const char* machName,
                        const char* processName);

/* Sends local config file across the pipe to all
   connected daemons.
   Also calls get_status_process to update our
   local GUI */
void
send_local_config(rcl::exp local_config);

/* Loads remote config file 
   Returns true on success, false on failure.
*/
bool load_config(const char* machName,
                 const char* configFilename);

/* Sends a set command */
void set_process(const char* machName,
                 const char* host,
                 const char* processName,
                 const char* cmd,
                 const char* working_dir,
                 const char* log_dir,
                 const char* env);
#endif /* __CLIENTIO_H__ */



/***************************************************************
 * Revision History:
 * $Log: clientIO.h,v $
 * Revision 1.17  2006/11/16 18:55:04  brennan
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
 * Revision 1.16  2004/04/28 18:58:50  dom
 * Appended log directive
 * 
 ***************************************************************/
