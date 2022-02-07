/********** tell emacs we use -*- c++ -*- style comments *******************
 * COPYRIGHT 2004, Carnegie Mellon University
 * $Revision: 1.3 $  $Author: dom $  $Date: 2004/04/28 17:15:21 $
 *  
 * PROJECT: FIRE Architecture Project
 *
 * @file    IPCMessage.cc
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

#include "mrIPCMessage.h"

using namespace std;
using namespace microraptor;

/***************************************************************************
 * MACROS
 ***************************************************************************/

// set to 1 to get debug output
#define IPC_MESSAGE_DEBUG_VERBOSE 0

/***************************************************************************
 * FUNCTIONS
 ***************************************************************************/

IPCMessageInternalGeneric::IPCMessageInternalGeneric
  (const string& _messageName, 
   const string& userSpecifiedFormat,
   const string& defaultFormat) :
  messageName(_messageName),
  format(("" == userSpecifiedFormat) ? defaultFormat : userSpecifiedFormat)
{
  IPCHelper::defineMessage(messageName, format);
  numSubscribers = -1;
}

IPCMessageInternalGeneric::~IPCMessageInternalGeneric(void) { }

void
IPCMessageInternalGeneric::connHandlerInternal(int oldNum, int newNum)
{
  if (0 == oldNum && 0 != newNum) {
    subscribe();
  } else if (0 != oldNum && 0 == newNum) {
    unsubscribe();
  }
#if IPC_MESSAGE_DEBUG_VERBOSE
  cout << "IPCMessage1: slot list change for msg" << endl
       << "  " << messageName << endl
       << "  there are now " << newNum << " connected slots" << endl;
#endif
}

void IPCMessageInternalGeneric::publishOnlyWhenSubscribedTo(void) {
  if (-1 == numSubscribers) {
    IPC_subscribeHandlerChange(messageName.c_str(),
			       &handlerChangeHandler,this);
    numSubscribers = IPC_numHandlers(messageName.c_str());
#if IPC_MESSAGE_DEBUG_VERBOSE
    cout << "IPCMessage1: starting # subscribers tracking for "
	 << messageName << endl
	 << "  there are now " << numSubscribers << " subscribers" << endl;
#endif
  }
}

void IPCMessageInternalGeneric::publishAlways(void) {
  if (-1 != numSubscribers) {
    IPC_unsubscribeHandlerChange(messageName.c_str(),
				 &handlerChangeHandler);
    numSubscribers = -1; // shows we are not tracking
  }
}

void IPCMessageInternalGeneric::subscribe(void) {
  IPC_subscribe(messageName.c_str(), &ipcHandler, this);
}

void IPCMessageInternalGeneric::unsubscribe(void) {
  IPC_unsubscribe(messageName.c_str(), &ipcHandler);
}

void IPCMessageInternalGeneric::publishInternal
  (const void *info, bool verifySubs)
{
#if IPC_MESSAGE_DEBUG_VERBOSE
  cout << "publish: verifySubs=" << (verifySubs ? "TRUE" : "FALSE")
       << "; there are " << numSubscribers << " subscribers for "
       << messageName << endl;
#endif
  
  int numSubscribersTemp = numSubscribers;
  if (verifySubs && -1 == numSubscribers) {
    // do low cost one-time check of # of handlers
    numSubscribersTemp = IPC_numHandlers(messageName.c_str());
  }
  
  if (0 == numSubscribersTemp) {
    if (verifySubs) {
      cerr << "ERROR: trying to publish " << messageName
	   << " in verified mode, but there are no subscribers" << endl;
      exit(EXIT_FAILURE);
    }
  } else {
#if IPC_MESSAGE_DEBUG_VERBOSE
    cout << "publish: calling IPC_publishData" << endl;
#endif
    IPC_publishData(messageName.c_str(), const_cast<void*>(info));
  }
}

void
IPCMessageInternalGeneric::ipcHandler(MSG_INSTANCE msgRef,
				      BYTE_ARRAY callData,
				      void *clientData)
{
  IPCMessageInternalGeneric *inst =
    (IPCMessageInternalGeneric *) clientData;
  
  if ("" != inst->format && "NULL" != inst->format
      && "{}" != inst->format) {
    void *info;
    FORMATTER_PTR formatter = IPC_msgInstanceFormatter(msgRef);
    IPC_unmarshall(formatter, callData, &info);
    IPC_freeByteArray(callData);
    inst->doEmit(info);
  } else {
    inst->doEmit(0);
  }
}

void
IPCMessageInternalGeneric::handlerChangeHandler(const char *_msgName,
						int numHandlers,
						void *clientData)
{
  IPCMessageInternalGeneric *inst =
    (IPCMessageInternalGeneric *) clientData;
#if IPC_MESSAGE_DEBUG_VERBOSE
  cout << "IPCMessage1: handler change for msg" << endl
       << "  " << inst->messageName << endl
       << "  there are now " << numHandlers << " subscribers" << endl;
#endif
  inst->numSubscribers = numHandlers;
}


/***************************************************************************
 * REVISION HISTORY:
 * $Log: mrIPCMessage.cc,v $
 * Revision 1.3  2004/04/28 17:15:21  dom
 * Added header with copyright information to source and Makefiles.
 *
 * Revision 1.2  2003/10/04 21:56:48  trey
 * tweaked things to eliminate the need for the common/overlap directory and libmrcommonoverlap.a
 *
 * Revision 1.1  2003/02/14 05:58:05  trey
 * initial atacama check-in
 *
 * Revision 1.2  2002/05/19 23:53:01  trey
 * turned off debugging messages
 *
 * Revision 1.1  2002/05/19 21:13:31  trey
 * factored out duplicate code in templates
 *
 *
 ***************************************************************************/
