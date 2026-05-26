/***** tell emacs we use -*- c++ -*- style comments *****
 * $Revision: 1.100 $ $Author: brennan $ $Date: 2007/05/30 19:10:31 $
 *
 * COPYRIGHT 2004, Carnegie Mellon University 
 *
 * PROJECT: Exploration Robotics (Life in the Atacama)
 *
 * MODULE:
 *
 * FILE: microraptor/client/callbacks.c
 *
 * DESCRIPTION:
 *
 ********************************************************/
#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <string>
#include <sys/types.h>
#include <unistd.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <X11/Xos.h>
#include <X11/extensions/shape.h>
#include <X11/Xmu/WinUtil.h>
#include <X11/cursorfont.h>
#include <X11/X.h>

#include <gdk/gdk.h>
#include <gdk/gdkx.h>

#include <sys/types.h>
#include <sys/wait.h>

#include "callbacks.h"
#include "interface.h"
#include "support.h"
#include "processWidget.h"

#include "clientIO.h"
#include "clientComm.h"
#include "mrIPCHelper.h"
#include "RCL.h"
#include "mrCommonDefs.h"
#include "mrCommonTime.h"
#include "processStatus.h" /* MR_IDLE_UPDATE_PERIOD */

#include "imageHeaders.h" // For taskbar icons

using namespace microraptor;

// The notebook pages of the error output window and chat window.
#define EAC_NB_PAGE_ERROR_OUTPUT (0)
#define EAC_NB_PAGE_CHAT (1)

// This has turned into an unholy mix of C++ and C with the integration of
/*  clientComm / MR_Comm.  Sigh. */

/*********************************************************************
 ************************ BEGIN MRCOMM CALLBACKS *********************
 *********************************************************************/

/* Called whenever a new daemon connects to central.
   We are guaranteed it's only called once per daemon per connection, right?
*/
void connect_handler(const string& which_daemon, void *client_data)
{
  DEBUGCB("connect_handler");
  
  GtkCTreeNode* machNode = NULL;
  GtkCTree* processTree = NULL;
  char* colData[3];
  EntryData* newRowData;
  EntryData targRowData;
  targRowData.pw = NULL;

  if(MR_Comm::extract_type_from_module_name(which_daemon) == "d") {
    string toBuffer = which_daemon;
    toBuffer += " just connected.  Welcome to the family!";
    print_to_common_buffer(toBuffer, OK_COL);

    /* Comm list is automagically taken care of by MR_Comm.  Sweet. */
    
    /* Add the daemon to our process tree (in case there aren't any processes defined
       yet, we want to give some visual feedback) 
    */
    processTree = GTK_CTREE(lookup_widget(HeadDino_g, "ProcessTree"));
    /* Check if the machine exists */
    targRowData.label = (char*) which_daemon.c_str();
    machNode = gtk_ctree_find_by_row_data_custom(processTree,
                                                 NULL, /* Where to start searching (root) */
                                                 &targRowData,
                                                 rowCompareFunc);
    /* If not, add it */
    if(machNode == NULL) {
      /* I'm pretty sure colData doesn't get modified in gtk_ctree_insert_node */
      colData[0] = (char*) which_daemon.c_str();
      colData[1] = (char *)"";
      colData[2] = (char *)"";
      
      machNode = gtk_ctree_insert_node(processTree, 
                                       NULL, NULL,
                                       /* I'm pretty sure colData isn't modified */
                                       (char**) colData, 0, 
                                       NULL, NULL, NULL, NULL,
                                       FALSE, prefs_get_view_mode() == DAEMONS_FIRST);
      /* Yup, that strdup is most likely a memory leak; need to figure out the best
         way of cleaning things up.
      */
      newRowData = new EntryData();
      newRowData->label = strdup(which_daemon.c_str());
      newRowData->pw = NULL;
      gtk_ctree_node_set_row_data (processTree, machNode, newRowData);   
      // Potentially need to resort
      gtk_ctree_sort_node(processTree, NULL);
    }
    
    /* First, load our most recent config file, but only if the the daemon connected
       before we did, in order to avoid a repeat of the Indy 500 (with more collisions)
       whenever a new daemon logs on.
       THIS IS NOW DEPRECATED IN FAVOR OF NEW FANCY SYNCHING SCHEME
    */
    //if(daemon_comm_g->get_connection_order() != MR_THIS_MODULE_BEFORE_OTHER_MODULE)
    //  send_local_config(local_config_g);

    /* If the daemon is in our ssh_display list (at the moment, this means we externally
       set up the ssh tunnel and passed in the resulting display), tell the daemon to
       route any displays we start over it. */
    try {
      bool setClient = false;
      string clientString;
      rcl::map sshBody = prefs_g[0]("ssh_displays").getMap();
      FOR_EACH(ssh_disp, sshBody) {
        if(which_daemon.find(ssh_disp->first) < which_daemon.length()) {
          clientString = string("set client {DISPLAY=\"") + (ssh_disp->second).getString() + "\"}";
          daemon_comm_g->send_message(which_daemon, clientString);
          setClient = true;
          break;
        }
      }
      // Always set default client data, otherwise processes that have %c's in them will fail
      //  to start.
      if(!setClient) {
        clientString = "set client {DISPLAY=\":0.0\"}";
        daemon_comm_g->send_message(which_daemon, clientString);
      }
    } catch(rcl::exception err) {
      cerr << "Caught RCL exception in while dealing with display forwarding in connect_handler: " << err.text << endl;
    }
    
    /* enable daemon output buffering to improve throughput */
    daemon_comm_g->send_message(which_daemon, "set buffer block");

    /* Subscribe to clock messages */
    get_clock(which_daemon.c_str());
    
    /* other initial subscriptions */
    daemon_comm_g->send_message(which_daemon, "sub status");
    daemon_comm_g->send_message(which_daemon, "sub config");
    daemon_comm_g->send_message(which_daemon, "sub shout");
    
    /* Get config as well, to get any existing groups 
       THIS IS NOW DEPRECATED IN FAVOR OF NEW FANCY SYNCHING SCHEME
    */
    //daemon_comm_g->send_message(which_daemon, "get config -a");

    /* Do the implicit get -a */
    get_status_process(which_daemon.c_str(), "-a");    
  } // End check if it's a daemon connecting
  else if(MR_Comm::extract_type_from_module_name(which_daemon) == "c") {
    string toBuffer = "Client ";
    toBuffer += which_daemon;
    toBuffer += " just connected.  They're watching... they're always watching...";
    print_to_common_buffer(toBuffer, OK_COL);
  }
}

/* Ugh.  No nice way to pass this into daemon_group_cleanup_helper w/o 
   creating a specialized struct, which is dumb.
*/
GList* dgch_death_list = NULL;

/* Helper for removing all group member nodes that belong to a daemon that's
   disconnecting.  Be sure this gets called before the daemon itself (or any
   of its children) is removed from the tree!!!
*/
void
daemon_group_cleanup_helper(GtkCTree *ctree,
                            GtkCTreeNode *node,
                            gpointer which_daemon_gptr)
{
  DEBUGCB("daemon_group_cleanup_helper\n");
  const char* which_daemon = (const char*) which_daemon_gptr;
  char* machName;
  EntryDataAug* curRowData;
  EntryData* realRowData;
  //GtkCTreeNode* specGrpNode;
  char* procname;
  get_names_from_node(ctree, node, &procname, NULL, NULL);
  //cerr << "Cleaning: " << which_daemon << "\t" << procname << endl;

  // Check if it's a group member and if its real node's parent
  //  is which_daemon.
  if(get_nodetype(ctree, node) == GROUP_MEMBER_NODE) {
    //cerr << "\tHosing it out" << endl;
    // Find parent of real node's name
    curRowData = (EntryDataAug*) gtk_ctree_node_get_row_data(ctree, node);
    if(curRowData != NULL && curRowData->realNode != NULL) {
      get_names_from_node(ctree, curRowData->realNode, NULL, NULL, &machName);
      // If our real parent is the daemon that's disconnecting, remove ourself from
      //   the group subtree
      if(strcmp(machName, which_daemon) == 0) {
        //cerr << "\tReally really hosing it out from " << machName << endl;
        // First, remove the pointer to the group member node from
        //  the real node
        realRowData = (EntryData*) gtk_ctree_node_get_row_data(ctree, curRowData->realNode);
        realRowData->grpMems.erase(node);
        gtk_ctree_node_set_row_data(ctree, node, realRowData);
        // Find our parent (the specific group we're a member of)
        //specGrpNode = GTK_CTREE_ROW(node)->parent;
        // Then, delete the group member node from the tree
        //gtk_ctree_remove_node(ctree, node);
        dgch_death_list = g_list_append(dgch_death_list, node);
        // If that was the last member of the group, remove that particular group
        //if(specGrpNode != NULL && GTK_CTREE_ROW(specGrpNode)->children == NULL) {      
        //  gtk_ctree_remove_node(ctree, specGrpNode);
        //}        
      }
    }
  }
  // Else, do nada
  //cerr << "\tLeaving alone" << endl;
}

/* Called whenever a daemon disconnects from central.
   We are guaranteed it's only called once per daemon per connection, right?
*/
void 
disconnect_handler(const string& which_daemon, 
                   void *client_data)
{
  DEBUGCB("disconnect_handler\n");
  GtkCTree *processTree = GTK_CTREE(lookup_widget(HeadDino_g, "ProcessTree"));
  GtkCTreeNode *node;
  GtkCTreeNode *grpNode;
  GtkCTreeNode *topGrpNode;
  string toBuffer = "WARNING: Daemon ";
  EntryData targRowData;
  targRowData.pw = NULL;

  if(MR_Comm::extract_type_from_module_name(which_daemon) != "d") {
    toBuffer = "FYI, client ";
    toBuffer += which_daemon;
    toBuffer += " has disconnected.  May it rest in pieces!";
    print_to_common_buffer(toBuffer, SPECIAL_COL);
    return;
  }

  /* Remove all processes running on this daemon from the various groups 
     This _must_ happen before any of the "real" nodes under the daemon
     are removed. */
  topGrpNode = get_node_from_grp_mem_name("Groups", NULL);
  if(topGrpNode != NULL) {
    dgch_death_list = NULL;
    // We can't actually remove nodes within a post_recursive without screwing
    //  it up royally
    gtk_ctree_post_recursive(processTree, topGrpNode,
                             daemon_group_cleanup_helper, (void*) which_daemon.c_str());
    // Walk through the list of the condemned, whacking them
    FOR_GLIST(walker, dgch_death_list) {
      // Cache group parent
      grpNode = GTK_CTREE_ROW(GTK_CTREE_NODE(walker->data))->parent;
      // Then, delete the group member node from the tree
      gtk_ctree_remove_node(processTree, GTK_CTREE_NODE(walker->data));
      // If that was the last member of the group, remove that particular group
      if(grpNode != NULL && GTK_CTREE_ROW(grpNode)->children == NULL) {      
        gtk_ctree_remove_node(processTree, grpNode);
        // If that was the last group, remove the "Groups" node
        if(topGrpNode != NULL && GTK_CTREE_ROW(topGrpNode)->children == NULL) {
          gtk_ctree_remove_node(processTree, topGrpNode);
        }
      }        
    }
    g_list_free(dgch_death_list);
    dgch_death_list = NULL;
  }

  /* Find the node of the process tree associated with this daemon */
  /* Check if the machine exists */
  targRowData.label = (char*) which_daemon.c_str();
  node = gtk_ctree_find_by_row_data_custom(processTree,
                                           NULL, /* Where to start searching (root) */
                                           &targRowData,
                                           rowCompareFunc);  

  /* Hide all associated processes */
  gtk_ctree_post_recursive(processTree, node, hide_all_helper, processTree);

  /* Remove chunk from tree */
  gtk_ctree_remove_node(processTree, node);

  /* Report to buffer */
  toBuffer += which_daemon;
  toBuffer += " has died / been disconnected!";
  print_to_common_buffer(toBuffer.c_str(), ERROR_COL);
}

/* Update our local process state; if the given machine and/or process
   do not exist, add them to the CTree.
*/
void update_local_process
(const char* machName, const char* procName, const char* status, int idleTime = -1) 
{
  DEBUGCB("update_local_process\n");
  const char* colData[3];
  GtkCTree* processTree = GTK_CTREE(lookup_widget(HeadDino_g, "ProcessTree"));
  GtkCTreeNode* machNode;
  GtkCTreeNode* procNode;
  //GtkTable      *processTable;
  //GtkTableChild *table_child;
  //GList         *list;
  string        compositeName;
  string        prettyLastStdout;
  char          *shortLastStdout;
  ProcessWidget *pw;
  //char          *searchName;
  GdkColor      col;
  EntryData     *rowData;
  EntryData     *newRowData;
  ProcessData    pd;
  EntryData targRowData;
  targRowData.pw = NULL;
  init_process_data(&pd);

  if(status == NULL) col = enum_to_color(OK_COL);
  else col = status_to_color(status);
  
  fill_process_data(&pd, procName, machName, status);
  
  if( !strcmp(status, "unknown")
      || !strcmp(status, "not_started")
      || !strcmp(status, "clean_exit")
      || !strcmp(status, "error_exit")
      || !strcmp(status, "signal_exit") ) {
    prettyLastStdout = " ";
    shortLastStdout = strdup(" ");
  } else if (idleTime == -1) {
    prettyLastStdout = (char *)"";
    shortLastStdout = strdup("");
  } else if(idleTime == 0) {
    prettyLastStdout = "Active";
    shortLastStdout = strdup("Active");
  } else {
    int idleUpperBound = idleTime + MR_IDLE_UPDATE_PERIOD;
    prettyLastStdout = "Idle " + rcl::toString(idleTime) + "-"
      + rcl::toString(idleUpperBound);
    shortLastStdout = strdup(prettyLastStdout.c_str());
  }

  /* Bail on bad input */
  if(machName == NULL || procName == NULL) return;
  
  /* Check if the machine exists */
  targRowData.label = (char*) machName;
  machNode = gtk_ctree_find_by_row_data_custom(processTree,
                                               NULL, /* Where to start searching (root) */
                                               &targRowData,
                                               rowCompareFunc);
  /* If not, add it */
  if(machNode == NULL) {
    colData[0] = machName;
    colData[1] = (char *)"";
    colData[2] = (char *)"";
    machNode = gtk_ctree_insert_node(processTree, NULL, NULL,
                                     /* I'm pretty sure colData isn't modified*/
                                     (char**) colData, 0, 
                                     NULL, NULL, NULL, NULL,
                                     FALSE, prefs_get_view_mode() == DAEMONS_FIRST);
    newRowData = new EntryData();
    newRowData->label = strdup(machName);
    newRowData->pw = NULL;
    gtk_ctree_node_set_row_data (processTree, machNode, newRowData);
    // This potentially affects the order of the tree; resort
    gtk_ctree_sort_node(processTree, NULL);
  }
  /* Check if the process exists under this machine */
  targRowData.label = (char*) procName;
  procNode = gtk_ctree_find_by_row_data_custom(processTree,
                                               machNode, /* Where to start searching */
                                               &targRowData,
                                               rowCompareFunc);
  /* If not, add it with new status */
  if(procNode == NULL) {    
    colData[0] = procName;
    colData[1] = status;
    colData[2] = shortLastStdout;
    procNode = gtk_ctree_insert_node(processTree, machNode, NULL,
                                     /* I'm pretty sure colData isn't modified*/
                                     (char**) colData, 0,
                                     NULL, NULL, NULL, NULL,
                                     TRUE, TRUE);
    rowData = new EntryData();
    rowData->label = strdup(procName);
    rowData->pw = NULL;

    // Find any groups that include us and add pointers
    // I don't _think_ this is necessary (groups should only add members after
    //  the real node is created), but it can't hurt...
    //GList* grpMembers = get_all_data(processTree, GROUP_MEMBER_NODE);
    //ProcessDataAug* pda;
    //FOR_GLIST(walker, grpMembers) {
    //  if(walker->data == NULL) continue;
    //  pda = (ProcessDataAug*) walker->data;
    //  if(strcmp(pda->procName, procName) == 0) {
    //    
    //  }
    //}
    gtk_ctree_node_set_row_data (processTree, procNode, rowData);
    gtk_clist_set_foreground(GTK_CLIST(processTree), 
                             gtk_clist_find_row_from_data(GTK_CLIST(processTree), rowData),
                             &col);
    gtk_widget_show(GTK_WIDGET(processTree));

  }
  /* Else, update status */
  else {
    EntryData* ed = (EntryData*) gtk_ctree_node_get_row_data(processTree, procNode);
    gtk_ctree_node_set_text(processTree, procNode, 1, status);
    // If we have a valid idle time, update it.  Otherwise, stick with the
    //  old one.
    if(strlen(shortLastStdout) > 0) 
      gtk_ctree_node_set_text(processTree, procNode, 2, shortLastStdout);
    gtk_clist_set_foreground(GTK_CLIST(processTree), 
                             gtk_clist_find_row_from_data(GTK_CLIST(processTree), 
                                                          GTK_CTREE_ROW(procNode)->row.data),
                             &col);
    // Update the status of all (if any) of the group members that point to us
    FOR_EACH(memNode, ed->grpMems) {
      gtk_ctree_node_set_text(processTree, *memNode, 1, status);
      // If we have a valid idle time, update it.  Otherwise, stick with the
      //  old one.
      if(strlen(shortLastStdout) > 0) 
        gtk_ctree_node_set_text(processTree, *memNode, 2, shortLastStdout);
      gtk_clist_set_foreground(GTK_CLIST(processTree), 
                               gtk_clist_find_row_from_data(GTK_CLIST(processTree), 
                                                            GTK_CTREE_ROW(*memNode)->row.data),
                               &col);
    }

    gtk_widget_show(GTK_WIDGET(processTree));
    /* This is the only case where a view could exist; the process didn't
       exist in the processTree otherwise (and thus couldn't have been viewed).
       Update the status in the titlebar of the processWidget... */

    if(ed != NULL && ed->pw != NULL) {
      pw = PROCESSWIDGET(ed->pw);
      // Set the status display 
      gtk_label_set_text(GTK_LABEL(PROCESSWIDGET(pw)->ProcessStateLabel),
                         status);
      gtk_widget_show(PROCESSWIDGET(pw)->ProcessStateLabel);
      // Set last stdout display 
      if(prettyLastStdout.length() > 0) {
        gtk_label_set_text(GTK_LABEL(PROCESSWIDGET(pw)->ProcessLastStdoutLabel),
                           prettyLastStdout.c_str());
        gtk_widget_show(PROCESSWIDGET(pw)->ProcessLastStdoutLabel);
      }
        
      //cerr << "Show and tell: ";
      //print_process_data(&pd);
      //cerr << endl;
      //cerr << "calling pwbs: update local process" << endl;
      processWidget_button_showandtell(GTK_WIDGET(pw), &pd);
    }

  } /* end "else we're already in the tree" */
  free_process_data_members(&pd);
  free(shortLastStdout); // Potential bug
}

void handle_proc_config(const string& machName, const string& procName, rcl::exp& procConfig)
{
  DEBUGCB("handle_proc_config\n");
  // NOTE: unlike handle_group_config, this doesn't actually add anything to the
  //       processTree; my bad on inconsistent software design - the groups stuff
  //       was added significantly after the inital process foo.  If you're looking
  //       for tree update code, look at update_local_process() in callbacks.c (here).

  // First, update the display to match the new data
  // The only thing we care about is "depends"; this should be cached, really.
  // FILLMEIN

  // Update any appropriate group displays
  string grpString;
  string grpName;
  GtkCTree* processTree = GTK_CTREE(lookup_widget(HeadDino_g, "ProcessTree"));
  GList* grpList = get_all_data(processTree, GROUP_NODE);
  // Check all the groups; this process may be part of multiple groups
  FOR_GLIST(walker, grpList) {
    if(walker->data != NULL) {
      // We're looking at "machName", because a list of group member processes is stored there.
      grpString = ((ProcessDataAug*) walker->data)->machName;
      if(grpString.find(procName) < grpString.length()) {
        grpName = string(((ProcessDataAug*) walker->data)->procName);
        // Hmm, we're part of the group!  Try to add ourselves.
        //  add_grp_mem_to_tree avoids double-adding
        //  Yeah, the re-search for grpNode is inefficient, but otherwise we'd need 
        //  yet another data structure, etc. etc. ad nauseum.  This gets called 
        //  infrequently anyhow.
        add_grp_mem_to_tree(procName, 
                            get_node_from_procname(grpName.c_str()), 
                            processTree, 
                            grpName);
      }
    }
  }


  // Now, launch any pending set_config dialogs
  string label = procName + " @ " + machName;
  list<string>::iterator lit;
  for(lit = pending_sets_g.begin(); lit != pending_sets_g.end(); lit++) {
    if(*lit == label) {
      /* Okay, the callback was a one-shot deal */
      pending_sets_g.erase(lit);
      /* Launch set_config dialog */
      GtkWidget* sd = create_editHashDialog(label, 
                                            procConfig, 
                                            procConfigDefault_g, 
                                            on_process_config_changed);
      if(sd == NULL) continue;
      /* Show the sucker */
      add_error_trap();
      gtk_window_set_position(GTK_WINDOW(sd), GTK_WIN_POS_MOUSE);
      gtk_widget_show(sd);
      add_taskbar_icon (sd, miniClawImage_xpm);
      clear_error_traps();
      break;
    }
  }
}

