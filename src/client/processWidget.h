/***** tell emacs we use -*- c++ -*- style comments *****
 * $Revision: 1.26 $ $Author: brennan $ $Date: 2007/05/30 19:10:31 $
 *
 * COPYRIGHT 2004, Carnegie Mellon University 
 *
 * PROJECT: Exploration Robotics (Life in the Atacama)
 *
 * MODULE:
 *
 * FILE: microraptor/client/processWidget.h
 *
 * DESCRIPTION:
 *
 ********************************************************/
#ifndef __PROCESSWIDGET_H__
#define __PROCESSWIDGET_H__

#include <gdk/gdk.h>
#include <gtk/gtkframe.h>
#include <gtk/gtktable.h>
#include <gtk/gtkentry.h>
#include <gtk/gtkscrolledwindow.h>
#include <gtk/gtktext.h>
#include <gtk/gtklabel.h>
#include <glib.h>
#include <set>
#include <regex.h>

#ifdef USE_ZVT_TERM
#include <gdk/gdkx.h>
#include <gdk/gdkprivate.h>
#include <gdk/gdkkeysyms.h>
//#include <zvt/zvtterm.h>
#endif

#include "ClawText.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define LOG_TEXT_OUT_BUF_MAX (8192)

#define PROCESSWIDGET(obj)          GTK_CHECK_CAST (obj, processWidget_get_type (), ProcessWidget)
#define PROCESSWIDGET_CLASS(klass)  GTK_CHECK_CLASS_CAST (klass, processWidget_get_type (), ProcessWidgetClass)
#define IS_PROCESSWIDGET(obj)       GTK_CHECK_TYPE (obj, processWidget_get_type ())

typedef struct _ProcessWidget       ProcessWidget;
typedef struct _ProcessWidgetClass  ProcessWidgetClass;

struct _ProcessWidget
{
  GtkFrame  mainFrame;
  GtkWidget *mainEventBox;
  GtkWidget *MainLayoutTable;
  GtkWidget *entryBox; // Used to hold stdin plus regexp entries
  GtkWidget *stdinEntry;
  bool entryExpanded; // Is the stdin or filter entry expanded?

  // Regexp foo; see http://www.delorie.com/gnu/docs/regex/regex_54.html
  GtkWidget *regexpLabel;
  GtkWidget *regexpEntry;
  GtkWidget *regexpToggle;
  GtkTooltips *RegexpTooltips; /* Tooltips for regexp label and entry widget */
  bool regexpActive;
  bool regexpEditing;

  // If I use a regular string, assignment crashes -- WTF???
  string* curRegexp;
  regex_t regexpPatternBuffer;
  GtkStyle *regexpActiveStyle;
  GtkStyle *regexpEditingStyle;
  GtkStyle *regexpInactiveStyle;

  // If I use a regular string, assignment crashes -- WTF???
  string* regexpBuffer;
  char regexpFastmap[256]; // ASCII encoding has 256 elements
  GtkWidget *regexpSubmenu;
  unsigned int regexpPreContext;
  unsigned int regexpPostContext;
  vector<string> regexpPreContextBuffer;
  int regexpPostContextRemaining;

#ifdef USE_ZVT_TERM
  GtkWidget *LogHbox;
  GtkWidget *LogScrollbar;
  ZvtTerm   *LogText;
  char last_line_type;
  /* buffer output to LogText for better performance */
  char LogTextOutBuf[LOG_TEXT_OUT_BUF_MAX];
  size_t LogTextOutBufBytes;
#else
  GtkWidget *LogScrolledWindow;
  ClawText  *LogText;
#endif
  GtkWidget *HeaderTable;
  GtkWidget *ProcessNameLabel;
  GtkWidget *ProcessStateLabel;
  GtkWidget *ProcessLastStdoutLabel;
  GtkWidget *ProcessURButtons; /* Buttons in upper right of window */
  GtkTooltips *ProcessURButtonsTooltips; /* Tooltips for buttons in upper right */
  GtkObject *windowAdjustment; // For "sticky" scrolling
  double     scrollValue;
  set<string>* viewers; // elements are the tab labels that are viewing this widget
  vector<string> entryHistory; // History of what lines have been entered
  int entryHistoryIndex; // 0 is text-in-progress; history adds to the end

