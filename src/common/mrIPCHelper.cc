/********** tell emacs we use -*- c++ -*- style comments *******************
 * COPYRIGHT 2004, Carnegie Mellon University
 * $Revision: 1.11 $  $Author: brennan $  $Date: 2006/11/16 18:56:05 $
 *  
 * PROJECT: FIRE Architecture Project
 *
 * @file    IPCHelper.cc
 * @brief   No brief
 ***************************************************************************/

/***************************************************************************
 * INCLUDES
 ***************************************************************************/

#include <assert.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>

#include <iostream>
#include <cstring>

#include "ipc.h"

#include "mrIPCHelper.h"

using namespace std;

namespace microraptor {

/***************************************************************************
 * STATIC MEMBER VARIABLES
 ***************************************************************************/

bool IPCHelper::verbose = true;

/***************************************************************************
 * FUNCTIONS
 ***************************************************************************/

string
IPCHelper::getshorthostname(void) {
  char buf[1024];
  if (0 != gethostname(buf, sizeof(buf))) {
    cerr << "gethostname() failed: " << strerror(errno) << endl;
    exit(EXIT_FAILURE);
  }
  // let's use the short version.  truncate at the first '.'
  char *dotpos;
  if (0 != (dotpos = strchr(buf, '.'))) *dotpos = '\0';

  return buf;
}

string
IPCHelper::defineMessage(const string& messageName,
			 const string& format)
{
  int lengthToUse;
  const char* formatToUse;
  if  ("" == format || "NULL" == format) {
    lengthToUse = 0;
    formatToUse = 0;
  } else {
    lengthToUse = IPC_VARIABLE_LENGTH;
    formatToUse = format.c_str();
  }

  if (verbose) {
    cout << "IPCHelper::defineMessage:" << endl
	 << "  name=\"" << messageName << "\"" << endl
	 << "  format=\"" << format << "\"" << endl;
  }

  if (!IPC_isMsgDefined(messageName.c_str())) {
    if (verbose) cout << "Msg was not previously defined." << endl;
    if (IPC_OK != IPC_defineMsg(messageName.c_str(), lengthToUse,
				formatToUse)) {
      cout << "IPC_defineMsg () failed to define " << messageName << endl;
      exit(EXIT_FAILURE);
    }
  } else if (0 != formatToUse) {
    IPC_setVerbosity(IPC_Silent);
    if (IPC_OK != IPC_checkMsgFormats(messageName.c_str(), formatToUse)) {
      cerr << "ERROR: IPC message formats are different for "
	   << messageName << endl
	   << "  local format is \"" << formatToUse << "\"" << endl;
      exit(EXIT_FAILURE);
    }
    IPC_setVerbosity(IPC_Exit_On_Errors);
  }

  return messageName;
}

void
IPCHelper::reliableConnect(const string& nameOfThisModule, void (*callback)(string, bool)) {

  /* suppress errors about how we can't connect while we're
     in this loop */
  IPC_setVerbosity(IPC_Silent);

  const char *centralHost = getenv("CENTRALHOST");
  if (centralHost == NULL) centralHost = "localhost";

  if (verbose) {
    fprintf(stdout,"Attempting to connect to IPC central server on %s", centralHost);
  }

  // Allow callback initialization, if necessary.
  if(callback != NULL) callback(centralHost, false);

  int printedUpdate = 0;
  while (1) {
    /* arranging this loop without a sleep doesn't max out the processor
       as you might think, because IPC is internally blocking while
       waiting for a reply from the server */
    if (IPC_OK == IPC_connect(nameOfThisModule.c_str())) break;
    if(callback != NULL) callback(centralHost, false);
    if (!verbose && !printedUpdate) {
      printf("(still trying to connect to central server on %s)", centralHost);
      printedUpdate = 1;
    }
    fprintf(stdout,".");
    fflush(stdout);
  }
  if(callback != NULL) callback(centralHost, true);
  if (verbose) {
    fprintf(stdout,"...Success.\n");
  }
  /* turn error reporting back up to the max */
  IPC_setVerbosity(IPC_Exit_On_Errors);
}


void
IPCHelper::reliableConnectUnique(const string& nameOfThisModule) {

  /* suppress errors about how we can't connect while we're
     in this loop */
  IPC_setVerbosity(IPC_Silent);
  string tmpName(string("IPCHelper::reliableConnectUnique test of ")
		 + nameOfThisModule); 
		 
  char *centralHost;
  fprintf(stdout,"Attempting to connect to IPC central server on %s",
         ((centralHost = getenv("CENTRALHOST")) ? centralHost : "local host"));
  fflush(stdout);
  // First connect with a dummy module name
  while (1) {
    /* arranging this loop without a sleep doesn't max out the processor
       as you might think, because IPC is internally blocking while
       waiting for a reply from the server */
    fprintf(stdout,".");
    fflush(stdout);

    if (IPC_OK == IPC_connect(tmpName.c_str())) break;
    fflush(stdout);
  }
  // We want each module to have a different name
  // so test to make sure the real name is not taken
  if (true == IPC_isModuleConnected(nameOfThisModule.c_str())) {
    cerr << "\nERROR: The name \"" << nameOfThisModule << 
      "\" already exists!\n   Must have a unique name.\n";
    exit(EXIT_FAILURE);
  } else {
    IPC_disconnect();
  }
  // If the name we want is not taken, then connect for real
  fprintf(stdout,"\nAttempting to connect using unique name");
  fflush(stdout);
  while (1) {
    /* arranging this loop without a sleep doesn't max out the processor
       as you might think, because IPC is internally blocking while
       waiting for a reply from the server */
    if (IPC_OK == IPC_connect(nameOfThisModule.c_str())) break;
    fprintf(stdout,".");
    fflush(stdout);
  }
  fprintf(stdout,"...Success.\n");
  /* turn error reporting back up to the max */
  IPC_setVerbosity(IPC_Exit_On_Errors);
}


static void
dummyMsgHandler(MSG_INSTANCE msgRef,
		BYTE_ARRAY callData,
		void *clientData) {
  /* do nothing */
}


void
IPCHelper::notify(const string& msgName) {
  if (verbose) {
    cout << "IPCHelper::notify: sending notification that event `"
	 << msgName << "' has occurred" << endl;
  }
  defineMessage(msgName,"");
  IPC_subscribe(msgName.c_str(), dummyMsgHandler, 0);
}

static void
handlerChangeHandler(const char *_msgName,
		     int numHandlers,
		     void *clientData) {
  *((int*) clientData) = numHandlers;
}

void
IPCHelper::waitForNotification(const string& msgName) {
  if (verbose) {
    cout << "IPCHelper::waitForNotification: waiting on event `"
	 << msgName << "'..." << endl;
    cout.flush();
  }
  defineMessage(msgName,"");
  if (IPC_numHandlers(msgName.c_str()) > 0) {
    cout << "IPCHelper::waitForNotification: `" 
	 << msgName << "' already exists" << endl;
    cout.flush();
    return;
  }
  int numHandlers;
  IPC_subscribeHandlerChange(msgName.c_str(),
			     &handlerChangeHandler,(void*) &numHandlers);
  numHandlers = IPC_numHandlers(msgName.c_str());
  while (1) {
    // waits for 1 second or for a message to arrive, whichever comes first
    IPC_handleMessage(1000);
    cout << ".";
    cout.flush();
    if (numHandlers > 0) {
      IPC_unsubscribeHandlerChange(msgName.c_str(), &handlerChangeHandler);
      if (verbose) {
	cout << "IPCHelper::waitForNotification: finished waiting on `" 
	     << msgName << "'" << endl;
	cout.flush();
      }
      return;
    }
  }
}

} // namespace microraptor

