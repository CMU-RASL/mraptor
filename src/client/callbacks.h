/***** tell emacs we use -*- c++ -*- style comments *****
 * $Revision: 1.37 $ $Author: brennan $ $Date: 2007/04/18 17:13:28 $
 *
 * COPYRIGHT 2004, Carnegie Mellon University 
 *
 * PROJECT: Exploration Robotics (Life in the Atacama)
 *
 * MODULE:
 *
 * FILE: microraptor/client/callbacks.h
 *
 * DESCRIPTION:
 *
 ********************************************************/
#ifndef __CALLBACKS_H__
#define __CALLBACKS_H__

#include <gtk/gtk.h>
#include <string>
#include "RCL.h"
#include "support.h"

#define DEBUGCB( expression ) fprintf(stderr, expression);
//#define DEBUGCB( expression ) 

/* List of MR_Comm's; data points to a MR_Comm */
extern GList* daemon_connections;

void
add_error_trap                         ();

void
clear_error_traps                      ();

std::string munge_due_to(std::string toProcess, const std::string& prefix);

void 
connect_handler                        (const std::string& which_daemon, 
                                        void *client_data);

void 
disconnect_handler                     (const std::string& which_daemon, 
                                        void *client_data);

void 
response_handler                       (const std::string& machName, 
                                        const std::string& text, 
                                        void *client_data);

void
on_HeadDino_destroy                    (GtkObject       *object,
                                        gpointer         user_data);

bool
on_MainMenuBar_button_press            (GtkMenuShell    *menushell,
                                        gboolean         force_hide,
                                        gpointer         user_data);

void
on_MainMenuBar_selection_done          (GtkMenuShell    *menushell,
                                        gboolean         force_hide,
                                        gpointer         user_data);

