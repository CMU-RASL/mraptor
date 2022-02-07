/********** tell emacs we use -*- c++ -*- style comments *******************
 * COPYRIGHT 2004, Carnegie Mellon University
 * $Revision: 1.6 $  $Author: brennan $  $Date: 2006/11/16 18:55:04 $
 *  
 * PROJECT: FIRE Architecture Project
 *
 * @file    ClawText.h
 * @brief   No brief
 ***************************************************************************/

#ifndef INCClawText_h
#define INCClawText_h

/*

A ClawText object is a wrapper around a GtkText object that supports

- tracking the length of the widget and deleting old text if it gets too
  long for efficient insertions.

- scrolling the widget to track the end of the buffer as new text is
  inserted iff we are currently at the end of the buffer.

- emulating the ability of a terminal to send the CR (\r) character so
  that the next line we receive overwrites the current one.

*/

/**********************************************************************
 * INCLUDES
 **********************************************************************/

#include <gtk/gtk.h>
#include <string>

using namespace std;

/**********************************************************************
 * MACROS
 **********************************************************************/

#define CLAW_TEXT(x) ((ClawText *) x)

/* when the buffer length exceeds CLAW_TEXT_BUFFER_LINES_LIMIT, reduce
   it by deleting CLAW_TEXT_LINE_DELETION_INCREMENT lines */
#define CLAW_TEXT_BUFFER_LINES_LIMIT (1000)
#define CLAW_TEXT_LINE_DELETION_INCREMENT (100)

/**********************************************************************
 * CLASSES
 **********************************************************************/

struct ClawText {
  ClawText(GtkWidget *_text_widget);
  ~ClawText(void);

  void insert(const string& text_in,
	      char line_type,
	      GdkFont *font,
	      GdkColor *fore,
	      GdkColor *back);

  GtkWidget *get_text_widget(void) { return (GtkWidget *) text_widget; }
  char get_last_line_type(void) { return last_line_type; }

  void set_scroll_value(float newval) { scroll_value = newval; }
  float get_scroll_value() { return scroll_value; }

  static void ref(ClawText *ct);
  static void unref(ClawText *ct);

  void set_font_from_string (const char *font);

protected:
  int ref_count;

  GtkTextView *text_widget;

  // this data forms a ring buffer where we store the length of lines,
  // which allows us to efficiently handle overwriting and deleting
  // old data.
  //
  // there are num_stored_lines lines in the buffer.  their lengths
  // are stored in line_length[start_line] .. line_length[end_line-1],
  // where start_line = end_line - num_stored_lines.  all of this is
  // with modulo arithmetic for the line indices.
  int line_length[CLAW_TEXT_BUFFER_LINES_LIMIT];
  int end_line, num_stored_lines;

  // Used to provide persistent storage for the scroller position of the
  //  scrolled window associated with this ClawText.  At the moment, only
  //  used for the common (error) buffer and the chat buffer.
  float scroll_value;

  char last_line_type;

  static int normalize(int line_num) {
    return (line_num + CLAW_TEXT_BUFFER_LINES_LIMIT) % CLAW_TEXT_BUFFER_LINES_LIMIT;
  }
  void add_line(int len);
  void delete_first_n_lines(int num_lines);
  void delete_last_line(void);
};

#endif // INCClawText_h

/***************************************************************************
 * REVISION HISTORY:
 * $Log: ClawText.h,v $
 * Revision 1.6  2006/11/16 18:55:04  brennan
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
 * Revision 1.5  2004/04/28 17:15:20  dom
 * Added header with copyright information to source and Makefiles.
 *
 * Revision 1.4  2003/10/20 04:35:06  brennan
 * Fixed dumb RCL error (missing getValue call) and a few warnings from
 * gcc 3.3.
 *
 * Revision 1.3  2003/08/19 16:03:15  brennan
 * Conflict resolution.
 *
 * Revision 1.2  2003/06/14 15:49:05  brennan
 * Fixed set-config pointer bug.
 * Fixed RCL errors on config handling
 * Changed integration of config files
 * Fixed the doubling of group members on config file reload
 * Made error/common buffer "sticky" again
 * Group members' status now updates along with the primary nodes'
 *
 * Revision 1.1  2003/03/18 22:00:28  trey
 * fixed performance issue with stdout updates by limiting the stdout buffer size
 *
 *
 ***************************************************************************/
