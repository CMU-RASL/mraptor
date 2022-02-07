/********** tell emacs we use -*- c++ -*- style comments *******************
 * COPYRIGHT 2004, Carnegie Mellon University
 * $Revision: 1.6 $  $Author: brennan $  $Date: 2006/11/16 18:56:05 $
 *  
 * PROJECT: FIRE Architecture Project
 *
 * @file    mrIPCHelper.h
 * @brief   No brief
 ***************************************************************************/

#ifndef INCmrIPCHelper_h
#define INCmrIPCHelper_h

/***************************************************************************
 * INCLUDES
 ***************************************************************************/

#include <string>

/***************************************************************************
 * MACROS
 ***************************************************************************/

/***************************************************************************
 * CLASSES AND TYPEDEFS
 ***************************************************************************/

namespace microraptor {

struct IPCHelper {
  static void setVerbose(bool _verbose) {
    verbose = _verbose;
  }
  static bool getVerbose(void) { return verbose; }
  static std::string defineMessage(const std::string& messageName,
			      const std::string& format);
  // Callback is called before the first and after every every
  // attempted IPC_Connect; string argument is the centralhost:port
  // setting; bool argument is true if connection has been
  // established, false otherwise.
  static void reliableConnect(const std::string& nameOfThisModule, void (*callback)(std::string, bool) = NULL);
  static void reliableConnectUnique(const std::string& nameOfThisModule);
  static void notify(const std::string& msgName);
  static void waitForNotification(const std::string& msgName);
  static std::string getshorthostname(void);
protected:
  static bool verbose;
};

} // namespace microraptor

/***************************************************************************
 * GLOBALS AND FUNCTION PROTOTYPES
 ***************************************************************************/

#endif // INCmrIPCHelper_h

/***************************************************************************
 * REVISION HISTORY:
 * $Log: mrIPCHelper.h,v $
 * Revision 1.6  2006/11/16 18:56:05  brennan
 * Added support for callbacks during a reliableConnect.
 *
 * Revision 1.5  2004/04/28 17:15:21  dom
 * Added header with copyright information to source and Makefiles.
 *
 * Revision 1.4  2003/10/04 21:56:48  trey
 * tweaked things to eliminate the need for the common/overlap directory and libmrcommonoverlap.a
 *
 * Revision 1.1  2003/03/28 17:38:15  trey
 * rearranged locations of files so that the atacama project can use rcl without causing namespace conflicts
 *
 * Revision 1.2  2003/02/16 06:20:08  trey
 * moved some things from daemon to common
 *
 * Revision 1.1  2003/02/13 16:04:49  trey
 * initial microraptor check-in
 *
 * Revision 1.7  2002/07/02 20:04:13  soa
 * Added using namespace std for compatibility with garbo's g++
 *
 * Revision 1.6  2002/05/16 23:29:44  trey
 * improved control over bandwidth of debug messages
 *
 * Revision 1.5  2002/05/14 18:43:18  trey
 * changed name of include guard to match name of file
 *
 * Revision 1.4  2002/01/22 23:02:41  danig
 * Added IPCHelper::reliableConnectUnique() to enforce unique names
 * for tasks/modules.
 *
 * Revision 1.3  2001/11/19 18:26:09  trey
 * added notify/waitForNotification functionality
 *
 * Revision 1.2  2001/11/08 23:10:47  trey
 * made debug statement more complete, and added user option of IPCHelper::setVerbose(false) to disable it
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
