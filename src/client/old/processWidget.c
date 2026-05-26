/***** tell emacs we use -*- c++ -*- style comments *****
 * $Revision: 1.46 $ $Author: brennan $ $Date: 2007/05/30 19:10:31 $
 *
 * COPYRIGHT 2004, Carnegie Mellon University 
 *
 * PROJECT: Exploration Robotics (Life in the Atacama)
 *
 * MODULE:
 *
 * FILE: microraptor/client/processWidget.c
 *
 * DESCRIPTION:
 *
 ********************************************************/
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <map>

#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>

#include "callbacks.h"
#include "interface.h"
#include "support.h"
#include "processWidget.h"
#include "clientIO.h"
#include "imageHeaders.h"

using namespace std;

// Declared in claw.c with the rest of the globals
extern map<string, vector<string> > persistentStdinCommandHistory_g;
extern map<string, vector<string> > persistentRegexpHistory_g;


// moved from header to avoid warning
static void    processWidget_class_init               (ProcessWidgetClass *pwclass);
static void    processWidget_init                     (ProcessWidget *pw);
static void    processWidget_stdin_activate           (GtkEntry *entry, ProcessWidget *ppw);

// Width of the shrunk stdin or filter input, in pixels
#define SHRUNK_WIDTH (80)

void
processWidget_button_smoke (ProcessWidget *pw, bool visible)
{
  if(pw == NULL) return;
  if(visible)
    gtk_widget_show(pw->ProcessURButtons);
  else
    gtk_widget_hide(pw->ProcessURButtons);
}

static gboolean
processWidget_generic_history_handling(GtkWidget *widget,
                                       GdkEventKey *event,
                                       vector<string>& history,
                                       int* index)
{
  if(event->keyval == GDK_Up) {
    gtk_signal_emit_stop(GTK_OBJECT(widget), gtk_signal_lookup("key-press-event", gtk_entry_get_type()));
    if(history.size() > 1) {
      if(*index == 0) {
        // Move to end of history
        *index = history.size() - 1;
        // Store current text
        history[0] = gtk_entry_get_text(GTK_ENTRY(widget));
      } else if(*index == 1) {
        // Do nothing; we're at the top of our history
        return true;
      } else {
        // Move one element up in the history
        (*index)--;
      }
    } else { return true; }
  } else if(event->keyval == GDK_Down) {
    gtk_signal_emit_stop(GTK_OBJECT(widget), gtk_signal_lookup("key-press-event", gtk_entry_get_type()));
    if(*index != 0) {
      if(*index == ((int) history.size()) - 1) {
        // Hit the end; wrap back around to temp
        *index = 0;
      } else {
        // Move one element down in the history
        (*index)++;
      }
    } else { return true; }
  } else {
    //return true;
    return false;
  }
  gtk_entry_set_text(GTK_ENTRY(widget), history[*index].c_str());
  return true;
}

static gboolean
processWidget_key_press_event (GtkWidget *widget,
                               GdkEventKey *event,
                               gpointer user_data)
{
  DEBUGCB("processWidget_key_press_event\n");
  ProcessWidget* pw = (ProcessWidget*) user_data;
  // If the key is keypad-enter, activate as if we had pressed Return.
  if(event->keyval == GDK_KP_Enter) {
    gtk_signal_emit_stop(GTK_OBJECT(widget), gtk_signal_lookup("key-press-event", gtk_entry_get_type()));
    processWidget_stdin_activate (GTK_ENTRY (widget), pw);
    return true;
  }
  return processWidget_generic_history_handling(widget, event, pw->entryHistory, &(pw->entryHistoryIndex));
}

// Forward declaration, since we need it here, but its proper place is
// further down.
static void
processWidget_regexp_activate (GtkEntry *entry, ProcessWidget *ppw);
static gboolean
processWidget_regexp_key_press_event (GtkWidget *widget,
                                      GdkEventKey *event,
                                      gpointer user_data)
{
  ProcessWidget* pw = (ProcessWidget*) user_data;
  // If the key is keypad-enter, activate as if we had pressed Return.
  if(event->keyval == GDK_KP_Enter) {
    gtk_signal_emit_stop(GTK_OBJECT(widget), gtk_signal_lookup("key-press-event", gtk_entry_get_type()));
    processWidget_regexp_activate (GTK_ENTRY (widget), pw);
    return true;
  }
  return processWidget_generic_history_handling(widget, event, pw->regexpHistory, &(pw->regexpHistoryIndex));
}


static void
processWidget_stdin_activate (GtkEntry *entry, ProcessWidget *ppw)
{
  char str[1024];
  char* p_str = str;
#ifndef USE_ZVT_TERM
//  GdkColor fore;
#endif
  char* label;
  char* spaceWalker;
  char* entryText = NULL;

  if(ppw == NULL || entry == NULL) return;
  entryText = (char*)gtk_entry_get_text(entry);
  strncpy(p_str, entryText, 1021);

  // If we have a non-empty string, stick it in our history
  ppw->entryHistoryIndex = 0;
  if(strlen(entryText) > 0) {
    ppw->entryHistory.push_back(entryText);
  }
  ppw->entryHistory[0] = "";

  // If we just hit enter, pass it on.
  if(strlen(p_str) <= 0){
    p_str[0] = '\n';
    p_str[1] = '\0';
  }

  if(strlen(p_str) > 0) {
    /* Fire off data to process */    
    gtk_label_get(GTK_LABEL(ppw->ProcessNameLabel), &label);
    spaceWalker = strstr(label, " @ ");
    *spaceWalker = '\0';
    stdin_to_process(spaceWalker+3, label, p_str);
    *spaceWalker = ' ';

    // We used to echo locally here; this is now superseded by the messages from mraptord
    gtk_entry_set_text(entry, "");    
  }
}

void
processWidget_regexp_change_style(ProcessWidget* pw, GtkStyle *style)
{
  gtk_widget_set_style(pw->regexpEntry, style);
  //gtk_widget_modify_style(pw->regexpLabel, style);
  //gtk_widget_modify_style(pw->regexpEntry, style);
}

static gboolean
processWidget_regexp_changed_event (GtkEditable *widget,
                                    gpointer user_data)
{
  ProcessWidget* pw = (ProcessWidget*) user_data;
  // If the regexp is active, recolor to indicate that the regexp is
  // being edited.
  if(pw->regexpActive && !pw->regexpEditing) {
    // Change the text color of the widget
    processWidget_regexp_change_style(pw, pw->regexpEditingStyle);
    pw->regexpEditing = true;
  } else if(!pw->regexpActive && !pw->regexpEditing) {
    processWidget_regexp_change_style(pw, pw->regexpInactiveStyle);
    pw->regexpEditing = true;
  }
  // The checkbox should be disabled if there's no data in the entry.
  // I'm not sure if this works or not.
  if(strlen (gtk_entry_get_text (GTK_ENTRY (pw->regexpEntry) ) ) > 0) {
    gtk_widget_set_sensitive(pw->regexpToggle, TRUE);
    // Also ungrey the enable-filter context menu item
    gtk_widget_set_sensitive (GTK_WIDGET(g_list_first(GTK_MENU_SHELL(pw->regexpSubmenu)->children)->data), TRUE);

  } else {
    // Uncheck it first!
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(pw->regexpToggle), FALSE);
    gtk_widget_set_sensitive(pw->regexpToggle, FALSE);
    // Also grey out the enable-filter context menu item
    gtk_widget_set_sensitive (GTK_WIDGET(g_list_first(GTK_MENU_SHELL(pw->regexpSubmenu)->children)->data), FALSE);
  }

  return true;
}

