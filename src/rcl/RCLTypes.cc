/********** tell emacs we use -*- c++ -*- style comments *******************
 * COPYRIGHT 2004, Carnegie Mellon University
 * $Revision: 1.20 $  $Author: brennan $  $Date: 2004/11/25 05:23:12 $
 *  
 * PROJECT: FIRE Architecture Project
 *
 * @file    RCLTypes.cc
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
#include <iomanip>
#include <cstring>

#include "RCLTypes.h"
#include "RCLExp.h"
#include "RCLVector.h"
#include "strxcpy.h"

using namespace std;

namespace rcl {

#define RCL_WRITE_COMPACT(type) \
  void writeCompact(ostream& out, const type& data) { \
    writeConfig(out, data, 0); \
  }
  
  RCL_WRITE_COMPACT(undefinedExp);
  RCL_REPEAT_MACRO_ALL(RCL_WRITE_COMPACT);

  void writeConfig(ostream& out, const undefinedExp& data, int indent) {
    out << "undef";
  }

  static bool charCanBePartOfBareString(char c) {
    return ((c >= 'A' && c <= 'Z')
	    || (c >= 'a' && c <= 'z')
	    || (c >= '0' && c <= '9')
	    || (c == '.')
	    || (c == '_')
	    || (c == '-')
	    || (c == '/')
	    || (c == '~'));
  }

  static bool canBeBareString(const string& s) {
    // make sure the bare version of the string won't be interpreted as a number
    if (strchr("0123456789.-", s[0])) return false;
    
    int n = s.size();
    for (int i=0; i < n; i++) {
      if (!charCanBePartOfBareString(s[i])) return false;
    }
    if (s == "undef") return false;
    return true;
  }

  void writeConfig(ostream& out, const string& data, int indent) {
#if USE_MEM_TRACKER
      char *escBuf;
      int n = 4*strlen(data.c_str()) + 1;
      MTR_NEW_ARR(escBuf, char[n], n);
#else
      char *escBuf = new char[4*strlen(data.c_str()) + 1];
#endif
      // the last arg keeps us from escaping single quotes (nice
      //   because we are in a double-quoted string, so single quotes
      //   are ok)
      streadd(escBuf, data.c_str(), "'");
      out << "\"" << escBuf << "\"";
#if USE_MEM_TRACKER
      MTR_DELETE(escBuf);
#else
      delete [] escBuf;
#endif
  }

  void writeStringOptQuotes(ostream& out, const string& data) {
    if (canBeBareString(data)) {
      out << data;
    } else {
      writeConfig(out,data,0);
    }
  }


#define RCL_WRITE_NUMBER(type) \
  void writeConfig(ostream& out, const type& data, int indent) { \
    out << setprecision(10) << data; \
  }

  RCL_WRITE_NUMBER(double);
  RCL_WRITE_NUMBER(float);
  RCL_WRITE_NUMBER(int);
  RCL_WRITE_NUMBER(long);

  void writeConfig(ostream& out, const bool& data, int indent) {
    out << (data ? "true" : "false");
  }

  const char *indent(int indent) {
    static char spcBuf[128];
    assert(((unsigned) indent+1) < sizeof(spcBuf));
    for (int i=0; i < indent; i++) {
      spcBuf[i] = ' ';
    }
    spcBuf[indent] = 0;
    return spcBuf;
  }

  string toString(const exp& data) {
    ostringstream os;
    data.writeCommand(os);
    return os.str();
  }
  string toString(const vector& data) {
    ostringstream os;
    data.writeCommand(os);
    return os.str();
  }

  string toStringCompact(const exp& data) {
    // Made change to compile under gcc 3.2 in Cygwin; should work elsewhere
    //ostrstream os;
    ostringstream os;
    data.writeCompact(os);
    return os.str();
  }

  string toStringConfig(const exp& data) {
    // Made change to compile under gcc 3.2 in Cygwin; should work elsewhere
    //ostrstream os;
    ostringstream os;
    data.writeConfig(os,0);
    return os.str();
  }
};

/***************************************************************************
 * REVISION HISTORY:
 *
 ***************************************************************************/
/***************************************************************
 * Revision History:
 * $Log: RCLTypes.cc,v $
 * Revision 1.20  2004/11/25 05:23:12  brennan
 * Fixed some new/delete mismatches turned up by valgrind - needed delete []'s.
 *
 * Revision 1.19  2004/08/02 02:55:59  trey
 * strings are now quoted more often
 *
 * Revision 1.18  2004/07/06 15:35:12  trey
 * added optional memory tracking and plugged some memory leaks
 *
 * Revision 1.17  2004/06/11 20:49:42  trey
 * increased precision for doubles
 *
 * Revision 1.16  2004/04/28 18:58:53  dom
 * Appended log directive
 * 
 ***************************************************************/