void
on_get_config_file1_activate           (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_get_local_config_file1_activate     (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_reload_local_config_file1_activate  (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_checkbox_prefs_activate             (GtkCheckMenuItem *menuitem,
                                        gpointer         user_data);

void
on_set_prefs_activate                  (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_save_prefs_activate                 (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_load_prefs_activate                 (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_set_process1_activate               (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_swallow_window1_activate            (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_kill_daemon_activate                (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_exit2_activate                      (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_processes1_activate                 (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_signals1_activate                   (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_run_selected1_activate              (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_display_selected1_activate          (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_send_signal1_activate               (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_send_ipc_message1_activate          (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_get_output1_activate                (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_launch_browser_activate             (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_about_activate                      (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

gboolean
on_aboutDialog_button_release_event    (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data);

gboolean
on_aboutDialog_key_release_event       (GtkWidget       *widget,
                                        GdkEventKey     *event,
                                        gpointer         user_data);

void
on_contextDialog_okButton_clicked          (GtkButton       *button,
                                            gpointer         user_data);

void
on_contextDialog_cancelButton_clicked      (GtkButton       *button,
                                            gpointer         user_data);

gboolean
on_ProcessTree_button_release_event    (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data);

gboolean
on_ProcessTree_key_press_event         (GtkWidget       *widget,
                                        GdkEventKey     *event,
                                        gpointer         user_data);

gboolean
on_ProcessTree_button_release_event    (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data);

gboolean
on_ProcessTree_enter_notify_event      (GtkWidget *widget,
                                        GdkEventCrossing *event,
                                        gpointer user_data);

gboolean
on_HeadDino_key_press_event            (GtkWidget       *widget,
                                        GdkEventKey     *event,
                                        gpointer         user_data);

gboolean
on_stdinEntry_key_release_event        (GtkWidget       *widget,
                                        GdkEventKey     *event,
                                        gpointer         user_data);

void
on_view_selected1_activate             (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_view_all1_activate                  (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_hide_selected1_activate             (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
hide_all_helper                       (GtkCTree *ctree,
                                       GtkCTreeNode *node,
                                       gpointer data);

void
on_hide_all1_activate                  (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_pw_popup_reset_buffer_item_activate (GtkMenuItem    *menuitem,
                                        gpointer        user_data);

void
on_clear_cur_buffer_activate           (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_switch_to_overview_activate         (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_switch_to_next_tab_activate         (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_switch_to_prev_tab_activate         (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_run_selected1_activate              (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_run_all_selected1_activate          (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_kill_selected1_activate             (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_kill_all_selected1_activate         (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_pw_popup_signal_item_activate       (GtkMenuItem    *menuitem,
                                        gpointer        user_data);

void
on_pw_popup_signal_button_press_event (GtkWidget     *widget,
                                       GdkEventButton *event,
                                       gpointer user_data);

void
on_menu_signal_selected_activate       (GtkMenuItem    *menuitem,
                                        gpointer        user_data);

void
on_signal_selected_activate            (GtkMenuItem     *menuitem,
                                        gpointer         user_data,
                                        int              popup_mode);

void
on_subscribe_selected1_activate        (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_unsubscribe_selected1_activate      (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_refresh_status_all_activate         (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_processwidget_text_expanded         (GtkAdjustment   *adjustment,
                                        gpointer         user_data);

void
on_processwidget_generic_scrolled      (GtkAdjustment   *adjustment,
                                        gpointer         user_data);

void
on_errorOrChat_output_text_expanded    (GtkAdjustment   *adjustment,
                                        gpointer         user_data);

void
on_errorOrChat_output_generic_scrolled (GtkAdjustment   *adjustment,
                                        gpointer         user_data);

gboolean
on_errorOrChat_enter_notify_event      (GtkWidget *widget,
                                        GdkEventCrossing *event,
                                        gpointer user_data);

void
on_chat_input                          (GtkEntry         *entry,
                                        gpointer         user_data);

gint 
on_chat_output_button_press_event       (GtkWidget       *widget, 
                                         GdkEvent        *event);

void
on_run_button_clicked                  (GtkButton       *button,
                                        gpointer         user_data);

void
on_kill_button_clicked                 (GtkButton       *button,
                                        gpointer         user_data);

void
on_signal_button_clicked               (GtkButton       *button,
                                        gpointer         user_data);

void
on_sub_button_clicked                  (GtkButton       *button,
                                        gpointer         user_data);

void
on_unsub_button_clicked                (GtkButton     *button,
                                        gpointer         user_data);

void
on_hide_button_clicked                 (GtkButton        *button,
                                        gpointer         user_data);

void
on_maximize_button_clicked             (GtkButton        *button,
                                        gpointer         user_data);

void
on_notebook_page_switched              (GtkNotebook *notebook,
                                        GtkNotebookPage *page,
                                        gint page_num,
                                        gpointer user_data);


void
on_error_and_chat_notebook_page_switched (GtkNotebook *notebook,
                                          GtkNotebookPage *page,
                                          gint page_num,
                                          gpointer user_data);


void
on_loadFile_loadButton_clicked         (GtkButton       *button,
                                        gpointer         user_data);

void
on_loadFile_cancelButton_clicked       (GtkButton       *button,
                                        gpointer         user_data);

void
on_loadFile_filenameEntry_activate     (GtkEntry        *entry, 
                                        gpointer         user_data);

void
on_setDialog_okButton_clicked          (GtkButton       *button,
                                        gpointer         user_data);

void
on_setDialog_cancelButton_clicked      (GtkButton       *button,
                                        gpointer         user_data);

void
on_localFileSelection_load_button_clicked (GtkButton       *button,
                                           gpointer         user_data);

void
on_localFileSelection_cancel_button1_clicked (GtkButton       *button,
                                              gpointer         user_data);

void
on_savePrefsFileSelection_load_button_clicked (GtkButton       *button,
                                               gpointer         user_data);

void
on_savePrefsFileSelection_cancel_button1_clicked (GtkButton       *button,
                                                  gpointer         user_data);

void
on_loadPrefsFileSelection_load_button_clicked (GtkButton       *button,
                                               gpointer         user_data);

void
on_loadPrefsFileSelection_cancel_button1_clicked (GtkButton       *button,
                                                  gpointer         user_data);

void
on_pickDaemon_daemonEntry_activate     (GtkEditable     *editable,
                                        gpointer         user_data);

void
on_pickDaemon_connectButton_clicked    (GtkButton       *button,
                                        gpointer         user_data);

void
on_pickDaemon_cancelButton_clicked     (GtkButton       *button,
                                        gpointer         user_data);

void
on_pickTargetDialog_killButton_clicked (GtkButton       *button,
                                        gpointer         user_data);

void
on_pickTargetDialog_cancelButton_clicked (GtkButton       *button,
                                          gpointer         user_data);

void
on_cancelLaunchDialog_cancelButton_clicked (GtkButton     *button,
                                            gpointer       user_data);

void
on_pickDialogForLocalFileSelection_done   (GtkWidget       *object,
                                           gpointer        user_data);

/* Callback for swallowed window killage */
void
on_swallowed_window_kill_button_clicked (GtkWidget       *button,
                                         gpointer         user_data);

void
on_swallowed_window_throwup_button_clicked (GtkWidget       *button,
                                            gpointer         user_data);

void
popup_menu_showandtell                 (GtkWidget* pw,
                                        ProcessData* pd,
                                        bool grpViewed = false);

/* Callbacks for popup menus */
gint
on_ProcessTree_button_press_event      (GtkWidget       *widget, 
                                        GdkEvent        *event);

gint 
on_pw_LogText_button_press_event       (GtkWidget       *widget, 
                                        GdkEvent        *event);
#if 1
gint 
on_pw_LogText_size_allocate_event      (GtkWidget       *widget, 
                                        GtkAllocation   *alloc);
#endif
void
on_pw_popup_run_item_activate          (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_pw_popup_kill_item_activate         (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_pw_popup_view_item_activate         (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_pw_popup_hide_item_activate         (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_pw_popup_sub_item_activate          (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_pw_popup_unsub_item_activate        (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_pw_popup_set_config_item_activate   (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

/* Dialog created dynamically from supplied rcl::exp 
   (see create_editHashDialog in interface.[c|h]) */

void
on_editHashDialog_okButton_clicked     (GtkWidget       *button,
                                        gpointer         user_data);

void
on_editHashDialog_cancelButton_clicked (GtkWidget       *button,
                                        gpointer         user_data);

gboolean
on_editHashDialog_key_press_event (GtkWidget *widget,
                                   GdkEventKey *event,
                                   gpointer user_data);

/* User callback functions for use with editHashDialog */
void
on_process_config_changed              (rcl::exp e);

#endif /*__CALLBACKS_H__ */








/***************************************************************
 * Revision History:
 * $Log: callbacks.h,v $
 * Revision 1.37  2007/04/18 17:13:28  brennan
 * Fixed occasional crash in claw.
 *
 * Revision 1.36  2007/02/15 18:37:06  brennan
 * Mapped keypad-enter to activate entries in autogenerated dialogs,
 * such as preferences and process configs.
 *
 * Revision 1.35  2007/02/15 18:07:06  brennan
 * Fixed a few memory leaks, and one rogue pointer bug.
 * Mapped keypad enter to regular enter in stdin and filter entries.
 * Added focus-follows-mouse option.
 *
 * Revision 1.34  2007/01/12 19:42:56  brennan
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
 * Revision 1.33  2006/12/20 19:07:18  brennan
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
 * Revision 1.32  2006/11/16 18:55:04  brennan
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
 * Revision 1.31  2006/08/11 22:05:37  brennan
 * Committed detection of the BadWindow errors which occur when you ssh -X
 * into an X server with the security extension.  A semi-informative error
 * message is printed, and claw now functions, but right-click menus are
 * funkily placed.  Use ssh -Y instead.
 *
 * Revision 1.30  2004/04/28 18:58:50  dom
 * Appended log directive
 * 
 ***************************************************************/