static void
processWidget_update_submenu (ProcessWidget* ppw,
                              gboolean filterActivated) {
  GList* walker;
  char* itemName;
  for(walker = g_list_first(GTK_MENU_SHELL(ppw->regexpSubmenu)->children);
      walker;
      walker = g_list_next(walker)) {
    if(walker->data == NULL) continue;
    gtk_label_get(GTK_LABEL(GTK_BIN(walker->data)->child), &itemName);
    
    if(strcmp(itemName, "Enable filter") == 0) {
      if(filterActivated) {
        gtk_widget_hide(GTK_WIDGET(walker->data));
      } else {
        gtk_widget_show(GTK_WIDGET(walker->data));
      }
    } else if(strcmp(itemName, "Disable filter") == 0) {
      if(filterActivated) {
        gtk_widget_show(GTK_WIDGET(walker->data));
      } else {
        gtk_widget_hide(GTK_WIDGET(walker->data));
      }
    }
  }
}

static void
processWidget_deactivate_filter (ProcessWidget* ppw,
                                 gboolean toggleButton) {
  processWidget_regexp_change_style(ppw, ppw->regexpInactiveStyle);
  ppw->regexpActive = false;
  ppw->regexpEditing = false;
  if(ppw->curRegexp == NULL) {
    ppw->curRegexp = new string("");
  } else {
    *(ppw->curRegexp) = "";
  }
  if(ppw->regexpBuffer == NULL) {
    ppw->regexpBuffer = new string("");
  } else {
    *(ppw->regexpBuffer) = "";
  }

  if(toggleButton) {
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ppw->regexpToggle), FALSE);
  }
  /* Update submenu */
  processWidget_update_submenu(ppw, FALSE);
}

// Returns TRUE if the regexp was valid, and has been activated.
// Returns FALSE if the regexp was invalid.
static gboolean
processWidget_activate_filter (ProcessWidget* ppw,
                               gboolean toggleButton,
                               gboolean pushOnHistory) {
  char* entryText = NULL;
  unsigned int errCode;
  char errBuf[4096];

  entryText = (char*)gtk_entry_get_text(GTK_ENTRY(ppw->regexpEntry));
  if(ppw->regexpBuffer == NULL) {
    ppw->regexpBuffer = new string("");
  } else {
    *(ppw->regexpBuffer) = "";
  }

  // Free any previous pattern
  if(ppw->regexpPatternBuffer.allocated) {
    regfree(&ppw->regexpPatternBuffer);
  }
  
  // Initialize pattern buffer
  ppw->regexpPatternBuffer.fastmap = ppw->regexpFastmap;
  ppw->regexpPatternBuffer.translate = 0;
  ppw->regexpPatternBuffer.not_bol = 0;
  ppw->regexpPatternBuffer.not_eol = 0;
  
  // Compile pattern; use extended POSIX regexps.  Echo errors.
  if((errCode = regcomp(&ppw->regexpPatternBuffer, entryText, REG_EXTENDED | REG_NOSUB)) != 0) {
    regerror(errCode, &ppw->regexpPatternBuffer, errBuf, 4096);
    string toBuffer = "Regexp Error: ";
    toBuffer += errBuf;
    print_to_common_buffer(toBuffer.c_str(), ERROR_COL);
    ppw->regexpActive = false;
    ppw->regexpEditing = false;
    if(ppw->curRegexp == NULL) {
      ppw->curRegexp = new string("");
    } else {
      *(ppw->curRegexp) = "";
    }
    return FALSE;
  } else {
    // We have a non-empty, valid, regexp
    processWidget_regexp_change_style(ppw, ppw->regexpActiveStyle);
    ppw->regexpActive = true;
    ppw->regexpEditing = false;
    if(toggleButton) {
      gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ppw->regexpToggle), TRUE);
    }
    if(ppw->curRegexp == NULL) {
      ppw->curRegexp = new string(entryText);
    } else {
      *(ppw->curRegexp) = entryText;
    }
    if(pushOnHistory) {
      // If we have a non-empty string that differs from the end of
      // the history stack, stick it in our history
      ppw->regexpHistoryIndex = 0;
      if(string(entryText) != ppw->regexpHistory.back())
        ppw->regexpHistory.push_back(entryText);
    }
  }
  processWidget_update_submenu(ppw, TRUE);
  return TRUE;
}

void
processWidget_regexp_refresh_buffer(ProcessWidget* ppw)
{
  // Ah, the joys of the STL.
  hash_map<ProcessData, pair<bool, timeval>, size_t (*)(ProcessData), bool (*)(ProcessData, ProcessData)>::iterator sit;        
  ProcessData pd;
  char* spaceWalker;
  char* label;

  // Clear buffer and request back data: we'll refilter it as it comes in.
  // Don't reset or request playback if we're not currently subscribed.
  init_process_data(&pd);
  fill_process_data_from_process_widget(&pd, ppw);
  sit = subscriptions_g->find(pd);
  free_process_data_members(&pd);

  if(ppw != NULL
     && sit != subscriptions_g->end()
     && (*sit).second.first) {
    //Comment until we gind zvt solution
    //zvt_term_reset (ZVT_TERM (ppw->LogText), TRUE);
    // Get machine and process names
    gtk_label_get(GTK_LABEL(ppw->ProcessNameLabel), &label);
    
    spaceWalker = strstr(label, " @ ");
    *spaceWalker = '\0';
    // This is a noop if we're not subscribed
    request_playback(spaceWalker+3, label, prefs_get_back_history_length());
    *spaceWalker = ' '; // This is a pointer to the label's data; reset.
  } else if(ppw == NULL) {
    cerr << "ERROR: ProcessWidget null in regexp callback!" << endl;
  }

}

// The checkbox has been pressed (or programatically changed!)
static void
processWidget_regexp_toggled (GtkToggleButton *togglebutton,
                              ProcessWidget* ppw)
{
  if(gtk_toggle_button_get_active(togglebutton)) {
    if(!ppw->regexpActive) {
      // Only refresh the buffer if the regexp is valid.
      if(processWidget_activate_filter(ppw, FALSE, FALSE)) {
        processWidget_regexp_refresh_buffer(ppw);
      }
    }
  } else {
    if(ppw->regexpActive) {
      processWidget_deactivate_filter(ppw, FALSE);
      processWidget_regexp_refresh_buffer(ppw);
    }
  }
}

static void
processWidget_regexp_activate (GtkEntry *entry, ProcessWidget *ppw)
{
  char* entryText = NULL;

  entryText = (char*)gtk_entry_get_text(GTK_ENTRY(entry));

  // If we just hit enter on an empty box, deactivate filtering.
  if(strlen(entryText) <= 0) {
    processWidget_deactivate_filter(ppw, TRUE);
    processWidget_regexp_refresh_buffer(ppw);
  } else {
    // Only refresh the buffer if the regexp is valid.
    if(processWidget_activate_filter(ppw, TRUE, TRUE)) {
      processWidget_regexp_refresh_buffer(ppw);
    }
  }
}

static void
processWidget_regexp_submenu_activate (GtkMenuItem     *menuitem,
                                       gpointer         user_data)
{
  ProcessWidget* ppw = (ProcessWidget*) user_data;
  if(processWidget_activate_filter(ppw, TRUE, FALSE)) {
    processWidget_regexp_refresh_buffer(ppw);
  }
}

// This is a horrible hack: for some reason this submenu requires a
// double-left-click to activate an item, so we're hijacking the
// button-press-event.
static void
processWidget_regexp_submenu_activate_button_press_event (GtkWidget     *widget,
                                                          GdkEventButton *event,
                                                          gpointer user_data)
{
  processWidget_regexp_submenu_activate (GTK_MENU_ITEM(widget), user_data);
  // Sigh.  Submenu isn't nuking itself in this case.
  gtk_menu_popdown(GTK_MENU(((ProcessWidget*) user_data)->regexpSubmenu));
}

