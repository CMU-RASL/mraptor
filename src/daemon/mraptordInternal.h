/********** tell emacs we use -*- c++ -*- style comments *******************
 * COPYRIGHT 2004, Carnegie Mellon University
 * $Revision: 1.3 $  $Author: trey $  $Date: 2004/06/28 15:46:18 $
 *  
 * PROJECT: Life in the Atacama
 *
 * @file    mraptordInternal.h
 * @brief   No brief
 ***************************************************************************/

#ifndef INCmraptordInternal_h
#define INCmraptordInternal_h

/***************************************************************************
 * INCLUDES
 ***************************************************************************/

#include <algorithm>
#include <queue>

#include <fcntl.h>
#include <sys/ioctl.h>
//#include <rpcsvc/rex.h>
#include <pty.h>

#ifdef NO_WORDEXP
#include <signal.h>
#else
#include <libgen.h> // dirname
#include <wordexp.h> // shell-style variable expansion
#endif
// Needed to add this when compiling on Valerie (RH 6.2) under gcc 2.95.3
// I think it should be forwards-compatible... we'll see.
#include <sys/resource.h> // struct rusage

#include "ipc.h"

#include "raptorDaemon.h"
#include "RCL.h"
#include "mrIPCHelper.h"
#include "mrSafeSystem.h"

/**********************************************************************
 * MACROS
 **********************************************************************/

#define MR_MAX_ARGS (50)

#define MR_IPC_SILENT_CLOSE_PATCH 0

/***************************************************************************
 * FUNCTION PROTOTYPES
 ***************************************************************************/

#if MR_IPC_SILENT_CLOSE_PATCH
// avoid including tca.h from IPC
extern "C" {
  extern void x_ipcCloseInternal(int informServer);
}
#endif

// avoid including tca.h from IPC
extern "C" {
  extern void x_ipcRegisterExitProc(void (*)(void));
}

/**********************************************************************
 * GLOBAL VARIABLES
 **********************************************************************/

extern Raptor_Daemon *active_daemon_g;

/**********************************************************************
 * CLASSES
 **********************************************************************/

#ifdef NO_WORDEXP
// this is obsoleted by use of glibc wordexp
// However, Cygwin doesn't have wordexp yet.

// avoids memory leak problems when building an argv for execv
struct ArgList {
  int argc;
  char *argv[MR_MAX_ARGS];

  ArgList(void) :
    argc(0)
  {
    FOR(i,MR_MAX_ARGS) {
      argv[i] = 0;
    }
  }
  ~ArgList(void) {
    FOR(i,argc) {
      free(argv[i]);
    }
  }
  void push_back(const string& arg) {
    if(!(argc < MR_MAX_ARGS)) {
      cerr << "assert failed in ArgList push_back" << endl;
    }
    assert(argc < MR_MAX_ARGS);
    argv[argc] = strdup(arg.c_str());
    argc++;
  }
  void from_rcl_vector(const rcl::exp& e) {
    FOR_EACH ( arg, body.getVector() ) {
      if (arg->getType() == rcl::typeName<string>()) {
	push_back(arg->getString());
      } else {
	push_back(rcl::toString(*arg));
      }
    }
  }
};
#endif

#endif // INCmraptordInternal_h

/***************************************************************************
 * REVISION HISTORY:
 * $Log: mraptordInternal.h,v $
 * Revision 1.3  2004/06/28 15:46:18  trey
 * fixed to properly find ipc.h and libipc.a in new atacama external area
 *
 * Revision 1.2  2004/04/28 17:15:21  dom
 * Added header with copyright information to source and Makefiles.
 *
 * Revision 1.1  2004/04/19 14:49:25  trey
 * split raptorDaemon.cc into two files
 *
 *
 ***************************************************************************/
