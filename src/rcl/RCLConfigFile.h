/********** tell emacs we use -*- c++ -*- style comments *******************
 * COPYRIGHT 2004, Carnegie Mellon University
 * $Revision: 1.4 $  $Author: trey $  $Date: 2004/07/17 18:31:16 $
 *  
 * @file    RCLConfigFile.h
 * @brief   No brief
 ***************************************************************************/

#ifndef INCRCLConfigFile_h
#define INCRCLConfigFile_h

#include <string>
#include "RCLExp.h"

namespace rcl {

  exp loadConfigFile(const std::string& file_name);

  // used to load a config file that is embedded in the binary as a string.
  // if the string is null-terminated, specifying the length is optional.
  exp loadConfigFromString(const char* config_string, int num_bytes = 0);

}; // namespace rcl

#endif // INCRCLConfigFile_h

/***************************************************************************
 * REVISION HISTORY:
 * $Log: RCLConfigFile.h,v $
 * Revision 1.4  2004/07/17 18:31:16  trey
 * added loadConfigFromString() function
 *
 * Revision 1.3  2004/04/28 17:15:21  dom
 * Added header with copyright information to source and Makefiles.
 *
 * Revision 1.2  2004/04/09 20:31:33  trey
 * major enhancement to rcl interface
 *
 * Revision 1.1  2003/03/28 17:38:15  trey
 * rearranged locations of files so that the atacama project can use rcl without causing namespace conflicts
 *
 * Revision 1.1  2003/02/24 16:29:52  trey
 * added "executing" config file ability
 *
 *
 ***************************************************************************/
