/********** tell emacs we use -*- c++ -*- style comments *******************
 * COPYRIGHT 2004, Carnegie Mellon University
 * $Revision: 1.4 $  $Author: brennan $  $Date: 2006/11/16 18:55:04 $
 *  
 * PROJECT: FIRE Architecture Project
 *
 * @file    ClawText.cc
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

#include "ClawText.h"
#include "support.h"
#include <gtk/gtktext.h>

using namespace std;

/***************************************************************************
 * MACROS
 ***************************************************************************/

#define CT_INCREMENT(x,inc) ((x) = normalize((x)+inc))
#define CT_LINE_LENGTH(x) (line_length[normalize(x)])

/***************************************************************************
 * FUNCTIONS
 ***************************************************************************/

// Only for the subset of colors enumerated in support.h
static const char *color_to_tag_name (GdkColor *col)
{
  if (!col) return "";

  const char *name =
    ((col->red == 0 && col->green == 0 && col->blue == 0) ? "OK COL" :
     (col->red == 0 && col->green == 32000 && col->blue == 0) ? "GOOD COL" :
     (col->red == 32000 && col->green == 0 && col->blue == 0) ? "WARNING COL" :
     (col->red == 65532 && col->green == 0 && col->blue == 0) ? "ERROR COL" :
     (col->red == 0 && col->green == 0 && col->blue == 65532) ? "SPECIAL COL" :
     (col->red == 0 && col->green == 0 && col->blue == 65532) ? "STDIN COL" :
     (col->red == 0 && col->green == 32000 && col->blue == 0) ? "STATUS COL" :
     (col->red == 35464 && col->green == 20559 &&
      col->blue == 0) ? "ACTION COL" :
     (col->red == 65532 && col->green == 65532 && 
      col->blue == 41118) ? "FILTER COL" :
     (col->red == 65532 && col->green == 65532 &&
      col->blue == 65532) ? "NOFILTER COL" : "");
  return name;
}

static GtkTextTag *get_text_color_tag (GtkTextBuffer *buffer,
				       const char *property, GdkColor *color)
{
  if (!color) return NULL;
  
  const char *name = color_to_tag_name(color);
  if (!name) {
    cerr << "set_text_color: Cannot handle: " << color->red << " "
	 << color->green << " " << color->blue << endl;
    return NULL;
  } else {
    GtkTextTagTable *tags = gtk_text_buffer_get_tag_table(buffer);
    GtkTextTag *tag = gtk_text_tag_table_lookup(tags, name);
    if (!tag) {
      tag = gtk_text_buffer_create_tag(buffer, name, property, color, NULL);
    }
    return tag;
  }
}

ClawText::ClawText(GtkWidget *_text_widget) :
  ref_count(0),
  text_widget(GTK_TEXT_VIEW(_text_widget)),
  end_line(0),
  num_stored_lines(0),
  scroll_value(-1),
  last_line_type('x')
{
  gtk_widget_ref((GtkWidget *) text_widget);
}

ClawText::~ClawText(void) {
  gtk_widget_unref((GtkWidget *) text_widget);
}

void ClawText::ref(ClawText *ct) {
  ct->ref_count++;
}

void ClawText::unref(ClawText *ct) {
  ct->ref_count--;
  if (ct->ref_count <= 0) {
    delete ct;
  }
}

void ClawText::set_font_from_string (const char *font)
{
  gtk_widget_set_font(get_text_widget(), font); 
}