void handle_group_config(const string& grpName, rcl::exp& grpConfig)
{
  DEBUGCB("handle_group_config\n");
  const char* colData[3];
  GtkCTree* processTree = GTK_CTREE(lookup_widget(HeadDino_g, "ProcessTree"));
  GtkCTreeNode* topGrpNode;
  GtkCTreeNode* grpNode;
  string grpMembers;
  EntryData* rowData;
  EntryData* newRowData;
  EntryData targRowData;
  targRowData.pw = NULL;
  
  // We only know about group bodies consisting of a string of members
  if(grpConfig.getType() != "string") {
    print_to_common_buffer("WARNING: claw doesn't understand group configs that aren't strings.",
                           WARNING_COL);
    return;
  }

  grpMembers = grpConfig.getString();

  // Check if the toplevel groups node exists
  targRowData.label = (char *)"Groups";
  topGrpNode = gtk_ctree_find_by_row_data_custom(processTree,
                                                 NULL, /* Where to start searching (root) */
                                                 &targRowData,
                                                 rowCompareFunc);
  // If it ain't there, add it
  if(topGrpNode == NULL) {
    colData[0] = "Groups";
    colData[1] = (char *)"";
    colData[2] = (char *)"";
    topGrpNode = gtk_ctree_insert_node(processTree, NULL, NULL,
                                     /* I'm pretty sure colData isn't modified*/
                                     (char**) colData, 0, 
                                     NULL, NULL, NULL, NULL,
                                     FALSE, TRUE);
    newRowData = new EntryData();
    newRowData->label = strdup("Groups");
    newRowData->pw = NULL;
    gtk_ctree_node_set_row_data (processTree, topGrpNode, newRowData);
    // If we added the toplevel group node, we potentially affected
    //    the order of top level of the tree; resort
    gtk_ctree_sort_node(processTree, NULL);
  }

  // Find the group (under the "Groups" node)
  // Yes, it would be slightly faster to pay attention to whether we just 
  //   added the top node, but this is easier to maintain.
  targRowData.label = (char*) grpName.c_str();
  grpNode = gtk_ctree_find_by_row_data_custom(processTree,
                                              topGrpNode, /* Where to start searching */
                                              &targRowData,
                                              rowCompareFunc);
  // If this group isn't in the display, add it
  if(grpNode == NULL) {
    string grpMemOnly = grpMembers;
    int dashpLoc = grpMemOnly.find("-p ");
    if(dashpLoc >= 0) {
      grpMemOnly = grpMemOnly.erase(dashpLoc, 3);
    } else {
      dashpLoc = grpMemOnly.find("-p");
      if(dashpLoc >= 0) {
        grpMemOnly = grpMemOnly.erase(dashpLoc, 2);
      }
    }
    colData[0] = grpName.c_str();
    colData[1] = grpMemOnly.c_str();
    colData[2] = (char *)"";
    grpNode = gtk_ctree_insert_node(processTree, topGrpNode, NULL,
                                    /* I'm pretty sure colData isn't modified*/
                                    (char**) colData, 0,
                                    NULL, NULL, NULL, NULL,
                                    FALSE, prefs_get_view_mode() == GROUPS_FIRST);
    rowData = new EntryData();
    rowData->label = strdup(grpName.c_str());
    rowData->pw = NULL;
    gtk_ctree_node_set_row_data (processTree, grpNode, rowData);
  }
  // If it is, update the display of its members
  else {
    gtk_ctree_node_set_text(processTree, grpNode, 1, grpMembers.c_str());
  }

  // Add the group members as children of grpNode if they're not already there
  // add_grp_mem_to_tree() is defined over in support.c
  // Yes, this could be stuck in the if() below, but it wouldn't be as clear
  bool parallel = add_grp_mem_to_tree(grpMembers, grpNode, processTree, grpName);
  
  // Set parallel / serial tag
  if(parallel) {
    gtk_ctree_node_set_text(processTree, grpNode, 2, "parallel");
  } else {
    gtk_ctree_node_set_text(processTree, grpNode, 2, "serial");
  }

  gtk_widget_show(GTK_WIDGET(processTree));    
}

/* Quicky helper function to ease parsing of 'MRAPTORD: ' lines.
 * Takes a line using the [due_to *] syntax, after removal of
 * 'MRAPTORD: ', and replaces the due_to clause with 'client>' */
string munge_due_to(string toProcess, const string& prefix) {
  //cout << "Munging due_to; prefix: '" << prefix << "'; input: '" << toProcess << "'." << endl;
  toProcess.erase(0, prefix.size());
  //cout << "Erased prefix: '" << toProcess << "'" << endl;
  toProcess.erase(0, strlen("[due_to "));
  //cout << "Erased due_to: '" << toProcess << "'" << endl;
  // find returns an index, not a length, so no need to -1 to skip ']'
  string client = toProcess.substr(0, toProcess.find("]"));
  // Also delete ']'
  toProcess.erase(0, client.size()+1);
  //cout << "Erased client: '" << toProcess << "' (client size: " << client.size() << ")" << endl;
  if(client.size() > 0) {
    client = strip_client_name(client);
    client += ">";
  } else {
    client = "unknown>";
  }
  toProcess.insert(0, client);
  //cout << "Inserted client: '" << toProcess << "'" << endl;
  //cout << "Munged output: '" << toProcess << "'." << endl;
  return toProcess;
}

/* If I'm understanding things correctly, this fires whenever there's a response
   from the daemon.  This will be where we update all of our local state from
   the daemon's data.
*/
void line_handler(const string& machName, const string& text, void *client_data) 
{
  DEBUGCB("line_handler\n");
  string toBuffer = text; /* Default to printing everything */
  string status; /* We use this a lot */
  string procName = (char *)"";
  string toProcess = (char *)"";
  enum printColors fore = OK_COL;
  char lineType;
  
  //cerr << "RH: from \"" << machName << "\"" << endl;

  //cerr << "RH: \"" << text << "\"" << endl;
  
  try {
    // This could throw an exception too
    rcl::exp e = rcl::readFromString(text);


    /* If there's nothing there, ditch */
    if (!e[0].defined()) {
      return;
    }
    
    /* Handle the various types of input */
    if(e[0].getType() == "string" && e[0].getString() == "response") {
      /* Print to buffer if it's an error or a warning */
      if(e[1].defined() 
         && e[1].getType() == "string" 
         && e[2].defined() /* Gotta have an actual error or warning */
         && e[2].getType() == "string"
         && e[1].getString() != "ok") {
        if(e[1].getString() == "error") {
          fore = ERROR_COL;
          toBuffer = "ERROR: ";
        } else if(e[1].getString() == "warning") {
          fore = WARNING_COL;
          toBuffer = "WARNING: ";
        }      
        toBuffer += e[2].getString();
      } else {
        /* Don't print out the "ok"s or empty errors / warnings */
        /* TODO: actually keep track of commands sent and error if we don't get a response */
        toBuffer = (char *)"";
      }
    }
    /* Status message */
    else if(e[0].getType() == "string" && e[0].getString() == "status") {
      /* Is this the right type? */
      if(e[1].getType() == RCL_MAP_TYPE) {
        string bufOut;
        bool statusChanged = true;
        string oldStatus;

        toBuffer = (char *)"";
        status = e[1]("status").getString();
        procName = e[1]("name").getString();
        // Get current status
        oldStatus = get_status_from_processtree(procName.c_str(), machName.c_str());

        int idleTime = -1;
        // Check if our status has changed; changes from unknown don't
        // count, so we don't print a bunch of stuff when we start up
        // and get the initial status reports.
        if(oldStatus != status && oldStatus != "unknown") statusChanged = true;
        else statusChanged = false;

        // Make change to last_stdout no matter what
        if(e[1]("idle").defined()) {
          idleTime = e[1]("idle").getLong();
        }

        update_local_process(machName.c_str(), procName.c_str(),
			     status.c_str(), idleTime);
        if(status == "running") {
          if(statusChanged) {
            char pidStr[255];
            sprintf(pidStr, "%ld", e[1]("pid").getLong());
            string bufOut = procName;
            bufOut += " is now running (PID = ";
            bufOut += pidStr;
            bufOut += ")";
            print_to_common_buffer(bufOut.c_str(), GOOD_COL);
          }          
          // Show stdin entry box, if we're displaying it
          toggle_stdin_entry(procName, machName, true);
        }
        else if(status == "not_started") {
          // No special printout for this
          // Hide stdin entry box, if we're displaying it
          toggle_stdin_entry(procName, machName, false);
        }
        else if(status == "pending") {
          if(statusChanged) {
            string bufOut = procName;
            bufOut += " is now pending.";
            print_to_common_buffer(bufOut.c_str(), SPECIAL_COL);
          }
          // Hide stdin entry box, if we're displaying it
          toggle_stdin_entry(procName, machName, false);
        } 
        else if(status == "starting") {
          if(statusChanged) {
            char pidStr[255];
            sprintf(pidStr, "%ld", e[1]("pid").getLong());
            string bufOut = procName;
            bufOut += " is now starting (PID = ";
            bufOut += pidStr;
            bufOut += ")";
            print_to_common_buffer(bufOut.c_str(), SPECIAL_COL);
          }          
          // Show stdin entry box
          toggle_stdin_entry(procName, machName, true);
        }
        // Wrap all exits so we can do common timestamp extraction
        else if(status == "clean_exit" \
                || status == "error_exit" 
                || status == "signal_exit"
                || status == "unknown") {
          // Hide stdin entry box, if we're displaying it
          toggle_stdin_entry(procName, machName, false);
          // Get timestamp
          //timeval ts_tv = timevalOf(e[1]("exit_time").getLong());
          
          // The below is all printouts
          if(statusChanged) {
            if(status == "clean_exit") {
              bufOut = procName;
              bufOut += " exited cleanly.";
              print_to_common_buffer(bufOut.c_str(), GOOD_COL);
            } // end "clean_exit"
            else if(status == "error_exit") {
              char exitVal[255];
              sprintf(exitVal, "%ld", e[1]("exit_value").getLong());
              bufOut = procName;
              bufOut += " exited with error value ";
              bufOut += exitVal;
              fore = ERROR_COL;
              print_to_common_buffer(bufOut.c_str(), ERROR_COL);
            } // end "error_exit"
            else if(status == "signal_exit") {
              char termVal[255];
              sprintf(termVal, "%ld", e[1]("terminating_signal").getLong());
              bufOut = procName;
              bufOut += " exited due to an uncaught signal (signal number ";
              bufOut += termVal;
              bufOut += ")";
              print_to_common_buffer(bufOut.c_str(), GOOD_COL);
            } // end "signal_exit"
          } // end "if statusChanged"
        } // end exits
        // End status cases  
      } else {
        fore = WARNING_COL;
        toBuffer = "WARNING: status message without a hash!";
      }
    }
    /* Chat output */
    else if(e[0].getType() == "string" && e[0].getString() == "shout") {
      string clientMachName = e[1].getString();
      //string userName = e[2].getString();
      string msg = e[2].getString();
      // Strip out client machine name
      clientMachName = strip_client_name(clientMachName);

      // username is now included in machine name.
      string out = clientMachName;
      out += ": ";
      out += msg;
      // Colorize lines we sent; leave everything else the same.
      if(e[1].getString() == daemon_comm_g->get_this_module_name()) {
        print_to_chat_buffer(out.c_str(), GOOD_COL);
      } else {
        print_to_chat_buffer(out.c_str(), OK_COL);
      }

      // Check if the chat buffer is currently up; if not, highlight its tab.
      GtkNotebook* eacNotebook = GTK_NOTEBOOK(lookup_widget(HeadDino_g, "errorAndChatNotebook"));
      // This notebook is static; let's cheat and just use the page number
      if(gtk_notebook_get_current_page(eacNotebook) == EAC_NB_PAGE_ERROR_OUTPUT) {
        // Also see on_error_and_chat_notebook_page_switched
        static GtkStyle *alertStyle = NULL;
        if(alertStyle == NULL) {
          GtkStyle* old = gtk_widget_get_style(GTK_WIDGET(eacNotebook));
          alertStyle = gtk_style_new();
          // We store the original themed ACTIVE value in INSENSITIVE
          alertStyle->bg[GTK_STATE_INSENSITIVE] = old->bg[GTK_STATE_INSENSITIVE];
          alertStyle->bg[GTK_STATE_ACTIVE].red = 65535; // < 65532
          alertStyle->bg[GTK_STATE_ACTIVE].green = 0;
          alertStyle->bg[GTK_STATE_ACTIVE].blue = 0;
        }
        gtk_widget_set_style(GTK_WIDGET(eacNotebook), alertStyle);
      }

      toBuffer = (char *)""; // Don't echo to error buffer too.
    }
    /* Output of process */
    else if(e[0].getType() == "string" && e[0].getString() == "stdout") {
      static hash_map<string, pair<bool, enum printColors> > colorizedPrinting;
      hash_map<string, pair<bool, enum printColors> >::iterator cpit;
      enum printColors outColor = OK_COL;

      /* Figure out which process we're dealing with */
      procName = e[1].getString();
      timeval ts_tv = timevalOf(e[2].getDouble());
      lineType = e[3].getString()[0];
      toProcess = /*string(timebuf) +*/ e[4].getString().c_str();

      /* Reformat and recolorize if it's a line that's been inserted
       * by mraptord. */
      string mrpdPref = "MRAPTORD: ";
      if(toProcess.substr(0, mrpdPref.size())  == mrpdPref) {
        toProcess.erase(0, mrpdPref.size());
        string warnPref = "[warning] ";
        string stdinPref = "[stdin] ";
        string actionPref = "[action] ";
        string errorPref = "[error] ";
        string statusPref = "[status] ";

        if(toProcess.substr(0, warnPref.size()) == warnPref) {
          toProcess.erase(0, warnPref.size());
          toProcess.insert(0, "WARNING: ");
          outColor = ERROR_COL;
        } else if(toProcess.substr(0, errorPref.size()) == errorPref) {
          toProcess.erase(0, errorPref.size());
          toProcess.insert(0, "ERROR: ");
          outColor = ERROR_COL;
        } else if(toProcess.substr(0, stdinPref.size()) == stdinPref) {
          toProcess = munge_due_to(toProcess, stdinPref);
          outColor = STDIN_COL;
        } else if(toProcess.substr(0, actionPref.size()) == actionPref) {
          toProcess = munge_due_to(toProcess, actionPref);
          outColor = ACTION_COL;
        } else if(toProcess.substr(0, statusPref.size()) == statusPref) {
          toProcess.erase(0, statusPref.size());
          toProcess.insert(0, "STATUS: ");
          outColor = STATUS_COL;
        }
        // We don't want to stop colorizing in mid-line
        if(lineType == 'c') {
          if((cpit = colorizedPrinting.find(procName)) != colorizedPrinting.end()) {
            (*cpit).second.first = true;
            (*cpit).second.second = outColor;
          } else {
            pair<bool, enum printColors> pc(true, outColor);
            colorizedPrinting.insert(pair<string, pair<bool, enum printColors> >(procName, pc));
          }
        }
      } else if((cpit = colorizedPrinting.find(procName)) != colorizedPrinting.end()
                && (*cpit).second.first) {
        outColor = (*cpit).second.second;
        if(lineType == 'x') {
          (*cpit).second.first = false;
        }
      } else {
        outColor = OK_COL;
      } // End "not a special line"

      status = get_status_from_processtree(procName.c_str(), machName.c_str());
      /* Update idletimes */      
#if 0
      /* shouldn't need this statement: the daemon will give us an idle
	 time update if a new stdout line comes in */
      update_local_process(machName.c_str(), procName.c_str(), status.c_str(), e[2].getDouble());
#endif

      /* Print to output buffers */
      print_to_processes(procName.c_str(), 
                         machName.c_str(), 
                         toProcess.c_str(),
                         HeadDino_g,
                         outColor, //fore,
                         &ts_tv,
                         lineType);
      toBuffer = (char *)""; /* That would be a bit of overload... */
    }
    /* Clock message */
    else if(e[0].getType() == "string" && e[0].getString() == "clock") {
      timeval ts_tv, curtime_tv, skew_tv;
      char skew_string[255];

      // Get current local time
      curtime_tv = getTime();

      // Decode timestamp
      ts_tv = timevalOf(e[1].getDouble());
      
      // Calculate skew
      // Possible TODO: do some sort of averaging / slew calculations
      //   (a la NTP), rather than just taking the most recent skew.
      skew_tv = timevalOf(timeDiff(curtime_tv, ts_tv));

      // Store skew in our global skew variable
      hash_map<string, timeval>::iterator it;
      if((it = clockSkew_g.find(machName)) == clockSkew_g.end()) {
        clockSkew_g.insert(pair<string, timeval>(machName, skew_tv));
      } else {
        (*it).second = skew_tv;
      }

      // Update processTree
      // Print informative message
      if(prefs_g[0]("display_skew").defined()
         && prefs_g[0]("display_skew").getLong()) {
        sprintf(skew_string, "%.2f", doubleOf(skew_tv));
        string msg = "Skew: ";
        msg += skew_string;
        msg += " s";
        update_skew_display(machName, msg);
      }
      toBuffer = (char *)"";
      // End clock case
    }
    /* Config message */
    else if(e[0].getType() == "string" && e[0].getString() == "config") {
      if (e[1].getType() == RCL_MAP_TYPE) {
        //cerr << "response handler: config message: full contents of e:" << endl;
        //e.writeConfig(cerr, 1);
        //cerr << endl;
        // Figure out if this is a prior file or not
        bool is_prior = (e[2].defined() && e[2].getString() == "prior");
        if(e[1]("processes").defined()) {
          // Incorporate into our view of reality
          if(is_prior) {
            rcl::exp myprocesses = local_config_g("processes");
            local_config_g("processes") = e[1]("processes");
            if(myprocesses.defined()) {
              local_config_g("processes").addFrom(myprocesses);
            }
          } else {
            rcl::exp theirprocesses = e[1]("processes");
            if(local_config_g("processes").defined())
              local_config_g("processes").addFrom(theirprocesses);
            else
              local_config_g("processes") = theirprocesses;
          }
          // Add to display
	  rcl::map procBody = e[1]("processes").getMap();
          string procName, hostName, curMachName, tmpMachName, status, idleString;
          rcl::exp procConfig;
          GList* machList = get_all_data(GTK_CTREE(lookup_widget(HeadDino_g, "ProcessTree")), 
                                         PROCESS_TOP_NODE);
          FOR_EACH(pr, procBody) {
            procName = pr->first;
            procConfig = pr->second;
            hostName = procConfig("host").getString();
            // Find the qualified name of the host this process is defined on
            curMachName = machName;
            // If the host this process is defined for isn't the one who sent us the config,
            //   try to find it and add it to the appropriate daemon if it's started up.
            // Also, ensure that the process that sent us the data is a daemon; if not, look
            //   for one.
            if(MR_Comm::extract_type_from_module_name(machName) != "d"
               || machName.find(hostName) >= machName.length()) {
              curMachName = (char *)"";
              FOR_GLIST(walker, machList) {
                if(walker->data != NULL) {
                  // Note that for daemon nodes, "procName" is the daemon name,
                  //  since really "procName" corresponds to the text in the
                  //  first column of the processTree
                  tmpMachName = ((ProcessDataAug*) walker->data)->procName;
                  if(tmpMachName.find(hostName) < tmpMachName.length()) {
                    curMachName = tmpMachName;
                    //cerr << "resp: config: " << procName << " on " << curMachName << endl;
                    break;
                  }
                }
              }
            } else {
              //cerr << "resp: config: " << procName << " on " << curMachName << endl;
            }
            
            // If the daemon this is defined on hasn't connected yet (isn't in
            //  the processTree yet), don't add the process to the display
            if(curMachName.length() > 0) {
              // Get current status; empty string if it's not there.
              status = get_status_from_processtree(procName.c_str(), curMachName.c_str());
              // If we don't have status, we haven't done anything yet... say we're not started
              if(status.length() <= 0) {
                status = "unknown";
              }
              // Add the sucker to the display
              update_local_process(curMachName.c_str(), procName.c_str(),
                                   status.c_str());
              // This just pops up config handlers
              handle_proc_config(curMachName, procName, procConfig);
            }
          }
        }
        // e[1]("groups") is a hash of group name => elements
        if(e[1]("groups").defined()) {
          // Incorporate into our view of reality
          if(is_prior) {
            rcl::exp mygroups = local_config_g("groups");
            local_config_g("groups") = e[1]("groups");
            if(mygroups.defined()) {
              local_config_g("groups").addFrom(mygroups);
            }
          } else {
            rcl::exp theirgroups = e[1]("groups");
            if(local_config_g("groups").defined())
              local_config_g("groups").addFrom(theirgroups);
            else
              local_config_g("groups") = theirgroups;
          }
          // Add to display; loop through whole thing to update display
	  rcl::map grpBody = local_config_g("groups").getMap();
          FOR_EACH(pr, grpBody) {
            string grpName;
            rcl::exp grpList;
            
            grpName = pr->first;
            grpList = pr->second;
            // Add it to the display if necessary / update display
            handle_group_config(grpName, grpList);
          }
        }
      } else {	
        // Figure out if this is a prior file or not
        bool is_prior = (e[3].defined() && e[3].getString() == "prior");
	string procName = e[1].getString();
	rcl::exp procConfig = e[2];
        if(!is_prior
           || (is_prior && !local_config_g("processes")(procName).defined())) {
          local_config_g("processes")(procName) = procConfig;
        }
	handle_proc_config(machName, procName, procConfig);
      }
      toBuffer = (char *)"";
    } /* End config message */

  } catch(rcl::exception err) {
    cerr << "Caught RCL exception in response_handler: " << err.text << endl;
    toBuffer = "RCL Exception: ";
    toBuffer += err.text;
    toBuffer += "\nProblematic line: ";
    toBuffer += text;
    fore = ERROR_COL;
  }

  /* Write it to the little mini-buffer we have at the bottom of the gui */
  if(toBuffer.length() > 0) {
    print_to_common_buffer(toBuffer.c_str(), fore);
  }

} /* End line_handler */

void flush_all_term_buffers(void)
{
  DEBUGCB("flush_all_term_buffers\n");
  GtkCTree* processTree = GTK_CTREE(lookup_widget(HeadDino_g, "ProcessTree"));
  GList* pdaList = get_all_data(processTree, PROCESS_NODE);
  ProcessDataAug* pda;

  if (NULL != pdaList)
  {
    FOR_GLIST(walker, pdaList) 
    {
      pda = (ProcessDataAug*) walker->data;
      if ( NULL != pda && NULL != pda->pw && 
           NULL != PROCESSWIDGET(pda->pw)->LogText ) 
      {
	processWidget_term_flush( PROCESSWIDGET(pda->pw) );
      }
    }
    free_process_data_list(pdaList, true);
  }
}

// this version of response_handler splits multi-line buffered input and feeds
// lines to line_handler
void response_handler
(const string& machName, const string& text, void *client_data)
{
  DEBUGCB("response_handler\n");
  int n;
  istringstream tstream(text);
  char* lbuf = new char[text.size()+1];

  while (!tstream.fail()) {
    tstream.getline( lbuf, text.size()+1, '\n' );
    n = strlen(lbuf);

    // ignore blank line
    if (0 == n) continue;
    // chop trailing newline
    if ('\n' == lbuf[n-1]) lbuf[n-1] = '\0';
    
    line_handler( machName, lbuf, client_data );
  }

  // cleanup
  delete[] lbuf;
  flush_all_term_buffers();
}