static void
processWidget_regexp_submenu_deactivate (GtkMenuItem     *menuitem,
                                         gpointer         user_data)
{
  ProcessWidget* ppw = (ProcessWidget*) user_data;
  processWidget_deactivate_filter(ppw, TRUE);
  processWidget_regexp_refresh_buffer(ppw);
}

// This is a horrible hack: for some reason this submenu requires a
// double-left-click to activate an item, so we're hijacking the
// button-press-event.
static void
processWidget_regexp_submenu_deactivate_button_press_event (GtkWidget     *widget,
                                                            GdkEventButton *event,
                                                            gpointer user_data)
{
  processWidget_regexp_submenu_deactivate (GTK_MENU_ITEM(widget), user_data);
  // Sigh.  Submenu isn't nuking itself in this case.
  gtk_menu_popdown(GTK_MENU(((ProcessWidget*) user_data)->regexpSubmenu));
}

static void
processWidget_regexp_submenu_context (GtkMenuItem     *menuitem,
                                      gpointer         user_data)
{
  //cerr << "context item pressed." << endl;
  ProcessWidget* ppw = (ProcessWidget*) user_data;
  GtkWidget* cd = create_contextDialog(ppw);
  gtk_window_set_position(GTK_WINDOW(cd), GTK_WIN_POS_MOUSE);
  gtk_widget_show(cd);
  add_taskbar_icon (cd, miniClawImage_xpm);
}

// This is a horrible hack: for some reason this submenu requires a
// double-left-click to activate an item, so we're hijacking the
// button-press-event.
static void
processWidget_regexp_submenu_context_button_press_event (GtkWidget     *widget,
                                                         GdkEventButton *event,
                                                         gpointer user_data)
{
  processWidget_regexp_submenu_context (GTK_MENU_ITEM(widget), user_data);
  // Sigh.  Submenu isn't nuking itself in this case.
  gtk_menu_popdown(GTK_MENU(((ProcessWidget*) user_data)->regexpSubmenu));
}

gboolean
processWidget_expand_filter(ProcessWidget* pw)
{
  pw->entryExpanded = false;
  // Change usize and packing type for the two widgets  
  gtk_widget_set_usize (pw->regexpEntry, -2, -2);
  gtk_widget_set_usize (pw->stdinEntry, SHRUNK_WIDTH, -2);
  gtk_box_set_child_packing (GTK_BOX (pw->entryBox), pw->regexpEntry, TRUE, TRUE, 0, GTK_PACK_START);
  gtk_box_set_child_packing (GTK_BOX (pw->entryBox), pw->stdinEntry, FALSE, FALSE, 0, GTK_PACK_START);
  return true;
}

gboolean
processWidget_expand_stdin(ProcessWidget* pw)
{
  pw->entryExpanded = true;
  // Change usize and packing type for the two widgets  
  gtk_widget_set_usize (pw->stdinEntry, -2, -2);
  gtk_widget_set_usize (pw->regexpEntry, SHRUNK_WIDTH, -2);
  gtk_box_set_child_packing (GTK_BOX (pw->entryBox), pw->stdinEntry, TRUE, TRUE, 0, GTK_PACK_START);
  gtk_box_set_child_packing (GTK_BOX (pw->entryBox), pw->regexpEntry, FALSE, FALSE, 0, GTK_PACK_START);
  return true;
}

// Hide the widgets, expand stdin, and disable any set filter, since
// there's no way to do so once the filter is hidden.
void
processWidget_hide_filter(ProcessWidget* pw)
{
  processWidget_deactivate_filter(pw, TRUE);
  processWidget_regexp_refresh_buffer(pw);
  gtk_widget_hide (pw->regexpLabel);
  gtk_widget_hide (pw->regexpEntry);
  gtk_widget_hide (pw->regexpToggle);
  processWidget_expand_stdin(pw);
}

void
processWidget_show_filter(ProcessWidget* pw)
{
  gtk_widget_show (pw->regexpLabel);
  gtk_widget_show (pw->regexpEntry);
  gtk_widget_show (pw->regexpToggle);
}

static gboolean
processWidget_regexp_focus_in_event (GtkWidget *widget,
                                     GdkEventFocus *event,
                                     gpointer user_data)
{
  return processWidget_expand_filter((ProcessWidget*) user_data);
}

static gboolean
processWidget_regexp_button_press_event (GtkWidget *widget,
                                         GdkEvent *event,
                                         gpointer user_data)
{
  ProcessWidget* ppw = (ProcessWidget*) user_data;
  GdkEventButton *event_button = (GdkEventButton *) event;
  // Launch the menu on a right-click
  if (event->type == GDK_BUTTON_PRESS
      && event_button->button == 3) {
    add_error_trap();
    gtk_menu_popup (GTK_MENU(ppw->regexpSubmenu), NULL, NULL, NULL, NULL, 
                    event_button->button, event_button->time);
    clear_error_traps();
    return TRUE;
  }
  return FALSE;
}

static gboolean
processWidget_stdin_focus_in_event (GtkWidget *widget,
                                    GdkEventFocus *event,
                                    gpointer user_data)
{
  return processWidget_expand_stdin((ProcessWidget*) user_data);
}

static gboolean
processWidget_main_enter_notify_event(GtkWidget *widget,
                                      GdkEventCrossing *event,
                                      gpointer user_data)
{
  if(prefs_get_focus_follows_mouse()) {
    ProcessWidget* pw = (ProcessWidget*) user_data;
    // If stdin is expanded and the process is running, set focus there
    if(pw->entryExpanded && GTK_WIDGET_IS_SENSITIVE(GTK_WIDGET(pw->stdinEntry))) {
      gtk_widget_grab_focus(pw->stdinEntry);    
    }
    // Otherwise, if the filter is visible, set focus to it.
    else if(GTK_WIDGET_VISIBLE (GTK_WIDGET(pw->regexpEntry) ) ) {
      gtk_widget_grab_focus(pw->regexpEntry);
    }
    return TRUE;
  }
  return FALSE;
}


guint
processWidget_get_type (void)
{
  static guint pw_type = 0;

  if (!pw_type)
    {
      GtkTypeInfo pw_info =
      {
        (char *)"ProcessWidget",
        sizeof (ProcessWidget),
        sizeof (ProcessWidgetClass),
        (GtkClassInitFunc) processWidget_class_init,
        (GtkObjectInitFunc) processWidget_init,
        (gpointer) NULL,
        (gpointer) NULL
      };
      pw_type = gtk_type_unique (gtk_frame_get_type (), &pw_info);
    }

  return pw_type;
}


static void
processWidget_class_init (ProcessWidgetClass *pwclass)
{
#if 0
  GtkObjectClass *object_class;

  object_class = (GtkObjectClass*) pwclass;
  
  processWidget_signals[PROCESSWIDGET_SIGNAL] = gtk_signal_new ("processWidget",
                                         GTK_RUN_FIRST,
                                         object_class->type,
                                         GTK_SIGNAL_OFFSET (ProcessWidgetClass, processWidget),
                                         gtk_signal_default_marshaller, GTK_TYPE_NONE, 0);
  gtk_object_class_add_signals (object_class, processWidget_signals, LAST_SIGNAL);
#endif

  pwclass->processWidget = NULL;
}