  vector<string> regexpHistory; // History of what regexps have been entered
  int regexpHistoryIndex; // 0 is text-in-progress; history adds to the end
};

struct _ProcessWidgetClass
{
  GtkFrameClass parent_class;

  void (* processWidget) (ProcessWidget *pw);
};

guint          processWidget_get_type                 (void);
GtkWidget*     processWidget_new                      (const char* procName,
                                                       const char* procStatus,
                                                       const char* machName);
void           processWidget_button_smoke             (ProcessWidget *pw,
                                                       bool visible);

//#ifdef USE_ZVT_TERM
void processWidget_term_feed(ProcessWidget* pw,
			     char* text,
			     size_t textBytes);
void processWidget_term_flush(ProcessWidget* pw);
//#endif

// Manually calling this from claw_quit(); I couldn't get a
// destroy-event callback to do anything.
void
processWidget_cleanup (ProcessWidget* pw);

// Management functions for the regexp filter widgets
gboolean processWidget_expand_filter(ProcessWidget* pw);
gboolean processWidget_expand_stdin(ProcessWidget* pw);
void processWidget_hide_filter(ProcessWidget* pw);
void processWidget_show_filter(ProcessWidget* pw);
void processWidget_regexp_refresh_buffer(ProcessWidget* pw);
void processWidget_loadHistories(ProcessWidget* pw);
void processWidget_saveHistories(ProcessWidget* pw);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __PROCESSWIDGET_H__ */



