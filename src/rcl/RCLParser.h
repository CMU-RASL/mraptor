/********** tell emacs we use -*- c++ -*- style comments *******************
 * $Revision: 1.4 $  $Author: trey $  $Date: 2004/04/09 20:31:33 $
 *
 * PROJECT:      Distributed Robotic Agents
 * DESCRIPTION:  
 *
 * (c) Copyright 1999 CMU. All rights reserved.
 ***************************************************************************/

#ifndef INCRCLParser_h
#define INCRCLParser_h

#include "RCL.h"

namespace rcl {

  exp readFromFD(const string& fileName, int fd);
  exp readFromFile(const string& configFile);
  exp readFromString(const string& s);
    
  exp parseInternal(const string& configFile);

}; // namespace rcl

#endif // INCRCLParser_h

/***************************************************************************
 * REVISION HISTORY:
 * $Log: RCLParser.h,v $
 * Revision 1.4  2004/04/09 20:31:33  trey
 * major enhancement to rcl interface
 *
 * Revision 1.3  2003/02/24 16:30:46  trey
 * added ability to read rcl from an already open file descriptor
 *
 * Revision 1.2  2003/02/05 20:23:20  trey
 * implemented command syntax and switched error behavior to throw exceptions
 *
 * Revision 1.1  2003/02/05 03:57:54  trey
 * initial atacama check-in
 *
 * Revision 1.2  2002/09/24 21:50:24  trey
 * rebuilt RCL from scratch to support nested vectors and hashes
 *
 * Revision 1.1  2002/07/24 21:22:01  trey
 * initial FIRE checkin
 *
 * Revision 1.2  1999/11/11 15:14:58  trey
 * changed to support logging
 *
 * Revision 1.1  1999/11/08 15:35:02  trey
 * major overhaul
 *
 *
 ***************************************************************************/