static void
processWidget_init (ProcessWidget *pw)
{
  int i;
  GtkButton* button;
  GdkPixmap* buttonPixmap;
  GtkWidget* buttonGtkPixmap;
  GtkStyle *style;
  GdkBitmap *mask;
  const char **xpmArray;
  char blabel[255];
  map<string, vector<string> >::iterator historyIt;

  pw->scrollValue = -1;
  pw->entryHistoryIndex = 0;
  pw->entryHistory.push_back("");
  pw->regexpHistoryIndex = 0;
  pw->regexpHistory.push_back("");
  pw->regexpPreContext = 0;
  pw->regexpPostContext = 0;
  pw->regexpPostContextRemaining = 0;
  pw->regexpPreContextBuffer.clear();  
  pw->entryExpanded = true;

  // This never fired... just calling cleanup by hand from claw_quit()
  //gtk_signal_connect (GTK_OBJECT(pw), "destroy-event",
  //                    GTK_SIGNAL_FUNC (processWidget_cleanup),
  //                    pw);

  pw->mainEventBox = gtk_event_box_new();
  gtk_widget_ref (pw->mainEventBox);
  gtk_object_set_data_full (GTK_OBJECT (pw), "mainEventBox", pw->mainEventBox,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (pw->mainEventBox);
  gtk_container_add (GTK_CONTAINER (pw), pw->mainEventBox);

  // Focus-follows-mouse support
  gtk_signal_connect (GTK_OBJECT (pw->mainEventBox), "enter-notify-event",
                      GTK_SIGNAL_FUNC (processWidget_main_enter_notify_event),
                      pw);

  pw->MainLayoutTable = gtk_table_new (3, 1, FALSE);
  gtk_widget_ref (pw->MainLayoutTable);
  gtk_object_set_data_full (GTK_OBJECT (pw), "MainLayoutTable", pw->MainLayoutTable,
                            (GtkDestroyNotify) gtk_widget_unref);
  GTK_WIDGET_UNSET_FLAGS (pw->MainLayoutTable, GTK_CAN_FOCUS);
  gtk_widget_show (pw->MainLayoutTable);
  gtk_container_add (GTK_CONTAINER (pw->mainEventBox), pw->MainLayoutTable);

  pw->entryBox = gtk_hbox_new (FALSE, 0);
  gtk_box_set_spacing (GTK_BOX (pw->entryBox), 2);
  gtk_container_set_border_width (GTK_CONTAINER (pw->entryBox), 2);
  GTK_WIDGET_UNSET_FLAGS (pw->entryBox, GTK_CAN_FOCUS);
  gtk_table_attach (GTK_TABLE (pw->MainLayoutTable), pw->entryBox, 0, 1, 2, 3,
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_widget_show (pw->entryBox);

  pw->stdinEntry = gtk_entry_new ();
  gtk_widget_ref (pw->stdinEntry);
  gtk_object_set_data_full (GTK_OBJECT (pw), "stdinEntry", pw->stdinEntry,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_box_pack_start (GTK_BOX (pw->entryBox), pw->stdinEntry, TRUE, TRUE, 0);
  gtk_widget_show (pw->stdinEntry);
  // Grey out stdinEntry when it's not editable.  This is a small memory leak.
  GtkStyle* greyStyle = gtk_style_new();
  GdkColor grey;
  // 50000 > x > 45000
  grey.red = 47000; grey.green = 47000; grey.blue = 47000;
  greyStyle->base[GTK_STATE_INSENSITIVE] = grey;
  gtk_widget_set_style(pw->stdinEntry, greyStyle);

  gtk_signal_connect (GTK_OBJECT (pw->stdinEntry), "focus-in-event",
                      GTK_SIGNAL_FUNC (processWidget_stdin_focus_in_event),
                      pw);
  gtk_signal_connect (GTK_OBJECT (pw->stdinEntry), "activate",
                      GTK_SIGNAL_FUNC (processWidget_stdin_activate),
                      pw);
  gtk_signal_connect (GTK_OBJECT (pw->stdinEntry), "key-press-event",
                      GTK_SIGNAL_FUNC (processWidget_key_press_event),
                      pw);

  // Always create the regexp widgets; hide and show based on preferences.
  pw->regexpLabel = gtk_label_new (_("Filter:"));
  gtk_widget_ref (pw->regexpLabel);
  gtk_object_set_data_full (GTK_OBJECT (pw), "regexpLabel", pw->regexpLabel,
                            (GtkDestroyNotify) gtk_widget_unref);
  GTK_WIDGET_UNSET_FLAGS (pw->regexpLabel, GTK_CAN_FOCUS);
  gtk_box_pack_start (GTK_BOX (pw->entryBox), pw->regexpLabel, FALSE, FALSE, 0);
  
  pw->regexpEntry = gtk_entry_new ();
  gtk_widget_ref (pw->regexpEntry);
  gtk_object_set_data_full (GTK_OBJECT (pw), "regexpEntry", pw->regexpEntry,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_set_usize (pw->regexpEntry, SHRUNK_WIDTH, -2);
  gtk_box_pack_start (GTK_BOX (pw->entryBox), pw->regexpEntry, FALSE, FALSE, 0);
  
  pw->regexpToggle = gtk_check_button_new();
  gtk_widget_ref (pw->regexpToggle);
  gtk_object_set_data_full (GTK_OBJECT (pw), "regexpToggle", pw->regexpToggle,
                            (GtkDestroyNotify) gtk_widget_unref);
  // Initially insensitive; entering any text into the entry will sensitize it.
  gtk_widget_set_sensitive(pw->regexpToggle, FALSE);
  gtk_box_pack_start (GTK_BOX (pw->entryBox), pw->regexpToggle, FALSE, FALSE, 0);

  gtk_signal_connect (GTK_OBJECT (pw->regexpEntry), "activate",
                      GTK_SIGNAL_FUNC (processWidget_regexp_activate),
                      pw);
  gtk_signal_connect (GTK_OBJECT (pw->regexpEntry), "key-press-event",
                      GTK_SIGNAL_FUNC (processWidget_regexp_key_press_event),
                      pw);
  gtk_signal_connect (GTK_OBJECT (pw->regexpEntry), "changed",
                      GTK_SIGNAL_FUNC (processWidget_regexp_changed_event),
                      pw);
  gtk_signal_connect (GTK_OBJECT (pw->regexpEntry), "focus-in-event",
                      GTK_SIGNAL_FUNC (processWidget_regexp_focus_in_event),
                      pw);
  gtk_signal_connect (GTK_OBJECT (pw->regexpEntry), "button_press_event",
                      GTK_SIGNAL_FUNC (processWidget_regexp_button_press_event),
                      pw);
  gtk_signal_connect (GTK_OBJECT (pw->regexpToggle), "toggled",
                      GTK_SIGNAL_FUNC (processWidget_regexp_toggled),
                      pw);


  // Set up tooltips
  string tip = "Enter a POSIX regexp to filter the process output.  A white background indicates an inactive regexp; green an active one; and red that a regexp is active, but that it is not the current text (e.g. you're editing it).  Press [enter] or check the box to activate filtering; uncheck the box to disable. Ctrl+U clears the text.";
  // MEMLEAK: This is a leak.  How do we properly delete a tooltips object?
  pw->RegexpTooltips = gtk_tooltips_new();
  gtk_tooltips_set_tip(GTK_TOOLTIPS (pw->RegexpTooltips), pw->regexpEntry,
                       tip.c_str(),
                       "");
                         
  gtk_tooltips_set_tip(GTK_TOOLTIPS (pw->RegexpTooltips), pw->regexpLabel,
                       tip.c_str(),
                       "");

  string checkTip = "Enable or disable the filter regexp entered to the left.";
  gtk_tooltips_set_tip(GTK_TOOLTIPS (pw->RegexpTooltips), pw->regexpToggle,
                       checkTip.c_str(),
                       "");    

  // Regexp compilation occurs in the callbacks

  // These styles are used to set both the label's fg and the text
  // of the regexp input widget.
  //
  // Beware GTK themes: they can somehow override these settings.
  // FIXME: Figure out how to trump themes.
  GdkColor green, red, /*black, */ white;
  green.red = 29184; green.green = 65535; green.blue = 34304;
  //red.red = 65535; red.green = 29952; red.blue = 29952;
  red.red = 65535; red.green = 0; red.blue = 0;
  //black.red = 0; black.green = 0; black.blue = 0;
  white.red = 65535; white.green = 65535; white.blue = 65535;

  /////////////////////////////////
  // ACTIVE settings
  pw->regexpActiveStyle = gtk_style_new();
  pw->regexpActiveStyle->base[GTK_STATE_NORMAL] = green;
  pw->regexpActiveStyle->base[GTK_STATE_ACTIVE] = green;

  /////////////////////////////////
  // EDITING settings
  pw->regexpEditingStyle = gtk_style_new();
  pw->regexpEditingStyle->base[GTK_STATE_NORMAL] = red;
  pw->regexpEditingStyle->base[GTK_STATE_ACTIVE] = red;

  /////////////////////////////////
  // INACTIVE settings
  pw->regexpInactiveStyle = gtk_style_new();
  pw->regexpInactiveStyle->base[GTK_STATE_NORMAL] = white;
  pw->regexpInactiveStyle->base[GTK_STATE_ACTIVE] = white;

  pw->regexpActive = false;
  pw->regexpEditing = false;
  if(pw->curRegexp == NULL) {
    pw->curRegexp = new string("");
  } else {
    *(pw->curRegexp) = "";
  }

  pw->regexpBuffer = new string("");

  // Build the right-click menu that will allow us to enable/disable
  // the filter, as well as set filtering context.
  pw->regexpSubmenu = gtk_menu_new();
  GtkWidget* enable_item;
  GtkWidget* disable_item;
  GtkWidget* set_context_item;

  /* Enable the filter */
  enable_item = gtk_menu_item_new_with_label(_("Enable filter"));
  gtk_menu_append(GTK_MENU(pw->regexpSubmenu), enable_item);
  gtk_signal_connect (GTK_OBJECT (enable_item), "activate",
                      GTK_SIGNAL_FUNC (processWidget_regexp_submenu_activate),
                      pw);
  // Sigh.  It's taking a double-left-click to activate a menu item.  This is a hack.
  gtk_signal_connect (GTK_OBJECT (enable_item), "button-press-event",
                      GTK_SIGNAL_FUNC (processWidget_regexp_submenu_activate_button_press_event),
                      pw);
  // Initially insensitive; it'll be sensitized as soon as text is typed into the entry.
  gtk_widget_set_sensitive (enable_item, FALSE);
  gtk_widget_show (enable_item);

  /* Disable the filter; the filter is initially disabled, so hide this to start. */
  disable_item = gtk_menu_item_new_with_label(_("Disable filter"));
  gtk_menu_append(GTK_MENU(pw->regexpSubmenu), disable_item);
  gtk_signal_connect (GTK_OBJECT (disable_item), "activate",
                      GTK_SIGNAL_FUNC (processWidget_regexp_submenu_deactivate),
                      pw);
  // Sigh.  It's taking a double-left-click to activate a menu item.  This is a hack.
  gtk_signal_connect (GTK_OBJECT (disable_item), "button-press-event",
                      GTK_SIGNAL_FUNC (processWidget_regexp_submenu_deactivate_button_press_event),
                      pw);
  gtk_widget_hide (disable_item);

  /* Set filtering context (launches a dialog) */
  set_context_item = gtk_menu_item_new_with_label(_("Set context..."));
  gtk_menu_append(GTK_MENU(pw->regexpSubmenu), set_context_item);
  gtk_signal_connect (GTK_OBJECT (set_context_item), "activate",
                      GTK_SIGNAL_FUNC (processWidget_regexp_submenu_context),
                      pw);
  // Sigh.  It's taking a double-left-click to activate a menu item.  This is a hack.
  gtk_signal_connect (GTK_OBJECT (set_context_item), "button-press-event",
                      GTK_SIGNAL_FUNC (processWidget_regexp_submenu_context_button_press_event),
                      pw);
  gtk_widget_show (set_context_item);


#ifndef USE_ZVT_TERM
  //cout << "No ZVT_TERM, building LogScrolledWindow" << endl;
  pw->LogScrolledWindow = gtk_scrolled_window_new (NULL, NULL);
  gtk_widget_ref (pw->LogScrolledWindow);
  gtk_object_set_data_full (GTK_OBJECT (pw), "LogScrolledWindow", pw->LogScrolledWindow,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (pw->LogScrolledWindow);
  gtk_table_attach (GTK_TABLE (pw->MainLayoutTable), pw->LogScrolledWindow, 0, 1, 1, 2,
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL), 0, 0);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (pw->LogScrolledWindow), GTK_POLICY_NEVER, GTK_POLICY_ALWAYS);  
  pw->windowAdjustment = GTK_OBJECT(gtk_scrolled_window_get_vadjustment(GTK_SCROLLED_WINDOW(pw->LogScrolledWindow)));
  
  gtk_signal_connect (GTK_OBJECT(pw->windowAdjustment), "value-changed",
                      GTK_SIGNAL_FUNC (on_processwidget_generic_scrolled),
                      pw);
  gtk_signal_connect (GTK_OBJECT(pw->windowAdjustment), "changed",
                      GTK_SIGNAL_FUNC (on_processwidget_text_expanded),
                      pw);
#endif

  //pw->textAdjustment = GTK_OBJECT(gtk_adjustment_new(0,0,0,0,0,0));

#ifdef USE_ZVT_TERM
  // Set up the hbox to hold the terminal and scrollbar
  pw->LogHbox = gtk_hbox_new (FALSE, 0);
  gtk_box_set_spacing (GTK_BOX (pw->LogHbox), 2);
  gtk_container_set_border_width (GTK_CONTAINER (pw->LogHbox), 2);
  //gtk_container_add (GTK_CONTAINER (window), pw->LogHbox);
  gtk_table_attach (GTK_TABLE (pw->MainLayoutTable), pw->LogHbox, 0, 1, 1, 2,
                    (GtkAttachOptions) (GTK_SHRINK | GTK_EXPAND | GTK_FILL),
                    (GtkAttachOptions) (GTK_SHRINK | GTK_EXPAND | GTK_FILL), 0, 0);
  GTK_WIDGET_UNSET_FLAGS (pw->LogHbox, GTK_CAN_FOCUS);
  gtk_widget_show (pw->LogHbox);

  // Set up the terminal
  pw->LogText = ZVT_TERM (zvt_term_new_with_size(80, 1));
  zvt_term_set_font_name(ZVT_TERM (pw->LogText), "-misc-fixed-medium-r-normal--10-*-*-*-*-*-*-*");
  zvt_term_set_blink (ZVT_TERM (pw->LogText), FALSE);
  zvt_term_set_bell (ZVT_TERM (pw->LogText), TRUE);
  zvt_term_set_scrollback(ZVT_TERM (pw->LogText), prefs_get_back_history_length());
  zvt_term_set_scroll_on_keystroke (ZVT_TERM (pw->LogText), TRUE);
  zvt_term_set_scroll_on_output (ZVT_TERM (pw->LogText), FALSE);
  zvt_term_set_background (ZVT_TERM (pw->LogText), NULL, 0, 0);
  zvt_term_set_wordclass (ZVT_TERM (pw->LogText), (unsigned char*) "-A-Za-z0-9/_:.,?+%=");
  GTK_WIDGET_UNSET_FLAGS (pw->LogText, GTK_CAN_FOCUS);
  gtk_widget_show(GTK_WIDGET(pw->LogText));

  pw->LogTextOutBufBytes = 0;

  //for(int i = 0; i < 24; i++) processWidget_term_feed(pw, "\n", 1);

  pw->LogScrollbar = gtk_vscrollbar_new (GTK_ADJUSTMENT (ZVT_TERM (pw->LogText)->adjustment));
  GTK_WIDGET_UNSET_FLAGS (pw->LogScrollbar, GTK_CAN_FOCUS);

  gtk_box_pack_start (GTK_BOX (pw->LogHbox), GTK_WIDGET(pw->LogText), 1, 1, 0);
  gtk_box_pack_start (GTK_BOX (pw->LogHbox), pw->LogScrollbar, FALSE, TRUE, 0);

  gtk_widget_show (pw->LogScrollbar);

  //gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(pw->LogScrolledWindow), GTK_WIDGET(pw->LogText));
  gtk_signal_connect(GTK_OBJECT(pw->LogText), "button_press_event",
                     GTK_SIGNAL_FUNC (on_pw_LogText_button_press_event),
                     NULL);
  #if 1
    gtk_signal_connect_after(GTK_OBJECT(pw->LogText), "size-allocate",
                             GTK_SIGNAL_FUNC (on_pw_LogText_size_allocate_event),
                             NULL);
  #endif
  //gtk_scrolled_window_set_vadjustment(GTK_SCROLLED_WINDOW(pw->LogScrolledWindow),
  //                                    GTK_ADJUSTMENT (ZVT_TERM (pw->LogText)->adjustment));

#else
  pw->LogText = new ClawText(gtk_text_view_new());
  pw->LogText->set_font_from_string("Fixed light normal 8");
  ClawText::ref(pw->LogText);
  gtk_object_set_data_full (GTK_OBJECT (pw), "LogText", pw->LogText,
                            (GtkDestroyNotify) &ClawText::unref);
  gtk_widget_show(pw->LogText->get_text_widget());
  gtk_container_add (GTK_CONTAINER (pw->LogScrolledWindow), pw->LogText->get_text_widget());
  GTK_WIDGET_UNSET_FLAGS (pw->LogText->get_text_widget(), GTK_CAN_FOCUS);
  // Reset adjustment; we need to deal with the text adjustments on our own
  //TODO: Commenting out adjustments for now
  //gtk_text_set_adjustments(GTK_TEXT(pw->LogText->get_text_widget()),
  //gtk_text_set_adjustments(pw->LogText->get_text_widget(),
  //			   NULL, GTK_ADJUSTMENT(pw->windowAdjustment));
  gtk_signal_connect(GTK_OBJECT(pw->LogText->get_text_widget()), "button_press_event",
                     GTK_SIGNAL_FUNC (on_pw_LogText_button_press_event),
                     NULL);
#endif

  pw->HeaderTable = gtk_table_new (1, 4, FALSE);
  gtk_widget_ref (pw->HeaderTable);
  gtk_object_set_data_full (GTK_OBJECT (pw), "HeaderTable", pw->HeaderTable,
                            (GtkDestroyNotify) gtk_widget_unref);
  GTK_WIDGET_UNSET_FLAGS (pw->HeaderTable, GTK_CAN_FOCUS);
  gtk_widget_show (pw->HeaderTable);
  gtk_table_attach (GTK_TABLE (pw->MainLayoutTable), pw->HeaderTable, 0, 1, 0, 1,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);

  GtkRcStyle *rc_bold_style;
  rc_bold_style = gtk_rc_style_new();
  //rc_bold_style->font_name = "-adobe-times-bold-r-*-*-*-100-*-*-*-*-*-*";

  pw->ProcessNameLabel = gtk_label_new (_("ProcessName"));
  gtk_widget_ref (pw->ProcessNameLabel);
  gtk_object_set_data_full (GTK_OBJECT (pw), "ProcessNameLabel", pw->ProcessNameLabel,
                            (GtkDestroyNotify) gtk_widget_unref);
  GTK_WIDGET_UNSET_FLAGS (pw->ProcessNameLabel, GTK_CAN_FOCUS);
  gtk_widget_modify_style(pw->ProcessNameLabel, rc_bold_style);
  gtk_widget_show (pw->ProcessNameLabel);
  gtk_table_attach (GTK_TABLE (pw->HeaderTable), pw->ProcessNameLabel, 0, 1, 0, 1,
                    (GtkAttachOptions) (GTK_EXPAND),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_misc_set_alignment (GTK_MISC (pw->ProcessNameLabel), 0.5, 0.5);


  pw->ProcessStateLabel = gtk_label_new (_("ProcessStateLabel"));
  gtk_widget_ref (pw->ProcessStateLabel);
  gtk_object_set_data_full (GTK_OBJECT (pw), "ProcessStateLabel", pw->ProcessStateLabel,
                            (GtkDestroyNotify) gtk_widget_unref);
  GTK_WIDGET_UNSET_FLAGS (pw->ProcessStateLabel, GTK_CAN_FOCUS);
  gtk_widget_modify_style(pw->ProcessStateLabel, rc_bold_style);
  gtk_widget_show (pw->ProcessStateLabel);
  gtk_table_attach (GTK_TABLE (pw->HeaderTable), pw->ProcessStateLabel, 1, 2, 0, 1,
                    (GtkAttachOptions) (GTK_EXPAND),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_misc_set_alignment (GTK_MISC (pw->ProcessStateLabel), 0.5, 0.5);

  pw->ProcessLastStdoutLabel = gtk_label_new (_("Idle: ???"));
  gtk_widget_ref (pw->ProcessLastStdoutLabel);
  gtk_object_set_data_full (GTK_OBJECT (pw), "ProcessLastStdoutLabel", pw->ProcessLastStdoutLabel,
                            (GtkDestroyNotify) gtk_widget_unref);
  GTK_WIDGET_UNSET_FLAGS (pw->ProcessLastStdoutLabel, GTK_CAN_FOCUS);
  gtk_widget_modify_style(pw->ProcessLastStdoutLabel, rc_bold_style);
  gtk_widget_show (pw->ProcessLastStdoutLabel);
  gtk_table_attach (GTK_TABLE (pw->HeaderTable), pw->ProcessLastStdoutLabel, 2, 3, 0, 1,
                    (GtkAttachOptions) (GTK_EXPAND),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_misc_set_alignment (GTK_MISC (pw->ProcessLastStdoutLabel), 0.5, 0.5);

  /* Set up button box for shortcut buttons */
  pw->ProcessURButtons = gtk_hbutton_box_new();
  gtk_button_box_set_child_size(GTK_BUTTON_BOX(pw->ProcessURButtons), 10, 10);
  gtk_button_box_set_child_ipadding(GTK_BUTTON_BOX(pw->ProcessURButtons), 2, 2);
  gtk_button_box_set_spacing(GTK_BUTTON_BOX(pw->ProcessURButtons), 2);
  gtk_widget_ref (pw->ProcessURButtons);
  gtk_object_set_data_full (GTK_OBJECT (pw), "ProcessURButtons", pw->ProcessURButtons,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (pw->ProcessURButtons);
  gtk_table_attach (GTK_TABLE (pw->HeaderTable), pw->ProcessURButtons, 3, 4, 0, 1,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  
  style = gtk_widget_get_style( HeadDino_g );

  /* Set up tooltips */
  // MEMLEAK: This is a leak.  How do we properly delete a tooltips object?
  pw->ProcessURButtonsTooltips = gtk_tooltips_new();

  /* Pack it with buttons */
  for(i = 0; i < 7; i++) {

    /* Switch on button-specific stuff */
    switch(i) {
    case 0: /* RUN  */ xpmArray = runImage_xpm;      sprintf(blabel, "R"); break;
    case 1: /* KILL */ xpmArray = killImage_xpm;     sprintf(blabel, "K"); break;
    case 2: /* SIG  */ xpmArray = flagImage_xpm;     sprintf(blabel, "I"); break;
    case 3: /* SUB  */ xpmArray = subImage_xpm;      sprintf(blabel, "S"); break;
    case 4: /* UNSUB*/ xpmArray = unsubImage_xpm;    sprintf(blabel, "U"); break;
    case 5: /* MAX  */ xpmArray = maximizeImage_xpm; sprintf(blabel, "M"); break;
    case 6: /* HIDE */ xpmArray = hideImage_xpm;     sprintf(blabel, "H"); break;
    default:           xpmArray = blankImage_xpm;    sprintf(blabel, "B"); break;
    }

    buttonPixmap = gdk_pixmap_create_from_xpm_d( HeadDino_g->window, &mask,
                                                 &style->bg[GTK_STATE_NORMAL],
                                                 (char **)xpmArray );
    buttonGtkPixmap = gtk_pixmap_new(buttonPixmap, mask);
    gtk_widget_show(buttonGtkPixmap);
    /* Common setup code */
    button = GTK_BUTTON(gtk_button_new());
    gtk_container_add(GTK_CONTAINER(button), buttonGtkPixmap);
    gtk_widget_ref (GTK_WIDGET(button));
    gtk_object_set_data_full (GTK_OBJECT (pw->ProcessURButtons), blabel, 
                              GTK_WIDGET(button),
                              (GtkDestroyNotify) gtk_widget_unref);
    gtk_object_set_user_data(GTK_OBJECT(button), strdup(blabel));
    gtk_widget_show (GTK_WIDGET(button));
    gtk_box_pack_start(GTK_BOX(pw->ProcessURButtons), 
                       GTK_WIDGET(button), FALSE, FALSE, 0);    
    GTK_WIDGET_UNSET_FLAGS (button, GTK_CAN_FOCUS);
  
    /* Final customized code */
    switch(i) {
    case 0: /* RUN  */   
      gtk_signal_connect (GTK_OBJECT (button), "clicked",
                          GTK_SIGNAL_FUNC (on_run_button_clicked),
                          pw);
      gtk_tooltips_set_tip(GTK_TOOLTIPS (pw->ProcessURButtonsTooltips), GTK_WIDGET (button),
                           "Run this process", 
                           "Clicking here will send a request to the appropriate daemon to launch this process.");
      break;
    case 1: /* KILL */
      gtk_signal_connect (GTK_OBJECT (button), "clicked",
                          GTK_SIGNAL_FUNC (on_kill_button_clicked),
                          pw);
      gtk_tooltips_set_tip(GTK_TOOLTIPS (pw->ProcessURButtonsTooltips), GTK_WIDGET (button),
                           "Kill this process", 
                           "Press here to have the appropriate daemon terminate this process.");
      break;
    case 2: /* SIGNAL */
    {
      gtk_signal_connect (GTK_OBJECT (button), "clicked",
                          GTK_SIGNAL_FUNC (on_signal_button_clicked),
                          pw);
      // Build the submenu once, and stash it
      GtkWidget* processTree = lookup_widget(HeadDino_g, "ProcessTree");
      gtk_object_set_user_data (GTK_OBJECT (buttonGtkPixmap), (void*) build_signal_submenu(processTree, 1, TRUE));
      gtk_tooltips_set_tip(GTK_TOOLTIPS (pw->ProcessURButtonsTooltips), GTK_WIDGET (button),
                           "Signal this process", 
                           "Press here to select a signal to send to this process.");
      break;
    }
    case 3: /* SUB  */
      gtk_signal_connect (GTK_OBJECT (button), "clicked",
                          GTK_SIGNAL_FUNC (on_sub_button_clicked),
                          pw);
      gtk_tooltips_set_tip(GTK_TOOLTIPS (pw->ProcessURButtonsTooltips), GTK_WIDGET (button),
                           "Subscribe to output", 
                           "Press here to have the output of this process echoed to this window.");
      break;
    case 4: /* UNSUB*/ 
      gtk_signal_connect (GTK_OBJECT (button), "clicked",
                          GTK_SIGNAL_FUNC (on_unsub_button_clicked),
                          pw);
      gtk_tooltips_set_tip(GTK_TOOLTIPS (pw->ProcessURButtonsTooltips), GTK_WIDGET (button),
                           "Unsubscribe from output", 
                           "Click here to stop viewing the output of this process.  This is useful when you want to examine what the process has already output, but there's too much data flying by.  You can resubscribe later, and you'll receive any backlog, so you won't miss anything.");
      break;        
    case 5: /* MAXIMIZE */
      gtk_signal_connect (GTK_OBJECT (button), "clicked",
                          GTK_SIGNAL_FUNC (on_maximize_button_clicked),
                          pw);
      gtk_tooltips_set_tip(GTK_TOOLTIPS (pw->ProcessURButtonsTooltips), GTK_WIDGET (button),
                           "Maximize view of this process", 
                           "Press here to view only this process.");
      break;              
    case 6: /* HIDE */ 
      gtk_signal_connect (GTK_OBJECT (button), "clicked",
                          GTK_SIGNAL_FUNC (on_hide_button_clicked),
                          pw);
      gtk_tooltips_set_tip(GTK_TOOLTIPS (pw->ProcessURButtonsTooltips), GTK_WIDGET (button),
                           "Hide this process (process state will be unaffected)", 
                           "Click here to hide this process.  The state of the process will not be affected; this merely removes the view of this process from the set of tabbed process views.");
      break;        
    default: break;
    }
  }

#if 0
  // WARNING: THIS MUST HAPPEN *AFTER* ALL SETUP!
  // It triggers various callbacks that depend on the processWidget
  // being initialized.
  if(prefs_get_stdout_filtering()) {
    processWidget_show_filter(pw);
  } else {
    processWidget_hide_filter(pw);
  }
#endif
}

GtkWidget*     
processWidget_new (const char* procName, const char* procStatus, const char* machName)
{
  ProcessWidget* pw = PROCESSWIDGET(gtk_type_new (processWidget_get_type ()));
  char* compositeName = (char*) malloc(sizeof(char) * (strlen(procName) + strlen(machName) + 4));

  strcpy(compositeName, procName);
  strcat(compositeName, " @ ");
  strcat(compositeName, machName);
  gtk_label_set_text(GTK_LABEL(pw->ProcessNameLabel), compositeName);
  gtk_label_set_text(GTK_LABEL(pw->ProcessStateLabel), procStatus);

  pw->scrollValue = -1;
  // MEMLEAK: This is a memory leak at the moment.
  pw->viewers = new set<string>();
  //TODO: Figure out what the eff last_line_type was in GTK_LABEL?????
  //pw->last_line_type = 'x';

  // WARNING: THIS MUST HAPPEN *AFTER* ALL SETUP!
  // It triggers various callbacks that depend on the processWidget
  // being initialized.
  if(prefs_get_stdout_filtering()) {
    processWidget_show_filter(pw);
  } else {
    processWidget_hide_filter(pw);
  }

  processWidget_loadHistories(pw);

  free(compositeName);
  return GTK_WIDGET ( pw );
}


// Called by hand from claw_quit().
void
processWidget_cleanup (ProcessWidget* pw)
{
  if(pw == NULL)
    return;

  processWidget_saveHistories(pw);

  // The tooltips aren't being cleaned up (allocated at
  // processWidget.c:405), but I'm not sure how to fix it.
  if(pw->viewers != NULL) {
    delete pw->viewers;
    pw->viewers = NULL;
  }

  if(pw->regexpPatternBuffer.allocated) {
    regfree(&pw->regexpPatternBuffer);
  }

  if(pw->regexpBuffer != NULL) {
    delete pw->regexpBuffer;
    pw->regexpBuffer = NULL;
  }

  if(pw->curRegexp != NULL) {
    delete pw->curRegexp;
    pw->curRegexp = NULL;
  }
}


void
processWidget_term_feed(ProcessWidget* pw, char* text, size_t textBytes)
{
  #ifdef USE_ZVT_TERM
    if (pw->LogTextOutBufBytes+textBytes > LOG_TEXT_OUT_BUF_MAX)
    {
      processWidget_term_flush(pw);
      if (textBytes > LOG_TEXT_OUT_BUF_MAX)
      {
        zvt_term_feed( ZVT_TERM(pw->LogText), text, textBytes );
        return;
      }
    }
    memcpy( &pw->LogTextOutBuf[pw->LogTextOutBufBytes], text, textBytes );
    pw->LogTextOutBufBytes += textBytes;
  #else
    GtkTextView* textViewTF = (GtkTextView*)(pw->LogText->get_text_widget());
    GtkTextBuffer* textBufferTF = gtk_text_view_get_buffer(textViewTF);
    size_t currentTBsize = gtk_text_buffer_get_char_count(textBufferTF);
    if(currentTBsize+textBytes > LOG_TEXT_OUT_BUF_MAX)
    {
      processWidget_term_flush(pw);
      if (textBytes > LOG_TEXT_OUT_BUF_MAX)
      {
        gtk_text_buffer_set_text(textBufferTF, text, textBytes);
        return;
      }
    }
    gtk_text_buffer_set_text(textBufferTF, text, textBytes);
  #endif
}

//flush any content destined for clawText
void processWidget_term_flush(ProcessWidget* pw)
{
  #ifdef USE_ZVT_TERM
  if (pw->LogTextOutBufBytes > 0)
  {
    zvt_term_feed( ZVT_TERM(pw->LogText), pw->LogTextOutBuf, pw->LogTextOutBufBytes );
    pw->LogTextOutBufBytes = 0;
  }
#elif 0
  //  #else
    GtkTextView* textViewTF = (GtkTextView*)(pw->LogText->get_text_widget());
    GtkTextBuffer* textBufferTF = gtk_text_view_get_buffer(textViewTF);
    if (gtk_text_buffer_get_char_count(textBufferTF) > 0)
      gtk_text_buffer_set_text(textBufferTF, "", 0);
  #endif
}

void loadHistory(string procName,
                 vector<string>& history,
                 map<string, vector<string> >& library)
{
  map<string, vector<string> >::iterator libraryIt;
  libraryIt = library.find(procName);
  if(libraryIt != library.end()) {
    FOR_EACH(h, libraryIt->second) {
      //cout << "\t" << *h << endl;
      history.push_back(*h);
    }
  }
}

void processWidget_loadHistories(ProcessWidget* pw)
{
  char* procNamePtr = NULL;
  gtk_label_get(GTK_LABEL(pw->ProcessNameLabel), &procNamePtr);
  string procName = strip_whitespace(strip_pid(procNamePtr));

  //cout << "Loading history of " << procName << "..." << endl;

  // Fill history buffers if data is available.
  //cout << "Entry history: " << endl;
  loadHistory(procName, pw->entryHistory, persistentStdinCommandHistory_g);

  //cout << "Regexp history: " << endl;
  loadHistory(procName, pw->regexpHistory, persistentRegexpHistory_g);
}

void saveHistory(string procName,
                 vector<string>& history,
                 map<string, vector<string> >& library,
                 int maxEntries)
{
  map<string, vector<string> >::iterator libraryIt;
  vector<string>::iterator curHistIt;
  vector<string> tmpStrVec;

  tmpStrVec.clear();
  curHistIt = history.end();
  // We want the *last* N entries
  for(int i = 0; i < maxEntries && curHistIt != history.begin(); i++)
    curHistIt--;

  // The first "entry" is always blank, due to the way we work things.
  if(curHistIt == history.begin() && curHistIt != history.end())
    curHistIt++;

  for(; curHistIt != history.end(); curHistIt++) {
    //cout << "\t" << *curHistIt << endl;
    tmpStrVec.push_back(*curHistIt);
  }

  libraryIt = library.find(procName);
  if(libraryIt != library.end()) {
    library.erase(libraryIt);
  }
  library.insert(pair<string, vector<string> >(procName, tmpStrVec));
}


void processWidget_saveHistories(ProcessWidget* pw)
{
  // Copy data out to the global history

  char* procNamePtr = NULL;
  gtk_label_get(GTK_LABEL(pw->ProcessNameLabel), &procNamePtr);
  string procName = strip_whitespace(strip_pid(procNamePtr));

  //cout << "Saving history of " << procName << "..." << endl;

  // Stdin history
  //cout << "Entry history: (" << (pw->entryHistory.size()-1) << " entries)" << endl;
  saveHistory(procName, pw->entryHistory, persistentStdinCommandHistory_g, prefs_get_stdin_history_length());

  // Filter / regexp history
  //cout << "Filter history: (" << (pw->regexpHistory.size()-1) << " entries)" << endl;
  saveHistory(procName, pw->regexpHistory, persistentRegexpHistory_g, prefs_get_filter_history_length());
}


/***************************************************************
 * Revision History:
 * $Log: processWidget.c,v $
 * Revision 1.46  2007/05/30 19:10:31  brennan
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
 * Revision 1.45  2007/02/15 18:07:06  brennan
 * Fixed a few memory leaks, and one rogue pointer bug.
 * Mapped keypad enter to regular enter in stdin and filter entries.
 * Added focus-follows-mouse option.
 *
 * Revision 1.44  2007/02/13 19:59:39  brennan
 * Fixed segfault due to premature stdout filtering activation/deactivation
 * in processWidget_init.
 *
 * Revision 1.43  2007/01/12 19:42:56  brennan
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
 * Revision 1.42  2006/12/20 19:07:19  brennan
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
 * Revision 1.41  2006/11/16 22:50:31  brennan
 * Made config window default wider (800 pixels).
 *
 * Fixed one echo of client shout per daemon bug.
 *
 * Make filter entry widget expand when it has focus, and vice-versa
 *
 * Claw preferences are auto-saved on exit to the last loaded file, or
 * the default (~/.claw.prefs) otherwise.
 *
 * Preferences load/save dialog is filled with .claw.prefs if no
 * existing prefs file is loaded.
 *
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
 * Revision 1.39  2005/05/18 01:04:58  trey
 * added terminal buffering
 *
 * Revision 1.38  2004/11/25 05:20:48  brennan
 * Fixed run-all bug (using a GList* instead of its data)
 *
 * Patched a bunch of memory leaks, and all the nasty ones.  There are four
 * known remaining leaks, all dealing with the widget used to view the output
 * of a process, with a sum total of ~90 bytes leaked per process viewed.
 * Not worth my time at the moment. :-)
 *
 * Revision 1.37  2004/11/23 07:01:54  brennan
 * A number of memory leaks and free/delete mismatches repaired.
 * Some errors detected by valgrind remain, and will be fixed soon.
 *
 * Revision 1.36  2004/05/23 17:43:35  brennan
 * Added tooltips to buttons in upper-right of processWidget.
 *
 * Revision 1.35  2004/04/28 18:58:50  dom
 * Appended log directive
 * 
 ***************************************************************/
