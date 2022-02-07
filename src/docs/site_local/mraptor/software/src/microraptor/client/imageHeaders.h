/***** tell emacs we use -*- c++ -*- style comments *****
 * $Revision: 1.5 $ $Author: brennan $ $Date: 2006/11/16 18:55:04 $
 *
 * COPYRIGHT 2004, Carnegie Mellon University 
 *
 * PROJECT: Exploration Robotics (Life in the Atacama)
 *
 * MODULE:
 *
 * FILE: microraptor/client/imageHeaders.h
 *
 * DESCRIPTION:
 *
 ********************************************************/

#ifndef NO_BIG_XPMS
#  if (__GNUC__ == 3 && __GNUC_MINOR__ > 0 && __GNUC_MINOR__ < 3)
#    define NO_BIG_XPMS 1
#  endif
#endif

#ifndef NO_BIG_XPMS

extern char *asteroid_h[];
extern char *atmosphereImpact1_h[];
extern char *atmosphereImpact2_h[];
extern char *oceanImpact_h[];
extern char *brontoMeteor_h[];
extern char *flamingAtmosphere_h[];
extern char *meteorBlue_xpm[];
extern char *meteorCartoon_xpm[];
extern char *meteorCrater_xpm[];
extern char *meteorFlaming_xpm[];
extern char *flamingDinos_h[];
extern char *microRaptor_xpm[];

#endif

extern char *blankImage_xpm[];
extern char *hideImage_xpm[];
extern char *killImage_xpm[];
extern char *maximizeImage_xpm[];
extern char *runImage_xpm[];
extern char *subImage_xpm[];
extern char *throwupImage_xpm[];
extern char *unsubImage_xpm[];
extern char *miniClawImage_xpm[];
extern char *flagImage_xpm[];
/***************************************************************
 * Revision History:
 * $Log: imageHeaders.h,v $
 * Revision 1.5  2006/11/16 18:55:04  brennan
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
 * Revision 1.4  2004/04/28 18:58:50  dom
 * Appended log directive
 * 
 ***************************************************************/