/*********************************************************************
 ************************** END MRCOMM CALLBACKS *********************
 *********************************************************************/

/*********************************************************************
 ************************ BEGIN INTERFACE CALLBACKS ******************
 *********************************************************************/

void on_HeadDino_destroy
(GtkObject *object, gpointer user_data)
{
  DEBUGCB("on_HeadDino_destroy\n");
  claw_quit();
}

/* Sigh.  This is a bit of a hack.  We want to trap X bad-window
 * errors and warn the user they should probably ssh -Y, but I can't
 * find a more appropriate entry-point into the menu activation
 * sequence.  Since multiple button presses may occur prior to
 * selection_done, we need to keep track. */

static int errorTrapCount = 0;
void add_error_trap ()
{
  DEBUGCB("add_error_trap\n");
  gdk_error_trap_push();
  errorTrapCount++;
}

void clear_error_traps()
{
  DEBUGCB("clear_error_traps\n");
  int error;
  while(errorTrapCount > 0) 
  {
    error = gdk_error_trap_pop();
    if(error == BadWindow || error == BadAccess) {
      printf("\n");
      printf("WARNING: I'm getting an X error that may be due to ssh -X'ing\n");
      printf("         into an X server with the security extension.  Claw\n");
      printf("         will *attempt* to work around, but the Microraptor team\n");
      printf("         strongly suggests the use of 'ssh -Y' instead.  Strange\n");
      printf("         placement of context (right-click) menus is an effect\n");
      printf("         of accessing an X server in untrusted mode, and is\n");
      printf("         resolved by using ssh -Y.  See the FAQ at\n");
      printf("         http://www.microraptor.org.\n");
      printf("\n");
    } else if(error != 0) {
      printf("ERROR: Received X error %d.  Et tu, Brutex?\n", error);
      exit(-1);
    }
    errorTrapCount--;
  }
}

bool on_MainMenuBar_button_press
(GtkMenuShell *menushell, gboolean force_hide, gpointer user_data)
{
  DEBUGCB("on_MainMenuBar_button_press\n");
  add_error_trap();
  return false;
}

void on_MainMenuBar_selection_done
(GtkMenuShell *menushell, gboolean force_hide, gpointer user_data)
{
  DEBUGCB("on_MainMenuBar_selection_done\n");
  clear_error_traps();
}

void on_get_config_file1_activate
(GtkMenuItem *menuitem, gpointer user_data)
{
  DEBUGCB("on_get_config_file1_activate\n");
  GtkWidget* lfd = create_loadFileDialog(); 
  gtk_window_set_position(GTK_WINDOW(lfd), GTK_WIN_POS_MOUSE);
  gtk_widget_show(lfd);
  add_taskbar_icon (lfd, miniClawImage_xpm);
}

void on_get_local_config_file1_activate
(GtkMenuItem *menuitem, gpointer user_data)
{
  DEBUGCB("on_get_local_config_file1_activate\n");
  GtkWidget* lfs = create_localFileSelection();
  gtk_window_set_position(GTK_WINDOW(lfs), GTK_WIN_POS_MOUSE);
  gtk_widget_show(lfs);
  add_taskbar_icon (lfs, miniClawImage_xpm);
}

void on_reload_local_config_file1_activate
(GtkMenuItem *menuitem, gpointer user_data)
{
  DEBUGCB("on_reload_local_config_file1_activate\n");
  if(!(local_config_g.defined()))
  {
    cerr << "Whups, not gonna reload a local config - you never loaded one!" << endl;
    print_to_common_buffer("Whups, not gonna reload a local config - you never loaded one!", WARNING_COL);
    return;
  }
  print_to_common_buffer("Reloading last local config file from disk.", OK_COL);
  local_config_g = load_local_config(local_config_name_g);
  send_local_config(local_config_g);
}

void on_preferences_changed
(rcl::exp e)
{
  DEBUGCB("on_preferences_changed\n");
  //cout << "on_preferences_changed called" << endl;
  GtkCTree* processTree = GTK_CTREE(lookup_widget(HeadDino_g, "ProcessTree"));
  // Show/hide cool graphics if that changed
  if(parse_bool_field("cool_graphics", e) != parse_bool_field("cool_graphics"))
  {
    // Reset to new value; set_gratuitous_image checks prefs_g
    prefs_g[0]("cool_graphics") = e[0]("cool_graphics");
    // Turn 'em on
    if(parse_bool_field("cool_graphics", e))
    {
      gtk_widget_show(lookup_widget(HeadDino_g, "RaptorImage"));
    }
    // Hide 'em
    else {
      gtk_widget_hide(lookup_widget(HeadDino_g, "RaptorImage"));
    }
  }
  // Resort the tree if we changed our view_mode
  if(prefs_g[0]("view_mode").getString() != e[0]("view_mode").getString()) {
    prefs_g = e;
    gtk_ctree_sort_node(processTree, NULL);
    // Expand / collapse things appropriately
    GList* nodeList = get_all_nodes(processTree, PROCESS_TOP_NODE);
    FOR_GLIST(walker, nodeList) {
      if(prefs_get_view_mode() == GROUPS_FIRST) {
        // Switched to groups; collapse daemons
        gtk_ctree_collapse(processTree, (GtkCTreeNode*) (walker->data));
      } else { // Default to daemons first
        // Switched to daemons; expand 'em
        gtk_ctree_expand(processTree, (GtkCTreeNode*) (walker->data));        
      }
    }
    // BUGCHECK: MEMLEAK: Is this one with the glist from above?
    nodeList = get_all_nodes(processTree, GROUP_NODE);
    FOR_GLIST(walker, nodeList) {
      if(prefs_get_view_mode() == GROUPS_FIRST) {
        // Switched to groups; expand 'em
        gtk_ctree_expand(processTree, (GtkCTreeNode*) (walker->data));
      } else { // Default to daemons first
        // Switched to daemons; collapse groups
        gtk_ctree_collapse(processTree, (GtkCTreeNode*) (walker->data));        
      }
    }
  }
  // Show/hide the filter box if we changed filtering
  if(prefs_g[0]("stdout_filtering").getLong() != e[0]("stdout_filtering").getLong()) {
    //cout << "Filtering option has changed." << endl;
    GList* nodeList = get_all_nodes(processTree, PROCESS_NODE);
    ProcessData tmpPd;
    GtkWidget* pw;
    init_process_data(&tmpPd);
    FOR_GLIST(walker, nodeList) {
      fill_process_data_from_treenode(&tmpPd, (GtkCTreeNode*) (walker->data), processTree);
      //cout << "Examining: " << flush;
      //print_process_data(&tmpPd);
      //cout << endl;
      if(tmpPd.procName != NULL && tmpPd.machName != NULL) {
        //cout << "  Proc and mach are nonnull." << endl;
        pw = get_process_widget(tmpPd.procName, tmpPd.machName);
        if(pw != NULL) {
          //cout << "  pw is nonnull" << endl;
          if(e[0]("stdout_filtering").getLong()) {
            //cout << "Showing widgets" << endl;
            processWidget_show_filter(PROCESSWIDGET(pw));
          } else {
            //cout << "  Hiding widgets" << endl;
            processWidget_hide_filter(PROCESSWIDGET(pw));
          }
        } else {
          //cout << "  pw is null" << endl;
        }
      }
    }
  }

  // Store prefs
  prefs_g = e;
  // Update menu checkedness
  GtkWidget* prefsMenu = lookup_widget(HeadDino_g, "preferences_menu");
  GList* prefsChildren = gtk_container_children (GTK_CONTAINER (prefsMenu));
  string checkPrefix = "check_menu_item_";

  GList* walker = NULL;
  gpointer udata = NULL;
  for(walker = g_list_first(prefsChildren);
      walker;
      walker = g_list_next(walker)) {
    if(walker->data == NULL) continue;
    // Pretty sure walker->data is a GtkWidget
    udata = gtk_object_get_user_data (GTK_OBJECT (walker->data));
    if(udata != NULL
       && strncmp(checkPrefix.c_str(),
                  (const char*) udata,
                  checkPrefix.size()) == 0) {
      const char* key = ((const char*) udata) + checkPrefix.size();
      gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (walker->data),
                                      prefs_g[0](key).getLong());
    }
  }
}

void on_checkbox_prefs_activate
(GtkCheckMenuItem *menuitem, gpointer user_data)
{
  DEBUGCB("on_checkbox_prefs_activate\n");
  string key = (const char*) user_data;
  long int val = menuitem->active;
  //cout << "on_checkbox_prefs_activate: " << key << " = " << val << " (was " << prefs_g[0](key) << ")" << endl
  //     << "  prefs_g is: " << endl;
  //prefs_g.writeConfig(cout, 1);

  rcl::exp e = prefs_g.copy();
  e[0](key) = val;
  // This is a bit repetitive, since it will also update the menu
  // checkboxes, but it handles updating any widgets and such affected
  // by the changed preference, as well as updating prefs_g.
  //cout << "Calling on_preferences_changed, with e = " << endl;
  //e.writeConfig(cout, 1);
  //cout << endl << "And prefs_g = " << endl;
  //prefs_g.writeConfig(cout, 1);
  on_preferences_changed(e);
  //cout << "Done calling opc; returning from on_checkbox_prefs_activate." << endl;
}

void on_set_prefs_activate
(GtkMenuItem *menuitem, gpointer user_data)
{
  DEBUGCB("on_set_prefs_activate\n");
  // Launch prefs dialog
  GtkWidget* pd = create_editHashDialog("Set Local Preferences",
                                        prefs_g[0],
                                        prefsDefault_g[0],
                                        on_preferences_changed);
  if(pd == NULL) {
    cerr << "ERROR: problem creating editHashDialog!" << endl;
    return;
  }
  /* Show the sucker */
  gtk_window_set_position(GTK_WINDOW(pd), GTK_WIN_POS_MOUSE);
  gtk_widget_show(pd);
  add_taskbar_icon (pd, miniClawImage_xpm);
}

void on_save_prefs_activate
(GtkMenuItem *menuitem, gpointer user_data)
{
  DEBUGCB("on_save_prefs_activate\n");
  // Launch pick file dialog
  GtkWidget* spfs = create_savePrefsFileSelection();
  string prefFile = get_prefs_file_with_default();
  gtk_file_selection_set_filename(GTK_FILE_SELECTION(spfs), prefFile.c_str());
  gtk_window_set_position(GTK_WINDOW(spfs), GTK_WIN_POS_MOUSE);
  gtk_widget_show(spfs);
  add_taskbar_icon (spfs, miniClawImage_xpm);
}

void on_load_prefs_activate
(GtkMenuItem *menuitem, gpointer user_data)
{
  DEBUGCB("on_load_prefs_activate\n");
  // Launch pick file dialog
  GtkWidget* lpfs = create_loadPrefsFileSelection();
  string prefFile = get_prefs_file_with_default();
  gtk_file_selection_set_filename(GTK_FILE_SELECTION(lpfs), prefFile.c_str());
  gtk_window_set_position(GTK_WINDOW(lpfs), GTK_WIN_POS_MOUSE);
  gtk_widget_show(lpfs);
  add_taskbar_icon (lpfs, miniClawImage_xpm);
}

void on_set_process1_activate
(GtkMenuItem *menuitem, gpointer user_data)
{
  DEBUGCB("on_set_process1_activate\n");
  GtkCTree *processTree = GTK_CTREE(lookup_widget(HeadDino_g, "ProcessTree"));
  GList* selection = get_multiple_selected_data(processTree,
                                                NULL, 
                                                (enum nodeType) (PROCESS_NODE | GROUP_NODE));
  GList* walker = NULL;
  ProcessData *pd;

  for(walker = g_list_first(selection);
      walker;
      walker = g_list_next(walker)) {
    if(walker->data == NULL) continue;
    pd = (ProcessData*) walker->data;
    if(get_nodetype(pd) == PROCESS_NODE) {
      if(pd->machName == NULL || pd->procName == NULL) continue;
      // Add this process/daemon to the list of things that should fire
      //   set_config dialogs on receipt of a config message from the
      //   daemon
      pending_sets_g.push_back(process_data_to_label(pd));
      //cerr << "Adding \"" << process_data_to_label(pd) 
      //     << "\" to set_config callback list." << endl;
      // Get the config of the process
      get_config_process(pd->machName, pd->procName);    
    } else if(get_nodetype(pd) == GROUP_NODE) {
      if(pd->procName != NULL && pd->status != NULL) {
        // FILLMEIN
        print_to_common_buffer("Sorry, no mechanism for runtime setting of groups exists.  Modify and reload the config file",
                               SPECIAL_COL);
      }
    } else {
      print_to_common_buffer("ERROR: get_multiple_selected_data lied to us!  My pysche is damaged forever!",
                             ERROR_COL);
    }
  }

  // Sigh... if we do a full free'ing, Bad Stuff happens...
  free_process_data_list(selection, false);

  return;
}

/* This handler is enabled when we are checking
   to see if the -id the user specified is valid. */

/* ARGSUSED */
int
bad_window_handler(Display *disp, XErrorEvent *err)
{
    DEBUGCB("bad_window_handler\n");
    char badid[20];

    sprintf(badid, "0x%lx", err->resourceid);
    cerr << "No such window with id " << badid << "." << endl;
    return 0;
}

/*
 * Routine to let user select a window using the mouse
 */

Window Select_Window(Display *dpy)
{
  DEBUGCB("Select_Window\n");
  int status;
  Cursor cursor;
  XEvent event;
  int screen = DefaultScreen(dpy);
  Window target_win = None, root = RootWindow(dpy,screen);
  int buttons = 0;

  /* Make the target cursor */
  cursor = XCreateFontCursor(dpy, XC_crosshair);

  /* Grab the pointer using target cursor, letting it room all over */
  status = XGrabPointer(dpy, root, False,
			ButtonPressMask|ButtonReleaseMask, GrabModeSync,
			GrabModeAsync, root, cursor, CurrentTime);
  if (status != GrabSuccess) {
    print_to_common_buffer("ERROR: Can't grab the mouse.", ERROR_COL);
  }

  /* Let the user select a window... */
  while ((target_win == None) || (buttons != 0)) {
    /* allow one more event */
    XAllowEvents(dpy, SyncPointer, CurrentTime);
    XWindowEvent(dpy, root, ButtonPressMask|ButtonReleaseMask, &event);
    switch (event.type) {
    case ButtonPress:
      if (target_win == None) {
	target_win = event.xbutton.subwindow; /* window selected */
	if (target_win == None) target_win = root;
      }
      buttons++;
      break;
    case ButtonRelease:
      if (buttons > 0) /* there may have been some down before we started */
	buttons--;
       break;
    }
  } 

  XUngrabPointer(dpy, CurrentTime);      /* Done with pointer */

  return(target_win);
}

void
on_swallow_window1_activate
(GtkMenuItem *menuitem, gpointer user_data)
{
  DEBUGCB("on_swallow_window1_activate\n");
  Window window;
  Display *dpy;
  char* winName;

  print_to_common_buffer("Please choose a window to swallow...  Mmm, windows.", SPECIAL_COL);

  dpy = XOpenDisplay(gdk_get_display());

  // Update the GUI display so it doesn't look funny and the above message displays
  gtk_widget_draw(HeadDino_g, NULL);
  // This runs the gtk mainloop once, without blocking
  gtk_main_iteration_do(FALSE);

  window = Select_Window(dpy);
  if (window) {
    Window root;
    int dummyi;
    unsigned int dummy;
    
    if (XGetGeometry (dpy, window, &root, &dummyi, &dummyi,
                      &dummy, &dummy, &dummy, &dummy) &&
        window != root)
      window = XmuClientWindow (dpy, window);
  }
  /*
   * make sure that the window is valid
   * We're not doing this (it causes a delay)... will this bite us?
  {
    Window root;
    int x, y;
    unsigned width, height, bw, depth;
    XErrorHandler old_handler;

    old_handler = XSetErrorHandler(bad_window_handler);
    XGetGeometry (dpy, window, &root, &x, &y, &width, &height, &bw, &depth);
    XSync (dpy, False);
    (void) XSetErrorHandler(old_handler);
  }
   */

  // "window" can be printed as a digit %ld
  // If we don't have a name, don't swallow
  if(!XFetchName(dpy, window, &winName)) {
    string errStr = "ERROR: cannot find selected window.";
    XCloseDisplay(dpy);
    print_to_common_buffer(errStr.c_str(), ERROR_COL);
    return;
  }

  // Do the actual swallowing
  swallow_window(winName, window);

  XCloseDisplay(dpy);
}

void on_swallowed_window_throwup_button_clicked 
(GtkWidget *button, gpointer user_data)
{
  DEBUGCB("on_swallowed_window_throwup_button_clicked\n");
  GtkNotebook   *nb;
  GtkWidget *scroller;
  string winName;

  scroller = GTK_WIDGET(gtk_widget_get_ancestor(GTK_WIDGET(button), 
                                                gtk_type_from_name("GtkScrolledWindow")));
  nb = GTK_NOTEBOOK(lookup_widget(HeadDino_g, "notebook1"));

  // Get the swallowed window's name
  winName = (char*) gtk_object_get_user_data(GTK_OBJECT(scroller));

  // Remove from processTree
  remove_swallowed_from_tree(winName);

  GtkWidget* sock;
  string sockName = "socket";
  sockName += winName;
  
  sock = lookup_widget(scroller, sockName.c_str());

  // Reparent the child window to the root window
  cerr << "Reparenting swallowed window " << winName << " to root..." << endl;
  gdk_window_reparent(GTK_SOCKET(sock)->plug_window, GDK_ROOT_PARENT(), 0, 0);
  cerr << "\t...done." << endl;

  // Removing tab & children
  gtk_notebook_remove_page(nb, gtk_notebook_page_num(nb, scroller));
}

void
on_swallowed_window_kill_button_clicked
(GtkWidget *button, gpointer user_data)
{
  DEBUGCB("on_swallowed_window_kill_button_clicked\n");
  GtkNotebook   *nb;
  GtkWidget *scroller;
  string winName;

  scroller = GTK_WIDGET(gtk_widget_get_ancestor(GTK_WIDGET(button), gtk_type_from_name("GtkScrolledWindow")));
  nb = GTK_NOTEBOOK(lookup_widget(HeadDino_g, "notebook1"));

  // Get the swallowed window's name
  winName = (char*) gtk_object_get_user_data(GTK_OBJECT(scroller));

  // Remove from processTree
  remove_swallowed_from_tree(winName);

  // Kill the swallowed window while removing tab & children
  // I /think/ this kills the child; at least, the process seems to
  //  have gone away.
  gtk_notebook_remove_page(nb, gtk_notebook_page_num(nb, scroller));
}


void on_kill_daemon_activate
(GtkMenuItem *menuitem, gpointer user_data)
{
  DEBUGCB("on_kill_daemon_activate\n");
  GtkWidget* ptd;
  ptd = create_pickTargetDialog();
  gtk_window_set_position(GTK_WINDOW(ptd), GTK_WIN_POS_MOUSE);
  if(ptd != NULL) {
    gtk_widget_show(ptd);
    add_taskbar_icon (ptd, miniClawImage_xpm);
  }
}

void on_exit2_activate
(GtkMenuItem *menuitem, gpointer user_data)
{
  DEBUGCB("on_exit2_activate\n");
  claw_quit();
}


void on_processes1_activate
(GtkMenuItem *menuitem, gpointer user_data)
{

}

