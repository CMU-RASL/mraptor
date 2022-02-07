/********** tell emacs we use -*- c++ -*- style comments *******************
 * COPYRIGHT 2004, Carnegie Mellon University
 * $Revision: 1.4 $  $Author: dom $  $Date: 2004/04/28 17:15:21 $
 *  
 * @file    processStatus.h
 * @brief   No brief
 ***************************************************************************/

#ifndef INCprocessStatus_h
#define INCprocessStatus_h

#include <string>

// time between checks of how long each process has been idle
// (put this here so claw can use it)
#define MR_IDLE_UPDATE_PERIOD (30)

enum Status_Values {
  MR_NOT_STARTED,
  MR_PENDING,
  MR_STARTING,
  MR_RUNNING,
  MR_CLEAN_EXIT,
  MR_ERROR_EXIT,
  MR_SIGNAL_EXIT,
  MR_MAX_STATUS
};

struct Process_Status {
  static std::string to_string(int status);
  static int from_string(const std::string& status_string);
  static bool is_runnable(int status);
  static bool has_console(int status);

  // syntactic sugar
  static bool is_runnable(const std::string& status_string) {
    return is_runnable(from_string(status_string));
  }
  static bool has_console(const std::string& status_string) {
    return has_console(from_string(status_string));
  }
};

#endif // INCprocessStatus_h

/***************************************************************************
 * REVISION HISTORY:
 * $Log: processStatus.h,v $
 * Revision 1.4  2004/04/28 17:15:21  dom
 * Added header with copyright information to source and Makefiles.
 *
 * Revision 1.3  2003/08/08 19:19:58  trey
 * made changes to enable compilation under g++ 3.2
 *
 * Revision 1.2  2003/03/14 04:23:34  trey
 * made MR_IDLE_UPDATE_PERIOD public by moving it to processStatus.h
 *
 * Revision 1.1  2003/03/10 16:02:40  trey
 * added new process status values: pending and starting
 *
 *
 ***************************************************************************/