void ClawText::insert(const string& text_in,
		      char line_type,
		      GdkFont *font,
		      GdkColor *fore,
		      GdkColor *back)
{
  //gtk_text_freeze(text_widget);

  string text = text_in;

  // chop final \n character, if any
  int n = text.size();
  if (text[n-1] == '\n') {
    text.erase(n-1);
  }

  // We don't want a blank line at the bottom of the buffer.  Prepend
  // a \n if the previous line ended; this way we should avoid blank
  // space at the buffer's tail.  Also, don't print a \n before the
  // very first line.
  if (last_line_type == 'x' && gtk_text_buffer_get_char_count(gtk_text_view_get_buffer(text_widget)) > 0) {
    text.insert(0, "\n");
  }

//  // add a \n if it's the right type of line
//  if (line_type == 'x') {
//    text += "\n";
//  }

  switch (last_line_type) {
  case 'x':
    // normal line
    if (num_stored_lines == CLAW_TEXT_BUFFER_LINES_LIMIT) {
      delete_first_n_lines(CLAW_TEXT_LINE_DELETION_INCREMENT);
    }
    add_line(text.size());
    break;
  case 'c':
    // continuation line
    CT_LINE_LENGTH(end_line-1) += text.size();
    break;
  case 'r':
    // line ending in cr
    delete_last_line();
    add_line(text.size());
    break;
  default:
    abort(); // never reach this point
  }
  last_line_type = line_type;

  // CONVERSION to GTK 2.0
  //gtk_text_insert(text_widget, font, fore, back, text.c_str(), -1);
  //gtk_text_thaw(text_widget);
  GtkTextBuffer *whatToAdd = gtk_text_view_get_buffer (text_widget);
  GtkTextIter whereToAdd;
  GtkTextIter* addToAdd = &whereToAdd;
  gtk_text_buffer_get_end_iter (whatToAdd, addToAdd);
  GtkTextTag *foreTag = get_text_color_tag(whatToAdd, "foreground-gdk", fore);
  GtkTextTag *backTag = get_text_color_tag(whatToAdd, "background-gdk", back);
  assert(!font); // Cannot handle this, at the moment
  gtk_text_buffer_insert_with_tags (whatToAdd, addToAdd, text.c_str(), 
				    text.length(), 
				    (back && !fore ? backTag : foreTag),
				    (back && !fore ? NULL : backTag), NULL);
}

void ClawText::add_line(int len) {
  CT_LINE_LENGTH(end_line) = len;
  CT_INCREMENT(end_line, 1);
  num_stored_lines++;
}

void ClawText::delete_first_n_lines(int lines_to_delete) {
  int start_line = normalize( end_line - num_stored_lines );

  // add up the number of characters we need to delete based
  // on line lengths
  int num_chars_to_delete = 0;
  for (int i=0; i < lines_to_delete; i++) {
    num_chars_to_delete += CT_LINE_LENGTH(start_line+i);
  }

  // delete the lines from the text widget

  //1.2 TO 2.0 CONVERSION
  GtkTextBuffer *whatToDelete = gtk_text_view_get_buffer (text_widget);
  GtkTextIter whereToDeleteObj, whereToEndObj;
  GtkTextIter *whereToDelete = &whereToDeleteObj;
  GtkTextIter *whereToEnd = &whereToEndObj;

  gtk_text_buffer_get_start_iter (whatToDelete, whereToDelete);
  gtk_text_buffer_get_start_iter (whatToDelete, whereToEnd);

  gtk_text_iter_forward_chars(whereToEnd, num_chars_to_delete);
  gtk_text_buffer_delete (whatToDelete, whereToDelete, whereToEnd);

  //int old_point = gtk_text_get_point(text_widget);
  //gtk_text_set_point(text_widget, 0);
  //gtk_text_forward_delete(text_widget, num_chars_to_delete);
  //gtk_text_set_point(text_widget, old_point-num_chars_to_delete);

  // update the ring buffer
  num_stored_lines -= lines_to_delete;
}

void ClawText::delete_last_line(void) {
  int num_chars_to_delete = CT_LINE_LENGTH(end_line-1);
  
  // delete the line from the text widget
  //1.2 TO 2.0 CONVERSION
  GtkTextBuffer *whatToDelete = gtk_text_view_get_buffer (text_widget);
  GtkTextIter *whereToDelete=NULL, *whereToEnd=NULL;
  
  gtk_text_buffer_get_end_iter (whatToDelete, whereToDelete);
  gtk_text_buffer_get_end_iter (whatToDelete, whereToEnd);

  gtk_text_iter_backward_chars(whereToDelete, num_chars_to_delete);
  gtk_text_buffer_delete (whatToDelete, whereToDelete, whereToEnd);  

  //int old_point = gtk_text_get_point(text_widget);
  //int end_of_text = gtk_text_get_length(text_widget);
  //gtk_text_set_point(text_widget, end_of_text);
  //gtk_text_backward_delete(text_widget, num_chars_to_delete);
  //old_point = MIN(old_point, end_of_text - num_chars_to_delete);
  //gtk_text_set_point(text_widget, old_point);

  // update the ring buffer
  CT_INCREMENT(end_line, -1);
  num_stored_lines--;
}

/***************************************************************************
 * REVISION HISTORY:
 * $Log: ClawText.cc,v $
 * Revision 1.4  2006/11/16 18:55:04  brennan
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
 * Revision 1.3  2004/04/28 17:15:20  dom
 * Added header with copyright information to source and Makefiles.
 *
 * Revision 1.2  2003/06/14 15:49:04  brennan
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
