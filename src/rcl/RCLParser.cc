/****************************************************************************
 * $Revision: 1.9 $  $Author: trey $  $Date: 2004/07/06 15:35:12 $
 *
 * PROJECT:      Distributed Robotic Agents
 * DESCRIPTION:  
 *
 * (c) Copyright 1999 CMU. All rights reserved.
 ***************************************************************************/

/***************************************************************************
 * INCLUDES
 ***************************************************************************/

#include <assert.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>

// Made change to compile under gcc 3.2 in Cygwin; should work elsewhere
//#include <iostream.h>
#include <iostream>

//#include "RCLParser.h"
#include "RCLInternal.h"

/***************************************************************************
 * GLOBALS
 ***************************************************************************/

namespace rcl {

  exp parseInternal(const string& configFile) {
#if 0
    // get parser trace
    RCL_YY_PREFIX(debug) = 1;
#endif
    
    snprintf(rcl::lexCurrentFile, sizeof(rcl::lexCurrentFile), "%s",
	     configFile.c_str());
    rcl::lexLineNo = 1;
    
    if (0 != RCL_YY_PREFIX(parse)()) {
      string s = "parser: parsing of `" + configFile + "' unsuccessful.";
      callErrorHandler(s);
    }
    rcl::lexBufferCleanup();

    exp e = *RCL_YY_PREFIX(Tree);
#if USE_MEM_TRACKER
    MTR_DELETE(RCL_YY_PREFIX(Tree));
#else
    delete RCL_YY_PREFIX(Tree);
#endif
    return e;
  }
  
  exp readFromFile(const string& configFile)
  {
    if (0 == (RCL_YY_PREFIX(in) = fopen(configFile.c_str(),"r"))) {
      string s = "parser::readFromFile: couldn't open `" + configFile
	+ "' for reading";
      callErrorHandler(s);
    }
    rcl::lexSwitchToFile(RCL_YY_PREFIX(in));
    
    exp e = parseInternal(configFile);
    
    fclose(RCL_YY_PREFIX(in));
    RCL_YY_PREFIX(in) = 0;
    
    return e;
  }
  
  exp readFromFD(const string& fileName, int fd)
  {
    if (0 == (RCL_YY_PREFIX(in) = fdopen(fd, "r"))) {
      string s = "parser::readFromFD: couldn't fdopen `" + fileName
	+ "' for reading";
      callErrorHandler(s);
    }
    rcl::lexSwitchToFile(RCL_YY_PREFIX(in));
    
    exp e = parseInternal(fileName);
    
    fclose(RCL_YY_PREFIX(in));
    RCL_YY_PREFIX(in) = 0;
    
    return e;
  }
  
  exp readFromString(const string& s) {
    rcl::lexSwitchToString(s);
    exp e = parseInternal("<string>");
    return e;
  }

}; // namespace rcl

/***************************************************************************
 * REVISION HISTORY:
 * $Log: RCLParser.cc,v $
 * Revision 1.9  2004/07/06 15:35:12  trey
 * added optional memory tracking and plugged some memory leaks
 *
 * Revision 1.8  2004/06/10 22:46:43  trey
 * changed default behavior for RCL so that it prints an error before throwing an exception; error handling is now configurable using setErrorHandler()
 *
 * Revision 1.7  2004/06/09 22:50:08  trey
 * made it possible to switch the name prefix from the default "yy" when running flex
 *
 * Revision 1.6  2004/04/09 20:31:33  trey
 * major enhancement to rcl interface
 *
 * Revision 1.5  2003/08/08 15:29:27  brennan
 * Attempted to make microraptor compile:
 *   (1) on Cygwin and
 *   (2) under gcc 3.2
 * This attempt has not yet succeeded
 *
 * Revision 1.4  2003/02/24 16:30:45  trey
 * added ability to read rcl from an already open file descriptor
 *
 * Revision 1.3  2003/02/16 06:19:45  trey
 * fixed reading from file after reading from string
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
 * Revision 1.3  2000/02/25 22:13:24  trey
 * added a new debugging statement which dumps cpp output, disabled by default
 *
 * Revision 1.2  1999/11/11 15:14:57  trey
 * changed to support logging
 *
 * Revision 1.1  1999/11/08 15:35:02  trey
 * major overhaul
 *
 * Revision 1.1  1999/11/01 17:30:14  trey
 * initial check-in
 *
 *
 ***************************************************************************/