/***************************************************************
 * Revision History:
 * $Log: processWidget.h,v $
 * Revision 1.26  2007/05/30 19:10:31  brennan
 *  - Fixed GTK warnings on preference editing cancellation, due to
 *    miscasting of checkboxes.
 *
 *  - New preferene: Remember location and size of window
 *
 *  - Improved initial auto-sizing of status/chat buffer
 *
 *  - Increased gutter sizes for pane resizers
 *
 *  - Fixed crash when claw is closed via the window manager
 *
 *  - When initially laying out the window, we now attempt to ensure that
 *    the process window will have enough space, so it's not cut off.
 *
 *  - Stdin and filter entry histories are now remembered across
 *    view/close of a process and quit/start of claw.  Added arg to
 *    specify history filename (defaults to ~/.claw.histories) and prefs
 *    to specify how many history entries to save per process.
 *
 *  - The initial running/exited state of all processes is no longer
 *    printed when claw starts up / daemons initially connect.  Status
 *    changes are still printed.
 *
 * Revision 1.25  2007/02/15 18:07:07  brennan
 * Fixed a few memory leaks, and one rogue pointer bug.
 * Mapped keypad enter to regular enter in stdin and filter entries.
 * Added focus-follows-mouse option.
 *
 * Revision 1.24  2007/01/12 19:42:56  brennan
 *  Added context menu to processWidget's filter entry that allows the
 *  user to enable or diable the filter (in the same way as the checkbox
 *  works), as well as launch a dialog to set the desired context
 *  (e.g. lines before and after a matching line that should be printed
 *  as well).
 *
 *  Hacked around the double-click-required on the signal and regexp
 *  context menus by responding to button-press-event as well as
 *  activate.
 *
 * Revision 1.23  2006/12/20 19:07:19  brennan
 *   We now use checkboxes in createHashDialog (e.g. local preferences)
 *   for elements whose default value is a length-two vector of 0 and 1,
 *   but whose value in the "real" hash is a long.
 *
 *   Turned filtering off by default
 *
 *   Claw now parses the standard X --geometry argument to set the
 *   default size and position of the window.
 *
 *   Moved load/save/edit preferences to a new 'Preferences' menu, and
 *   added all checkbox preferences directly to the menu (they still
 *   also appear in the "edit all preferences" dialog)
 *
 *   Fixed bug in which timestamps appeared multiple times in continued
 *   (>80 char) lines.
 *
 *   Fixed shout/dshout sillyness.
 *
 *   Fixed sysv init script errors.
 *
 *   Added buffer clear/reset option to right-click menus.  Will not add
 *   a button for this.  It was already in the 'Monitoring' menu.
 *
 *   Fixed bug resulting in incorrectly populated right-click menu for a
 *   process viewed in a group when the process' name was a substring of
 *   the group's name.  How's that for obscure bugs? :-)
 *
 *   When stdout filtering is turned on or off (either through the prefs
 *   dialog, or the prefs checkbox menu), immediately toggle filtering on
 *   all active process widgets, rather than waiting until they're
 *   hidden, then viewed.
 *
 *   Fixed localizeSite.sh to point search results to local pages, not
 *   gs295.
 *
 *   Updated dshout/shout to eliminate the username field. (code cleanup)
 *
 *   User's own chat messages are now displayed in green; everything else
 *   is in black.
 *
 *   Widened default filter width; now controlled by SHRUNK_WIDTH
 *   #define in processWidget.c
 *
 *   Just a note: Ctrl+U in a GtkEntry (e.g. stdin or filter) will clear
 *   all text in the entry.
 *
 *   The stdin entry / filter now remain visible when a process is dead,
 *   although stdin is disabled.  This allows the user to change the
 *   filtering and examine filtered output after a process has crashed.
 *   In addition, when a process is not running, clicks on the process
 *   widget focus the filter input.
 *
 *   The filter entry now retains a history, just like the stdin entry,
 *   that can be accessed via the up/down arrow keys.
 *
 *   Added taskbar icons for all dialogs.  For the moment, they're the
 *   same claw image as the main window; we may want to specialize them
 *   later.
 *
 *   Added a checkbox to the right of the filter that allows the user to
 *   enable or disable filtering without changing the filter text.  The
 *   [enter] approach is still supported (and checks the box when
 *   appropriate), and the checkbox is disabled when there is no text in
 *   the entry.
 *
 *   Filter now only goes red only if actual editing has occurred (not
 *   just a keypress).
 *
 *   The stdin entry line for a process widget is now greyed out when
 *   the process is dead.
 *
 *   Added footer pointing to www.microraptor.org to localized site.
 *
 *   Added release version stamp to the title png of the help page (pass
 *   version number as first argument to localizeSite.sh)
 *
 *   Added Help menu, and entry that launches a user-specified browser
 *   pointing at the RPM/APT-installed help pages.  The new preference is
 *   'help_browser'.
 *
 *   The background color of the filter entry now changes based on the
 *   active or editing state.  For reference, one must create a new
 *   GtkStyle and set it in order to override a theme engine set by the
 *   user.  GtkRcStyles do *not* do the trick.
 *
 *   We now set the background color of the 'Chat' tab if a new chat
 *   message comes in while the 'Status' tab is in the foreground.  To
 *   accomplish this, I had to disable user-theming of those two notebook
 *   tabs.
 *
 *   The 'Help' menu is now right-justified
 *
 *   Added an 'About' menu option under 'Help'.  We'll need to update the
 *   image with every release.  It's dismissed via a left-click, [enter],
 *   or [esc].
 *
 *   Fixed focus setup for filter/stdin.  When focus enters the filter or
 *   the stdin, it expands.  Much simpler, and does what we want.
 *
 *   Removed "get status", as it's no longer needed.  Left "Get status
 *   all" as "Refresh all processes' status" in the Monitoring menu,
 *   since there's an offchance you'll want to do that right at the
 *   beginning.
 *
 * Revision 1.22  2006/11/16 18:55:04  brennan
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
 * Revision 1.21  2005/05/18 01:04:58  trey
 * added terminal buffering
 *
 * Revision 1.20  2004/11/25 05:20:48  brennan
 * Fixed run-all bug (using a GList* instead of its data)
 *
 * Patched a bunch of memory leaks, and all the nasty ones.  There are four
 * known remaining leaks, all dealing with the widget used to view the output
 * of a process, with a sum total of ~90 bytes leaked per process viewed.
 * Not worth my time at the moment. :-)
 *
 * Revision 1.19  2004/11/23 07:01:54  brennan
 * A number of memory leaks and free/delete mismatches repaired.
 * Some errors detected by valgrind remain, and will be fixed soon.
 *
 * Revision 1.18  2004/05/23 17:43:35  brennan
 * Added tooltips to buttons in upper-right of processWidget.
 *
 * Revision 1.17  2004/04/28 18:58:50  dom
 * Appended log directive
 * 
 ***************************************************************/