void
on_signals1_activate                   (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{

}

void
on_display_selected1_activate          (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{

}


void
on_send_signal1_activate               (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{

}


void
on_send_ipc_message1_activate          (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{

}


void
on_get_output1_activate                (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{

}


#define NOEXEC_RETVAL (142)

static gint wait_timeout_handler(gpointer data)
{
  // Make sure child processes aren't zombified.  For now, this is
  // just the help browser.  If we get a specific return value, we
  // failed to exec the process.  Don't block.
  
  DEBUGCB("wait_timeout_handler\n");
  int status = 0;
  pid_t pid;
  pid = waitpid(-1, &status, WNOHANG);
  if(pid > 0 && WIFEXITED(status)) {
    if(WEXITSTATUS(status) == NOEXEC_RETVAL) {
      // Help browser failed to launch.
      string msg = "Could not launch specified help browser:\n\"";
      msg += (char*) data; // To avoid ambiguous overload error
      msg += "\"\nCheck your 'Help browser' preference setting.\n";
      createAndShow_messageDialog(msg, true);
    }
    free((char*) data);
    printf("Sucessfully waitpid()'d on process %d.\n", pid);
    return FALSE; // Remove the handler: we have one timeout per child process
  } 
  return TRUE;
}

void on_launch_browser_activate
(GtkMenuItem *menuitem, gpointer user_data)
{
  DEBUGCB("on_launch_browser_activate\n");

  // Hardcoding location of help files.  Eh.
  string helpRoot = "/usr/share/doc/mraptor/index.html";
  string browser = prefs_help_browser();
  size_t urlLoc = browser.find("%s");
  if(urlLoc != string::npos) {
    browser.replace(urlLoc, 2, helpRoot);
  } else {
    browser += " ";
    browser += helpRoot;
  }

  // Build the argument list
  printf("Launching help browser with command: '%s'\n", browser.c_str());
  // Split up command into arguments.  Sigh.  Seems like there should
  // be a better way to do this.
  string cmd;
  vector<string> args;
  size_t first, second;
  first = browser.find_first_not_of(" \t\n\r");
  second = browser.find_first_of(" \t\n\r", first+1);
  cmd = browser.substr(first, second-first);
  while(first <= browser.size() && first != string::npos) {
    first = browser.find_first_not_of(" \t\n\r", second);
    second = browser.find_first_of(" \t\n\r", first+1);
    if(first <= browser.size() && first != string::npos) {
      args.push_back(browser.substr(first, second-first));
    }
  }
  
  /*
  printf("Executable: '%s'\n", cmd.c_str());
  for(vector<string>::iterator ait = args.begin(); ait != args.end(); ait++) {
    printf("  Arg: '%s'\n", (*ait).c_str());
  }
  */
  
  char** browser_argv = (char**) malloc(sizeof(char*)*(args.size() + 2));
  int i;
  vector<string>::iterator ait;
  // Need to have the name of the binary as arg0
  browser_argv[0] = strdup(cmd.c_str());
  for(ait = args.begin(), i = 1; ait != args.end(); ait++, i++) {
    browser_argv[i] = strdup((*ait).c_str());
  }
  browser_argv[i] = NULL;

  // Fork and exec the sucker
  pid_t pid;
  switch(pid=fork()){
  case -1:
  {
    printf("Unable to fork help browser!\n");
    string msg = "Unable to fork, prepatory to\nlaunching help browser!  Kaboom!";
    createAndShow_messageDialog(msg, true);
    break;
  }
  case 0:
    // I'm pretty sure the malloc'd memory goes away on the exec.
    /*
    printf("Child exec'ing browser.  Commandline:\n");
    printf("Cmd: '%s'\n", cmd.c_str());
    for(int i = 0; browser_argv[i] != NULL; i++) {
      printf("  Arg: '%s'\n", browser_argv[i]);
    }
    */
    execvp(cmd.c_str(), browser_argv);
    // If we get here, exec failed.  Return the specified value, and let claw pop up a warning.
    // FIXME: This crashes claw!
    printf("Exec of help browser FAILED!\n");
    // Regular exit() calls gtk cleanup stuff that fails miserably.
    _exit(NOEXEC_RETVAL);
  default:
    //printf("Parent done forking.  Cleaning up argument memory.\n");
    for(unsigned int i = 0; i < args.size(); i++) {
      free(browser_argv[i]);
      browser_argv[i] = NULL;
    }
    free(browser_argv);
    browser_argv = NULL;
    // The handler will wait for the child to exit, and report any
    // failure to exec.
    //printf("Adding wait handler (every half second).");
    // wait_timeout_handler will free the string.
    gtk_timeout_add(500, wait_timeout_handler, strdup(browser.c_str()));
  }
}

void on_about_activate
(GtkMenuItem *menuitem, gpointer user_data)
{
  DEBUGCB("on_about_activate\n");
  // This instance also shows the dialog.
  GtkWidget* about = create_aboutDialog();
  add_error_trap();
  gtk_window_set_position (GTK_WINDOW (about), GTK_WIN_POS_CENTER);
  gtk_widget_show (about);
  add_taskbar_icon (about, miniClawImage_xpm);
  clear_error_traps();
}


void on_subscribe_to_data_stream1_activate
(GtkMenuItem *menuitem, gpointer user_data)
{

}


gboolean on_aboutDialog_button_release_event
(GtkWidget *widget, GdkEventButton *event, gpointer user_data)
{
  DEBUGCB("on_aboutDialog_button_release_event\n");
  //printf("About: got button release event.  Type: %d (BP = %d, 2BP = %d), button: %d\n", event->type, GDK_BUTTON_PRESS, GDK_2BUTTON_PRESS, event->button);

  /* If this was a left-click, nuke the dialog.  Otherwise, noop. */
  if ( event->type == GDK_BUTTON_RELEASE
       && event->button == 1) {
    /* Destroy the dialog */
    GtkWidget* topWid = gtk_widget_get_toplevel(GTK_WIDGET(widget));
    gtk_widget_destroy(topWid);
  }
  return FALSE;
}

gboolean on_aboutDialog_key_release_event
(GtkWidget *widget, GdkEventKey *event, gpointer user_data)
{
  DEBUGCB("on_aboutDialog_key_release_event\n");
  //printf("About: got key release event.\n");
  /* Nuke the dialog on [enter] or [escape] */
  if(event != NULL
     && event->type == GDK_KEY_PRESS
     && (event->keyval == GDK_KP_Enter
         || event->keyval == GDK_ISO_Enter
         || event->keyval == GDK_KEY_Return
         || event->keyval == GDK_KEY_Escape)) {
    /* Destroy the dialog */
    GtkWidget* topWid = gtk_widget_get_toplevel(GTK_WIDGET(widget));
    gtk_widget_destroy(topWid);
  }
  return FALSE;
}

void on_contextDialog_okButton_clicked
(GtkButton *button, gpointer user_data)
{
  DEBUGCB("on_contextDialog_okButton_clicked\n");
  ProcessWidget* pw = (ProcessWidget*) user_data;
  GtkWidget* topWid = gtk_widget_get_toplevel(GTK_WIDGET(button));
  GtkWidget* preSpin = lookup_widget(topWid, "preSpin");
  GtkWidget* postSpin = lookup_widget(topWid, "postSpin");
  unsigned int oldPre = pw->regexpPreContext;
  unsigned int oldPost = pw->regexpPostContext;
  pw->regexpPreContext = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (preSpin));
  pw->regexpPostContext = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (postSpin));
  //cerr << "Updating context values: pre = " << pw->regexpPreContext
  //     << "; post = " << pw->regexpPostContext << endl;
  // If the context changed and the filter is active, refilter the buffer.
  if( (oldPre != pw->regexpPreContext || oldPost != pw->regexpPostContext)
      && pw->regexpActive ) {
    processWidget_regexp_refresh_buffer(pw);
  }
  gtk_widget_destroy(topWid);  
}

void on_contextDialog_cancelButton_clicked
(GtkButton *button, gpointer user_data)
{
  DEBUGCB("on_contextDialog_cancelButton_clicked\n");
  /* Destroy the dialog */
  GtkWidget* topWid = gtk_widget_get_toplevel(GTK_WIDGET(button));
  gtk_widget_destroy(topWid);  
}


gboolean on_ProcessTree_button_release_event
(GtkWidget *widget, GdkEventButton *event, gpointer user_data)
{
  DEBUGCB("on_ProcessTree_button_release_event\n");
  return FALSE;
}


gboolean on_ProcessTree_key_press_event
(GtkWidget *widget, GdkEventKey *event, gpointer user_data)
{
  DEBUGCB("on_ProcessTree_key_press_event\n");
  if(event != NULL && event->type == GDK_KEY_PRESS)
  {
    if(event->keyval == GDK_KP_Enter
       || event->keyval == GDK_ISO_Enter
       || event->keyval == GDK_KEY_Return) {
      on_view_selected1_activate(GTK_MENU_ITEM(lookup_widget(HeadDino_g, "view_selected1")),
                                 lookup_widget(HeadDino_g, "ProcessTree"));
      return TRUE;
    }
  }
  return FALSE;
}


gboolean on_HeadDino_key_press_event
(GtkWidget *widget, GdkEventKey *event, gpointer user_data)
{
  DEBUGCB("on_HeadDino_key_press_event\n");
  if(event != NULL && event->type == GDK_KEY_PRESS) {
    fprintf(stderr, "Got key: 0x%X\n", event->keyval);
    /*
    if(event->state & GDK_MOD1_MASK) cerr << "mod1" << endl;
    if(event->state & GDK_MOD2_MASK) cerr << "mod2" << endl;
    if(event->state & GDK_MOD3_MASK) cerr << "mod3" << endl;
    if(event->state & GDK_MOD4_MASK) cerr << "mod4" << endl;
    if(event->state & GDK_MOD5_MASK) cerr << "mod5" << endl;
    if(event->state & GDK_SHIFT_MASK) cerr << "shift" << endl;
    if(event->state & GDK_CONTROL_MASK) cerr << "ctrl" << endl;
    if(event->state & GDK_LOCK_MASK) cerr << "lock" << endl;
    */

    // This is really really weird... ctrl+shift+tab results in
    //   a ctrl+shift+ISO_Left_Tab keysym, while ctrl+tab gives us
    //   a regular ol' tab...  <sigh>

    if(event->keyval == GDK_KEY_Tab
       || event->keyval == GDK_KEY_ISO_Left_Tab) {
      //cerr << "It's a tab" << endl;

      if(event->state & GDK_CONTROL_MASK
         && event->state & GDK_SHIFT_MASK) {
        // Switch page
        gtk_notebook_prev_page(GTK_NOTEBOOK(user_data));
        // Don't allow any more processing of this event
        gtk_signal_emit_stop_by_name(GTK_OBJECT(widget), "key_press_event");
        // Focus is dealt with in the notebook_page_switched handler
        return TRUE;
      } else if(event->state & GDK_CONTROL_MASK) {
        //cerr << "next" << endl;
        gtk_notebook_next_page(GTK_NOTEBOOK(user_data));
        // Don't allow any more processing of this event
        gtk_signal_emit_stop_by_name(GTK_OBJECT(widget), "key_press_event");
        // Focus is dealt with in the notebook_page_switched handler
        return TRUE;
      }
    }
  }
  
  return FALSE;
}
gboolean on_stdinEntry_key_release_event
(GtkWidget *widget, GdkEventKey *event, gpointer user_data)
{
  DEBUGCB("on_stdinEntry_key_release_event\n");
  return FALSE;
}

/* If pdpp is a group member, replace it with the corresponding process
   node data.  
   Returns true if pdpp was a group member and was updated
   NOTE: THIS MALLOC'S MEMORY (for the ProcessData only, not its members)! */
bool filter_group_members(ProcessData** pdpp)
{
  DEBUGCB("filter_group_members\n");
  if(pdpp == NULL || *pdpp == NULL) return false;
  GtkCTree* processTree = GTK_CTREE(lookup_widget(HeadDino_g, "ProcessTree"));
  // If a group member node, transmute into a process node
  if(get_nodetype(*pdpp) == GROUP_MEMBER_NODE)
  {
    EntryDataAug* curRowData = (EntryDataAug*) gtk_ctree_node_get_row_data(processTree,
                                                                           get_node_from_process_data(*pdpp));
    *pdpp = (ProcessData*) malloc(sizeof(ProcessData));
    init_process_data(*pdpp);
    // Doesn't malloc
    if(!fill_process_data_from_treenode(*pdpp, curRowData->realNode, processTree))
    {
      print_to_common_buffer("Ooops! Problems filling process data in run selected...\n", ERROR_COL);
      return false;
    }
    return true;    
  } else {
    return false;
  }
}


void on_view_selected1_activate
(GtkMenuItem *menuitem, gpointer user_data)
{
  DEBUGCB("on_set_selected1_activate\n");
  GtkWidget *topWid;
  GtkCTree *processTree;
  GList    *nameList;
  GList    *walker;
  ProcessData *pd;
  int       numSelected;

  processTree = GTK_CTREE(user_data);
  topWid = gtk_widget_get_toplevel(GTK_WIDGET(processTree));

  nameList = get_multiple_selected_data(processTree, 
                                        &numSelected, 
                                        (enum nodeType) (PROCESS_NODE | GROUP_NODE | GROUP_MEMBER_NODE) );
  
  for(walker = g_list_first(nameList); walker; walker = g_list_next(walker)) {
    if(walker->data == NULL) continue;
    pd = (ProcessData*) walker->data;    
    if(get_nodetype(pd) == PROCESS_NODE) {
      /* Add the sucker, if we have a process name and a machine
         (if no machine, we tried to view the machine, not a process) */
      if(pd->procName != NULL && pd->machName != NULL) {
        if(pd->status == NULL) pd->status = (char *)"";
        if(add_process_display(GTK_WIDGET(topWid), 
                               pd->machName, pd->procName, pd->status)) {
          /* Update button status, switch to it _if_ we haven't viewed it before */
          //cerr << "calling pwbs: view selected" << endl;
          processWidget_button_showandtell(get_process_widget(pd->procName, pd->machName), pd);
          switch_to_tab(pd);
        } else if(numSelected == 1) {
          /* Switch to this process' tab */        
          switch_to_tab(pd);
        }
      }
    } else if(get_nodetype(pd) == GROUP_NODE) {
      if(pd->procName != NULL && pd->status != NULL) {
        add_group_display(get_node_from_procname(pd->procName));
        switch_to_tab(string(pd->procName));
      }
    } else if(get_nodetype(pd) == GROUP_MEMBER_NODE) {
      ProcessData pdMem;
      init_process_data(&pdMem);
      fill_process_data_from_treenode(&pdMem, 
                                      get_node_from_procname(pd->procName), 
                                      processTree);
      /* Add the sucker, if we have a process name and a machine
         (if no machine, we tried to view the machine, not a process) */
      if(pdMem.procName != NULL && pdMem.machName != NULL) {
        if(pdMem.status == NULL) pdMem.status = (char *)"";
        if(add_process_display(GTK_WIDGET(topWid), 
                               pdMem.machName, pdMem.procName, pdMem.status)) {
          // Sync up button status, switch to the viewed process' tab
          //cerr << "calling pwbs: view selected (2)" << endl;
          processWidget_button_showandtell(get_process_widget(pdMem.procName, pdMem.machName), &pdMem);
          switch_to_tab(&pdMem);
        } else if(numSelected == 1) {
          /* Switch to this process' tab */        
          switch_to_tab(&pdMem);
        }
      }
    } else {
      print_to_common_buffer("ERROR: get_multiple_selected_data lied to us!  My pysche is damaged forever!",
                             ERROR_COL);
    }
  } /* End walk through nameList */
  /* Free it all up */
  free_process_data_list(nameList, false);

  if(numSelected > 1) {
    /* Switch to overview tab */
    switch_to_tab(NULL);
  }

} /* End on_view_selected1_activate */

void view_all_helper 
(GtkCTree *ctree, GtkCTreeNode *node, gpointer data)
{
  DEBUGCB("view_all_helper\n");
  char *procName = NULL;
  char *status = NULL;
  char *machName = NULL;
  GtkWidget *topWid = gtk_widget_get_toplevel(GTK_WIDGET(ctree));
  ProcessData pd;
  init_process_data(&pd);

  get_names_from_node(GTK_CTREE(data), node, &procName, &status, &machName);
  fill_process_data(&pd, procName, machName, status);

  if(machName != NULL 
     && procName != NULL
     && get_nodetype(&pd) == PROCESS_NODE) {
    if(add_process_display(GTK_WIDGET(topWid), machName, procName, status)) {
      /* Implicit subscribe, if we have the requisite data 
         and are not currently viewing it.
       */
      sub_process(machName, procName, prefs_get_back_history_length());
      //cerr << "calling pwbs: view all helper" << endl;
      processWidget_button_showandtell(get_process_widget(procName, machName), &pd);
    }
  }
  // FIXME: MEMORYLEAK
  free_process_data_members(&pd);
}



void on_view_all1_activate
(GtkMenuItem *menuitem, gpointer user_data)
{
  DEBUGCB("on_view_all1_activate\n");
  GtkWidget *processTree = GTK_WIDGET(user_data);
  gtk_ctree_post_recursive(GTK_CTREE(user_data), NULL, view_all_helper, processTree);
  gtk_widget_show(processTree);
  switch_to_tab(NULL);
}


void on_hide_selected1_activate
(GtkMenuItem *menuitem, gpointer user_data)
{
  DEBUGCB("on_hide_selected1_activate\n");
  GtkWidget *topWid;
  GtkCTree *processTree;
  GList    *nameList;
  GList    *walker;
  ProcessData *pd;

  processTree = GTK_CTREE(user_data);
  topWid = gtk_widget_get_toplevel(GTK_WIDGET(processTree));

  nameList = get_multiple_selected_data(processTree,
                                        NULL,
                                        (enum nodeType) (GROUP_MEMBER_NODE | PROCESS_NODE | GROUP_NODE));
  
  for(walker = g_list_first(nameList); walker; walker = g_list_next(walker)) {
    if(walker->data == NULL) continue;
    pd = (ProcessData*) walker->data;
    // If pd is a group member, turns it into the associated process node
    filter_group_members(&pd);
    if(get_nodetype(pd) == PROCESS_NODE) {    
      /* Hide the sucker, if we have a process name and a machine
         (if no machine, we tried to hide the machine, not a process) */
      if(pd->procName != NULL && pd->machName != NULL) {
        if(pd->status == NULL) pd->status = (char *)"";
        //cerr << "Removing process display:\n\t";
        //print_process_data(pd);
        //cerr << endl;
        remove_process_display(GTK_WIDGET(topWid), pd->machName, pd->procName);
      }
    } else if(get_nodetype(pd) == GROUP_NODE) {
      if(pd->procName != NULL && pd->status != NULL) {
        //cerr << "Removing group display:\n\t";
        //print_process_data(pd);
        //cerr << endl;
        remove_group_display(pd->procName);
      }
    } else {
      print_to_common_buffer("ERROR: get_multiple_selected_data lied to us!  My pysche is damaged forever!",
                             ERROR_COL);
    }
  } /* End walk through nameList */
  /* Free it all up */
  free_process_data_list(nameList, false);
}


void
hide_all_helper
(GtkCTree *ctree, GtkCTreeNode *node, gpointer data)
{
  DEBUGCB("hide_all_helper\n");
  char *procName = NULL;
  char *status = NULL;
  char *machName = NULL;
  GtkWidget *topWid = gtk_widget_get_toplevel(GTK_WIDGET(ctree));
  
  get_names_from_node(GTK_CTREE(data), node, &procName, &status, &machName);

  if(machName != NULL 
     && procName != NULL
     && get_nodetype(procName, machName) == PROCESS_NODE) {
    remove_process_display(GTK_WIDGET(topWid), machName, procName);
  }
}


void on_hide_all1_activate
(GtkMenuItem *menuitem, gpointer user_data)
{
  DEBUGCB("on_hide_all1_activate\n");
  GtkWidget *processTree = GTK_WIDGET(user_data);
  gtk_ctree_post_recursive(GTK_CTREE(processTree), NULL, hide_all_helper, processTree);
  gtk_widget_show(processTree);
}

void on_pw_popup_reset_buffer_item_activate
(GtkMenuItem *menuitem, gpointer user_data)
{
  DEBUGCB("on_paw_popup_reset_buffer_item_activate\n");
  ProcessData *pd = popup_menu_info_g.second;
  if(pd == NULL) return;
  if(pd->machName == NULL && pd->procName != NULL) return;

  // Are we being viewed?
  GtkWidget* pw = get_process_widget(pd->procName, pd->machName);
  if(pw == NULL) return;

  //Added this here:
  #ifdef USE_ZVT_TERM
      zvt_term_reset (ZVT_TERM (PROCESSWIDGET (pw)->LogText), TRUE);
  #endif
}

void on_clear_cur_buffer_activate
(GtkMenuItem *menuitem, gpointer user_data)
{
  DEBUGCB("on_clear_cur_buffer_activate\n");
  // Find the label of the current tab
  GtkNotebook * nb;
  GtkWidget* tabLabel;
  GList* chilluns;
  SwitchableChildData* scd;
  int curPage;

  nb = GTK_NOTEBOOK(lookup_widget(HeadDino_g, "notebook1"));
  curPage = gtk_notebook_get_current_page(nb);
  tabLabel = gtk_notebook_get_tab_label(nb, gtk_notebook_get_nth_page(nb, curPage));

  // Now, loop through the children of this tab
  chilluns = g_list_first((GList*) gtk_object_get_user_data(GTK_OBJECT(tabLabel)));
  //int numChildren = g_list_length(chilluns);
  //cerr << "We have " << numChildren << " children." << endl;
  FOR_GLIST(walker, chilluns) {
    scd = (SwitchableChildData*) walker->data;

    if(scd != NULL && scd->switchedChild != NULL) {
      //cerr << "Resetting..." << endl;
        #ifdef USE_ZVT_TERM
          zvt_term_reset(ZVT_TERM(PROCESSWIDGET(scd->switchedChild)->LogText), TRUE);
        #endif
    } else if(scd == NULL) cerr << "scd null!" << endl;
    else if(scd->switchedChild == NULL) cerr << "sc null!" << endl;
  }
}


void on_switch_to_overview_activate
(GtkMenuItem *menuitem, gpointer user_data)
{
  DEBUGCB("on_switch_to_overview_activate\n");
  switch_to_tab("Overview");
}

void on_switch_to_next_tab_activate
(GtkMenuItem *menuitem, gpointer user_data)
{
  DEBUGCB("on_switch_to_next_tab_activate\n");
  gtk_notebook_next_page(GTK_NOTEBOOK(user_data));
}

void on_switch_to_prev_tab_activate
(GtkMenuItem *menuitem, gpointer user_data)
{
  DEBUGCB("on_switch_to_prev_tab_activate\n");
  gtk_notebook_prev_page(GTK_NOTEBOOK(user_data));
}

void on_run_selected1_activate
(GtkMenuItem *menuitem, gpointer user_data)
{
  DEBUGCB("on_selected1_activate\n");
  //GtkWidget *topWid;
  GtkCTree *processTree;
  GList    *nameList;
  GList    *walker;
  ProcessData *pd;
  set<string> pdSet;

  processTree = GTK_CTREE(user_data);
  //topWid = gtk_widget_get_toplevel(GTK_WIDGET(processTree));

  nameList = get_multiple_selected_data(processTree, 
                                        NULL, 
                                        (enum nodeType) (GROUP_MEMBER_NODE | PROCESS_NODE | GROUP_NODE));
  
  for(walker = g_list_first(nameList); walker; walker = g_list_next(walker)) {
    if(walker->data == NULL) continue;
    pd = (ProcessData*) walker->data;
    if(pdSet.count(string(pd->procName)) > 0) {
      continue;
    } else {
      pdSet.insert(string(pd->procName));
    }
    
    // If pd is a group member, turns it into the associated process node
    filter_group_members(&pd);
    if(get_nodetype(pd) == PROCESS_NODE) {
      /* Run the sucker, if we have a process name and a machine
         (if no machine, we tried to run the machine, not a process) */
      if(pd->procName != NULL && pd->machName != NULL) {
        if(prefs_get_clear_on_run()) {
          GtkCTreeNode* targetNode = get_node_from_process_data(pd);
          EntryData* targetEd = (EntryData*) gtk_ctree_node_get_row_data(processTree, targetNode);
          if(targetEd->pw != NULL) {
            #ifdef USE_ZVT_TERM
                zvt_term_reset(ZVT_TERM(PROCESSWIDGET(targetEd->pw)->LogText), TRUE);
            #endif
          }
        }
        run_process(pd->machName, pd->procName);
      }
    } else if(get_nodetype(pd) == GROUP_NODE) {
      if(pd->procName != NULL && pd->status != NULL) {
        string runstr = "Running group ";
        runstr += pd->procName;
        print_to_common_buffer(runstr, GOOD_COL);
        run_group(pd->procName);
      }
    } else {
      print_to_common_buffer("ERROR: get_multiple_selected_data lied to us!  My pysche is damaged forever!",
                             ERROR_COL);
    }
  } /* End walk through nameList */
  /* Free it all up */
  free_process_data_list(nameList, false);
}

void on_run_all_selected1_activate
(GtkMenuItem *menuitem, gpointer user_data)
{
  DEBUGCB("on_run_all_selected1_activate\n");
  if(prefs_get_clear_on_run()) {
    GtkCTree* processTree = GTK_CTREE(lookup_widget(HeadDino_g, "ProcessTree"));
    GList* pdaList = get_all_data(processTree, PROCESS_NODE);
    if(pdaList != NULL) {
      FOR_GLIST(walker, pdaList) {
        if((ProcessDataAug*) (walker->data) != NULL
           && ((ProcessDataAug*) (walker->data))->pw != NULL
           && PROCESSWIDGET(((ProcessDataAug*) (walker->data))->pw)->LogText != NULL) {
           #ifdef USE_ZVT_TERM
               zvt_term_reset(ZVT_TERM(PROCESSWIDGET(((ProcessDataAug*) (walker->data))->pw)->LogText), TRUE);
           #endif
        }
        
      }
      free_process_data_list(pdaList, true);
    }
  }
  run_process(NULL, NULL);
}

void on_kill_selected1_activate
(GtkMenuItem *menuitem, gpointer user_data)
{
  DEBUGCB("on_kill1_selected_activate\n");
  //GtkWidget *topWid;
  GtkCTree *processTree;
  GList    *nameList;
  GList    *walker;
  ProcessData *pd;
  set<string> pdSet;

  processTree = GTK_CTREE(user_data);
  //topWid = gtk_widget_get_toplevel(GTK_WIDGET(processTree));

  nameList = get_multiple_selected_data(processTree,
                                        NULL,
                                        (enum nodeType) (GROUP_MEMBER_NODE | PROCESS_NODE | GROUP_NODE));
  
  for(walker = g_list_first(nameList); walker; walker = g_list_next(walker)) {
    if(walker->data == NULL) continue;
    pd = (ProcessData*) walker->data;
    if(pdSet.count(string(pd->procName)) > 0) {
      continue;
    } else {
      pdSet.insert(string(pd->procName));
    }
    // If pd is a group member, turns it into the associated process node
    filter_group_members(&pd);
    if(get_nodetype(pd) == PROCESS_NODE) {
      /* Kill the sucker, if we have a process name and a machine
         (if no machine, we tried to kill the machine, not a process) */
      if(pd->procName != NULL && pd->machName != NULL) {
        kill_process(pd->machName, pd->procName);
      }
    } else if(get_nodetype(pd) == GROUP_NODE) {
      if(pd->procName != NULL && pd->status != NULL) {
        string runstr = "Killing group ";
        runstr += pd->procName;
        cerr << runstr << endl;
        print_to_common_buffer(runstr, GOOD_COL);
        kill_group(pd->status);
      }
    } else {
      print_to_common_buffer("ERROR: get_multiple_selected_data lied to us!  My pysche is damaged forever!",
                             ERROR_COL);
    }
  } /* End walk through nameList */
  /* Free it all up */
  free_process_data_list(nameList, false);
}


void on_kill_all_selected1_activate
(GtkMenuItem *menuitem, gpointer user_data)
{
  DEBUGCB("on_kill_all_selected1_activate\n");
  kill_process(NULL, NULL);
}

void on_pw_popup_signal_item_activate
(GtkMenuItem *menuitem, gpointer user_data)
{
  DEBUGCB("on_pw_popup_signal_item_activate\n");
  on_signal_selected_activate(menuitem, user_data, 1);
}

// This is a horrible hack: for some reason this submenu requires a
// double-left-click to activate an item, so we're hijacking the
// button-press-event.
void on_pw_popup_signal_button_press_event
(GtkWidget     *widget, GdkEventButton *event, gpointer user_data)
{
  DEBUGCB("on_pw_popup_signal_button_press_event\n");
  cerr << "on_pw_popup_signal_button_press_event" << endl;
  on_signal_selected_activate(GTK_MENU_ITEM(widget), user_data, 1);
  // Sigh.  Submenu isn't nuking itself in this case.
  gtk_menu_popdown(GTK_MENU (widget->parent) );
}


void on_menu_signal_selected_activate
(GtkMenuItem *menuitem, gpointer user_data)
{
  DEBUGCB("on_menu_signal_selected_activate\n");
  cerr << "on_menu_signal_selected_activate" << endl;
  on_signal_selected_activate(menuitem, user_data, 0);
}

/* Send the signal defined in user_data to all processes or groups
 * selected in the process list.
 *
 * If popup_mode is nonzero, use the ProcessData stored in
 * popup_menu_info_g.second: this is used when signalling from a
 * right-click menu, and is only invoked from
 * on_pw_popup_signal_item_activate (above) */
void on_signal_selected_activate
(GtkMenuItem *menuitem, gpointer user_data, int popup_mode)
{
  DEBUGCB("on_signal_selected_activate\n");
  //GtkWidget *topWid;
  GtkCTree *processTree;
  GList    *nameList;
  GList    *walker;
  ProcessData *pd;
  set<string> pdSet;
  pair<GtkWidget*, string>* callbackData;

  callbackData = (pair<GtkWidget*, string>*) user_data;
  cerr << "on_signal_selected_activate" << endl;

  if(popup_mode) {
    ProcessData *pd = popup_menu_info_g.second;
    if(pd == NULL) return;
    if(pd->machName == NULL && pd->procName != NULL) return;
    nameList = g_list_alloc();
    nameList = g_list_append(nameList, (void*) pd);
    cerr << "on_signal_selected_activate: popup mode; proc name: " << pd->procName << endl;
  } else {
    processTree = GTK_CTREE(callbackData->first);
    //topWid = gtk_widget_get_toplevel(GTK_WIDGET(processTree));

    nameList = get_multiple_selected_data(processTree,
                                          NULL,
                                          (enum nodeType) (GROUP_MEMBER_NODE | PROCESS_NODE | GROUP_NODE));
    cerr << "on_signal_selected_activate: non-popup mode.  List:" << endl;
    print_process_data_list(nameList, false);
    cerr << endl;
  }

  for(walker = g_list_first(nameList); walker; walker = g_list_next(walker)) {
    if(walker->data == NULL) continue;
    pd = (ProcessData*) walker->data;
    cerr << "Processing: " << endl << "\t" << flush;
    print_process_data(pd);
    cerr << endl;
    if(pdSet.count(string(pd->procName)) > 0) {
      continue;
    } else {
      pdSet.insert(string(pd->procName));
    }
    // If pd is a group member, turns it into the associated process node
    filter_group_members(&pd);
    if(get_nodetype(pd) == PROCESS_NODE) {
      /* Signal the sucker, if we have a process name and a machine
         (if no machine, we tried to signal the machine, not a process) */
      if(pd->procName != NULL && pd->machName != NULL) {
        signal_process(pd->machName, pd->procName, callbackData->second.c_str());
      }
    } else if(get_nodetype(pd) == GROUP_NODE) {
      if(pd->procName != NULL && pd->status != NULL) {
        string runstr = "Signalling group ";
        runstr += pd->procName;
        runstr += " with ";
        runstr += callbackData->second;
        cerr << runstr << endl;
        print_to_common_buffer(runstr, GOOD_COL);
        signal_group(pd->status, callbackData->second.c_str());
      }
    } else {
      print_to_common_buffer("ERROR: get_multiple_selected_data lied to us!  My pysche is damaged forever!",
                             ERROR_COL);
    }
  } /* End walk through nameList */

  /* Free it all up */
  if(popup_mode) {
    g_list_free(nameList);
  } else {
    free_process_data_list(nameList, false);
  }
}


void on_subscribe_selected1_activate
(GtkMenuItem *menuitem, gpointer user_data)
{
  DEBUGCB("on_subscribe_selected1_activate\n");
  //GtkWidget *topWid;
  GtkCTree *processTree;
  GList    *nameList;
  GList    *walker;
  ProcessData *pd;
  set<string> pdSet;

  processTree = GTK_CTREE(user_data);
  //topWid = gtk_widget_get_toplevel(GTK_WIDGET(processTree));

  nameList = get_multiple_selected_data(processTree,
                                        NULL, 
                                        (enum nodeType) (GROUP_MEMBER_NODE | PROCESS_NODE | GROUP_NODE));
  cerr << "on_subscribe_selected1_activate" << endl;
  for(walker = g_list_first(nameList); walker; walker = g_list_next(walker)) {
    if(walker->data == NULL) continue;
    pd = (ProcessData*) walker->data;
    if(pdSet.count(string(pd->procName)) > 0) {
      continue;
    } else {
      pdSet.insert(string(pd->procName));
    }
    // If pd is a group member, turns it into the associated process node
    filter_group_members(&pd);
    if(get_nodetype(pd) == PROCESS_NODE) {
      /* Subscribe to it */
      if(pd->procName != NULL && pd->machName != NULL) {
        if(sub_process(pd->machName, pd->procName, prefs_get_back_history_length())) {
          string msg = "Subscribed to ";
          msg += pd->procName;
          print_to_common_buffer(msg.c_str(), OK_COL);
          msg += "\n";
          print_to_processes(pd->procName, pd->machName, 
                             msg.c_str(), HeadDino_g, OK_COL, NULL);
          //cerr << "calling pwbs: subscribe selected" << endl;
          processWidget_button_showandtell(get_process_widget(pd->procName, pd->machName), pd);
        }
      }
    } else if(get_nodetype(pd) == GROUP_NODE) {
      if(pd->procName != NULL && pd->status != NULL) {
        apply_func_to_grp_members(get_node_from_process_data(pd), processTree, sub_process_prefs_playback);
        string msg = "Subscribing to members of group ";
        msg += pd->procName;
        msg += ".";
        print_to_common_buffer(msg.c_str(), OK_COL);
      }
    } else {
      print_to_common_buffer("ERROR: get_multiple_selected_data lied to us!  My pysche is damaged forever!",
                             ERROR_COL);
    }
  } /* End walk through nameList */
  /* Free it all up */
  free_process_data_list(nameList, false);
}

void
on_unsubscribe_selected1_activate      (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
  DEBUGCB("on_unsubscribe_selected1_activate\n");
  //  GtkWidget *topWid;
  GtkCTree *processTree;
  GList    *nameList;
  GList    *walker;
  ProcessData *pd;

  processTree = GTK_CTREE(user_data);
  //  topWid = gtk_widget_get_toplevel(GTK_WIDGET(processTree));

  nameList = get_multiple_selected_data(processTree, 
                                        NULL, 
                                        (enum nodeType) (GROUP_MEMBER_NODE | PROCESS_NODE | GROUP_NODE));

  cerr << "on_subscribe_selected1_activate" << endl;

  for(walker = g_list_first(nameList); walker; walker = g_list_next(walker)) {
    if(walker->data == NULL) continue;
    pd = (ProcessData*) walker->data;
    // If pd is a group member, turns it into the associated process node
    filter_group_members(&pd);
    if(get_nodetype(pd) == PROCESS_NODE) {
      /* Unsubscribe from it */
      if(pd->procName != NULL && pd->machName != NULL) {
        if(unsub_process(pd->machName, pd->procName)) {
          //cerr << "calling pwbs: unsubscribe selected" << endl;
          processWidget_button_showandtell(get_process_widget(pd->procName, pd->machName), pd);
          string msg = "Unsubscribed from ";
          msg += pd->procName;
          print_to_common_buffer(msg.c_str(), OK_COL);
          msg += "\n";
          print_to_processes(pd->procName, pd->machName, 
                             msg.c_str(), HeadDino_g, OK_COL, NULL);
        }
      }
    } else if(get_nodetype(pd) == GROUP_NODE) {
      if(pd->procName != NULL && pd->status != NULL) {
        string msg = "Unsubscribing from members of group ";
        msg += pd->procName;
        msg += ".";
        apply_func_to_grp_members(get_node_from_process_data(pd), processTree, unsub_process);
        print_to_common_buffer(msg.c_str(), OK_COL);
      }
    } else {
      print_to_common_buffer("ERROR: get_multiple_selected_data lied to us!  My pysche is damaged forever!",
                             ERROR_COL);
    }
  } /* End walk through nameList */
  /* Free it all up */
  free_process_data_list(nameList, false);
}

void
on_refresh_status_all_activate            (GtkMenuItem     *menuitem,
                                           gpointer         user_data)
{
  DEBUGCB("on_refresh_status_all_activate\n");
  get_status_process(NULL, "-a");
}

// Called when the GtkAdjustment used by both the scrolledwindow and
//   text objects has its value changed
//
// Okay, the key appears to be freezing and thawing the text widget; that prevents
//   it from emitting any signals, apparently.  Unfortunately, this means we also
//   aren't getting the resize signals, so we subscribe to changed with the
//   _text_expanded handler.
//
// With freeze/thaw, we get fired:
//   - once every time the text widget expands, until upper > page_size (we go off-
//     screen)
//   - once for every incremental hand-scroll
//
// Note that without freeze/thaw, Strange Things happen.
void
on_processwidget_generic_scrolled      (GtkAdjustment   *adjustment,
                                        gpointer         user_data)
{
  DEBUGCB("on_processwidget_generic_scrolled\n");
  ProcessWidget* pw = (ProcessWidget*) user_data;
  if(adjustment == NULL || pw == NULL) return;


  // This chunk of code updates scrollValue whenever we have a value not equal to
  //  "all the way to the bottom".  If we have an updated scrollValue and a bottom
  //  signal is sent, we fix it.  Unfortunately, this doesn't appear to match what
  //  actually happens...


  // We haven't overflowed beyond the available screen real estate yet;
  //   no scrolling can possibly have happened.
  if(adjustment->upper == adjustment->page_size) {
    return;
  }

  // We get fired during initialization with a page_size of 1; ignore it
  //   so our logic based on adjustment->value will work and pw->scrollValue
  //   won't get messed up.
  if(adjustment->page_size == 1) {
    return;
  }

  // Update our scrollValue if we're anywhere but the bottom of the page.
  // I don't remember why the != 0 clause is in there, but it's necessary.
  if(adjustment->value != 0 
     && adjustment->upper != adjustment->value + adjustment->page_size) {
    pw->scrollValue = adjustment->value;
    return;
  }

  // If we scrolled down to the bottom, "stick" there.
  if(adjustment->upper == adjustment->value + adjustment->page_size) {
    pw->scrollValue = -1;
    return;
  }
}


// Even with the freeze/thaw stuff, we get expanded's firing:
//   - once on an expansion of the length of the text
//     ...and that's it.  Both before and after upper has exceeded
//     page_size.
void
on_processwidget_text_expanded
(GtkAdjustment *adjustment, gpointer user_data)
{
  DEBUGCB("on_processwidget_text_expanded\n");
#ifdef USE_ZVT_TERM
  cerr << "OOPS! This shouldn't be called when using a ZVT terminal..." << endl;
#else
  ProcessWidget* pw = (ProcessWidget*) user_data;
  if(adjustment == NULL || pw == NULL) return;

  // If we're not hand scrolling, and we're not at the bottom of the page,
  //   clamp to the bottom of the page.  Use freeze/thaw to avoid firing
  //   the generic_scrolled handler.  Without freeze/thaw, it gets fired
  //   an infinite number of times.  As far as I can tell, this is because
  //   there's an irritable gremlin inside my computer that leans on the 
  //   "emit signal" button, unless we freeze him into immobility.  Really.
  if(pw->scrollValue < 0
    && adjustment->value + adjustment->page_size != adjustment->upper) {
#if 0
    gtk_text_freeze(GTK_TEXT(pw->LogText->get_text_widget()));
#endif
    gtk_adjustment_clamp_page(adjustment, adjustment->upper, adjustment->upper);
#if 0
    gtk_text_thaw(GTK_TEXT(pw->LogText->get_text_widget()));
#endif
    return;
  }
#endif
}


// See notes above for on_processwidget_generic_scrolled
void
on_errorOrChat_output_generic_scrolled
(GtkAdjustment   *adjustment, gpointer         user_data)
{  
  DEBUGCB("on_errorOrChat_output_generic_scrolled\n");
  ClawText* ctext = (ClawText*) user_data;
  if(adjustment == NULL || ctext == NULL) return;

  if(adjustment->upper == adjustment->page_size) {
    return;
  }

  if(adjustment->page_size == 1) {
    return;
  }

  if(adjustment->value != 0 
     && adjustment->upper != adjustment->value + adjustment->page_size) {
    ctext->set_scroll_value(adjustment->value);
    return;
  }

  if(adjustment->upper == adjustment->value + adjustment->page_size) {
    ctext->set_scroll_value(-1);
    return;
  }
}


// See notes above for on_processwidget_text_expanded
void on_errorOrChat_output_text_expanded
(GtkAdjustment *adjustment, gpointer user_data)
{
  DEBUGCB("on_errorOrChat_output_text_expanded\n");
  ClawText* ctext = (ClawText*) user_data;
  if(adjustment == NULL || ctext == NULL) return;

  if(ctext->get_scroll_value() < 0
    && adjustment->value + adjustment->page_size != adjustment->upper) {
#if 0
        gtk_text_freeze(GTK_TEXT(ctext->get_text_widget()));
#endif
        gtk_adjustment_clamp_page(adjustment, adjustment->upper, adjustment->upper);
#if 0
        gtk_text_thaw(GTK_TEXT(ctext->get_text_widget()));
#endif
    return;
  }
}

gboolean on_errorOrChat_enter_notify_event
(GtkWidget *widget, GdkEventCrossing *event, gpointer user_data)
{
  // If we're focus-following, and the chat tab is selected, grab
  // focus for its entry.
  DEBUGCB("on_errorOrChat_enter_notify_event\n");
  if(prefs_get_focus_follows_mouse()
     && widget != NULL
     && user_data != NULL) {
    GtkWidget* chatEntry = GTK_WIDGET (user_data);
    GtkWidget* eacNotebook = gtk_widget_get_ancestor(chatEntry, gtk_type_from_name("GtkNotebook"));
    if(eacNotebook != NULL
       && gtk_notebook_get_current_page (GTK_NOTEBOOK (eacNotebook) ) == EAC_NB_PAGE_CHAT) {
      gtk_widget_grab_focus(chatEntry);
      return TRUE;
    }
  }
  return FALSE;
}

void on_chat_input
(GtkEntry *entry, gpointer user_data)
{
  // No longer need to set the user -- it's done as part of our
  // hostname, inside MRComm.
  DEBUGCB("on_chat_input\n");
  if(entry == NULL) return;
  string entryText = gtk_entry_get_text(entry);
  // Escape the double-quotes
  string::size_type pos = 0, found;
  if(entryText.size() > 0) {
    while((found = entryText.find("\"", pos)) != string::npos) {
      entryText.replace(found, 1, "\\\"");
      pos = found + 2;
    }
  }

  string chatMsg = "shout ";
  chatMsg += " \"";
  chatMsg += entryText;
  chatMsg += "\"";
  try {
    // Heh.  All daemons reply with a shout message
    //daemon_comm_g->send_message_all("shout", chatMsg);
    daemon_comm_g->send_message(daemon_comm_g->get_module_of_type("d", true), chatMsg);
  } catch (MRException err) {
    print_to_common_buffer(err.text.c_str(), ERROR_COL);
  }
  gtk_entry_set_text(entry, "");    
}

gint on_chat_output_button_press_event
(GtkWidget *widget, GdkEvent *event)
{
  DEBUGCB("on_chat_output_button_press_event\n");
  GdkEventButton *event_button;

  g_return_val_if_fail (widget != NULL, FALSE);
  g_return_val_if_fail (event != NULL, FALSE);

  if (event->type == GDK_BUTTON_PRESS)
    {
      event_button = (GdkEventButton *) event;
      if (event_button->button == 1)       
      {
        // If it's a left-click, focus the chat text entry box        
        GtkWidget *chatEntry = lookup_widget(HeadDino_g, "chatEntry");
        gtk_widget_grab_focus(chatEntry);
      } /* End "if it's a left-click" */
    } /* End "if it's a button press" */

  return FALSE;
}


void on_run_button_clicked
(GtkButton *button,gpointer user_data)
{
  DEBUGCB("on_run_button_clicked\n");
  ProcessWidget* pw = PROCESSWIDGET(user_data);
  char* procName = NULL;
  char* machName = NULL;
  char* label;
  char* labPtr;

  gtk_label_get(GTK_LABEL(pw->ProcessNameLabel), &label);
  if(label == NULL) return;

  label = strdup(label);
  labPtr = strstr(label, " @ ");
  if(labPtr != NULL) {
    *labPtr = '\0';
    machName = labPtr + 3; /* Move to machine name */
  }
  procName = label;

  if(procName != NULL && machName != NULL) {
    if(prefs_get_clear_on_run()) {
      GtkCTree* processTree = GTK_CTREE(lookup_widget(HeadDino_g, "ProcessTree"));
      GtkCTreeNode* targetNode = get_node_from_procname(procName);
      EntryData* targetEd = (EntryData*) gtk_ctree_node_get_row_data(processTree, targetNode);
      if(targetEd->pw != NULL) {
        #ifdef USE_ZVT_TERM
            zvt_term_reset(ZVT_TERM(PROCESSWIDGET(targetEd->pw)->LogText), TRUE);
        #endif
      }
    }
    run_process(machName, procName);
  }  

  free(label);
}

void on_kill_button_clicked
(GtkButton *button, gpointer user_data)
{
  DEBUGCB("on_kill_button_clicked\n");
  ProcessWidget* pw = PROCESSWIDGET(user_data);
  char* procName = NULL;
  char* machName = NULL;
  char* label;
  char* labPtr;

  gtk_label_get(GTK_LABEL(pw->ProcessNameLabel), &label);
  if(label == NULL) return;

  label = strdup(label);
  labPtr = strstr(label, " @ ");
  if(labPtr != NULL) {
    *labPtr = '\0';
    machName = labPtr + 3; /* Move to machine name */
  }
  procName = label;

  if(procName != NULL && machName != NULL) {
    kill_process(machName, procName);
  }  

  free(label);
}

// Pop up a signal menu for this process
void on_signal_button_clicked
(GtkButton *button, gpointer user_data)
{
  DEBUGCB("on_signal_button_clicked\n");
  GtkWidget* signalMenu = GTK_WIDGET (gtk_object_get_user_data (GTK_OBJECT ( GTK_BIN(button)->child )));

  GtkWidget *pw = gtk_widget_get_ancestor(GTK_WIDGET(button), processWidget_get_type()); 
  ProcessData* pd = new ProcessData(); 
  char* nameLabel; 
  char* statusLabel;                              
                                                      
  gtk_label_get(GTK_LABEL(PROCESSWIDGET(pw)->ProcessNameLabel), &nameLabel); 
  gtk_label_get(GTK_LABEL(PROCESSWIDGET(pw)->ProcessStateLabel), &statusLabel); 
  process_data_from_label(pd, nameLabel, statusLabel); 
 
  add_error_trap(); 
  popup_menu_showandtell(pw, pd);
  // FIXME: For some reason, this menu requires a double-click.  Huh?
  // I've tried grabbing focus, emitting a button-press-event, and
  // doing a little dance.  I'm very confused.
  gtk_menu_popup (GTK_MENU (signalMenu), NULL, NULL, NULL, NULL,
                  3, (int) (doubleOf(getTime())*1000.0));
  clear_error_traps();
  // Noop.
  //gtk_widget_grab_focus(signalMenu);
}

void on_sub_button_clicked
(GtkButton *button, gpointer user_data)
{
  DEBUGCB("on_sub_button_clicked\n");
  ProcessWidget* pw = PROCESSWIDGET(user_data);
  ProcessData pd;
  char* procName = NULL;
  char* machName = NULL;
  char* label;
  char* status;
  char* labPtr;
  init_process_data(&pd);

  gtk_label_get(GTK_LABEL(pw->ProcessNameLabel), &label);
  if(label == NULL) return;
  gtk_label_get(GTK_LABEL(pw->ProcessStateLabel), &status);

  label = strdup(label);
  labPtr = strstr(label, " @ ");
  if(labPtr != NULL) {
    *labPtr = '\0';
    machName = labPtr + 3;
  }
  procName = label;

  if(procName != NULL) {
    if(sub_process(machName, procName, prefs_get_back_history_length())) {
      string msg = "Subscribed to ";
      msg += procName;
      print_to_common_buffer(msg.c_str(), OK_COL);
      msg += "\n";
      print_to_processes(procName, machName, 
                         msg.c_str(), HeadDino_g, OK_COL, NULL);
      fill_process_data(&pd, procName, machName, status);
      //cerr << "calling pwbs: sub button" << endl;
      processWidget_button_showandtell(get_process_widget(procName, machName), &pd);
      free_process_data_members(&pd);
    }
  }  

  free(label);
}

void on_unsub_button_clicked
(GtkButton *button, gpointer user_data)
{
  DEBUGCB("on_unsub_button_clicked\n");
  ProcessWidget* pw = PROCESSWIDGET(user_data);
  ProcessData pd;
  char* procName = NULL;
  char* machName = NULL;
  char* label;
  char* status;
  char* labPtr;
  init_process_data(&pd);

  gtk_label_get(GTK_LABEL(pw->ProcessNameLabel), &label);
  if(label == NULL) return;
  gtk_label_get(GTK_LABEL(pw->ProcessStateLabel), &status);

  label = strdup(label);
  labPtr = strstr(label, " @ ");
  if(labPtr != NULL) {
    *labPtr = '\0';
    machName = labPtr + 3;
  }
  procName = label;

  fill_process_data(&pd, procName, machName, status);

  if(procName != NULL) {
    if(unsub_process(machName, procName)) {
      string msg = "Unsubscribed from ";
      msg += procName;
      //cerr << "calling pwbs: unsub button" << endl;
      processWidget_button_showandtell(get_process_widget(procName, machName), &pd);
      print_to_common_buffer(msg.c_str(), OK_COL);
      msg += "\n";
      print_to_processes(procName, machName, msg.c_str(), HeadDino_g, OK_COL, NULL);
    }
  }  

  free_process_data_members(&pd);
  free(label);
}


void on_hide_button_clicked
(GtkButton *button, gpointer user_data)
{
  DEBUGCB("on_hide_button_clicked\n");
  ProcessWidget* pw = PROCESSWIDGET(user_data);
  char* procName = NULL;
  char* machName = NULL;
  char* label;
  char* labPtr;

  gtk_label_get(GTK_LABEL(pw->ProcessNameLabel), &label);
  if(label == NULL) return;

  label = strdup(label);
  labPtr = strstr(label, " @ ");
  if(labPtr != NULL) {
    *labPtr = '\0';
    machName = labPtr + 3;
  }
  procName = label;

  if(procName != NULL && machName != NULL) {
    remove_process_display(GTK_WIDGET(HeadDino_g), machName, procName);
  }  
  free(label);
}

void on_maximize_button_clicked
(GtkButton *button, gpointer user_data)
{
  DEBUGCB("on_maximize_button_clicked\n");
  ProcessWidget* pw = PROCESSWIDGET(user_data);
  ProcessData pd;
  char* label;

  init_process_data(&pd);

  gtk_label_get(GTK_LABEL(pw->ProcessNameLabel), &label);
  if(label == NULL) return;
  
  process_data_from_label(&pd, label, "Unknown");

  if(pd.procName != NULL && pd.machName != NULL) {
    if(add_process_display(GTK_WIDGET(HeadDino_g), pd.machName, pd.procName, pd.status)) {
      /* Implicit subscribe, if we have the requisite data 
         and are not currently viewing it.
      */
      sub_process(pd.machName, pd.procName, prefs_get_back_history_length());
      //cerr << "calling pwbs: maximize button" << endl;
      processWidget_button_showandtell(GTK_WIDGET(pw), &pd);
    }
    switch_to_tab(&pd);
  } 
  // Looks like add_process_display doesn't dup things appropriately within it.
  //  Oops.
  free_process_data_members(&pd);
}

void on_notebook_page_switched
(GtkNotebook *notebook, GtkNotebookPage *page,
 gint page_num, gpointer user_data)
{
  DEBUGCB("on_notebook_page_switched\n");
  GtkWidget* tabLabel;
  GList* chilluns;
  SwitchableChildData* scd;
  GtkCTree* processTree = NULL; // Only init if we need it
  ProcessData pd;
  int numChildren;

  if(notebook == NULL || page == NULL) return;

  init_process_data(&pd);

  if(page == NULL) 
  { 
      cerr << "\tpage null; returning" << endl;
      return; 
  }
  //This is throwing an error.
  //Apparently page->tab_label isn't a thing?
  //We'll short circuit and return early

  //GTK NOTEBOOK PAGE
  tabLabel = gtk_notebook_get_tab_label(notebook, gtk_notebook_get_nth_page(notebook, page_num));
  chilluns = g_list_first((GList*) gtk_object_get_user_data(GTK_OBJECT(tabLabel)));

  //numChildren = g_list_length(chilluns);
  numChildren = gtk_notebook_get_n_pages(notebook);  

  // TODO: maybe hide notebook here, if things look cruddy

  // Unselect everything; we want selection to follow the tabbing if the prefs say to
  // If there aren't any children (we just hid the last window and switched to the
  // Overview tab), don't deselect either.
  if(prefs_get_tie_selection_to_tab() && numChildren > 0)
  {
    processTree = GTK_CTREE(lookup_widget(HeadDino_g, "ProcessTree"));
    gtk_ctree_unselect_recursive(processTree, NULL);
  }
  
  // Loop through children, reparenting them to this tab and selecting if need be
  FOR_GLIST(walker, chilluns) 
  {
  //GtkWidget* walker;
  //for(int ii = 0; ii < numChildren; ii++)
  //{
  //  walker = gtk_notebook_get_nth_page(notebook, ii);

    scd = (SwitchableChildData*) walker->data;
    fill_process_data_from_process_widget(&pd, PROCESSWIDGET(scd->switchedChild));

    if(scd == NULL || scd->parent == NULL || scd->switchedChild == NULL)
      continue;
    
    // Don't do anything if the parent hasn't changed...
    if(GTK_WIDGET(scd->switchedChild)->parent != scd->parent)
    {
      gtk_object_ref (GTK_OBJECT(scd->switchedChild));
      // If we haven't seen it yet, don't remove
      if(GTK_WIDGET(scd->switchedChild)->parent != NULL)
        gtk_container_remove (GTK_CONTAINER(GTK_WIDGET(scd->switchedChild)->parent), scd->switchedChild);
      
      gtk_container_add (GTK_CONTAINER(scd->parent), scd->switchedChild);
      gtk_object_unref (GTK_OBJECT(scd->switchedChild));
    }
    
    // Recalculate what buttons should be visible
    processWidget_button_showandtell(scd->switchedChild, &pd, page_num);
    //cerr << "\t...done." << endl;
    // Handle selection foo
    if(prefs_get_tie_selection_to_tab())
    {
      if(!(numChildren > 1 && !prefs_get_groups_select_members() ) ) 
      {
        // This is horribly inefficient...
        GList* nodeList = get_all_nodes(processTree, 
                                      PROCESS_NODE | GROUP_MEMBER_NODE, 
                                      pd.procName);
        FOR_GLIST(walker, nodeList) 
        {
          if(walker == NULL || walker->data == NULL) 
            continue;
          gtk_ctree_select(processTree, (GtkCTreeNode*) (walker->data));
          // Don't break; we can have more than one node (group members)
        }
      }
    }
    
    gtk_widget_show(scd->switchedChild);
    gtk_widget_show(scd->parent);
  }

  // If a group and not selecting members, select the group node
  if(numChildren > 1 && !prefs_get_groups_select_members())
  {
    char* label;
    gtk_label_get(GTK_LABEL(tabLabel), &label);
    GList* nodeList = get_all_nodes(processTree, GROUP_NODE, label);
    FOR_GLIST(walker, nodeList) 
    {
      if(walker == NULL || walker->data == NULL) 
        continue;
      gtk_ctree_select(processTree, (GtkCTreeNode*) (walker->data));
    }    
  }

  // Put focus in entry line of first child that is running
  // Else, put focus in the filter entry of the first child if filters are enabled
  // Else, put focus on tab label
  int focussed = 0;
  GtkWidget* firstChildFilter = NULL;
  if(numChildren > 0) 
  {
    FOR_GLIST(walker, chilluns) 
    {
    //GtkWidget* walker;
    //for(int ii = 0; ii < numChildren; ii++)
    //{
    //  walker = gtk_notebook_get_nth_page(notebook, ii);
      
      scd = (SwitchableChildData*) walker->data;
      if(scd->switchedChild != NULL) 
      {
        if(GTK_WIDGET_VISIBLE(PROCESSWIDGET(scd->switchedChild)->stdinEntry)) 
        {
          bool isEditable = gtk_editable_get_editable(GTK_EDITABLE(PROCESSWIDGET(scd->switchedChild)->stdinEntry));
          if(isEditable)
          {
            gtk_widget_grab_focus(PROCESSWIDGET(scd->switchedChild)->stdinEntry);
            focussed = 1;
            break;
          } 
          else if(firstChildFilter != NULL) 
          {
            firstChildFilter = PROCESSWIDGET(scd->switchedChild)->regexpEntry;
          }
        }
      }
    } // End loop through children
  }
  if(!focussed) 
  {
    if(prefs_get_stdout_filtering() && firstChildFilter != NULL)
    {
      gtk_widget_grab_focus(firstChildFilter);
    } 
    else
    {
      gtk_widget_grab_focus(tabLabel);
    }
  }

  // Allow clearing of the buffer if we have children; disallow otherwise
  if(numChildren > 0)
  {
    gtk_widget_set_sensitive(lookup_widget(HeadDino_g, "clear_cur_buffer"), TRUE);
  } 
  else
  {
    gtk_widget_set_sensitive(lookup_widget(HeadDino_g, "clear_cur_buffer"), FALSE);
  }

  gtk_widget_show(GTK_WIDGET(notebook));
  
}

void on_error_and_chat_notebook_page_switched
(GtkNotebook *notebook, GtkNotebookPage *page,
 gint page_num, gpointer user_data) 
{
  DEBUGCB("on_error_and_chat_notebook_page_switched\n");
  // This notebook is static; let's cheat and just use the page number
  if(page_num == EAC_NB_PAGE_CHAT) {
    // Also see line_handler
    static GtkStyle *normalStyle = NULL;
    if(normalStyle == NULL) {
      GtkStyle* old = gtk_widget_get_style(GTK_WIDGET(notebook));
      normalStyle = gtk_style_new();
      // We stored the themed active color here when we turned the tab red 
      normalStyle->bg[GTK_STATE_ACTIVE] = old->bg[GTK_STATE_INSENSITIVE];
    }
    gtk_widget_set_style(GTK_WIDGET(notebook), normalStyle);
  }
}

void on_loadFile_loadButton_clicked
(GtkButton *button, gpointer user_data)
{
  DEBUGCB("on_loadFile_loadButton_clicked\n");
  char* daemonName;
  /* Get text from entry1 */
  GtkWidget* topWid = gtk_widget_get_toplevel(GTK_WIDGET(button));
  GtkWidget *option_menu;

  /* Find the daemon name we picked */
  option_menu = lookup_widget (topWid, "daemonOptionMenu");
  gtk_label_get(GTK_LABEL(GTK_BIN(option_menu)->child), &daemonName);

  /* Load the config */
  try {
    load_config(daemonName,
                gtk_entry_get_text(GTK_ENTRY(lookup_widget(topWid, "filenameEntry"))));
  } catch(MRException e) {
    string errStr = "ERROR loading file: ";
    errStr += e.text;
    cerr << errStr << endl;
    print_to_common_buffer(errStr.c_str(), ERROR_COL);
  }

  /* Update our display */
  get_status_process(daemonName, "-a");

  /* Destroy the dialog */
  gtk_widget_destroy(topWid);
}

void on_loadFile_cancelButton_clicked
(GtkButton *button, gpointer user_data)
{
  DEBUGCB("on_loadFile_cancelButton_clicked\n");
  /* Destroy the dialog */
  GtkWidget* topWid = gtk_widget_get_toplevel(GTK_WIDGET(button));
  gtk_widget_destroy(topWid);
}


void on_loadFile_filenameEntry_activate
(GtkEntry *entry, gpointer user_data)
{
  DEBUGCB("on_loadFilefilenameEntry_activate\n");
  char* daemonName;
  GtkWidget* topWid = gtk_widget_get_toplevel(GTK_WIDGET(entry));
  GtkWidget *option_menu;

  /* Find the daemon name we picked */
  option_menu = lookup_widget (topWid, "daemonOptionMenu");
  gtk_label_get(GTK_LABEL(GTK_BIN(option_menu)->child), &daemonName);

  /* Load the config */
  try {
    load_config(daemonName,
                gtk_entry_get_text(GTK_ENTRY(entry)));
  } catch(MRException e) {
    string errStr = "ERROR loading file: ";
    errStr += e.text;
    cerr << errStr << endl;
    print_to_common_buffer(errStr.c_str(), ERROR_COL);
  }

  /* Update our display */
  get_status_process(daemonName, "-a");

  /* Destroy the dialog */
  gtk_widget_destroy(topWid);
}

void on_setDialog_okButton_clicked
(GtkButton *button, gpointer user_data)
{
  DEBUGCB("on_setDialog_okButton_clicked\n");
  GtkWidget* topWid = gtk_widget_get_toplevel(GTK_WIDGET(button));
  GtkWidget* commandEntry = lookup_widget(topWid, "commandEntry");
  GtkWidget* workingDirEntry = lookup_widget(topWid, "workingDirEntry");
  GtkWidget* logDirEntry = lookup_widget(topWid, "logDirEntry");
  GtkWidget* envHashText = lookup_widget(topWid, "envHashText");
  GtkWidget* machineEntry = lookup_widget(topWid, "machineEntry");
  GtkWidget* processNameEntry = lookup_widget(topWid, "processNameEntry");
  char* host = strdup(gtk_entry_get_text(GTK_ENTRY(machineEntry)));
  char* walker;

  host += 2; // Skip d_
  walker = strstr(host, "_");
  if(walker != NULL) *walker = '\0';
  /* Send the set command */
  set_process(gtk_entry_get_text(GTK_ENTRY(machineEntry)),
              host,
              gtk_entry_get_text(GTK_ENTRY(processNameEntry)),
              gtk_entry_get_text(GTK_ENTRY(commandEntry)),
              gtk_entry_get_text(GTK_ENTRY(workingDirEntry)),
              gtk_entry_get_text(GTK_ENTRY(logDirEntry)),
              gtk_editable_get_chars(GTK_EDITABLE(envHashText), 0, -1));
  /* Destroy the dialog */
  host -= 2;
  if(host != NULL) free(host);
  else cerr << "Weird; host is null..." << endl;
  gtk_widget_destroy(topWid);  
}

void on_setDialog_cancelButton_clicked
(GtkButton *button, gpointer user_data)
{
  DEBUGCB("on_setDialog_cancelButton_clicked\n");
  /* Destroy the dialog */
  GtkWidget* topWid = gtk_widget_get_toplevel(GTK_WIDGET(button));
  gtk_widget_destroy(topWid);  
}

/* Dialog to pick daemon target for local config file loading */
void on_pickDialogForLocalFileSelection_done
(GtkWidget *object,  gpointer user_data)
{
  DEBUGCB("on_pickDialogForLocalFileSelection_done\n");
  char *daemon_name;
  GtkWidget* topWid = gtk_widget_get_toplevel(GTK_WIDGET(object)); 
  GtkWidget *option_menu;

  /* Find the daemon name we picked */
  option_menu = lookup_widget (object, "daemonOptionMenu");
  gtk_label_get(GTK_LABEL(GTK_BIN(option_menu)->child), &daemon_name);
  
  /* Are we connected to 'em? */
  if(daemon_comm_g->get_connections().find(daemon_name)
     == daemon_comm_g->get_connections().end()) {
    string errStr = "ERROR: no comm link for daemon ";
    errStr += daemon_name;
    errStr += "!";
    print_to_common_buffer(errStr.c_str(), ERROR_COL);
    /* Destroy the dialog */
    gtk_widget_destroy(topWid);  
    return;
  }
    
  try {
    send_local_config(local_config_g);
  } catch (MRException err) {
    print_to_common_buffer(err.text.c_str(), ERROR_COL);
  }

  /* Destroy the dialog */
  gtk_widget_destroy(topWid);  
}

void on_localFileSelection_load_button_clicked
(GtkButton *button, gpointer user_data)
{
  DEBUGCB("on_localFileSelection_load_button_clicked\n");
  GtkWidget* topWid = gtk_widget_get_toplevel(GTK_WIDGET(button)); 
  GtkWidget* fileSel = lookup_widget(topWid, "localFileSelection");
  string loadMsg = "Loading local file ";
  loadMsg += gtk_file_selection_get_filename(GTK_FILE_SELECTION(fileSel));

  cerr << loadMsg << endl;

  /* Read the sucker */
  try {
    local_config_name_g = gtk_file_selection_get_filename(GTK_FILE_SELECTION(fileSel));
    local_config_g = rcl::loadConfigFile(local_config_name_g);
  } catch (MRException err) {
    string errStr = "ERROR: ";
    errStr += err.text;
    cerr << errStr << endl;
    print_to_common_buffer(errStr.c_str(), ERROR_COL);
    set_gratuitous_image(true);
    return;
  }

  /* Destroy the dialog */
  if(topWid != NULL) {
    gtk_widget_destroy(topWid);  
  }

  print_to_common_buffer(loadMsg.c_str(), OK_COL);
  send_local_config(local_config_g);

  // Okay, we can now reload.  Don't worry about checking if we've already done this;
  //  it's not likely to be a performance bottleneck. ;)
  gtk_widget_set_sensitive(lookup_widget(HeadDino_g, "reload_local_config_file1"), TRUE);
}

void on_localFileSelection_cancel_button1_clicked
(GtkButton *button, gpointer user_data)
{
  DEBUGCB("on_localFileSelection_cancel_button1_clicked\n");
  /* Destroy the dialog */
  GtkWidget* topWid = gtk_widget_get_toplevel(GTK_WIDGET(button)); 
  gtk_widget_destroy(topWid);  
}


void on_savePrefsFileSelection_load_button_clicked
(GtkButton *button, gpointer user_data)
{
  DEBUGCB("on_savePrefsFileSelection_load_button_clicked\n");
  GtkWidget* topWid = gtk_widget_get_toplevel(GTK_WIDGET(button)); 
  GtkWidget* fileSel = lookup_widget(topWid, "savePrefsFileSelection");
  string filename = gtk_file_selection_get_filename(GTK_FILE_SELECTION(fileSel));
  string saveMsg = "Saving current preferences to file ";
  saveMsg += filename;

  /* Save the sucker */
  try {
    prefs_g.writeToFile(filename);
  } catch (MRException err) {
    string errStr = "ERROR: ";
    errStr += err.text;
    cerr << errStr << endl;
    print_to_common_buffer(errStr.c_str(), ERROR_COL);
    set_gratuitous_image(true);
    return;
  }

  /* Destroy the dialog */
  if(topWid != NULL) {
    gtk_widget_destroy(topWid);  
  }

  print_to_common_buffer(saveMsg.c_str(), OK_COL);
}

void on_savePrefsFileSelection_cancel_button1_clicked
(GtkButton *button, gpointer user_data)
{
  DEBUGCB("on_savePrefsFileSelection_cancel_button1_clicked\n");
  /* Destroy the dialog */
  GtkWidget* topWid = gtk_widget_get_toplevel(GTK_WIDGET(button)); 
  gtk_widget_destroy(topWid);  
}

void on_loadPrefsFileSelection_load_button_clicked
(GtkButton *button, gpointer user_data)
{
  DEBUGCB("on_loadPrefsFileSelection_load_button_clicked\n");
  GtkWidget* topWid = gtk_widget_get_toplevel(GTK_WIDGET(button)); 
  GtkWidget* fileSel = lookup_widget(topWid, "loadPrefsFileSelection");
  string filename = gtk_file_selection_get_filename(GTK_FILE_SELECTION(fileSel));
  string loadMsg = "Loading preferences from file ";
  loadMsg += filename;

  /* Read the sucker */
  try {
    rcl::exp filePrefs = rcl::readFromFile(filename);
    // BUGCHECK: Can this add fields that we don't have in prefs_g?
    if(filePrefs[0].defined()) 
      on_preferences_changed(filePrefs[0]);
    else {
      string errStr = "file ";
      errStr += filename;
      errStr += " unable to be parsed.";
      throw rcl::exception(errStr);
    }
    prefsFilename_g = filename;
  } catch (rcl::exception err) {
    string errStr = "ERROR: ";
    errStr += err.text;
    cerr << errStr << endl;
    print_to_common_buffer(errStr.c_str(), ERROR_COL);
    set_gratuitous_image(true);
    return;
  }

  /* Destroy the dialog */
  if(topWid != NULL) {
    gtk_widget_destroy(topWid);  
  }

  print_to_common_buffer(loadMsg.c_str(), OK_COL);
}

void on_loadPrefsFileSelection_cancel_button1_clicked
(GtkButton *button, gpointer user_data)
{
  DEBUGCB("on_loadPrefsFileSelection_cancel_button1_clicked\n");
  /* Destroy the dialog */
  GtkWidget* topWid = gtk_widget_get_toplevel(GTK_WIDGET(button)); 
  gtk_widget_destroy(topWid);  
}

void on_pickDaemon_daemonEntry_activate
(GtkEditable *editable, gpointer user_data)
{
  DEBUGCB("on_pickDaemon_daemonEntry_activate\n");
  GtkWidget* topWid = gtk_widget_get_toplevel(GTK_WIDGET(editable)); 
  //connect_to_daemon(gtk_entry_get_text(GTK_ENTRY(editable)));
  cerr << "WARNING: deprecated function called; daemon connections" << endl
       << "         are now handled automagically within MR_Comm."  << endl;
  /* Destroy the dialog */
  gtk_widget_destroy(topWid);
}


void on_pickDaemon_connectButton_clicked
(GtkButton *button, gpointer user_data)
{
  DEBUGCB("on_pickDaemon_connectButton_clicked\n");
  GtkWidget* topWid = gtk_widget_get_toplevel(GTK_WIDGET(button)); 
  //connect_to_daemon(gtk_entry_get_text(GTK_ENTRY(lookup_widget(topWid, "daemonEntry"))));
  cerr << "WARNING: deprecated function called; daemon connections" << endl
       << "         are now handled automagically within MR_Comm."  << endl;
  /* Destroy the dialog */
  gtk_widget_destroy(topWid);  
}


void on_pickDaemon_cancelButton_clicked
(GtkButton *button, gpointer user_data)
{
  /* Destroy the dialog */
  DEBUGCB("on_pickDaemon_cancelButton_clicked\n");
  GtkWidget* topWid = gtk_widget_get_toplevel(GTK_WIDGET(button)); 
  gtk_widget_destroy(topWid);  
}

void on_pickTargetDialog_killButton_clicked 
(GtkButton *button, gpointer user_data)
{
  DEBUGCB("on_pickTargetDialog_killButton_clicked\n");
  char* daemonName = NULL;
  GtkWidget* topWid = gtk_widget_get_toplevel(GTK_WIDGET(button)); 
  GtkWidget *option_menu;

  option_menu = lookup_widget (GTK_WIDGET (button), "daemonOptionMenu");
  // This is wrong in so many ways...
  //   A GtkOptionMenu has a menu member, which contains the menu items
  //   that pop up.  However, the currently selected item (and that item
  //   only) gets reparented to the GtkOptionMenu.  Weird.  So very, very
  //   weird...
  gtk_label_get(GTK_LABEL(GTK_BIN(option_menu)->child), &daemonName);

  kill_server(daemonName);
  /* Destroy the dialog */
  gtk_widget_destroy(topWid);  
}

void on_pickTargetDialog_cancelButton_clicked
(GtkButton *button, gpointer user_data)
{
  DEBUGCB("on_pickTargetDialog_cancelButton_clicked\n");
  /* Destroy the dialog */
  GtkWidget* topWid = gtk_widget_get_toplevel(GTK_WIDGET(button)); 
  gtk_widget_destroy(topWid);  
}

void on_cancelLaunchDialog_cancelButton_clicked 
(GtkButton *button, gpointer user_data)
{
  DEBUGCB("on_cancelLaunchDialog_cancelButton_clicked\n");
  /* Die die die! */
  cerr << "User cancelled attempt to connect to IPC's central." << endl
       << "  Your CENTRALHOST environment variable or the" << endl
       << "  --centralhost argument probably points to the wrong" << endl
       << "  host and/or port." << endl;
  exit(1);
}


/***********************************************************
 ***************  POPUP MENU CALLBACKS  ********************
 ***********************************************************/
void popup_menu_showandtell
(GtkWidget* pw, ProcessData* pd, bool grpViewed /* = false */)
{
  DEBUGCB("on_popup_menu_showandtell\n");
  GtkMenu* menu;
  char* itemName;
  GList* walker;
  hash_map<ProcessData, pair<bool, timeval>, size_t (*)(ProcessData), bool (*)(ProcessData, ProcessData)>::iterator sit;        
  enum nodeType nType = get_nodetype(pd);
  bool grpMemViewedOnly = false;
  bool viewed = false;
  //  bool viewedInGroup = false; // Are we *currently* being viewed as part of a group?
  string pdProcName = pd->procName;
  GtkNotebook* nb;
  GtkWidget* tabLabelWid;
  int curPage;
  char* curTabLabelPtr;
  string curTabLabel;


  sit = subscriptions_g->find(*pd);
  menu = GTK_MENU (popup_menu_g);

  /* Store these in a place the menu handlers can get at them. */
  if(popup_menu_info_g.second != NULL) free_process_data(popup_menu_info_g.second);
  if(pw != NULL) {
    popup_menu_info_g.first = PROCESSWIDGET(pw);
    viewed = true;
  }
  else {
    popup_menu_info_g.first = NULL;
  }
  popup_menu_info_g.second = new ProcessData();
  copy_process_data(popup_menu_info_g.second, pd);

  /* Check on the current tab label; this will let us figure out if a
   * group is currently viewed / on top or not. */
  nb = GTK_NOTEBOOK(lookup_widget(HeadDino_g, "notebook1"));
  curPage = gtk_notebook_get_current_page(nb);
  tabLabelWid = gtk_notebook_get_tab_label(nb, gtk_notebook_get_nth_page(nb, curPage));
  gtk_label_get (GTK_LABEL (tabLabelWid), &curTabLabelPtr); // No copying; don't free curTabLabelPtr
  curTabLabel = curTabLabelPtr;

  /* Loop through children (aka menu items), 
     showing or hiding them as we see fit */
  for(walker = g_list_first(GTK_MENU_SHELL(menu)->children);
      walker;
      walker = g_list_next(walker)) {
    if(walker->data == NULL) continue;
    gtk_label_get(GTK_LABEL(GTK_BIN(walker->data)->child), &itemName);
    // We want to know if the process widget was right-clicked
    // directly when within a group.  This is true if the current tab
    // label does not match the process name, and is in the list of
    // viewing processes.  This also includes the
    // right-click-the-process-tree-while-the-group-is-viewed case,
    // but I think that's okay.

    // Check if we're only viewed as members of groups
    grpMemViewedOnly = false;
    if(pw != NULL) {
      grpMemViewedOnly = true;
      FOR_EACH(pw_walk, *(PROCESSWIDGET(pw)->viewers)) {
#if 0
        // We're currently being viewed, and are in the notebook's current page
        if(*pw_walk == curTabLabel && *pw_walk != pdProcName) {
          viewedInGroup = true;
        }
#endif
        // If our name is the tab label, we're viewed
        //  on our own
        if(*pw_walk == pdProcName) {
          grpMemViewedOnly = false;
        }
      }
    }
    
    if(strcmp(itemName, "Run") == 0) {
      if(nType != GROUP_NODE && !is_running(pd->status)) {
        gtk_widget_show(GTK_WIDGET(walker->data));
      } else {
        gtk_widget_hide(GTK_WIDGET(walker->data));
      }
    } else if(strcmp(itemName, "Run group") == 0) {
      if(nType == GROUP_NODE && (is_group_running(pd) & ALL_DEAD) ) {
        gtk_widget_show(GTK_WIDGET(walker->data));
      } else {
        gtk_widget_hide(GTK_WIDGET(walker->data));
      }
    } else if(strcmp(itemName, "Run remainder") == 0) {
      if(nType == GROUP_NODE && (is_group_running(pd) & DEAD_AND_ALIVE) ) {
        gtk_widget_show(GTK_WIDGET(walker->data));
      } else {
        gtk_widget_hide(GTK_WIDGET(walker->data));
      }
    } else if(strcmp(itemName, "Kill") == 0) {
      if(nType != GROUP_NODE && is_running(pd->status)) {
        gtk_widget_show(GTK_WIDGET(walker->data));
      } else {
        gtk_widget_hide(GTK_WIDGET(walker->data));
      }
    } else if(strcmp(itemName, "Kill group") == 0) {
      if(nType == GROUP_NODE && (is_group_running(pd) & ALL_ALIVE) ) {
        gtk_widget_show(GTK_WIDGET(walker->data));
      } else {
        gtk_widget_hide(GTK_WIDGET(walker->data));
      }
    } else if(strcmp(itemName, "Kill remainder") == 0) {
      if(nType == GROUP_NODE && (is_group_running(pd) & DEAD_AND_ALIVE) ) {
        gtk_widget_show(GTK_WIDGET(walker->data));
      } else {
        gtk_widget_hide(GTK_WIDGET(walker->data));
      }
    } else if(strcmp(itemName, "Signal") == 0) {
      if(nType != GROUP_NODE && is_running(pd->status)) {
        gtk_widget_show(GTK_WIDGET(walker->data));
      } else {
        gtk_widget_hide(GTK_WIDGET(walker->data));
      }
    } else if(strcmp(itemName, "Signal group") == 0) {
      if(nType == GROUP_NODE && (is_group_running(pd) & ALL_ALIVE) ) {
        gtk_widget_show(GTK_WIDGET(walker->data));
      } else {
        gtk_widget_hide(GTK_WIDGET(walker->data));
      }
    } else if(strcmp(itemName, "Signal remainder") == 0) {
      if(nType == GROUP_NODE && (is_group_running(pd) & DEAD_AND_ALIVE) ) {
        gtk_widget_show(GTK_WIDGET(walker->data));
      } else {
        gtk_widget_hide(GTK_WIDGET(walker->data));
      }
    } else if(strcmp(itemName, "View") == 0) {
      if( pw != NULL
          || (nType == GROUP_NODE && grpViewed) ) {
        gtk_widget_hide(GTK_WIDGET(walker->data));
      } else {
        gtk_widget_show(GTK_WIDGET(walker->data));
      }
    } else if(strcmp(itemName, "View maximized") == 0) {
      // Only show "view maximized" if we're in a group that's currently being displayed      
      if(pw != NULL && grpMemViewedOnly) {
        gtk_widget_show(GTK_WIDGET(walker->data));
      } else {
        gtk_widget_hide(GTK_WIDGET(walker->data));
      }
    } else if(strcmp(itemName, "Hide") == 0) {
      if((pw != NULL && !grpMemViewedOnly) 
         || (nType == GROUP_NODE && grpViewed)) {
        gtk_widget_show(GTK_WIDGET(walker->data));
      } else {
        gtk_widget_hide(GTK_WIDGET(walker->data));
      }
    } else if(strcmp(itemName, "Subscribe") == 0) {
      if((nType == GROUP_NODE)
         | (sit != subscriptions_g->end()
            && (*sit).second.first) ){
        gtk_widget_hide(GTK_WIDGET(walker->data));
      } else {
        gtk_widget_show(GTK_WIDGET(walker->data));
      }
    } else if(strcmp(itemName, "Subscribe group") == 0) {
      if(nType == GROUP_NODE && (is_group_subscribed(pd) & ALL_UNSUB) ) {
        gtk_widget_show(GTK_WIDGET(walker->data));
      } else {
        gtk_widget_hide(GTK_WIDGET(walker->data));
      }
    } else if(strcmp(itemName, "Subscribe remainder") == 0) {
      if(nType == GROUP_NODE && (is_group_subscribed(pd) & SUB_AND_UNSUB) ) {
        gtk_widget_show(GTK_WIDGET(walker->data));
      } else {
        gtk_widget_hide(GTK_WIDGET(walker->data));
      }
    } else if(strcmp(itemName, "Unsubscribe") == 0) {
      if(sit == subscriptions_g->end()
         || !(*sit).second.first) {
        gtk_widget_hide(GTK_WIDGET(walker->data));
      } else {
        gtk_widget_show(GTK_WIDGET(walker->data));
      }
    } else if(strcmp(itemName, "Unsubscribe group") == 0) {
      if(nType == GROUP_NODE && (is_group_subscribed(pd) & ALL_SUB) ) {
        gtk_widget_show(GTK_WIDGET(walker->data));
      } else {
        gtk_widget_hide(GTK_WIDGET(walker->data));
      }
    } else if(strcmp(itemName, "Unsubscribe remainder") == 0) {
      if(nType == GROUP_NODE && (is_group_subscribed(pd) & SUB_AND_UNSUB) ) {
        gtk_widget_show(GTK_WIDGET(walker->data));
      } else {
        gtk_widget_hide(GTK_WIDGET(walker->data));
      }
    } else if(strcmp(itemName, "Get status") == 0) {
      /* We can always get status; we're shown 
         during construction.*/
    } else if(strcmp(itemName, "Set config") == 0) {
      // If we're a group, we can't set config.      
      //  Otherwise, we can.
      if(nType != GROUP_NODE) 
        gtk_widget_show(GTK_WIDGET(walker->data));
      else
        gtk_widget_hide(GTK_WIDGET(walker->data));
    } else if(strcmp(itemName, "Clear buffer") == 0) {
      if(nType != GROUP_NODE && viewed)
        gtk_widget_show(GTK_WIDGET(walker->data));
      else
        gtk_widget_hide(GTK_WIDGET(walker->data));
    }
  }
}

gint on_ProcessTree_button_press_event
(GtkWidget *widget, GdkEvent *event)
{
  DEBUGCB("on_ProcessTree_button_press_event\n");
  cerr << "BPE: " << event->type << " " << ((GdkEventButton *) event)->button << endl;
  GtkMenu *menu;
  GdkEventButton *event_button;

  g_return_val_if_fail (widget != NULL, FALSE);
  g_return_val_if_fail (event != NULL, FALSE);

  menu = GTK_MENU (popup_menu_g);

  event_button = (GdkEventButton *) event;

  if (event->type == GDK_BUTTON_PRESS
      || event->type == GDK_2BUTTON_PRESS)
  {
    GtkWidget *pw = NULL;
    ProcessData* pd = new ProcessData();
    GtkCTreeNode *clickednode;
    gint row, column;
    GList *nameList = NULL;
    init_process_data(pd);

    // Figure out where in the tree we clicked
    gtk_clist_get_selection_info(GTK_CLIST(widget), 
                                 (gint) event_button->x,
                                 (gint) event_button->y,
                                 &row, &column);
    
    // Double left-click
    if(event->type == GDK_2BUTTON_PRESS
       && event_button->button == 1) {
    } 
    // Single right-click
    if(event->type == GDK_BUTTON_PRESS
       && event_button->button == 3) {      
      // Select the row to provide some feedback and allow
      //   us to reuse code from the x_selected callbacks. :)
      gtk_clist_unselect_all(GTK_CLIST(widget));
      gtk_clist_select_row(GTK_CLIST(widget), row, column);

    }
    else if(!(event->type == GDK_2BUTTON_PRESS
              && event_button->button == 1)){
      delete pd;
      cerr << "RETURN 1\n";
      return FALSE;
    }

    clickednode = gtk_ctree_node_nth(GTK_CTREE(widget), row);

    // If we clicked a member of a group co-opt it into its "real" node
    if(get_nodetype(GTK_CTREE(widget), clickednode) == GROUP_MEMBER_NODE) {
      EntryDataAug* curRowData = (EntryDataAug*) gtk_ctree_node_get_row_data(GTK_CTREE(widget), 
                                                                       clickednode);
      clickednode = curRowData->realNode;
    }
    
    if(clickednode == NULL) {
      delete pd;
      cerr << "RETURN 2\n";
      return FALSE;
    }
    if(!fill_process_data_from_treenode(pd, clickednode, GTK_CTREE(widget))) {
      delete pd;
      cerr << "RETURN 3\n";
      return FALSE;
    }
    
    // We only care if we're a process (we have a parent)
    if(pd->procName == NULL || pd->machName == NULL) {
      free_process_data_list(nameList, false);
      delete pd;
      cerr << "RETURN 4\n";
      return FALSE;
    }

    // Dup everything, so we don't go screwing around with the labels
    //   in the ProcessTree
    copy_process_data(pd, pd);
    
    /* Free it all up; f_p_d_l handles nulls okay */
    free_process_data_list(nameList, false);
      
    if(get_nodetype(GTK_CTREE(widget), clickednode) == PROCESS_NODE) {
      pw = get_process_widget(pd->procName, pd->machName);
      
      if (event_button->button == 3 
          && event->type == GDK_BUTTON_PRESS) {        
        add_error_trap();
        popup_menu_showandtell(pw, pd);
        
        gtk_menu_popup (menu, NULL, NULL, NULL, NULL, 
                        event_button->button, event_button->time);
        delete pd;
        clear_error_traps();
	cerr << "RETURN 5\n";
        return TRUE;
      } /* End "if it's a right-single-click */
      else if(event_button->button == 1
              && event->type == GDK_2BUTTON_PRESS) {
        if(add_process_display(HeadDino_g,
                               pd->machName, pd->procName, pd->status)) {
	  cerr << "add_process_display" << endl;
          /* Implicit subscribe, if we have the requisite data 
             and are not currently viewing it.
          */
          sub_process(pd->machName, pd->procName, prefs_get_back_history_length());
          //cerr << "calling pwbs: process tree button (left dbl)" << endl;
          processWidget_button_showandtell(get_process_widget(pd->procName, pd->machName), pd);
        }
        switch_to_tab(pd);      
        delete pd;
	cerr << "RETURN 6\n";
        return TRUE;
      } /* End "if it's a left-double-click */
    } /* End process node case */
    else if(get_nodetype(GTK_CTREE(widget), clickednode) == SWALLOWED_NODE) {
      // If it's a swallowed node, we only care about double-left-clicks
      if(event_button->button == 1
         && event->type == GDK_2BUTTON_PRESS) {
        switch_to_tab(string(pd->procName));      
        delete pd;
	cerr << "RETURN 7\n";
        return TRUE;
      } /* End "if it's a left-double-click */
    } else if(get_nodetype(GTK_CTREE(widget), clickednode) == GROUP_NODE) {
      // Right single-click
      if (event_button->button == 3 && event->type == GDK_BUTTON_PRESS)
      {        
        add_error_trap();
        // FILLMEIN: pop up a menu; same options as for processes
        //           need to figure out good way to suborn popup handlers
        //cerr << "WARNING: right-click of group element still in beta" << endl;
        EntryData* ed = (EntryData*) gtk_ctree_node_get_row_data(GTK_CTREE(widget), 
                                                                 clickednode);
        //cerr << "Checking if tab exists for \"" << ed->label << "\"" << endl;
        popup_menu_showandtell(NULL, pd, tab_exists(string(ed->label)));
        
        gtk_menu_popup (menu, NULL, NULL, NULL, NULL, 
                        event_button->button, event_button->time);
        delete pd;
        clear_error_traps();
	cerr << "RETURN 8\n";
        return TRUE;
      } /* End "if it's a right-single-click */
      // Left double-click
      else if(event_button->button == 1 && event->type == GDK_2BUTTON_PRESS) 
      {
        // FILLMEIN: pop open a display that incorporates all members
        //           of the group.
        cerr << "WARNING: double-click of group element still in beta" << endl;
        add_group_display(clickednode);
        switch_to_tab(string(pd->procName));      
	cerr << "RETURN 9\n";
        return TRUE;
      } /* End "if it's a left-double-click */
    }
    delete pd;
  } /* End "if it's a button press" */
  cerr << "RETURN 10\n";
  return FALSE; // Didn't handle it; return false
}

gboolean on_ProcessTree_enter_notify_event
(GtkWidget *widget, GdkEventCrossing *event, gpointer user_data)
{
  DEBUGCB("on_ProcessTree_enter_notify_event\n");
  // If we're focus-following, grab focus to the process tree
  if(prefs_get_focus_follows_mouse()
     && user_data != NULL) {
    GtkWidget* processTree = GTK_WIDGET(user_data);
    gtk_widget_grab_focus(processTree);
    return TRUE;
  }
  return FALSE;
}

gint on_pw_LogText_button_press_event
(GtkWidget *widget, GdkEvent *event)
{
  DEBUGCB("on_pw_LogText_button_press_event\n");
  GtkMenu *menu;
  GdkEventButton *event_button;

  g_return_val_if_fail (widget != NULL, FALSE);
  g_return_val_if_fail (event != NULL, FALSE);

  menu = GTK_MENU (popup_menu_g);

  if (event->type == GDK_BUTTON_PRESS)
    {
      event_button = (GdkEventButton *) event;
      if (event_button->button == 3)
      {
        
        GtkWidget *pw = gtk_widget_get_ancestor(widget, processWidget_get_type());
        ProcessData* pd = new ProcessData();
        char* nameLabel;
        char* statusLabel;

        gtk_label_get(GTK_LABEL(PROCESSWIDGET(pw)->ProcessNameLabel), &nameLabel);
        gtk_label_get(GTK_LABEL(PROCESSWIDGET(pw)->ProcessStateLabel), &statusLabel);
        process_data_from_label(pd, nameLabel, statusLabel);

        add_error_trap();
        popup_menu_showandtell(pw, pd);

        gtk_menu_popup (menu, NULL, NULL, NULL, NULL, 
                        event_button->button, event_button->time);
        clear_error_traps();
        return TRUE;
      } /* End "if it's a right-click" */
      else if (event_button->button == 1)       
      {
        // If it's a left-click, focus the associated stdin entry box        
        GtkWidget *pw = gtk_widget_get_ancestor(widget, processWidget_get_type());
        //GTK_EDITABLE ERROR
        /*if(GTK_EDITABLE(PROCESSWIDGET(pw)->stdinEntry)->editable) {
          gtk_widget_grab_focus(PROCESSWIDGET(pw)->stdinEntry);
        }
        else*/ if(GTK_WIDGET_VISIBLE(PROCESSWIDGET(pw)->regexpEntry)) {
          gtk_widget_grab_focus(PROCESSWIDGET(pw)->regexpEntry);
        }
      } /* End "if it's a left-click" */
    } /* End "if it's a button press" */

  return FALSE;
}

gint on_pw_LogText_size_allocate_event
(GtkWidget *widget, GtkAllocation   *alloc)
{
  DEBUGCB("on_pw_LogText_size_allocate_event\n");
  ProcessWidget* pw = PROCESSWIDGET(gtk_widget_get_ancestor(widget, processWidget_get_type()));
  char* lab;
  gtk_label_get(GTK_LABEL(pw->ProcessNameLabel), &lab);

    cerr << "Allocate event (" << lab << "): x,y = " << alloc->x << "," << alloc->y
         << ", w,h = " << alloc->width << "," << alloc->height << endl;


  // Resize the terminal to fit
  // Replace this section with a clawText object
  //ZvtTerm* term = ZVT_TERM(widget);
  //ClawText* term = new ClawText(widget);
  //int base_height = (GTK_WIDGET (term)->style->klass->ythickness * 2);
  //int num_rows = (alloc->height - base_height) / term->charheight;
  //int base_width = (GTK_WIDGET (term)->style->klass->xthickness * 2) + 2;
  //int num_cols = (alloc->width - base_width) / term->charwidth;
  

  //num_rows = 1;
  //num_cols = 80;
  //cerr << "Num rows: " << num_rows << ", num cols: " << num_cols << endl;
  //cerr << "Grid height: " << term->grid_height << ", " 
  //     << "grid width: " << term->grid_width << endl << endl;
  /*
  bool heightMatch = ((unsigned) num_rows) == term->grid_height;
  bool widthMatch  = ((unsigned) num_cols) == term->grid_width;
  if(!heightMatch || !widthMatch)
  {
    zvt_term_set_size(term, num_cols, num_rows);  
  }
  */
  //return FALSE;
  return TRUE;
}

void on_pw_popup_run_item_activate
(GtkMenuItem *menuitem, gpointer user_data)
{
  DEBUGCB("on_my_popup_run_item_activate\n");
  ProcessData *pd = popup_menu_info_g.second;
  if(pd == NULL) return;
  if(pd->machName == NULL && pd->procName != NULL) return;
  if(get_nodetype(pd) == GROUP_NODE) {
    string runstr = "Running group ";
    runstr += pd->procName;
    print_to_common_buffer(runstr, GOOD_COL);
    run_group(pd->procName);
  } else {
    if(prefs_get_clear_on_run()) {
      GtkCTree* processTree = GTK_CTREE(lookup_widget(HeadDino_g, "ProcessTree"));
      GtkCTreeNode* targetNode = get_node_from_process_data(pd);
      EntryData* targetEd = (EntryData*) gtk_ctree_node_get_row_data(processTree, targetNode);
      if(targetEd->pw != NULL) {
         #ifdef USE_ZVT_TERM
             zvt_term_reset(ZVT_TERM(PROCESSWIDGET(targetEd->pw)->LogText), TRUE);
         #endif
      }
    }
    run_process(pd->machName, pd->procName);
  }
}

void on_pw_popup_kill_item_activate
(GtkMenuItem *menuitem, gpointer user_data)
{
  DEBUGCB("on_pw_popup_kill_item_activate\n");
  ProcessData *pd = popup_menu_info_g.second;
  if(pd == NULL) return;
  if(pd->machName == NULL && pd->procName != NULL) return;
  if(get_nodetype(pd) == GROUP_NODE) {
    string runstr = "Killing group ";
    runstr += pd->procName;
    cerr << runstr << endl;
    print_to_common_buffer(runstr, GOOD_COL);
    kill_group(pd->status);
  } else {
    kill_process(pd->machName, pd->procName);
  }
}

void on_pw_popup_view_item_activate
(GtkMenuItem *menuitem, gpointer user_data)
{
  DEBUGCB("on_pw_popup_view_item_activate\n");
  ProcessData *pd = popup_menu_info_g.second;
  if(pd == NULL) return;
  if(pd->machName == NULL || pd->procName == NULL) return;
  if(pd->status == NULL) pd->status = (char *)"";
  if(get_nodetype(pd) == GROUP_NODE) {
    add_group_display(get_node_from_procname(pd->procName));
    switch_to_tab(string(pd->procName));
  } else {
    if(add_process_display(HeadDino_g,
                           pd->machName, pd->procName, pd->status)) {
      /* Implicit subscribe, if we have the requisite data 
         and are not currently viewing it.
      */
      sub_process(pd->machName, pd->procName, prefs_get_back_history_length());
      //cerr << "calling pwbs: pw_popup_view" << endl;
      processWidget_button_showandtell(get_process_widget(pd->procName, pd->machName), pd);
    }
    switch_to_tab(pd);
  }
}

void on_pw_popup_hide_item_activate
(GtkMenuItem *menuitem, gpointer user_data)
{
  DEBUGCB("on_pw_popup_hide_item_activate\n");
  ProcessData *pd = popup_menu_info_g.second;
  if(pd == NULL) return;
  if(pd->machName == NULL || pd->procName == NULL) return;
  if(pd->status == NULL) pd->status = (char *)"";
  if(get_nodetype(pd) == GROUP_NODE) {
    remove_group_display(pd->procName);
  } else {
    remove_process_display(HeadDino_g, pd->machName, pd->procName);
  }
}

void on_pw_popup_sub_item_activate
(GtkMenuItem *menuitem, gpointer user_data)
{
  DEBUGCB("on_pw_popup_sub_item_activate\n");
  ProcessData *pd = popup_menu_info_g.second;
  if(pd == NULL) return;
  if(pd->machName == NULL || pd->procName == NULL) return;

  if(get_nodetype(pd) == GROUP_NODE) {
    string msg = "Subscribing to members of group ";
    msg += pd->procName;
    msg += ".";
    print_to_common_buffer(msg.c_str(), OK_COL);
    apply_func_to_grp_members(get_node_from_process_data(pd), 
                              GTK_CTREE(lookup_widget(HeadDino_g, "ProcessTree")), 
                              sub_process_prefs_playback);
  } else {
    if(sub_process(pd->machName, pd->procName, prefs_get_back_history_length())) {
      string msg = "Subscribed to ";
      msg += pd->procName;
      print_to_common_buffer(msg.c_str(), OK_COL);
      msg += "\n";
      print_to_processes(pd->procName, pd->machName, 
                         msg.c_str(), HeadDino_g, OK_COL, NULL);
      //cerr << "calling pwbs: pw_popup_sub" << endl;
      processWidget_button_showandtell(get_process_widget(pd->procName, pd->machName), pd);
    }
  }
}

void on_pw_popup_unsub_item_activate
(GtkMenuItem *menuitem, gpointer user_data)
{
  DEBUGCB("on_pw_popup_unsub_item_activate\n");
  ProcessData *pd = popup_menu_info_g.second;
  if(pd == NULL) return;
  if(pd->machName == NULL || pd->procName == NULL) return;
  if(get_nodetype(pd) == GROUP_NODE) {
    string msg = "Unsubscribing from members of group ";
    msg += pd->procName;
    msg += ".";
    print_to_common_buffer(msg.c_str(), OK_COL);
    apply_func_to_grp_members(get_node_from_process_data(pd),
                              GTK_CTREE(lookup_widget(HeadDino_g, "ProcessTree")), 
                              unsub_process);
  } else {
    if(unsub_process(pd->machName, pd->procName)) {
      //cerr << "calling pwbs: pw_popup_unsub" << endl;
      processWidget_button_showandtell(get_process_widget(pd->procName, pd->machName), pd);
      string msg = "Unsubscribed from ";
      msg += pd->procName;
      print_to_common_buffer(msg.c_str(), OK_COL);
      msg += "\n";
      print_to_processes(pd->procName, pd->machName, 
                         msg.c_str(), HeadDino_g, OK_COL, NULL);
    }
  }
}

void on_pw_popup_set_config_item_activate
(GtkMenuItem *menuitem, gpointer user_data)
{
  DEBUGCB("on_pw_popup_set_config_item_activate\n");
  ProcessData *pd = popup_menu_info_g.second;
  if(pd == NULL) return;
  if(pd->machName == NULL || pd->procName == NULL) return;

  if(get_nodetype(pd) == GROUP_NODE) {
    cerr << "ERROR: A group can't has it's config set!" << endl;
    return;
  }

  // Add this process/daemon to the list of things that should fire
  //   set_config dialogs on receipt of a config message from the
  //   daemon
  pending_sets_g.push_back(process_data_to_label(pd));

  //cerr << "Adding \"" << process_data_to_label(pd) 
  //     << "\" to set_config callback list." << endl;
  
  // Get the config of the process
  get_config_process(pd->machName, pd->procName);
}

void on_process_config_changed (rcl::exp e)
{
  DEBUGCB("on_process_config_changed\n");
  //cerr << "Process config: e: " << endl;
  //e.writeConfig(cerr, 4);
  // We just changed the config for a given process; send it out
  rcl::map tosend;
  tosend("processes") = e[0];
  //cerr << "Process config: tosend: " << endl;
  //tosend[0].writeConfig(cerr, 4);
  //daemon_comm_g->send_message
  //  (MR_ALL_MODULES, string("set config ") + rcl::toString(tosend));  

  // New config synch method: only send changed stuff
  daemon_comm_g->broadcast_config(tosend /*local_config_g*/);
}


void on_editHashDialog_okButton_clicked
 (GtkWidget *button, gpointer  user_data)
{  
  DEBUGCB("on_editHashDialog_okButton_clicked\n");
  if(button == NULL) return;
  GtkWidget* topWid = gtk_widget_get_toplevel(GTK_WIDGET(button));
  GtkWidget* table = lookup_widget(topWid, "gridTable");
  GtkWidget* entry;
  rcl::exp ret = rcl::vector();
  rcl::exp tmpExp;
  rcl::exp e = rcl::map();
  rcl::exp t = rcl::map();
  rcl::exp candidate;
  GList* walker;
  ProcessData* pd = NULL;
  gchar* data;
  string realProcName = "";
  string hostName = "";
  GtkWindow* window;
  void (*userCallback) (rcl::exp);
  rcl::exp* originalConfig = NULL;
  if(gtk_object_get_user_data(GTK_OBJECT(table)) != NULL) {
    originalConfig = (rcl::exp*) gtk_object_get_user_data(GTK_OBJECT(table));
  }

  //cerr << "editHashDialog: start" << endl;

  // Figure out the name of the process
  // If we're not doing processes, ignore this
  //window = gtk_widget_get_parent_window(button);
  window = GTK_WINDOW(gtk_widget_get_ancestor(button, gtk_type_from_name("GtkWindow")));
  data = (gchar*) gtk_object_get_user_data(GTK_OBJECT(window));
  if(data != NULL && strstr(data, " @ ") != NULL) {
    char* patient = strdup(data);
    char* knife = strstr(patient, " @ ");
    knife[0] = '\0';
    realProcName = string(patient);
    hostName = string(knife + 3);
    hostName = MR_Comm::extract_host_from_module_name(hostName);
    free(patient);
    free(data);
  } else {
    realProcName = hostName = "";    
  }

  //cerr << "\tproc: " << realProcName << endl;

  // Okay, we need to reconstitute the rcl::exp, then call the user-defined
  //   callback function with it.
  for(walker = gtk_container_children(GTK_CONTAINER(table));
      walker;
      walker = g_list_next(walker)) {
    if(walker == NULL || ((GtkTableChild*) walker)->widget == NULL) continue;
    entry = ((GtkTableChild*) walker)->widget;
    // Hashes generate a text widget that's a child of a scrolled window
    if(gtk_type_is_a(GTK_WIDGET_TYPE(entry), gtk_type_from_name("GtkScrolledWindow"))) {
      entry = GTK_BIN(entry)->child;
    }
    // If our user data is non-null, it's an entry field of some sort
    if((pd = (ProcessData*) gtk_object_get_user_data(GTK_OBJECT(entry))) != NULL) {
      // procName == key (name of field)
      // machName == RCL type
      if(pd->procName == NULL || pd->machName == NULL) {
        print_to_common_buffer("ERROR: Blast! Null data from one of the dialog elements! This shouldn't happen.", ERROR_COL);
        free_process_data_members(pd);
        delete pd;
        continue;
      }      
      //cerr << "\tChecking: " << pd->procName << " => " << pd->machName << endl;
      // For almost all RCL types, the entry item derives from GtkEditable...
      if(gtk_type_is_a(GTK_WIDGET_TYPE(entry), gtk_type_from_name("GtkCheckButton"))) {
        data = (gchar*) malloc(sizeof(char)*2);
        sprintf(data, "%d", gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (entry)) ? 1 : 0);
      } else {
        data = gtk_editable_get_chars(GTK_EDITABLE(entry), 0, -1);
      }
      try {
        // Used to be candidate = new rcl::exp(), but since this
        // invokes the copy constructor on a variable allocated on the
        // heap which we can now no longer deallocate, it's a
        // leak. Enter ctmp.
        rcl::exp* ctmp = new rcl::exp();
        candidate = ctmp;
        // Case on type; strings need to be treated differently
        if(strcmp(pd->machName, "string") == 0) {
          candidate = string(data);
        } else {
          candidate = rcl::readFromString(data)[0];
        }
        //cerr << "\tType: " << candidate.getType() << endl;
        if(!( (candidate.getType() == "string" && candidate.getString().length() <= 0)
              || (candidate.getType() == RCL_MAP_TYPE && candidate.getMap().empty())
              || (candidate.getType() == RCL_VECTOR_TYPE && candidate.getVector().empty()) ) ) {
          //cerr << "\tAdding: " << pd->procName << " => " << candidate << endl;
          // Add it to the hash
          t(pd->procName) = candidate;
        }
        delete ctmp;
      } catch(rcl::exception err) {
        // Need to reset previous value for this element - we don't
        // want to clear a field if they edit it wrong (esp. the
        // environment field!)
        // FIXME
        cerr << "Caught RCL exception in dialog handler; trying to parse:" << endl
             << "\t\"" << data << "\" for field " << pd->procName << endl;
        cerr << "\tRCL Exception: " << err.text << endl;
        string errStr = "Parse error";
        if(realProcName.length() > 0) {
          errStr += " in config for process ";
          errStr += realProcName;
        }
        errStr += " in field ";
        errStr += pd->procName;
        errStr += ": ";
        errStr += err.text;
        print_to_common_buffer(errStr.c_str(), ERROR_COL);

        // Revert to the original data if it's available
        if(originalConfig != NULL 
           && originalConfig->defined()
           && originalConfig->getType() == RCL_MAP_TYPE) {
          cerr << "Using original data instead." << endl;
          t(pd->procName) = (*originalConfig)(pd->procName);
        }
      }
      free_process_data_members(pd);
      delete pd;
      if(gtk_type_is_a(GTK_WIDGET_TYPE(entry), gtk_type_from_name("GtkCheckButton"))) {
        free(data);
      } else {
        // Free data malloc'd by gtk
        g_free(data);
      }
      data = NULL;
    } // End "if user_data is not null"
  } // End loop through table elements

  try {
    // If we have a host name (we won't for non-setconfig dialogs), add it
    if(hostName.length() > 0) {
      //cerr << "\tAdding host: " << rcl::readFromString(hostName)[0];

      // Add in the host name
      t("host") = rcl::readFromString(hostName)[0];
    }

    // If we figured out a title (process name), sub it.  Otherwise, keep
    //   it flat.
    if(realProcName.length() > 0) {
      //cerr << "\tAdding procname: " << realProcName << " => " << endl;
      //tmpExp.writeConfig(cerr, 8);
      e(realProcName) = t;
    } else {
      e = t;
    }

    ret[0] = e;
  } catch(rcl::exception err) {
    cerr << "Caught RCL exception in dialog handler; adding host and process name" << endl;
    cerr << "\tRCL Exception: " << err.text << endl;
  }
  // Okay, now call user-supplied callback function with resulting exp
  if(user_data == NULL) {
    userCallback = (void (*) (rcl::exp)) (gtk_object_get_user_data(GTK_OBJECT(button)));
  } else {
    userCallback = (void (*) (rcl::exp)) (user_data);
  }
  userCallback(ret);

  /* Delete the copy of the initial config if it was passed in */
  if(originalConfig != NULL) {
    delete originalConfig;
  }

  /* Destroy the dialog */
  gtk_widget_destroy(topWid);  
}

void on_editHashDialog_cancelButton_clicked 
(GtkWidget *button, gpointer user_data)
{
  DEBUGCB("on_edithashDialog_cancelButton_clicked\n");
  if(button == NULL) return;
  GtkWidget* topWid = gtk_widget_get_toplevel(GTK_WIDGET(button));
  GtkWidget* table = lookup_widget(topWid, "gridTable");
  GtkWidget* entry;
  GList* walker;
  ProcessData* pd = NULL;
  gchar* data;
  GtkWindow* window;

  // Figure out the name of the process
  // If we're not doing processes, ignore this
  //window = gtk_widget_get_parent_window(button);
  window = GTK_WINDOW(gtk_widget_get_ancestor(button, gtk_type_from_name("GtkWindow")));
  data = (gchar*) gtk_object_get_user_data(GTK_OBJECT(window));

  // Walk the children and free memory
  for(walker = gtk_container_children(GTK_CONTAINER(table));
      walker;
      walker = g_list_next(walker)) {
    if(walker == NULL || ((GtkTableChild*) walker)->widget == NULL) continue;
    entry = ((GtkTableChild*) walker)->widget;
    // Hashes generate a text widget that's a child of a scrolled window
    if(gtk_type_is_a(GTK_WIDGET_TYPE(entry), gtk_type_from_name("GtkScrolledWindow"))) {
      entry = GTK_BIN(entry)->child;
    }
    // If our user data is non-null, it's an entry field of some sort
    if((pd = (ProcessData*) gtk_object_get_user_data(GTK_OBJECT(entry))) != NULL) {

      data = NULL;
      if(GTK_IS_EDITABLE(entry)) {
        data = gtk_editable_get_chars(GTK_EDITABLE(entry), 0, -1);
      }

      free_process_data_members(pd);
      delete pd;
      if(data) {
        // Free data malloc'd by gtk
        g_free(data);
      }

    } // End "if user_data is not null"
  } // End loop through table elements

  // Clean up the new'd original config
  if(gtk_object_get_user_data(GTK_OBJECT(table)) != NULL) {
    rcl::exp* originalConfig = (rcl::exp*) gtk_object_get_user_data(GTK_OBJECT(table));
    delete originalConfig;
  }

  /* Destroy the dialog */
  gtk_widget_destroy(topWid);  
}


gboolean on_editHashDialog_key_press_event 
(GtkWidget *widget, GdkEventKey *event, gpointer user_data)
{
  DEBUGCB("on_editHashDialog_key_press_event\n");
  // If the key is keypad-enter, activate as if we had pressed Return;
  // otherwise noop.
  //Another GDK_KP_Enter
  if(event->keyval){// == GDK_KP_Enter) {
    gtk_signal_emit(GTK_OBJECT(widget), gtk_signal_lookup("activate", gtk_editable_get_type()));
    return TRUE;
  }
  return FALSE;
}



/***************************************************************
 * Revision History:
 * $Log: callbacks.c,v $
 * Revision 1.100  2007/05/30 19:10:31  brennan
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
 * Revision 1.99  2007/04/19 15:48:37  brennan
 * Moved an rcl call into a try/catch in order to catch malformed strings
 * from mraptord without crashing claw.
 *
 * Revision 1.98  2007/04/18 17:13:28  brennan
 * Fixed occasional crash in claw.
 *
 * Revision 1.97  2007/02/15 18:37:05  brennan
 * Mapped keypad-enter to activate entries in autogenerated dialogs,
 * such as preferences and process configs.
 *
 * Revision 1.96  2007/02/15 18:07:06  brennan
 * Fixed a few memory leaks, and one rogue pointer bug.
 * Mapped keypad enter to regular enter in stdin and filter entries.
 * Added focus-follows-mouse option.
 *
 * Revision 1.95  2007/01/12 19:42:56  brennan
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
 * Revision 1.94  2006/12/20 19:07:18  brennan
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
 * Revision 1.93  2006/11/16 22:50:31  brennan
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
 * Revision 1.92  2006/11/16 19:35:16  brennan
 * Removed unused variable.
 *
 * Revision 1.91  2006/11/16 18:55:04  brennan
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
 * Revision 1.90  2006/08/11 22:31:14  brennan
 * *** empty log message ***
 *
 * Revision 1.89  2006/08/11 22:05:37  brennan
 * Committed detection of the BadWindow errors which occur when you ssh -X
 * into an X server with the security extension.  A semi-informative error
 * message is printed, and claw now functions, but right-click menus are
 * funkily placed.  Use ssh -Y instead.
 *
 * Revision 1.88  2005/06/16 15:26:19  brennan
 * Plugged leak whenever a hashDialog was cancelled; introduced by patch to
 * handle errorful entries more gracefully.
 *
 * Revision 1.87  2005/06/16 04:30:49  brennan
 * Invalid RCL expressions in process config dialogs no longer nuke the
 * contents; instead, the original data is used.  This is especially important
 * for the env hash.  An error message is now also printed to the common
 * buffer.
 *
 * Revision 1.86  2005/05/18 01:04:35  trey
 * added support for multi-line buffered daemon responses
 *
 * Revision 1.85  2005/05/17 22:25:35  trey
 * added preliminary support for multi-line daemon responses
 *
 * Revision 1.84  2004/11/25 05:20:48  brennan
 * Fixed run-all bug (using a GList* instead of its data)
 *
 * Patched a bunch of memory leaks, and all the nasty ones.  There are four
 * known remaining leaks, all dealing with the widget used to view the output
 * of a process, with a sum total of ~90 bytes leaked per process viewed.
 * Not worth my time at the moment. :-)
 *
 * Revision 1.83  2004/11/23 07:01:54  brennan
 * A number of memory leaks and free/delete mismatches repaired.
 * Some errors detected by valgrind remain, and will be fixed soon.
 *
 * Revision 1.82  2004/11/17 22:31:10  trey
 * set color code to OK_COL when printing stdout responses; apparently it was uninitialized before (?)
 *
 * Revision 1.81  2004/04/28 18:58:50  dom
 * Appended log directive
 * 
 ***************************************************************/
