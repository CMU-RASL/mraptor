/********** tell emacs we use -*- c++ -*- style comments *******************
 * COPYRIGHT 2004, Carnegie Mellon University
 * $Revision: 1.6 $  $Author: trey $  $Date: 2005/05/16 21:10:42 $
 *  
 * @file    RCLDebugInfo.cc
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
#include <sstream>

#include "RCLDebugInfo.h"

using namespace std;

namespace rcl {

  void DebugInfo::setFileLineNumber(std::string _inputFile, int _lineNumber) {
    inputFile = _inputFile;
    lineNumber = _lineNumber;
  }

  string DebugInfo::parseObjectName(void) const {
    string::size_type pos;
    string parent, field, entry;

    if (objectName.size() >= 4 && ')' == objectName[objectName.size()-1]
	&& (string::npos != (pos = objectName.find_last_of("("))
	    && objectName.size()-pos >= 4)) {
      // objectName looks like 'parent("field")': parse out the pieces
      parent = objectName.substr(0,pos);
      field = objectName.substr(pos+2, objectName.size()-pos-4);
      return "field \"" + field + "\" in " + parent;
    } else if (objectName.size() >= 3 && ']' == objectName[objectName.size()-1]
	       && (string::npos != (pos = objectName.find_last_of("["))
		   && objectName.size()-pos >= 3)) {
      // objectName looks like 'parent[index]': parse out the pieces
      parent = objectName.substr(0,pos);
      entry = objectName.substr(pos+1, objectName.size()-pos-2);
      return "entry " + entry + " in vector " + parent;
    } else {
      return objectName;
    }
  }

  string DebugInfo::getDebugTraceInternal(bool undefFormat) const {
    ostringstream out;
    if (objectName != "") {
      if (undefFormat) {
	out << parseObjectName() << " ";
      } else {
	out << objectName << " ";
      }
    }
    if (inputFile != "") {
      out << "(defined at " << inputFile << ":" << lineNumber << ")";
    }
    return out.str();
  }

  string DebugInfo::getUndefDebugTrace(void) const {
    return getDebugTraceInternal(true);
  }

  string DebugInfo::getDebugTrace(void) const {
    return getDebugTraceInternal(false);
  }

}; // namespace rcl

/***************************************************************************
 * REVISION HISTORY:
 * $Log: RCLDebugInfo.cc,v $
 * Revision 1.6  2005/05/16 21:10:42  trey
 * improved debug output
 *
 * Revision 1.5  2005/05/12 00:17:01  trey
 * fixed bugs in error messages
 *
 * Revision 1.4  2005/04/21 18:58:23  trey
 * made getDebugTrace() output more readable
 *
 * Revision 1.3  2004/04/28 17:15:21  dom
 * Added header with copyright information to source and Makefiles.
 *
 * Revision 1.2  2004/04/09 20:31:33  trey
 * major enhancement to rcl interface
 *
 * Revision 1.1  2004/04/08 16:42:56  trey
 * overhauled exp
 *
 *
 ***************************************************************************/