/***************************************************************************
 * REVISION HISTORY:
 * $Log: mrIPCHelper.cc,v $
 * Revision 1.11  2006/11/16 18:56:05  brennan
 * Added support for callbacks during a reliableConnect.
 *
 * Revision 1.10  2006/06/14 02:02:46  trey
 * reliableConnect() is now more verbose when it fails to connect to central on the first attempt
 *
 * Revision 1.9  2004/07/19 03:50:01  trey
 * switched some output to go to stdout instead of stderr so microraptor handles it better
 *
 * Revision 1.8  2004/06/28 15:46:18  trey
 * fixed to properly find ipc.h and libipc.a in new atacama external area
 *
 * Revision 1.7  2004/04/28 17:15:21  dom
 * Added header with copyright information to source and Makefiles.
 *
 * Revision 1.6  2003/10/04 21:56:48  trey
 * tweaked things to eliminate the need for the common/overlap directory and libmrcommonoverlap.a
 *
 * Revision 1.3  2003/08/27 20:05:39  trey
 * made non-verbose less verbose
 *
 * Revision 1.2  2003/08/08 15:20:23  brennan
 * client: added ZVT terminal display option
 * daemon: no net effect.
 *
 * Revision 1.1  2003/03/28 17:38:15  trey
 * rearranged locations of files so that the atacama project can use rcl without causing namespace conflicts
 *
 * Revision 1.4  2003/02/17 16:21:40  trey
 * made IPCHelper output its data on stderr instead of stdout, to avoid confusing a client connected to stdout
 *
 * Revision 1.3  2003/02/16 06:20:08  trey
 * moved some things from daemon to common
 *
 * Revision 1.2  2003/02/14 00:13:54  trey
 * fixed include issues
 *
 * Revision 1.1  2003/02/13 16:04:49  trey
 * initial microraptor check-in
 *
 * Revision 1.10  2002/05/16 23:29:44  trey
 * improved control over bandwidth of debug messages
 *
 * Revision 1.9  2002/04/09 22:18:51  danig
 * Added call to IPC_numHandlers() in waitForNotifications() to fix bug.
 *
 * Revision 1.8  2002/04/04 23:41:10  danig
 * Modified ReliableConnectUnique to give more informative messages.
 *
 * Revision 1.7  2002/03/22 23:09:43  danig
 * In IPCHelper::waitForNotification, initialized numHandlers to zero to
 * fix a bug.
 *
 * Revision 1.6  2002/01/22 23:02:55  danig
 * Added IPCHelper::reliableConnectUnique() to enforce unique names
 * for tasks/modules.
 *
 * Revision 1.5  2001/11/19 18:26:09  trey
 * added notify/waitForNotification functionality
 *
 * Revision 1.4  2001/11/08 23:10:47  trey
 * made debug statement more complete, and added user option of IPCHelper::setVerbose(false) to disable it
 *
 * Revision 1.3  2001/11/08 16:25:10  trey
 * tweaked error message
 *
 * Revision 1.2  2001/11/02 21:06:07  trey
 * fixed problem with returning FSignal datatype from a function by adding a wrapper layer around everything
 *
 * Revision 1.1  2001/10/30 05:34:08  trey
 * initial check-in
 *
 * Revision 1.2  2001/10/25 22:51:44  trey
 * changed char* arguments to strings for convenience
 *
 * Revision 1.1  2001/10/25 21:07:17  trey
 * initial check-in
 *
 *
 ***************************************************************************/
