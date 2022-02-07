/********** tell emacs we use -*- c++ -*- style comments *******************
 * COPYRIGHT 2004, Carnegie Mellon University
 * $Revision: 1.4 $  $Author: trey $  $Date: 2005/06/01 00:46:34 $
 *  
 * PROJECT: Life in the Atacama
 *
 * @file    mrQuickInterface.h
 * @brief   No brief
 ***************************************************************************/

#ifndef INCmrQuickInterface_h
#define INCmrQuickInterface_h

#include <string>

namespace microraptor {

  enum Quick_Interface_Return_Values {
    MR_OK,
    MR_FAILED
  };

  enum Quick_Interface_Debug_Levels {
    DEBUG_SILENT,
    DEBUG_QUIET,
    DEBUG_VERBOSE
  };

  /* send_mraptord_command: runs mrterm and uses it to send the given
     command to an active instance of mraptord.  See testMRQuick.cc for
     example usage.

     If the command fails or times out (e.g., when mraptord is not
     running), send_mraptord_command calls the RCL error handler, which
     normally prints a message to stderr and throws an exception.

     If there are multiple active instances of mraptord, mrterm will
     connect and send the command to the first one that responds.  For
     most commands of interest (specifically including "run", "kill",
     and "restart") it does not matter which daemon is involved, because
     the command will automatically be forwarded to the right daemon(s)
     according to the "host" field of the configuration for the
     processes involved.
     
     send_mraptord_command() uses two environment variables:

       MRTERM_BINARY -------- Specifies the path to the mrterm binary.
                              Defaults to 'mrterm'.

       MRAPTOR_CENTRALHOST -- Specifies what host/port mrterm should use
                              to connect to the IPC central server.
			      Defaults to using the standard CENTRALHOST
			      environment variable.  (This option is only
			      needed if mraptord is connected to a different
			      central server from the rest of the system.)

   */
  void send_mraptord_command(const std::string& cmd,
			    int debug_level = DEBUG_VERBOSE);

  /* A shortcut for sending a "restart <process>" command to mraptord.
     See above for details of the behavior.  */
  inline void restart_process(const std::string& processName,
			      int debug_level = DEBUG_VERBOSE)
  {
    send_mraptord_command(std::string("restart ") + processName, debug_level);
  }

};

#endif // INCmrQuickInterface_h

/***************************************************************************
 * REVISION HISTORY:
 * $Log: mrQuickInterface.h,v $
 * Revision 1.4  2005/06/01 00:46:34  trey
 * added error checking
 *
 * Revision 1.3  2004/08/29 20:11:57  trey
 * introduced a way to not suppress all mrterm output when debugging
 *
 * Revision 1.2  2004/08/21 19:49:21  trey
 * provided default value for verbose argument
 *
 * Revision 1.1  2004/08/21 19:39:45  trey
 * initial check-in
 *
 *
 ***************************************************************************/
